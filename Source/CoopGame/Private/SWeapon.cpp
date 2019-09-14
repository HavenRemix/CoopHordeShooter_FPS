// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "SCharacter.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing, 
	TEXT("Draw Debug Lines for Weapons"), 
	ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{

//Mesh Comp

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

//Set default values for all the variables

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

//Weapon float variables

	BaseDamage = 20.0f;
	BulletSpread = 2.0f;
	BulletSpreadWhileAiming = 0.0f;
	RateOfFire = 600;
	HeadshotMultiplier = 4.0f;

	IsAiming = false;

	//Make it so this actor is replicated
	SetReplicates(true);

	//Variables for replication
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
	
	//Ammo
	CurrentAmmo = ClipSize;
}


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

// ------- FUNCTIONS ------- \\

void ASWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	//Only run if you have an owner and have enough ammo to shoot your weapon
	AActor* MyOwner = GetOwner();
	if (MyOwner && CurrentAmmo > 0)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		//Get owner's (player) eyes position
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		float HalfRad = FMath::DegreesToRadians(BulletSpread);

		// Bullet Spread
		if (MyOwner)
		{
			//If owner is currently aiming then the bullet spread is BulletSpreadWhileAiming if the owner is not aiming then the BulletSpread is used
			if (IsAiming)
			{
				HalfRad = FMath::DegreesToRadians(BulletSpreadWhileAiming);
			} else {
				HalfRad = FMath::DegreesToRadians(BulletSpread);
			}
		}
		
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		//Find the end of the trace
		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		//QueryParams for the line trace
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		// Particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		//Sets default surface type
		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		//Only run this is was a blocking hit as in we hit someting
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			// Blocking hit! Process damage
			AActor* HitActor = Hit.GetActor();

			//Get what kind of surface we hit
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			//Set variable to change if we hit a headshot
			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				//Multiplay the damage with HeadshotMultiplier if we hit a headshot
				ActualDamage *= HeadshotMultiplier;
			}

			//Apply damage t hit object
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			//Play effect for hitting something
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			TracerEndPoint = Hit.ImpactPoint;

		}
		
		//If debugging is enabled draw debug lines
		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
		}

		//Play the fire effects at the tracer end point
		PlayFireEffects(TracerEndPoint);
		
		//Only run this if we are the server
		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}
		
		//Make sure that you dont spam click to shoot the weapon faster
		LastFireTime = GetWorld()->TimeSeconds;

		//Remove ammo from the clip every time we fire our weapon
		CurrentAmmo--;
	}
}


void ASWeapon::Reload()
{
	//Only run if the clip isn't full already
	if (CurrentAmmo < ClipSize)
	{
		//Get how much ammo we took from storage to fill the clip
		float AmmoTaken = ClipSize - CurrentAmmo;

		//Only run if we have enough ammo in storage to fill the clip
		if (MaxAmmo >= ClipSize)
		{
			//Fill the clip
			CurrentAmmo = ClipSize;

			//Remove the ammo taken to fill the clip from the storage
			MaxAmmo = MaxAmmo - AmmoTaken;
		}
		//Only run if there isnt enough ammo in storage to fill the clip completely
		else if (MaxAmmo < ClipSize)
		{
			//Add all of the ammo in storage to the clip
			CurrentAmmo = CurrentAmmo + MaxAmmo;

			//Remove the ammo taken to fill the clip from the storage
			MaxAmmo = MaxAmmo - AmmoTaken;
		}
	}
}


void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}


void ASWeapon::ServerFire_Implementation()
{
	Fire();
}


bool ASWeapon::ServerFire_Validate()
{
	//Put some checking code in here
	return true;
}


void ASWeapon::StartFire()
{
	//How long to wait to fire the first shot
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	//Make a loop to fire every time that the TimeBetweenShots is done
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}


void ASWeapon::StopFire()
{
	//Clear the loop so we stop firing
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


void ASWeapon::StartReload()
{
	Reload();
}


void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}


void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

// ------- ONLINE ------- \\

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}