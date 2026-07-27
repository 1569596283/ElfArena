#include "Player/ElfPlayerController.h"

#include "ElfGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Input/ElfInputComponent.h"
#include "GameFramework/Character.h"
#include "Game/ElfSaveGame.h"
#include "Game/ElfGameInstance.h"
#include "Data/NPCData.h"
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
		InitDefaultTeam();
	}

	UIManager = NewObject<UUIManager>(this);
	UIManager->Init(this);
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

void AElfPlayerController::Client_EnterBattleMode_Implementation(AActor* Opponent, EBattleType BattleType)
{
	EnterBattleMode(Opponent, BattleType);
}

void AElfPlayerController::EnterBattleMode(AActor* Opponent, EBattleType BattleType)
{
	if (bIsInBattle) return;
	bIsInBattle = true;
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
