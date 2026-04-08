// Fill out your copyright notice in the Description page of Project Settings.


#include "GrabbedExecutionCalculation.h"

void UGrabbedExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
	UE_LOG(LogTemp, Warning, TEXT("hello effect execution"));
}
