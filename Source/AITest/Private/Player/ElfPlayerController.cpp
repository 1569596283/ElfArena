#include "Player/ElfPlayerController.h"

#include "ElfGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Input/ElfInputComponent.h"
#include "GameFramework/Character.h"
#include "Game/ElfSaveGame.h"
#include "Game/ElfGameInstance.h"
#include "Data/NPCData.h"
#include "Data/ElfBaseData.h"
#include "Data/ElfSkillData.h"
#include "UI/ElfGMWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ElfPlayerState.h"
#include "Elf/ElfManager.h"
#include "Elf/ElfBattleSceneActor.h"
#include "UI/UIManager.h"
#include "Battle/ElfBattleManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"

static FElfCreatureInstance MakeCreatureFromMember(const FElfMemberData& Member)
{
	FElfCreatureInstance Instance;
	Instance.CreatureRowName = Member.CreatureRowName;
	Instance.Level = Member.Level;
	Instance.Sex = Member.Sex;
	Instance.NatureID = Member.NatureID;
	Instance.EquippedSkills = Member.Skills;
	return Instance;
}

void AElfPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(WorldInputContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem) {
		Subsystem->AddMappingContext(WorldInputContext, 0);
	}

	PlayerCameraManager->ViewPitchMin = -60.0f;
	PlayerCameraManager->ViewPitchMax = 50.0f;

	if (HasAuthority())
	{
		UElfSaveGame* Save = Cast<UElfSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Default"), 0));
		if (Save)
		{
			LoadGame(TEXT("Default"));
		}
		else
		{
			InitDefaultTeam();
		}
	}

	UIManager = NewObject<UUIManager>(this);
	UIManager->Init(this);

	// GM 面板进入游戏时自动打开
	OpenGM();
}

void AElfPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AElfPlayerController, bIsInBattle);
}

void AElfPlayerController::InitDefaultTeam()
{
	AElfPlayerState* PS = GetPlayerState<AElfPlayerState>();
	UElfGameInstance* GI = GetGameInstance<UElfGameInstance>();
	if (!PS || !GI || GI->StartingTeam.IsEmpty()) return;

	PS->GetTeamCreatures().Empty();

	for (const FName& MemberID : GI->StartingTeam)
	{
		FElfMemberData Member;
		if (GI->GetElfMemberData(MemberID, Member))
		{
			PS->GetTeamCreatures().Add(MakeCreatureFromMember(Member));
		}
	}
}

void AElfPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UElfInputComponent* ElfInputComponent = CastChecked<UElfInputComponent>(InputComponent);

	ElfInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AElfPlayerController::Move);
	ElfInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AElfPlayerController::Look);
	ElfInputComponent->BindActions(InputConfig, this, &AElfPlayerController::InputTagPressed, &AElfPlayerController::InputTagReleased, &AElfPlayerController::InputTagHeld);
}

void AElfPlayerController::Look(const FInputActionValue& InputActionValue)
{
	if (bIsInBattle) return;

	FVector2D LookVector = InputActionValue.Get<FVector2D>();
	AddYawInput(LookVector.X * LookYawSensitivity);
	AddPitchInput(LookVector.Y * LookPitchSensitivity * (bInvertPitch ? -1.f : 1.f));
}

void AElfPlayerController::Move(const FInputActionValue& InputActionValue)
{
	if (bIsInBattle) return;

	FVector2D MovementVector = InputActionValue.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn())
	{
		FRotator YawRotation(0, GetControlRotation().Yaw, 0);

		FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		ControlledPawn->AddMovementInput(Forward, MovementVector.Y);
		ControlledPawn->AddMovementInput(Right, MovementVector.X);
	}
}

void AElfPlayerController::JumpStarted()
{
	if (bIsInBattle) return;

	if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
	{
		Char->Jump();
	}
}

void AElfPlayerController::JumpCompleted()
{
	if (bIsInBattle) return;

	if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
	{
		Char->StopJumping();
	}
}

void AElfPlayerController::OnGMWidgetClosed()
{
	CloseGMWidget();
}

void AElfPlayerController::CloseGMWidget()
{
	if (GMWidget)
	{
		GMWidget->RemoveFromParent();
		GMWidget = nullptr;
	}
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AElfPlayerController::OpenGM()
{
	// 只有本地玩家控制器才能创建/拥有 UI（避免服务器上为远程玩家控制器建 Widget 报错）
	if (!IsLocalController()) return;

	// 进入战斗后本次运行不再弹出
	if (bGMDismissed) return;

	if (GMWidget)
	{
		CloseGMWidget();
		return;
	}

	if (!UIManager) return;

	UClass* WidgetClass = GMWidgetClass ? GMWidgetClass.Get() : UElfGMWidget::StaticClass();
	GMWidget = UIManager->OpenUI(WidgetClass, this);
	if (GMWidget)
	{
		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
	}
}

bool AElfPlayerController::GMReplaceElf(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex)
{
	// 非服务器：发给服务器执行，保证各客户端都拿到自己的正确队伍
	if (!HasAuthority())
	{
		Server_GMReplaceElf(ElfRowName, SkillRowNames, SlotIndex);
		return true;
	}
	return GMReplaceElf_Authority(ElfRowName, SkillRowNames, SlotIndex);
}

void AElfPlayerController::Server_GMReplaceElf_Implementation(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex)
{
	GMReplaceElf_Authority(ElfRowName, SkillRowNames, SlotIndex);
}

bool AElfPlayerController::GMReplaceElf_Authority(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex)
{
	AElfPlayerState* PS = GetPlayerState<AElfPlayerState>();
	UElfGameInstance* GI = GetGameInstance<UElfGameInstance>();
	if (!PS || !GI) return false;

	FElfBaseData BaseData;
	if (ElfRowName.IsNone() || !GI->GetElfBaseData(ElfRowName, BaseData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] 精灵行名无效: %s"), *ElfRowName.ToString());
		return false;
	}

	FElfCreatureInstance NewElf;

	// 等级继承（优先目标索引；目标索引越界时取第 0 只；队伍空则随机）
	TArray<FElfCreatureInstance>& Team = PS->GetTeamCreatures();
	int32 TargetIndex = FMath::Max(0, SlotIndex);
	int32 InheritIdx = Team.IsValidIndex(TargetIndex) ? TargetIndex : (Team.IsValidIndex(0) ? 0 : -1);
	if (InheritIdx >= 0)
	{
		const FElfCreatureInstance& Old = Team[InheritIdx];
		NewElf.CreatureID = Old.CreatureID;
		NewElf.Level = Old.Level;
		NewElf.Exp = Old.Exp;
		NewElf.Sex = Old.Sex;
		NewElf.NatureID = Old.NatureID;
		NewElf.bShiny = Old.bShiny;
		NewElf.CurrentEnergy = Old.CurrentEnergy;
	}
	else
	{
		NewElf.Level = FMath::RandRange(1, 50);
		NewElf.Sex = (FMath::RandBool() ? EElfSex::Male : EElfSex::Female);
	}

	// 个体值随机 0~31，努力值清零
	NewElf.IV_HP = FMath::RandRange(0, 31);
	NewElf.IV_ATK = FMath::RandRange(0, 31);
	NewElf.IV_MATK = FMath::RandRange(0, 31);
	NewElf.IV_DEF = FMath::RandRange(0, 31);
	NewElf.IV_MDEF = FMath::RandRange(0, 31);
	NewElf.IV_SPD = FMath::RandRange(0, 31);
	NewElf.EV_HP = NewElf.EV_ATK = NewElf.EV_MATK = NewElf.EV_DEF = NewElf.EV_MDEF = NewElf.EV_SPD = 0;

	NewElf.CreatureRowName = ElfRowName;

	// 技能：先用有效的输入技能，不足4个时从该精灵可学技能随机补充到4个（不重复、过滤无效技能）
	TArray<FName> LearnPool;
	for (const FSskillLearnCondition& Cond : BaseData.LearnableSkills)
	{
		FSkillData Tmp;
		if (!Cond.SkillID.IsNone() && GI->GetSkillData(Cond.SkillID, Tmp))
			LearnPool.AddUnique(Cond.SkillID);
	}
	if (LearnPool.IsEmpty() && GI->DefaultSkillIDs.Num() > 0)
		LearnPool = GI->DefaultSkillIDs;

	for (int32 i = 0; i < 4; i++)
	{
		FName SkillRow = SkillRowNames.IsValidIndex(i) ? SkillRowNames[i] : NAME_None;
		FSkillData SkillData;
		if (!SkillRow.IsNone() && GI->GetSkillData(SkillRow, SkillData) && !NewElf.EquippedSkills.Contains(SkillRow))
		{
			NewElf.EquippedSkills.Add(SkillRow);
		}
	}

	// 补到 4 个：随机从可学技能取，避免与已有技能重复
	while (NewElf.EquippedSkills.Num() < 4 && !LearnPool.IsEmpty())
	{
		FName Fill = NAME_None;
		for (int32 Try = 0; Try < 8; Try++)
		{
			FName Cand = LearnPool[FMath::RandRange(0, LearnPool.Num() - 1)];
			if (!NewElf.EquippedSkills.Contains(Cand))
			{
				Fill = Cand;
				break;
			}
		}
		if (Fill.IsNone())
			break;
		NewElf.EquippedSkills.Add(Fill);
	}

	// 清战斗状态
	NewElf.ActiveBuffs.Empty();
	NewElf.LastUsedSkillType = ESkillType::Attack;
	NewElf.bWishActive = false;
	NewElf.bPendingEvolution = false;
	NewElf.BackupFirstSkill = FName();

	// 替换指定索引（越界按最后一只；队伍空则新增）
	if (Team.Num() == 0)
	{
		Team.Add(NewElf);
	}
	else
	{
		TargetIndex = FMath::Min(TargetIndex, Team.Num() - 1);
		Team[TargetIndex] = NewElf;
	}

	SaveGame(TEXT("Default"));

	FString SkillsStr;
	for (const FName& S : NewElf.EquippedSkills)
		SkillsStr += S.ToString() + TEXT(" ");
	UE_LOG(LogTemp, Warning, TEXT("[GM] 已替换第%d只精灵为 %s Lv.%d 技能[%s]"), TargetIndex + 1, *ElfRowName.ToString(), NewElf.Level, *SkillsStr);
	return true;
}

void AElfPlayerController::Client_EnterBattleMode_Implementation(AActor* Opponent, EBattleType BattleType)
{
	EnterBattleMode(Opponent, BattleType);
}

void AElfPlayerController::EnterBattleMode(AActor* Opponent, EBattleType BattleType)
{
	if (bIsInBattle) return;
	bIsInBattle = true;

	// 进入战斗：关闭 GM 面板，本次运行不再弹出
	bGMDismissed = true;
	CloseGMWidget();

	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());

	SavedViewTarget = GetViewTarget();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem) return;

	if (WorldInputContext)
	{
		Subsystem->RemoveMappingContext(WorldInputContext);
	}

	if (BattleInputContext)
	{
		Subsystem->AddMappingContext(BattleInputContext, 1);
	}

	UClass* BMClass = BattleManagerClass ? BattleManagerClass.Get() : UElfBattleManager::StaticClass();
	BattleManager = NewObject<UElfBattleManager>(this, BMClass);
	BattleManager->Init(UIManager, this);
	BattleManager->StartBattle(this, BattleType, Opponent);

	if (BattleManager->GetBattleController())
	{
		BattleManager->GetBattleController()->OnCameraRequested.AddDynamic(this, &AElfPlayerController::MoveCameraToBattle);
		BattleManager->GetBattleController()->OnIntroComplete.AddDynamic(this, &AElfPlayerController::MoveCameraToBattle);
	}
}

void AElfPlayerController::ExitBattleMode()
{
	bIsInBattle = false;
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	MoveCameraBackToPlayer();
	SavedViewTarget = nullptr;

	BattleManager = nullptr;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem) return;

	if (BattleInputContext)
	{
		Subsystem->RemoveMappingContext(BattleInputContext);
	}
	if (WorldInputContext)
	{
		Subsystem->AddMappingContext(WorldInputContext, 0);
	}
}

void AElfPlayerController::MoveCameraToBattle()
{
	if (bCameraAtBattle) return;

	AElfBattleSceneActor* BattleScene = Cast<AElfBattleSceneActor>(
		UGameplayStatics::GetActorOfClass(this, AElfBattleSceneActor::StaticClass()));
	if (BattleScene && BattleScene->BattleCameraActor)
	{
		SetViewTarget(BattleScene->BattleCameraActor);
		bCameraAtBattle = true;
	}
}

void AElfPlayerController::MoveCameraBackToPlayer()
{
	if (SavedViewTarget)
	{
		SetViewTarget(SavedViewTarget);
		bCameraAtBattle = false;
	}
}

void AElfPlayerController::InputTagPressed(FGameplayTag InputTag)
{
	if (bIsInBattle && BattleManager)
	{
		BattleManager->HandleInput(InputTag);
		return;
	}

	if (InputTag == FElfGameplayTags::Get().Input_Jump)
	{
		JumpStarted();
	}
}

void AElfPlayerController::InputTagReleased(FGameplayTag InputTag)
{
	if (InputTag == FElfGameplayTags::Get().Input_Jump)
	{
		JumpCompleted();
	}
}

void AElfPlayerController::InputTagHeld(FGameplayTag InputTag)
{
}

void AElfPlayerController::SaveGame(const FString& SlotName)
{
	AElfPlayerState* PS = GetPlayerState<AElfPlayerState>();
	if (!PS) return;

	UElfSaveGame* Save = NewObject<UElfSaveGame>();
	Save->TeamCreatures = PS->GetTeamCreatures();
	Save->WarehouseCreatures = PS->GetWarehouseCreatures();

	if (APawn* ControlledPawn = GetPawn())
	{
		Save->PlayerLocation = ControlledPawn->GetActorLocation();
		Save->PlayerRotation = ControlledPawn->GetActorRotation();
	}

	UElfManager* Manager = GetGameInstance()->GetSubsystem<UElfManager>();
	if (Manager)
	{
		Save->WildCreatureCache = Manager->GetAllCreatureCache();
	}

	UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}

void AElfPlayerController::LoadGame(const FString& SlotName)
{
	UElfSaveGame* Save = Cast<UElfSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save) return;

	AElfPlayerState* PS = GetPlayerState<AElfPlayerState>();
	if (!PS) return;

	PS->GetTeamCreatures() = Save->TeamCreatures;
	PS->GetWarehouseCreatures() = Save->WarehouseCreatures;

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->SetActorLocation(Save->PlayerLocation);
		ControlledPawn->SetActorRotation(Save->PlayerRotation);
	}

	UElfManager* Manager = GetGameInstance()->GetSubsystem<UElfManager>();
	if (Manager)
	{
		Manager->RestoreCreatureCache(Save->WildCreatureCache);
	}
}
