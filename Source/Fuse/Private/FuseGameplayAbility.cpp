// Fill out your copyright notice in the Description page of Project Settings.


#include "FuseGameplayAbility.h"

#include "CollisionDebugDrawingPublic.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/PhysicsConstraintActor.h"
#include "RootMotionModifier_PrecomputedWarp.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

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

		if (HasAuthority(&ActivationInfo))
		{
			auto HitActor = HitResult.GetActor();
			FActorSpawnParameters ConstraintSpawnParams;
			auto PhysicsConstraintActorInstance = World->SpawnActor<APhysicsConstraintActor>(
				PhysicsConstraintActor, Actor->GetActorLocation(),
				FRotator::ZeroRotator,
				ConstraintSpawnParams);
			auto ConstraintComp = PhysicsConstraintActorInstance->GetConstraintComp();
			ConstraintComp->SetConstrainedComponents(
				Cast<UPrimitiveComponent, USceneComponent>(Actor->GetRootComponent()),
				FName(""), Cast<UPrimitiveComponent, USceneComponent>(
					HitActor->GetRootComponent()),
				FName(""));
			PhysicsConstraintActorInstance->SetReplicates(true);
			ConstraintComp->SetIsReplicated(true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Actor of fuse ability is null, can't do it"));
	}
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
// should be to -TODOdid- test my understanding of how the motors will work by trying to
// implement an effect that dynamically attaches my pawn to the grabbed object with a motor
// I can then try to get this working on the network
//
// I attached to network predicted physics object with a constraint actor and it went
// about as well as I would hope they staid more or less in sync on the server and
// the client. Now I think I will add a constraint component dynamically on the server
// and replicate that constraint to the clients I will do this in my ability.

// I spent some time today thinking about why I need to network my inputs when moving
// an object that is grabbed. I went back and rewatched some of the networked physics
// presentation, and I have remembered that it is so that all the timelines, the autonomous
// proxy the server and the sim proxy align so that the different players can interact with
// the physics that is ultimately determined by what is happening on the server smoothly.
// It's to ensure that in most cases what you see is what you get. So I should network the
// inputs.
//
// TODO I think my next task is to get some controls working for moving the grabbed object
// around with the mouse input. So up and down mainly.
// If I understand what is going on correctly, (which I don't think is likely), then
// what I need to do is I need to sync the updates to the constraint actor between
// the client and the server and the simulated proxies. I think I should do this using
// the gameplay system. On the client I will use an ability and then an effect will
// handle the logic of adding a constraint. Since an effect is replicated this effect
// will run on the server and sim proxies. Then everyone will have the new constraint.
// I will then use the same technique to update the constraint with user input to rotate
// the grabbed object and move it up and down. I will try this and see how it works.
//
// TODO implement effect for grabbing. Abilities are not replicated to sim proxies
// I think it will work because resimulation conciders contraints so as long as the
// constraint is kept in sync on the server and client I think things should look good.
// Now this could go bad if the constraints are slow to sync via the gameplay system.
// For example a constrain could be upated quickely on the server resulting in movement
// of the object. Movement that isn't done on the clients till later. Same problem if it
// is first done on the clients. I guess I'll just try it and see how it feels. I am not
// fully understanding how the resimulation thing works.
//
//
// this https://youtu.be/_jRLlTDqoGI?si=rsoUFMb1R5I5gxZd&t=2182 might help get it working
// well in a networked scenario.
