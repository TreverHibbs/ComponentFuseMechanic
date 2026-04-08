// Fill out your copyright notice in the Description page of Project Settings.


#include "FuseGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "GrabbingGameplayEffect.h"
#include "MoverPawn.h"
#include "RepPhysicsConstraintActor.h"
#include "RepPhysicsConstraintComponent.h"

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
	auto Actor = Cast<AMoverPawn, AActor>(ActorInfo->AvatarActor.Get());
	if (Actor && World)
	{
		auto GrabbingTag = FGameplayTag::RequestGameplayTag(TEXT("State.Grabbing"));
		bool HasGrabbingTag = Actor->AbilitySystemComponent.Get()->HasMatchingGameplayTag(GrabbingTag);
		if (auto PhysicsConstraintActorInstance = Actor->PhysicsConstraintActorInstance.Get();
			PhysicsConstraintActorInstance || HasGrabbingTag)
		{
			if (PhysicsConstraintActorInstance)
			{
				//Only the server should have a valid physics constraint instance,
				//therefore, this code should only run on the server.
				PhysicsConstraintActorInstance->ConstraintComponent->BreakConstraint();
				PhysicsConstraintActorInstance->Destroy();
				Actor->PhysicsConstraintActorInstance = nullptr;
				auto SpecHandle = MakeOutgoingGameplayEffectSpec(LetGoEffect);
				auto ActiveEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			}
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

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

		FActiveGameplayEffectHandle GrabbingActiveEffectHandle;
		if (HasAuthority(&ActivationInfo) && HitResult.IsValidBlockingHit())
		{
			auto SpecHandle = MakeOutgoingGameplayEffectSpec(GrabbingEffect);
			GrabbingActiveEffectHandle = ApplyGameplayEffectSpecToOwner(
				Handle, ActorInfo, ActivationInfo, SpecHandle);
		}

		if (HasAuthority(&ActivationInfo) && GrabbingActiveEffectHandle.WasSuccessfullyApplied())
		{
			auto HitActor = HitResult.GetActor();
			FActorSpawnParameters ConstraintSpawnParams;
			Actor->PhysicsConstraintActorInstance = World->SpawnActor<ARepPhysicsConstraintActor>(
				PhysicsConstraintActor, Actor->GetActorLocation(),
				FRotator::ZeroRotator,
				ConstraintSpawnParams);
			Actor->PhysicsConstraintActorInstance->SetConstraints(
				Actor,
				FName(""),
				HitActor,
				FName(""));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Actor of fuse ability is null, can't do it"));
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
// TODOdid implement effect for grabbing. Abilities are not replicated to sim proxies
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
//
// 4/5/26
// Today I am excited about my progress from yesterday. I got the creation of
// a replicated constraint working. I am now thinking that I will work on
// setting up a system for removing the constraint. I should also clean
// up the logic and handle the case where no valid object is found.
//
// My current working mental model is that I should apply a grabbed
// effect from this ability and in that grabbed effect I should
// set up a tag for indicating object is grabbed and pawn is
// grabbing, check for if I can apply effect based on tags and
// validity of target, if checks pass and tags are set create
// networked constraint. Maybe in slightly different order but this
// is what I should do.
// I am doing this to implement a state machine for the grabbing pawn
// A state machine that allows it to know when it is grabbing so it
// can then have the ability to let go
// That would be the next thing I do. Once I have a grabbing state
//
// 4/6/26
// I am worried about resimulation and the creation of a constraint actor.
// based on my current understanding the problem is this.
// Using networked physics resimulation system can you spawn 
// in a constraint actor in respons to user input and have that 
// actor's creationg be in sync with the server? 
// How would you handle resimulation in this example?
// I am worried that when the rewind step of resimulation happens 
// the system will not remove the old constraint actor from 
// before the rewind. Therefore when the logic that creates the 
// actor is run again you will end up with two constraint actors. 
// How can you prevent that?
// TODO find and test solutions for the above issue
//
// I will also have to be careful about how I handle creating the forward-
// predicted constraint on the client. I will have to make sure that when
// the server creates it's replicated counter-part that the one created on
// the client is hooked up to listen for replication of that constraint from
// the server
//
//Look into this response from the Unreal AI
//In Unreal Engine 5’s Networked Physics (Async Physics), you must respect 
//the thread boundary: Actors and Components exist on the Game Thread (GT), 
//while the Simulation Particles and Joints exist on the Physics Thread (PT).
//While you cannot directly touch the APhysicsConstraintActor or 
//UPhysicsConstraintComponent from the PT, you can—and should—modify 
//the Chaos Joint Handle that represents that constraint on the PT. 
//This is the only way to ensure the “break” or property change is 
//deterministic and synced during resimulation.
