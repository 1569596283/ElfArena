#include "Battle/ElfBattleManager.h"
#include "Battle/ElfTurnManager.h"
#include "UI/UIManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "UI/Battle/ElfBattleIntro.h"
#include "UI/Battle/ElfBattleHUD.h"
#include "Elf/ElfBattleSceneActor.h"
#include "Elf/ElfBattleBase.h"
#include "Elf/ElfCharacterBase.h"
#include "Player/ElfPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Data/ElfBaseData.h"
#include "Game/ElfGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

UElfBattleManager::UElfBattleManager()
{
}

void UElfBattleManager::PlaySpawnAnimation(AActor* CreatureActor, float Duration)
{
	if (!CreatureActor) return;

	CreatureActor->SetActorScale3D(FVector::ZeroVector);

	FScaleAnim Anim;
	Anim.Actor = CreatureActor;
	Anim.Duration = Duration;
	ActiveAnimations.Add(Anim);

	if (!AnimTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimer(AnimTimerHandle, this, &UElfBattleManager::TickScaleAnimations, 0.016f, true);
	}
}

void UElfBattleManager::TickScaleAnimations()
{
	for (int32 i = ActiveAnimations.Num() - 1; i >= 0; --i)
	{
		FScaleAnim& Anim = ActiveAnimations[i];
		AActor* Actor = Anim.Actor.Get();
		if (!Actor)
		{
			ActiveAnimations.RemoveAt(i);
			continue;
		}

		Anim.Elapsed += 0.016f;
		float T = FMath::Clamp(Anim.Elapsed / Anim.Duration, 0.0f, 1.0f);
		FVector Scale = FMath::Lerp(Anim.StartScale, Anim.TargetScale, T);
		Actor->SetActorScale3D(Scale);

		if (T >= 1.0f)
		{
			ActiveAnimations.RemoveAt(i);
		}
	}

	if (ActiveAnimations.IsEmpty() && AnimTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AnimTimerHandle);
	}
}

void UElfBattleManager::Init(UUIManager* InUIManager, APlayerController* InOwner)
{
	UIManager = InUIManager;
	OwnerPC = InOwner;
}

void UElfBattleManager::StartBattle(APlayerController* Initiator, EBattleType Type, AActor* Opponent)
{
	BattleType = Type;
	OwnerPC = Initiator;

	BattleController = NewObject<UElfBattleController>(this);
	BattleController->Init(OwnerPC, Type, Opponent);

	ShowIntro();
}

void UElfBattleManager::ShowIntro()
{
	CurrentPhase = EBattlePhase::Intro;

	if (BattleController)
	{
		BattleController->OnPlayerReadyStateChanged.Clear();
		BattleController->OnPlayerReadyStateChanged.AddDynamic(this, &UElfBattleManager::OnPlayerReadyStateChanged);

		BattleController->OnIntroComplete.Clear();
		BattleController->OnIntroComplete.AddDynamic(this, &UElfBattleManager::OnIntroComplete);
	}

	if (UIManager)
	{
		IntroWidget = UIManager->OpenUI(IntroWidgetClass, BattleController);
	}
}

void UElfBattleManager::OnIntroComplete()
{
	CloseCurrentUI();
	EnterBattle();
}

void UElfBattleManager::OnPlayerReadyStateChanged(bool bIsReady)
{
	if (!bIsReady || !IntroWidget) return;

	bool bBothReady = false;

	switch (BattleType)
	{
	case EBattleType::Wild:
	case EBattleType::Trainer:
		bBothReady = true;
		break;

	case EBattleType::PvP:
		// TODO: 等待双方就绪
		break;
	}

	if (bBothReady)
	{
		Cast<UElfBattleIntro>(IntroWidget)->PlayExitAnimation();
	}
}

void UElfBattleManager::OnForcedSwitchRequested(EInfoSide Side, int32 NextSlotIndex)
{
	RecallCreature(Side);
	ReleaseCreature(Side, NextSlotIndex);

	if (TurnManager)
		TurnManager->OnForcedSwitchComplete();
}

void UElfBattleManager::OnTurnSwitchRequested(EInfoSide Side, int32 NextSlotIndex)
{
	RecallCreature(Side);
	ReleaseCreature(Side, NextSlotIndex);
}

void UElfBattleManager::OnTurnPhaseChanged(ETurnPhase NewPhase)
{
	if (BattleController)
	{
		BattleController->OnBattlePhaseChanged.Broadcast(NewPhase);
	}
}

void UElfBattleManager::OnTurnBattleEnded(EBattleResult Result)
{
	// 战斗结束清除所有 Buff
	if (UElfBattleModel* Model = BattleController ? BattleController->GetBattleModel() : nullptr)
	{
		for (FElfCreatureInstance& C : Model->PlayerSide.Team)
			C.ActiveBuffs.Empty();
		for (FElfCreatureInstance& C : Model->EnemySide.Team)
			C.ActiveBuffs.Empty();
		Model->PlayerSide.SideBuffs.Empty();
		Model->EnemySide.SideBuffs.Empty();
	}

	CloseCurrentUI();

	for (AActor* Creature : BattleCreatures)
	{
		if (Creature)
		{
			Creature->Destroy();
		}
	}
	BattleCreatures.Empty();

	if (AElfPlayerController* PC = Cast<AElfPlayerController>(OwnerPC))
	{
		PC->ExitBattleMode();
		PC->SaveGame();
	}
}

void UElfBattleManager::RecallCreature(EInfoSide Side)
{
	if (!BattleController) return;

	UElfBattleModel* Model = BattleController->GetBattleModel();
	if (!Model) return;

	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &Model->PlayerSide : &Model->EnemySide;
	if (SideData)
	{
		FElfCreatureInstance* Creature = SideData->GetActiveCreature();
		if (Creature)
		{
			// 切换精灵时取消愿力
			if (Side == EInfoSide::Self && Creature->bWishActive)
			{
				BattleController->CancelWish();
			}

			// 退场时清除非持久增益
			for (int32 i = Creature->ActiveBuffs.Num() - 1; i >= 0; i--)
			{
				if (!Creature->ActiveBuffs[i].bPersistent)
				{
					Creature->ActiveBuffs.RemoveAt(i);
				}
			}
		}
		SideData->MoveActiveToEnd();
	}
}

AActor* UElfBattleManager::ReleaseCreature(EInfoSide Side, int32 SlotIndex)
{
	if (!BattleController) return nullptr;

	UElfBattleModel* Model = BattleController->GetBattleModel();
	if (!Model) return nullptr;

	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &Model->PlayerSide : &Model->EnemySide;
	if (!SideData) return nullptr;

	if (SideData->Team.IsValidIndex(SlotIndex))
	{
		SideData->MoveToFront(SlotIndex);
	}

	AActor* SpawnPoint = nullptr;
	AElfBattleSceneActor* Scene = Cast<AElfBattleSceneActor>(
		UGameplayStatics::GetActorOfClass(OwnerPC->GetWorld(), AElfBattleSceneActor::StaticClass()));
	if (Scene)
	{
		SpawnPoint = (Side == EInfoSide::Self) ? Scene->PlayerSpawnPoint : Scene->EnemySpawnPoint;
	}

	if (!SideData->GetActiveCreature()) return nullptr;
	AActor* Spawned = SpawnCreature(*SideData->GetActiveCreature(), SpawnPoint);
	if (Spawned)
	{
		PlaySpawnAnimation(Spawned, 0.4f);
		if (BattleController)
		{
			BattleController->OnCreatureSwitched.Broadcast(Side);
			BattleController->BroadcastHP();
		}

		// 上场触发印记效果
		if (TurnManager)
		{
			TurnManager->OnCreatureEnteredField(Side);
		}
	}
	return Spawned;
}

AActor* UElfBattleManager::SpawnCreature(const FElfCreatureInstance& CreatureData, AActor* SpawnPoint)
{
	if (!OwnerPC || !SpawnPoint) return nullptr;

	UWorld* World = OwnerPC->GetWorld();
	UElfGameInstance* GI = OwnerPC->GetGameInstance<UElfGameInstance>();
	if (!World || !GI) return nullptr;

	FElfBaseData BaseData;
	if (!GI->GetElfBaseData(CreatureData.CreatureRowName, BaseData)) return nullptr;

	TSubclassOf<AActor> BPClass = BaseData.BattleBlueprint.LoadSynchronous();
	if (!BPClass) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FVector SpawnLoc = SpawnPoint->GetActorLocation();
	AActor* Spawned = World->SpawnActor<AActor>(BPClass, SpawnLoc, SpawnPoint->GetActorRotation(), Params);
	if (AElfCharacterBase* Char = Cast<AElfCharacterBase>(Spawned))
	{
		Char->CreatureData = CreatureData;

		UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
		if (Capsule)
		{
			SpawnLoc.Z += Capsule->GetScaledCapsuleHalfHeight();
			Char->SetActorLocation(SpawnLoc);
		}
	}

	if (Spawned)
	{
		BattleCreatures.Add(Spawned);
	}

	return Spawned;
}

void UElfBattleManager::SpawnBattleCreatures()
{
	if (!BattleController) return;

	AElfBattleSceneActor* Scene = Cast<AElfBattleSceneActor>(
		UGameplayStatics::GetActorOfClass(OwnerPC->GetWorld(), AElfBattleSceneActor::StaticClass()));
	if (!Scene) return;

	UElfBattleModel* Model = BattleController->GetBattleModel();
	if (!Model) return;

	if (BattleType == EBattleType::Wild)
	{
		SpawnCreature(Model->EnemySide.Team[Model->EnemySide.ActiveIndex], Scene->EnemySpawnPoint);
	}
}

void UElfBattleManager::EnterBattle()
{
	CurrentPhase = EBattlePhase::Battle;

	// Create TurnManager early so buff system can hook into creature releases
	TurnManager = NewObject<UElfTurnManager>(this);
	UElfBattleModel* Model = BattleController ? BattleController->GetBattleModel() : nullptr;
	TurnManager->Init(BattleController, Model);

	TurnManager->OnSwitchRequested.Clear();
	TurnManager->OnSwitchRequested.AddDynamic(this, &UElfBattleManager::OnTurnSwitchRequested);
	TurnManager->OnForcedSwitchRequested.Clear();
	TurnManager->OnForcedSwitchRequested.AddDynamic(this, &UElfBattleManager::OnForcedSwitchRequested);
	TurnManager->OnBattleEnded.Clear();
	TurnManager->OnBattleEnded.AddDynamic(this, &UElfBattleManager::OnTurnBattleEnded);
	TurnManager->OnTurnPhaseChanged.AddDynamic(this, &UElfBattleManager::OnTurnPhaseChanged);

	SpawnBattleCreatures();
	ReleaseCreature(EInfoSide::Self, BattleController ? BattleController->SelectedSlotIndex : 0);

	if (Model)
	{
		for (int32 i = 0; i < Model->PlayerSide.Team.Num(); ++i)
		{
			Model->PlayerSide.Team[i].CurrentEnergy = 10;
			if (Model->PlayerSide.CalculatedStats.IsValidIndex(i))
			{
				Model->PlayerSide.Team[i].CurrentHP = Model->PlayerSide.CalculatedStats[i].MaxHP;
			}
		}
		for (int32 i = 0; i < Model->EnemySide.Team.Num(); ++i)
		{
			Model->EnemySide.Team[i].CurrentEnergy = 10;
			if (Model->EnemySide.CalculatedStats.IsValidIndex(i))
			{
				Model->EnemySide.Team[i].CurrentHP = Model->EnemySide.CalculatedStats[i].MaxHP;
			}
		}

		if (BattleType == EBattleType::Trainer && Model->EnemySide.Team.Num() > 0)
		{
			int32 RandomIdx = FMath::RandRange(0, Model->EnemySide.Team.Num() - 1);
			ReleaseCreature(EInfoSide::Enemy, RandomIdx);
		}

		// Wild: enemy creature already spawned via SpawnBattleCreatures, trigger its enter effects
		if (BattleType == EBattleType::Wild)
		{
			TurnManager->OnCreatureEnteredField(EInfoSide::Enemy);
		}
	}

	if (UIManager)
	{
		if (IntroWidget)
		{
			UIManager->CloseUI(IntroWidget);
			IntroWidget = nullptr;
		}
		BattleWidget = UIManager->OpenUI(BattleWidgetClass, BattleController);
	}

	if (BattleController)
	{
		BattleController->OnCreatureSwitched.Broadcast(EInfoSide::Self);
		if (BattleType == EBattleType::Wild || BattleType == EBattleType::Trainer)
		{
			BattleController->OnCreatureSwitched.Broadcast(EInfoSide::Enemy);
		}
		BattleController->BroadcastHP();
	}

	TurnManager->StartTurn();
}

void UElfBattleManager::CloseCurrentUI()
{
	if (UIManager)
	{
		if (IntroWidget) { UIManager->CloseUI(IntroWidget); IntroWidget = nullptr; }
		if (BattleWidget) { UIManager->CloseUI(BattleWidget); BattleWidget = nullptr; }
	}
}

void UElfBattleManager::SkipToBattle()
{
	if (CurrentPhase == EBattlePhase::Intro)
	{
		if (BattleController)
		{
			BattleController->SelectCreature(0);
		}
		OnIntroComplete();
	}
}

void UElfBattleManager::HandleInput(const FGameplayTag& InputTag)
{
	if (BattleController)
	{
		BattleController->HandleInput(InputTag);
	}
}
