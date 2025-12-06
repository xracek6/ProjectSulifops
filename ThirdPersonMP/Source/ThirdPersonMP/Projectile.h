// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class THIRDPERSONMP_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Sphere component used to test collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class USphereComponent> SphereComponent;
	
	// Static mesh used to provide a visual representation of the projectile object
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;
	
	// Movement component for handling projectile movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovementComponent;
	
	// Particle used when the projectile impacts another object and explodes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TObjectPtr<class UParticleSystem> ExplosionEffect;
	
	// Sound effect that will play on projectile impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TObjectPtr<class USoundBase> SoundEffect;
	
	// Damage type that will be dealt by this projectile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Damage")
	TSubclassOf<class UDamageType> DamageType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	float Damage;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void Destroyed() override;
	
	UFUNCTION(Category="Projectile")
	void OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPCSpawnExplosion();
};
