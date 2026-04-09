// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "EnhancedInputSubsystemInterface.h"
#include "FuseGameplayAbility.h"
#include "MoverSimulationTypes.h"
#include "Chaos/ConstraintHandle.h"
#include "GameFramework/Pawn.h"
#include "MoverPawn.generated.h"

class AMoverPawn;
class UCharacterMoverComponent;
class UChaosCharacterMoverComponent;
class UInputAction;
class UInputMappingContext;
struct FGameplayAbilitySpecHandle;

// GameThread to PhysicsThread input data struct
struct FAsyncInputPhysicsPawn : public Chaos::FSimCallbackInput
{
	bool bShouldCreateConstraint = false;
	Chaos::FConstPhysicsObjectHandle TargetPhysicsObject = nullptr;

	void Reset()
	{
		bShouldCreateConstraint = false;
		TargetPhysicsObject = nullptr;
	}
};

// PhysicsThread to GameThread output data struct
struct FAsyncOutputPhysicsPawn : public Chaos::FSimCallbackOutput
{
	void Reset()
	{
	}
};

class FPhysicsPawnAsync : public Chaos::TSimCallbackObject<FAsyncInputPhysicsPawn, FAsyncOutputPhysicsPawn,
                                                           (Chaos::ESimCallbackOptions::Presimulate |
	                                                           Chaos::ESimCallbackOptions::PhysicsObjectUnregister)>
{
	friend AMoverPawn;

	~FPhysicsPawnAsync()
	{
	}

	// TSimCallbackObject callbacks
	virtual void OnPostInitialize_Internal() override;
	virtual void OnPreSimulate_Internal() override;
	virtual void OnPhysicsObjectUnregistered_Internal(Chaos::FConstPhysicsObjectHandle InPhysicsObject) override;

private:
	bool bShouldCreateConstraint_Internal;
	Chaos::FConstPhysicsObjectHandle TargetPhysicsObject_Internal = nullptr;
	Chaos::FConstPhysicsObjectHandle PawnPhysicsObject = nullptr;

public:
	void SetShouldCreateConstraint_Internal(bool bInShouldCreateConstraint);
	void SetTargetPhysicsObject_Internal(Chaos::FConstPhysicsObjectHandle InTargetPhysicsObject);
};

UCLASS(Blueprintable)
class FUSE_API AMoverPawn : public APawn, public IMoverInputProducerInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMoverPawn();

	UPROPERTY(EditAnywhere, Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FuseAction;

	UPROPERTY(Category = Ability, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category="Ability")
	TSubclassOf<UFuseGameplayAbility> FuseAbility;

	UPROPERTY(Category = Ability, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	FGameplayAbilitySpecHandle FuseAbilityHandle;

	UPROPERTY(Category = Physics, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ARepPhysicsConstraintActor> PhysicsConstraintActorInstance;
	FPhysicsPawnAsync* PhysicsPawnAsync = nullptr;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(Category = Movement, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChaosCharacterMoverComponent> ChaosCharacterMotionComponent;

	// Entry point for input production. Do not override.
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	virtual void PossessedBy(AController* NewController) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

	void OnMoveTriggered(const FInputActionValue& InputActionValue);
	void OnLookTriggered(const FInputActionValue& InputActionValue);
	void OnFuseTriggered(const FInputActionValue& InputActionValue);
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	FVector CachedMoveInputIntent = FVector::ZeroVector;
	FVector CachedMoveInputVelocity = FVector::ZeroVector;

	FRotator CachedTurnInput = FRotator::ZeroRotator;
	FRotator CachedLookInput = FRotator::ZeroRotator;
};
