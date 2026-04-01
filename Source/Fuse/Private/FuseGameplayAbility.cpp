// Fill out your copyright notice in the Description page of Project Settings.


#include "FuseGameplayAbility.h"

void UFuseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	//TODO get a hello world going for this ability
	UE_LOG(LogTemp, Warning, TEXT("Hello Ability"));
}
