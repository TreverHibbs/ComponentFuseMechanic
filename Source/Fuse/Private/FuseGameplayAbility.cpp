// Fill out your copyright notice in the Description page of Project Settings.


#include "FuseGameplayAbility.h"

#include "CollisionDebugDrawingPublic.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "RootMotionModifier_PrecomputedWarp.h"

UFuseGameplayAbility::UFuseGameplayAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void UFuseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UWorld* World = GetWorld();
	auto Actor = ActorInfo->AvatarActor.Get();
	if (Actor && World)
	{
		// Example: Start from actor's location, shoot forward 1000 units
		FVector TraceStart = Actor->GetActorLocation();
		FVector TraceEnd = TraceStart + Actor->GetActorForwardVector() * 1000.0f;

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Actor);
		//TODO Configure trace to only get first actor and get a reference to that actor
		//then creat logic to add a constraint between that actor and the player character
		auto bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			Params
		);

		DrawDebugLine(World, TraceStart, TraceEnd, FColor::Red, false, 2.0f);

		if (bHit)
		{
			// Handle your hit logic
			UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s at Location: %s Normal: %s"),
			       *GetNameSafe(HitResult.GetActor()),
			       *HitResult.ImpactPoint.ToString(),
			       *HitResult.ImpactNormal.ToString());
		}
		else
		{
			// No hit
			UE_LOG(LogTemp, Warning, TEXT("No Actor hit by trace."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Actor of fuse ability is null, can't do it"));
	}
	UE_LOG(LogTemp, Warning, TEXT("Hello Ability"));
}

//NOTES 4/3/26
//
// This morning I learned that two potential options for controlling the position of
// a grabbed object are the UPhysicsHandleComponent, a linear motore, or an angular motor
// The original fuse character uses the handle component in tandem witha contraint actor
// However, I think a motor might be a better solution for what I want to achieve.
// However, I don't yet have a mental model for how the motor will interact with
// networked physics, on the other hand, I do feel like I have a mental model for how
// the handle component would interact with networked physics I think my next task
// should be to -TODO- test my understanding of how the motors will work by trying to
// implement an effect that dynamically attaches my pawn to the grabbed object with a motor
// I can then try to get this working on the network
// TODO before doing the above just attach two boxes together with a motor and see what
// happens