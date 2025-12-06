// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "ThirdPersonMP.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AProjectile::AProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Definition for the SphereComponent that will serve as the Root component for the projectile and its collision
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	SphereComponent->InitSphereRadius(37.5f);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = SphereComponent;

	if (HasAuthority())
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnProjectileImpact);
	}

	// todo: this vs blueprint?
	// Definition for the Mesh that will serve as visual representation of the projectile
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);

	// Set the Static Mesh and its position/scale if mesh asset was successfully found
	if (DefaultMesh.Succeeded())
	{
		StaticMeshComponent->SetStaticMesh(DefaultMesh.Object);
		StaticMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -37.5f));
		StaticMeshComponent->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	}

	// Explosion effect + sound effect is now set in derived blueprint (BP_Projectile)

	// // Set explosion effect
	// static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	// if (DefaultExplosionEffect.Succeeded())
	// {
	// 	ExplosionEffect = DefaultExplosionEffect.Object;
	// }
	//
	// // Set sound effect
	// static ConstructorHelpers::FObjectFinder<USoundWave> DefaultSoundEffect(TEXT("/Game/StarterContent/Audio/Explosion01.Explosion01"));
	// if (DefaultSoundEffect.Succeeded())
	// {
	// 	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString(TEXT("Successfully loaded sound effect")));
	// 	SoundEffect = DefaultSoundEffect.Object;
	// }

	// Definition for the ProjectileMovementComponent
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->SetUpdatedComponent(SphereComponent);
	ProjectileMovementComponent->InitialSpeed = 2000.0f;
	ProjectileMovementComponent->MaxSpeed = 2000.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	// Set damage
	DamageType = UDamageType::StaticClass();
	Damage = 10.0f;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectile::Destroyed()
{
	MulticastRPCSpawnExplosion();
	// const FString message = FString::Printf(TEXT("Local role in Destroyed: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);
}

// ReSharper disable once CppPassValueParameterByConstReference
void AProjectile::OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                                     const FHitResult& Hit)
{
	// const FString message = FString::Printf(TEXT("Local role in OnProjectileImpact: %d."), GetLocalRole());
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, message);
	if (OtherActor)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, NormalImpulse, Hit, GetInstigator()->Controller, this, DamageType);
	}

	Destroy();
}

void AProjectile::MulticastRPCSpawnExplosion_Implementation()
{
	const UWorld* World = GetWorld();

	if (World == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("GetWorld() returned nullptr in AProjectile::MulticastRPCSpawnExplosion_Implementation()"));
		return;
	}

	// Don't spawn explosion or play sound effect on dedicated server
	if (World->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator, true, EPSCPoolMethod::AutoRelease);
	UGameplayStatics::PlaySoundAtLocation(this, SoundEffect, GetActorLocation(), 0.5f);
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
