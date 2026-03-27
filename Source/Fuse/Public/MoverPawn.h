// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputSubsystemInterface.h"
#include "MoverSimulationTypes.h"
#include "GameFramework/Pawn.h"
#include "MoverPawn.generated.h"

class UCharacterMoverComponent;
class UChaosCharacterMoverComponent;
class UInputAction;
class UInputMappingContext;

UCLASS(Blueprintable)
class FUSE_API AMoverPawn : public APawn, public IMoverInputProducerInterface
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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(Category = Movement, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChaosCharacterMoverComponent> ChaosCharacterMotionComponent;
	
	// Entry point for input production. Do not override.
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;
	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void PostInitializeComponents() override;

	void OnMoveTriggered(const FInputActionValue& InputActionValue);
	void OnLookTriggered(const FInputActionValue& InputActionValue);
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
private: 
	FVector CachedMoveInputIntent = FVector::ZeroVector;
	FVector CachedMoveInputVelocity = FVector::ZeroVector;

	FRotator CachedTurnInput = FRotator::ZeroRotator;
	FRotator CachedLookInput = FRotator::ZeroRotator;
};
