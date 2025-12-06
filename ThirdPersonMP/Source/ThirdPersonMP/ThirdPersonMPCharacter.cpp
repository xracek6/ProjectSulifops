// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonMPCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Projectile.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ThirdPersonMP.h"
#include "ThirdPersonMPPlayerController.h"
#include "Engine/StaticMeshActor.h"

AThirdPersonMPCharacter::AThirdPersonMPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

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
	
	// Create first person camera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh());
	
	SetThirdPersonCamera();
	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	
	// Initialize projectile class.
	// This should be set in BP_ThirdPersonMPCharacter
	// ProjectileClass = AProjectile::StaticClass();
	
	// Initialize fire rate.
	FireRate = 0.15f;
	bIsFiringWeapon = false;
}

// FirstPersonCamera gets attached to the "head" socket of the Mesh component in this method instead of constructor because sockets are not initialized yet in constructor
void AThirdPersonMPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (GetMesh())
	{
		TArray<FName> SocketNames = GetMesh()->GetAllSocketNames();
		UE_LOG(LogThirdPersonMP, Warning, TEXT("Mesh exists obtaining all sockets:"));
		for (const FName& SocketName : SocketNames)
		{

			UE_LOG(LogThirdPersonMP, Warning, TEXT("Socket: %s"), *SocketName.ToString());
		}
		if (SocketNames.Num() == 0)
		{
			UE_LOG(LogThirdPersonMP, Warning, TEXT("No sockets"));
		}
	}
	else
	{
		UE_LOG(LogThirdPersonMP, Warning, TEXT("No Mesh"));
	}
	
	FirstPersonCamera->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("head"));
	// Position the camera slightly above the eyes and rotate it to behind the player's head
	FirstPersonCamera->SetRelativeLocationAndRotation(FVector(2.8f, 8.9f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;
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
	ServerRPCHandleFire();
}

void AThirdPersonMPCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void AThirdPersonMPCharacter::StartSprint()
{
	SetMaxWalkSpeed(this->SprintingMaxWalkSpeed);
	
	// if this character has no authority, also start sprinting on server via rpc
	if (!HasAuthority())
	{
		// todo: limit server calls with timer?
		ServerRPCStartSprint();
	}
}

void AThirdPersonMPCharacter::StopSprint()
{
	SetMaxWalkSpeed(this->DefaultMaxWalkSpeed);
	
	// if this character has no authority, also stop sprinting on server via rpc
	if (!HasAuthority())
	{
		// todo: limit server calls with timer?
		ServerRPCStopSprint();
	}
}

void AThirdPersonMPCharacter::ServerRPCHandleFire_Implementation()
{
	// const FString message = FString::Printf(TEXT("Local role in HandleFire: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);

	FVector SpawnLocation = GetActorLocation() + (GetActorRotation().Vector()  * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator SpawnRotation = GetActorRotation();
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.Owner = this;

	[[maybe_unused]] AProjectile* spawnedProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParameters);
}


void AThirdPersonMPCharacter::ServerRPCStartSprint_Implementation()
{
	// const FString message = FString::Printf(TEXT("Local role in ServerStartSprint: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);
	SetMaxWalkSpeed(SprintingMaxWalkSpeed);
}

void AThirdPersonMPCharacter::ServerRPCStopSprint_Implementation()
{
	// const FString message = FString::Printf(TEXT("Local role in ServerStopSprint: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);
	SetMaxWalkSpeed(DefaultMaxWalkSpeed);
}

void AThirdPersonMPCharacter::ServerRPCSpawnStaticMeshActor_Implementation()
{
	if (StaticMeshToSpawn == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("Unable to spawn Static Mesh Actor in AThirdPersonMPCharacter::ServerRPCSpawnStaticMeshActor_Implementation() as StaticMeshToSpawn is nullptr"));
		return;
	}
	
	AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass());
	if (StaticMeshActor == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("StaticMeshActor is nullptr in AThirdPersonMPCharacter::ServerRPCSpawnStaticMeshActor_Implementation()"));
		return;
	}
	
	StaticMeshActor->SetReplicates(true);
	StaticMeshActor->SetReplicateMovement(true);
	StaticMeshActor->SetMobility(EComponentMobility::Movable);
	
	// Spawn location set to 300 units in front and 100 units above the current character location
	FVector SpawnLocation = GetActorLocation() + GetActorRotation().Vector() * 300.0f + GetActorUpVector() * 100.0f;
	StaticMeshActor->SetActorLocation(SpawnLocation);

	UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
	if (StaticMeshComponent == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("StaticMeshComponent is nullptr in AThirdPersonMPCharacter::ServerRPCSpawnStaticMeshActor_Implementation()"));
		return;
	}
	
	StaticMeshComponent->SetIsReplicated(true);
	StaticMeshComponent->SetSimulatePhysics(true);
	
	if (StaticMeshMaterial == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("StaticMeshMaterial is nullptr in AThirdPersonMPCharacter::ServerRPCSpawnStaticMeshActor_Implementation()"));
	} else
	{
		StaticMeshComponent->SetMaterial(0, StaticMeshMaterial);
	}
	
	StaticMeshComponent->SetStaticMesh(StaticMeshToSpawn);
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
		
		// Menu
		EnhancedInputComponent->BindAction(OpenMenuAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::ToggleMenu);
		
		// Camera toggle
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::ToggleCamera);
		
		// Spawn static mesh actor
		EnhancedInputComponent->BindAction(SpawnStaticMeshActorAction, ETriggerEvent::Triggered, this, &AThirdPersonMPCharacter::SpawnStaticMeshActor);
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
		const FString HealthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
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
	AThirdPersonMPPlayerController* MyController = GetController<AThirdPersonMPPlayerController>();
	
	if (MyController == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("Player Controller is null in AThirdPersonMPCharacter::ToggleMenu()"));
		
		const FString Message = FString(TEXT("Player Controller is null in AThirdPersonMPCharacter::ToggleMenu()"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
		return;
	}

	MyController->ToggleMenu();
}

void AThirdPersonMPCharacter::ToggleCamera()
{
	FirstPersonCamera->IsActive() ? SetThirdPersonCamera() : SetFirstPersonCamera();
}

void AThirdPersonMPCharacter::SetFirstPersonCamera()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;

	FollowCamera->SetActive(false, true);
	FirstPersonCamera->SetActive(true, true);
}

void AThirdPersonMPCharacter::SetThirdPersonCamera()
{
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	FirstPersonCamera->SetActive(false, true);
	FollowCamera->SetActive(true, true);
}

void AThirdPersonMPCharacter::SpawnStaticMeshActor()
{
	// todo: limit server calls
	// todo: move spawning to separate class?
	ServerRPCSpawnStaticMeshActor();
}

void AThirdPersonMPCharacter::DoMove(const float Right, const float Forward)
{
	if (GetController() == nullptr)
	{
		return;
	}
	
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

void AThirdPersonMPCharacter::DoLook(const float Yaw, const float Pitch)
{
	if (GetController() == nullptr)
	{
		return;
	}
	// add yaw and pitch input to controller
	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
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
	if (HasAuthority())
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
