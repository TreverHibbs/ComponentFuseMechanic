// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/RigidBodyBase.h"
#include "RepPhysicsConstraintActor.generated.h"

class URepPhysicsConstraintComponent;
/**
 * 
 */
UCLASS()
class FUSE_API ARepPhysicsConstraintActor : public ARigidBodyBase
{
	GENERATED_BODY()

	ARepPhysicsConstraintActor();

	UPROPERTY(Category = Physics, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URepPhysicsConstraintComponent> ConstraintComponent;

	UPROPERTY(ReplicatedUsing=OnRep_ConstraintPropertyUpdated, Category = Constraint, VisibleAnywhere)
	TObjectPtr<AActor> ConstrainedActor1;

	UPROPERTY(ReplicatedUsing=OnRep_ConstraintPropertyUpdated, Category = Constraint, VisibleAnywhere)
	FName ConstrainedActor1Name;

	UPROPERTY(ReplicatedUsing=OnRep_ConstraintPropertyUpdated, Category = Constraint, VisibleAnywhere)
	TObjectPtr<AActor> ConstrainedActor2;

	UPROPERTY(ReplicatedUsing=OnRep_ConstraintPropertyUpdated, Category = Constraint, VisibleAnywhere)
	FName ConstrainedActor2Name;
	
	UFUNCTION()
	void OnRep_ConstraintPropertyUpdated();

public:
	void SetConstraints(TObjectPtr<AActor> ConstrainedActor1Input, FName ConstrainedActor1NameInput,
	                    TObjectPtr<AActor> ConstrainedActor2Input,
	                    FName ConstrainedActor2NameInput);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
