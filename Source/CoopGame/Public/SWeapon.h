// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShake;

// Contains information of a single hitscan weapon linetrace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};


UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

protected:

	virtual void BeginPlay() override;

// ------- COMPONENTS ------- \\

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

// ------- FUNCTIONS ------- \\

	void Fire();

	void Reload();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	UFUNCTION()
	void OnRep_HitScanTrace();

	void PlayFireEffects(FVector TraceEnd);

	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

// ------- VARIABLES ------- \\

//FName

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

//Float

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	float LastFireTime;

	/* RPM - Bullets per minute fired by weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float HeadshotMultiplier;

	/* Bullet Spread in Degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpread;

	/* Bullet Spread in Degrees While Aiming Down Sights */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpreadWhileAiming;

	// Derived from RateOfFire
	float TimeBetweenShots;

//FTimerHandle

	FTimerHandle TimerHandle_TimeBetweenShots;

//FHitScanTrace

	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

//Pointers

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

public:	

// ------- FUNCTIONS ------- \\

	void StartFire();

	void StopFire();

	void StartReload();

// ------- VARIABLES ------- \\

//Bool

	bool IsAiming;

//Int

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Ammo")
	int CurrentAmmo;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Ammo")
	int MaxAmmo;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Ammo")
	int ClipSize;
	
};
