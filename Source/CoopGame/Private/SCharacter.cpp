// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "CoopGame.h"
#include "SHealthComponent.h"
#include "SWeapon.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

//Spring Arm

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

//Mesh Comp

	GetMesh()->SetOwnerNoSee(true);

//Make it so the player and crouch using the default engine function
	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

//Make it so that bullets (and any other object) ignore the collision of the capsule and go for the mesh instead
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

//Health Comp

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

//Camera Comp

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(RootComponent);

//FPSMesh

	// Create a first person mesh component for the owning player.
	FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	// Only the owning player sees this mesh.
	FPSMesh->SetOnlyOwnerSee(true);
	// Attach the FPS mesh to the FPS camera.
	FPSMesh->SetupAttachment(CameraComp);
	// Disable some environmental shadowing to preserve the illusion of having a single mesh.
	FPSMesh->bCastDynamicShadow = false;
	FPSMesh->CastShadow = false;

//Death Camera Comp

	DeathCameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("DeathCameraComp"));
	DeathCameraComp->SetupAttachment(SpringArmComp);

//Set the aim down sights (zoom) variables
	ZoomedFOV = 65.0f;
	ZoomInterpSpeed = 20;

//Set the weapon attach socket name
	WeaponAttachSocketName = "WeaponSocket";
	WeaponAttachSocketNameFPS = "WeaponSocketFPS";

	IsRunning = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
//Make default field of view the camera current field of view
	DefaultFOV = CameraComp->FieldOfView;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

//Check if we are the server
	if (Role == ROLE_Authority)
	{
		// Spawn a default weapon - only if we are the server
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		//Set the current weapon
		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			if (IsAI)
			{
				CurrentWeapon->SetOwner(this);
				CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
			}
			else {
				CurrentWeapon->SetOwner(this);
				CurrentWeapon->AttachToComponent(FPSMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketNameFPS);
			}
		}
	}
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::Turn);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::Reload);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	// CHALLENGE CODE
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
}

// ------- INPUT ------- \\

void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}


void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}


void ASCharacter::LookUp(float value)
{
	//This is the since that is value times the set mouse Y sens by the player
	float MainSens = value * MouseYSens;

	//This is the sens for when your targeting
	float NewSensitivity = MainSens * MouseTargetingSens;

	if (bWantsToZoom)
	{
		AddControllerPitchInput(NewSensitivity);
	}
	else {
		AddControllerPitchInput(MainSens);
	}
}


void ASCharacter::Turn(float value)
{
	//This is the since that is value times the set mouse X sens by the player
	float MainSens = value * MouseXSens;

	//This is the sens for when your targeting
	float NewSensitivity = MainSens * MouseTargetingSens;

	if (bWantsToZoom)
	{
		AddControllerYawInput(NewSensitivity);
	}
	else {
		AddControllerYawInput(MainSens);
	}
}


void ASCharacter::BeginCrouch()
{
	Crouch();
}


void ASCharacter::EndCrouch()
{
	UnCrouch();
}


void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;
	CurrentWeapon->IsAiming = true;
}


void ASCharacter::EndZoom()
{
	bWantsToZoom = false;
	CurrentWeapon->IsAiming = false;
}


void ASCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}


void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}


void ASCharacter::Reload()
{
	CurrentWeapon->StartReload();
}

// ------- FUNCTIONS ------- \\

void ASCharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	//If you dont have any health left and you havent died this spawn yet
	if (Health <= 0.0f && !bDied)
	{
		//This function runs and does evething that should happen once your dead 
		OnDead();
	}
}

void ASCharacter::OnDead()
{
	/*This function runs and does everything that should happen once your dead*/

	// Die! (set the died variable to true)
	bDied = true;

	//Stop movement
	GetMovementComponent()->StopMovementImmediately();

	//Set collision to false
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Detach from controller
	DetachFromControllerPendingDestroy();

	//Set your life span to 10 seconds
	SetLifeSpan(10.0f);

	//Stop firing so it doesn't keep firing once your dead
	StopFire();

	//Stop zoom so it doesn't keep zooming in once your dead
	EndZoom();
}


FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		//Change the `PawnEyesViewLocation` to the camera component
		return CameraComp->GetComponentLocation();
	}

	//In case that doesnt work and returns false, go back to the previous meathod
	return Super::GetPawnViewLocation();
}


void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate variables
	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}