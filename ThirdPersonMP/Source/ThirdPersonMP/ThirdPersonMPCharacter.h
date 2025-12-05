// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ThirdPersonMPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AThirdPersonMPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AThirdPersonMPCharacter();
	
	virtual void PostInitializeComponents() override;
	
	// Property replication
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
		
	// Handles move inputs from either controls or UI interfaces
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	// Handles look inputs from either controls or UI interfaces
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	// Handles jump pressed inputs from either controls or UI interfaces
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	// Handles jump pressed inputs from either controls or UI interfaces 
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	// Returns CameraBoom subobject
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// Returns FollowCamera subobject
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	// Getter for max health
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	
	// Getter for current health
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }
	
	// Setter for current health. Clamps the value between 0 nad MaxHealth and calls OnHealthUpdate. Should only be called on the server.
	UFUNCTION(BlueprintCallable, Category="Health")
	void SetCurrentHealth(float HealthValue);
	
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

private:
	// Camera boom positioning the camera behind the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Follow camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;
	
protected:
	static constexpr float DefaultMaxWalkSpeed = 500.0f;
	static constexpr float SprintingMaxWalkSpeed = 1000.0f;

	// Jump Input Action
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	// Move Input Action
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;
	
	// Sprint Input Action
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;

	// Look Input Action
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	// Mouse Look Input Action
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;
	
	// Fire Input Action
	UPROPERTY(EditAnyWhere, Category="Input")
	TObjectPtr<UInputAction> FireAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> OpenMenuAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ToggleCameraAction;
	
	UPROPERTY(EditDefaultsOnly, Category="Health")
	float MaxHealth;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;
	
	UPROPERTY(EditDefaultsOnly, Category="Gameplay|Projectile")
	TSubclassOf<class AThirdPersonMPProjectile> ProjectileClass;
	
	// Delay between shots in seconds. Used to control fire rate for your test projectile, but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.
	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	float FireRate;
	
	// If true, character is in process of firing projectiles.
	bool bIsFiringWeapon;
	
	// Function for beginning weapon fire.
	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StartFire();
	
	// Function for ending weapon fire. Once this is called, the player can use StartFire again.
	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StopFire();
	
	// Server function for spawning projectiles.
	UFUNCTION(Server, Reliable)
	void HandleFire();
	
	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StartSprint();
	
	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StopSprint();
	
	UFUNCTION(Server, Reliable)
	void ServerStartSprint();
	
	UFUNCTION(Server, Reliable)
	void ServerStopSprint();
	
	void SetMaxWalkSpeed(float MaxWalkSpeed) const;
	
	// A timer handle used for providing the fire rate delay in-between spawns.
	FTimerHandle FiringTimer;
	
	UFUNCTION()
	void OnRep_CurrentHealth() const;

	// Initialize input action bindings
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Called for movement input
	void Move(const FInputActionValue& Value);

	// Called for looking input
	void Look(const FInputActionValue& Value);
	
	// Response to health being updated. Called on the server immediately after modification, and on clients in response to a RepNotify
	void OnHealthUpdate() const;
	
	void ToggleMenu();
	
	void ToggleCamera();
	
	void SetFirstPersonCamera();
	
	void SetThirdPersonCamera();
};

