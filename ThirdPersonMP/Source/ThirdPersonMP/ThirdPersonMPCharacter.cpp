// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonMPCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "ThirdPersonMPProjectile.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MenuWidget.h"
#include "ThirdPersonMP.h"
#include "ThirdPersonMPPlayerController.h"
#include "Blueprint/UserWidget.h"

AThirdPersonMPCharacter::AThirdPersonMPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	
	// Initialize projectile class.
	ProjectileClass = AThirdPersonMPProjectile::StaticClass();
	
	// Initialize fire rate.
	FireRate = 0.15f;
	bIsFiringWeapon = false;
}

void AThirdPersonMPCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Replicate current health
	DOREPLIFETIME(AThirdPersonMPCharacter, CurrentHealth);
}

void AThirdPersonMPCharacter::StartFire()
{
	if (bIsFiringWeapon)
	{
		return;
	}
	
	bIsFiringWeapon = true;
	const UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(FiringTimer, this, &AThirdPersonMPCharacter::StopFire, FireRate, false);
	HandleFire();
}

void AThirdPersonMPCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void AThirdPersonMPCharacter::StartSprint()
{
	SetMaxWalkSpeed(this->SprintingMaxWalkSpeed);
	// todo: limit server calls
	ServerStartSprint();
}

void AThirdPersonMPCharacter::StopSprint()
{
	SetMaxWalkSpeed(this->DefaultMaxWalkSpeed);
	// todo: limit server calls
	ServerStopSprint();
}

void AThirdPersonMPCharacter::HandleFire_Implementation()
{
	// const FString message = FString::Printf(TEXT("Local role in HandleFire: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);

	const FVector SpawnLocation = GetActorLocation() + (GetActorRotation().Vector()  * 100.0f) + (GetActorUpVector() * 50.0f);
	const FRotator SpawnRotation = GetActorRotation();
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.Owner = this;

	[[maybe_unused]] AThirdPersonMPProjectile* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonMPProjectile>(SpawnLocation, SpawnRotation, SpawnParameters);
}


void AThirdPersonMPCharacter::ServerStartSprint_Implementation()
{
	// const FString message = FString::Printf(TEXT("Local role in ServerStartSprint: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);
	SetMaxWalkSpeed(SprintingMaxWalkSpeed);
}

void AThirdPersonMPCharacter::ServerStopSprint_Implementation()
{
	// const FString message = FString::Printf(TEXT("Local role in ServerStopSprint: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);
	SetMaxWalkSpeed(DefaultMaxWalkSpeed);
}

void AThirdPersonMPCharacter::SetMaxWalkSpeed(const float MaxWalkSpeed) const
{
	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
}

void AThirdPersonMPCharacter::OnRep_CurrentHealth() const
{
	OnHealthUpdate();
}

void AThirdPersonMPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::Look);
		
		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AThirdPersonMPCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AThirdPersonMPCharacter::StopSprint);
		
		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::Look);
		
		// Firing projectiles
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::StartFire);
		
		EnhancedInputComponent->BindAction(OpenMenuAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::ToggleMenu);
	}
	else
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AThirdPersonMPCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AThirdPersonMPCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AThirdPersonMPCharacter::OnHealthUpdate() const
{
	// Client-specific functionality
	if (IsLocallyControlled())
	{
		const FString HealthMessage = FString::Printf(TEXT("You now have %f remaining."), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, HealthMessage);
		
		if (CurrentHealth <= 0)
		{
			const FString DeathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, DeathMessage);
		}
	}
	
	
	if (HasAuthority())
	{
		const FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}
	
	//Functions that occur on all machines.
	/*
		Any special functionality that should occur as a result of damage or death should be placed here.
	*/
}

void AThirdPersonMPCharacter::ToggleMenu()
{
	AThirdPersonMPPlayerController* MyController = Cast<AThirdPersonMPPlayerController>(GetController());
	
	if (MyController == nullptr)
	{
		const FString Message = FString::Printf(TEXT("Controller is null in AThirdPersonMPCharacter::OpenMenu()"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
		return;
	}
	
	const FString Message = FString::Printf(TEXT("Controller is valid in AThirdPersonMPCharacter::OpenMenu()"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);

	const TObjectPtr<UUserWidget> Menu = MyController->GetMenuWidget();
	
	if (Menu->IsInViewport())
	{
		Menu->RemoveFromParent();

		FInputModeGameOnly GameInputMode;
		MyController->SetInputMode(GameInputMode);
		MyController->SetShowMouseCursor(false);
		MyController->SetIgnoreMoveInput(false);
		return;
	}
	
	Menu->AddToViewport(0);
	
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(Menu->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	
	MyController->SetIgnoreMoveInput(true);
	MyController->SetInputMode(Mode);
	MyController->SetShowMouseCursor(true);
}

void AThirdPersonMPCharacter::DoMove(const float Right, const float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AThirdPersonMPCharacter::DoLook(const float Yaw, const float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AThirdPersonMPCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}


void AThirdPersonMPCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AThirdPersonMPCharacter::SetCurrentHealth(const float HealthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{ 
		CurrentHealth = FMath::Clamp(HealthValue, 0.0f, MaxHealth);
		OnHealthUpdate();
	}
}

float AThirdPersonMPCharacter::TakeDamage(const float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	const float NewHealth = CurrentHealth - DamageAmount;
	SetCurrentHealth(NewHealth);
	return NewHealth;
}
