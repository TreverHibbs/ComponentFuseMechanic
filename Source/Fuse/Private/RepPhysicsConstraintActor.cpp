// Fill out your copyright notice in the Description page of Project Settings.


#include "RepPhysicsConstraintActor.h"

#include "RepPhysicsConstraintComponent.h"
#include "Net/UnrealNetwork.h"

ARepPhysicsConstraintActor::ARepPhysicsConstraintActor()
{
	bReplicates = true;
	ConstraintComponent = CreateDefaultSubobject<URepPhysicsConstraintComponent>(TEXT("ConstraintComponent "));
}

void ARepPhysicsConstraintActor::OnRep_ConstraintPropertyUpdated()
{
	const auto LocalConstraintActor1 = ConstrainedActor1.Get();
	const auto LocalConstraintActor2 = ConstrainedActor2.Get();
	if (LocalConstraintActor1 && LocalConstraintActor2)
	{
		ConstraintComponent->SetConstrainedComponents(
			Cast<UPrimitiveComponent, USceneComponent>(LocalConstraintActor1->GetRootComponent()),
			ConstrainedActor1Name,
			Cast<UPrimitiveComponent, USceneComponent>(LocalConstraintActor2->GetRootComponent()),
			ConstrainedActor2Name);
	}
}

void ARepPhysicsConstraintActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		ConstraintComponent->BreakConstraint();
	}
}

void ARepPhysicsConstraintActor::SetConstraints(TObjectPtr<AActor> ConstrainedActor1Input,
                                                FName ConstrainedActor1NameInput,
                                                TObjectPtr<AActor> ConstrainedActor2Input,
                                                FName ConstrainedActor2NameInput)
{
	ConstrainedActor1 = ConstrainedActor1Input;
	ConstrainedActor2 = ConstrainedActor2Input;
	ConstrainedActor1Name = ConstrainedActor1NameInput;
	ConstrainedActor2Name = ConstrainedActor2NameInput;
	ConstraintComponent->SetConstrainedComponents(
		Cast<UPrimitiveComponent, USceneComponent>(ConstrainedActor1Input->GetRootComponent()),
		ConstrainedActor1NameInput,
		Cast<UPrimitiveComponent, USceneComponent>(ConstrainedActor2Input->GetRootComponent()),
		ConstrainedActor2NameInput);
}

void ARepPhysicsConstraintActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARepPhysicsConstraintActor, ConstrainedActor1);
	DOREPLIFETIME(ARepPhysicsConstraintActor, ConstrainedActor1Name);
	DOREPLIFETIME(ARepPhysicsConstraintActor, ConstrainedActor2);
	DOREPLIFETIME(ARepPhysicsConstraintActor, ConstrainedActor2Name);
}
