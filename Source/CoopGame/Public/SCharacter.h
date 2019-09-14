// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;
class USkeletalMeshComponent;

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

// ------- INPUT ------- \\

	void MoveForward(float Value);

	void MoveRight(float Value);

	void Turn(float value);

	void LookUp(float value);

	void BeginCrouch();

	void EndCrouch();

	void Reload();

	void BeginZoom();

	void EndZoom();

// ------- COMPONENTS ------- \\

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FPSMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* DeathCameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

// ------- VARIABLES ------- \\

//Float

	/* Default FOV set during begin play */
	float DefaultFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterpSpeed;

//Bool

	//bool bWantsToZoom;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	bool IsAI;

	/* Pawn died previously */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bDied;

// ------- WEAPONS ------- \\

	UPROPERTY(Replicated, BlueprintReadOnly)
	ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ASWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketNameFPS;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

// ------- FUNCTIONS ------- \\

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void OnDead();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

// ------- FUNCTIONS ------- \\

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopFire();

// ------- VARIABLES ------- \\

	virtual FVector GetPawnViewLocation() const override;

//Float

	UPROPERTY(BlueprintReadWrite, Category = "Player")
	float MouseXSens;

	UPROPERTY(BlueprintReadWrite, Category = "Player")
	float MouseYSens;

	UPROPERTY(BlueprintReadWrite, Category = "Player")
	float MouseTargetingSens;

//Bool

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bWantsToZoom;

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool IsRunning;

};
