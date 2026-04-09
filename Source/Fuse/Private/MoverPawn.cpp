// Fill out your copyright notice in the Description page of Project Settings.


#include "MoverPawn.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "ChaosMover/Character/ChaosCharacterMoverComponent.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PBDRigidsSolver.h"
#include "Chaos/PhysicsObjectInternalInterface.h"
#include "Chaos/PBDJointConstraints.h"
#include "Runtime/Experimental/Chaos/Private/Chaos/PhysicsObjectInternal.h"

enum class TransformType : uint8;

// Workaround for missing CHAOS_API on base class methods in UE 5.7 experimental Chaos API
namespace Chaos
{
	void FPBDConstraintContainer::OnDisableParticle(FGeometryParticleHandle* DisabledParticle)
	{
		for (FConstraintHandle* ConstraintHandle : DisabledParticle->ParticleConstraints())
		{
			if ((ConstraintHandle->GetContainerId() == ContainerId) && ConstraintHandle->IsEnabled())
			{
				ConstraintHandle->SetEnabled(false);
			}
		}
	}

	void FPBDConstraintContainer::OnEnableParticle(FGeometryParticleHandle* EnabledParticle)
	{
		for (FConstraintHandle* ConstraintHandle : EnabledParticle->ParticleConstraints())
		{
			if ((ConstraintHandle->GetContainerId() == ContainerId) && !ConstraintHandle->IsEnabled())
			{
				ConstraintHandle->SetEnabled(true);
			}
		}
	}
}

void FPhysicsPawnAsync::SetShouldCreateConstraint_Internal(bool bInShouldCreateConstraint)
{
	bShouldCreateConstraint_Internal = bInShouldCreateConstraint;
}

void FPhysicsPawnAsync::SetTargetPhysicsObject_Internal(Chaos::FConstPhysicsObjectHandle InTargetPhysicsObject)
{
	TargetPhysicsObject_Internal = InTargetPhysicsObject;
}

// Sets default values
AMoverPawn::AMoverPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

UAbilitySystemComponent* AMoverPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void AMoverPawn::BeginPlay()
{
	Super::BeginPlay();
	ChaosCharacterMotionComponent->InputProducer = this;

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (!InputMappingContext.IsNull())
			{
				InputSystem->AddMappingContext(InputMappingContext.LoadSynchronous(), 0);
			}
		}
	}
}

void AMoverPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	IMoverInputProducerInterface::ProduceInput_Implementation(SimTimeMs, InputCmdResult);

	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<
		FCharacterDefaultInputs>();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		CharacterInputs.ControlRotation = PC->GetControlRotation();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Player Controller is not found, can't get rotation for updating mover component"));
	}

	if (MoveAction)
	{
		CharacterInputs.OrientationIntent = FVector::ZeroVector;
		CharacterInputs.OrientationIntent = CharacterInputs.ControlRotation.Vector().GetSafeNormal();
		auto XYPlaneControlRotation = CharacterInputs.ControlRotation;
		XYPlaneControlRotation.Pitch = 0;
		XYPlaneControlRotation.Roll = 0;
		//TODO get this to reorientate the move input to the direction of the character
		const FQuat ControlRotationQuat = XYPlaneControlRotation.Quaternion();
		const FVector RotatedMoveInputXAxis = ControlRotationQuat.RotateVector(FVector(CachedMoveInputIntent.X, 0, 0));
		const FVector RotatedMoveInputYAxis = ControlRotationQuat.RotateVector(FVector(0, CachedMoveInputIntent.Y, 0));
		const FVector RotatedMoveInput = RotatedMoveInputXAxis + RotatedMoveInputYAxis;
		CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, RotatedMoveInput);
		CachedMoveInputIntent = FVector::ZeroVector;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Move Action is null on the mover pawn, can't get user input"));
	}
}

void AMoverPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority() && AbilitySystemComponent)
	{
		if (FuseAbility)
		{
			const FGameplayAbilitySpec AbilitySpec(FuseAbility, 1);
			FuseAbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		}

		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

// Called every frame
void AMoverPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Spin camera based on input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Simple input scaling. A real game will probably map this to an acceleration curve
		static float LookRateYaw = 100.f; // degs/sec
		static float LookRatePitch = 100.f; // degs/sec

		PC->AddYawInput(CachedLookInput.Yaw * LookRateYaw * DeltaTime);
		PC->AddPitchInput(-CachedLookInput.Pitch * LookRatePitch * DeltaTime);

		CachedLookInput = FRotator::ZeroRotator;
	}
}

void AMoverPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ChaosCharacterMotionComponent = FindComponentByClass<UChaosCharacterMoverComponent>();
	AbilitySystemComponent = FindComponentByClass<UAbilitySystemComponent>();

	if (UPrimitiveComponent* RootSimulatedComponent = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		if (UWorld* World = GetWorld())
		{
			if (FPhysScene* PhysScene = World->GetPhysicsScene())
			{
				if (Chaos::FPhysicsSolver* Solver = PhysScene->GetSolver())
				{
					// Create async callback object to run on Physics Thread
					PhysicsPawnAsync = Solver->CreateAndRegisterSimCallbackObject_External<FPhysicsPawnAsync>();
					if (ensure(PhysicsPawnAsync))
					{
						PhysicsPawnAsync->PawnPhysicsObject = RootSimulatedComponent->GetPhysicsObjectByName(NAME_None);
					}
				}
			}
		}
	}
}

//NOTES 4/8/26
//I think that I should actually just create a joint on the physics thread.
//I don't think I need to pass the constraint component like I am trying
//Next thing to do is to try this. TODO create a join only on physics thread
//using the networked physics component for replication.

//Don't let the pawn go to sleep on the physics thread because this could cause
//problems
//notes 4/8/26
//I'm not sure if I need this
void FPhysicsPawnAsync::OnPostInitialize_Internal()
{
	if (PawnPhysicsObject)
	{
		Chaos::FWritePhysicsObjectInterface_Internal Interface = Chaos::FPhysicsObjectInternalInterface::GetWrite();
		if (Chaos::FPBDRigidParticleHandle* ParticleHandle = Interface.GetRigidParticle(PawnPhysicsObject))
		{
			ParticleHandle->SetSleepType(Chaos::ESleepType::NeverSleep);
		}
	}
}

template <typename TransformType>
static void FixConnectorTransformsForRoot(
	const Chaos::FParticlePair& OriginalHandles, const Chaos::FParticlePair& RootHandles,
	TransformType& InOutTransformsToFix)
{
	for (int32 Index = 0; Index < 2; Index++)
	{
		if (OriginalHandles[Index] && RootHandles[Index] && RootHandles[Index] != OriginalHandles[Index])
		{
			const FTransform TransformOffset = OriginalHandles[Index]->GetTransformXR().GetRelativeTransform(
				RootHandles[Index]->GetTransformXR());
			InOutTransformsToFix[Index] *= TransformOffset;
		}
	}
}

//TODO get this working in single player mode
//Learn more about how things really works so you can effectively debug it
//Right now constraint doesn't seem to actually get created.
//good place to start would probably be in the constrains settings.
void FPhysicsPawnAsync::OnPreSimulate_Internal()
{
	if (const FAsyncInputPhysicsPawn* AsyncInput = GetConsumerInput_Internal())
	{
		SetShouldCreateConstraint_Internal(AsyncInput->bShouldCreateConstraint);
		SetTargetPhysicsObject_Internal(AsyncInput->TargetPhysicsObject);
	}
	if (!bShouldCreateConstraint_Internal)
	{
		return;
	}

	// Reset trigger so it only happens once
	bShouldCreateConstraint_Internal = false;

	Chaos::FReadPhysicsObjectInterface_Internal Interface = Chaos::FPhysicsObjectInternalInterface::GetWrite();
	Chaos::FPBDRigidsSolver* RigidsSolver = static_cast<Chaos::FPBDRigidsSolver*>(GetSolver());
	auto JointConstraints = RigidsSolver->GetJointCombinedConstraints();
	Chaos::FPBDRigidParticleHandle* PawnParticleHandle = Interface.GetRigidParticle(PawnPhysicsObject);
	Chaos::FPBDRigidParticleHandle* TargetParticleHandle = Interface.GetRigidParticle(TargetPhysicsObject_Internal);
	//TODO I need to figure out the API for getting these particle handles into
	//the constraint creation interface.
	//I think I need to make the serializable somehow.
	//Create joint constraint
	Chaos::FPBDJointSettings Settings;
	//Settings.LinearMotionType = Chaos::EJointMotionType::Locked;

	Chaos::FParticlePair RootHandles
	{
		PawnPhysicsObject->GetRootParticle<Chaos::EThreadContext::Internal>(),
		TargetPhysicsObject_Internal->GetRootParticle<Chaos::EThreadContext::Internal>(),
	};
	auto ConstParticlePair = Chaos::TVec2<const Chaos::FGeometryParticleHandle*>(
		PawnParticleHandle, TargetParticleHandle);
	Chaos::TVec2<Chaos::FGeometryParticleHandle*> ParticlePair;
	//Because of the API protection for the async input, I must cast to mutable
	//data.
	ParticlePair[0] = const_cast<Chaos::FGeometryParticleHandle*>(ConstParticlePair[0]);
	ParticlePair[1] = const_cast<Chaos::FGeometryParticleHandle*>(ConstParticlePair[1]);

	Chaos::FRigidTransform3 WorldTransform0 = ParticlePair[0]->GetTransformXR();
	Chaos::FRigidTransform3 WorldTransform1 = ParticlePair[1]->GetTransformXR();

	Chaos::FRigidTransform3 JointWorldTransform = WorldTransform1;

	Settings.ConnectorTransforms[0] = JointWorldTransform.GetRelativeTransform(WorldTransform0);
	Settings.ConnectorTransforms[1] = JointWorldTransform.GetRelativeTransform(WorldTransform1);

	Settings.LinearMotionTypes[0] = Chaos::EJointMotionType::Locked;
	Settings.LinearMotionTypes[1] = Chaos::EJointMotionType::Locked;
	Settings.LinearMotionTypes[2] = Chaos::EJointMotionType::Locked;

	Settings.bLinearPositionDriveEnabled = {true, true, true};

	Settings.LinearDriveStiffness = {1000, 1000, 1000};
	Settings.LinearDriveDamping = {100, 100, 100};
	
	Settings.LinearDrivePositionTarget = FVector::ZeroVector;
	
	Settings.bCollisionEnabled = true;

	FixConnectorTransformsForRoot(ParticlePair, RootHandles, Settings.ConnectorTransforms);

	auto JointHandle = RigidsSolver->GetEvolution()->CreateJointConstraint(ParticlePair, Settings);
	//TODO next thing is to understand how to configure constrain better
	//  TODO decide on what kind of mechanic I want for the prototype repo or totk
	//TODO get constraint working in multiplayer
}


void AMoverPawn::OnMoveTriggered(const FInputActionValue& Value)
{
	const FVector MovementVector = Value.Get<FVector>();
	CachedMoveInputIntent.X = FMath::Clamp(MovementVector.X, -1.0f, 1.0f);
	CachedMoveInputIntent.Y = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);
	CachedMoveInputIntent.Z = FMath::Clamp(MovementVector.Z, -1.0f, 1.0f);
}

void AMoverPawn::OnLookTriggered(const FInputActionValue& InputActionValue)
{
	const FVector2D LookVector = InputActionValue.Get<FVector2D>();
	CachedLookInput.Yaw = CachedTurnInput.Yaw = FMath::Clamp(LookVector.X, -1.0f, 1.0f);
	CachedLookInput.Pitch = CachedTurnInput.Pitch = FMath::Clamp(LookVector.Y, -1.0f, 1.0f);
}

void AMoverPawn::OnFuseTriggered(const FInputActionValue& InputActionValue)
{
	FGameplayTagContainer TargetTag;
	TargetTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Fuse")));
	AbilitySystemComponent->TryActivateAbilitiesByTag(TargetTag);
}

// Called to bind functionality to input
void AMoverPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Setup some bindings - we are currently using Enhanced Input and just using some input actions assigned in editor for simplicity
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMoverPawn::OnMoveTriggered);
		//Input->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &AMoverExamplesCharacter::OnMoveCompleted);
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMoverPawn::OnLookTriggered);
		Input->BindAction(FuseAction, ETriggerEvent::Completed, this, &AMoverPawn::OnFuseTriggered);
		//Input->BindAction(LookInputAction, ETriggerEvent::Completed, this, &AMoverExamplesCharacter::OnLookCompleted);
		//Input->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AMoverExamplesCharacter::OnJumpStarted);
		//Input->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &AMoverExamplesCharacter::OnJumpReleased);
		//Input->BindAction(FlyInputAction, ETriggerEvent::Triggered, this, &AMoverExamplesCharacter::OnFlyTriggered);
	}
}

void FPhysicsPawnAsync::OnPhysicsObjectUnregistered_Internal(Chaos::FConstPhysicsObjectHandle InPhysicsObject)
{
	if (PawnPhysicsObject == InPhysicsObject)
	{
		PawnPhysicsObject = nullptr;
	}
}

//Notes 4/9/26
// I am trying to understand how to create a constraint on the physics thread
// in order to do that I feel like I need to understand chaos physics better.
// I think I should use the brisker method therefore my TODO is to come up with
// a bunch of questions that can help me gain an understanding of the chaos
// physics system.
//
// Q. What API does unreal have for integrating physics engines?
// Q. Where is the main tick function for chaos physics? does it have it's
// own main loop?
// Q. What data comprises a joint constraint?
// Q. What systems use join constraints at runtime?
// Q. where are joint constraints stored at runtime?
// Q. What are the types of join constraints available?
// Q. What mechanism is used to create a constraint in the physics
// scene when a constraint actor is created on the game thread?
// Q. What data comprises a physics scene? Is that even a real thing?
// Q. What data comprises a solver?
// Q. What data comprises a particle?
// Q. What mechanism allows constraints to affect the physics simulation?
// Q. What mechanism allows the physics solver to move objects in the game world?
// Q. There is something called the chaos physics framework. What API does the
// chaos physics framework have

// After reading through blog post on how the chaos physics engine worked in
// 5.1 My new belief for how I should attach the player to the grabbed object
// with a constraint is this. TODO create a constraint component and at runtime
// include the constraint components physics thread handle in the data sent as input
// I am not sure about how to connect the constrain component to it's physics thread
// hand so TODO figure out how you get the physics thread handle of a constraint if
// you have it's game thread component counterpart.
// I think this is the path to take because the creation and registering of a physics
// representation of an object is complicated. I think it is best to let the engine
// handle that for me and to just use the data it generates.
//
// It looks like when a constraint component exists in the editor it is instantiated
// at runtime by pushing it's data to the physics thread. The FPBDevolution thing
// the object that is responsible for all the real physics work is the place where
// the constrain eventually get's created for real. This is not what I was doing.
// This makes me think that I should create the constraint in the editor and try to
// manipulate it in my code rather than try to make it from scratch.
// my next TODO is to create that constraint and see whether or not it is created
// for real on the physics thread.

//AI GAVE ME THIS ON HOW TO ENABLE JOINT DEBUG DRAW STUFF
//To visualize Chaos Physics constraints during Play In Editor (PIE), you can use runtime console commands, the viewport “Show” flags, or the Chaos Visual Debugger for deeper inspection.
//1. Primary Console Commands (Runtime)
//The most direct way to see constraints while playing is to enable the Chaos Debug Draw system. Open the console (~) and enter:
//
//Enable Debug Drawing:
//p.Chaos.DebugDraw.Enabled 1
//Visualize Joint Constraints:
//p.Chaos.Solver.DebugDrawJoints 1
//2. Customizing the Visualization
//Once joints are visible, they may appear as a simple line or a cluttered mess of axes. Use these commands to refine the view:
//
//Show Joint Axes: p.Chaos.Solver.DebugDraw.JointFeatures.Axes 1
//(Displays the local coordinate frames of the joint anchors.)
//Show Actor Connectors: p.Chaos.Solver.DebugDraw.JointFeatures.ActorConnector 1
//(Draws lines from the joint to the center of the connected physics bodies.)
//Show Stretch/Strain: p.Chaos.Solver.DebugDraw.JointFeatures.Stretch 1
//(Changes color or adds indicators when a joint is under high load.)
//Adjust Scale: p.Chaos.Solver.DebugDraw.ConstraintAxisLen 40
//(Increases or decreases the size of the debug axes.)
//3. Editor Viewport Flags
//If you are in the editor and have ejected (F8) or are looking at the scene, you can toggle physics visualization through the UI:
//
//In the Viewport, click the Show menu (top left).
//Navigate to Physics.
//Check Constraints.
//Note: This usually shows the authored UPhysicsConstraintComponent helpers. For the actual simulated Chaos joints, the console commands in Step 1 are more reliable.
//4. The Chaos Visual Debugger (CVD)
//For professional debugging—especially to see why a constraint was eliminated or broken—use the Chaos Visual Debugger.
//
//Go to Tools > Control Chaos Visual Debugger.
//Click Record and play your game.
//Stop recording and scrub to the frame of interest.
//In the CVD Show menu, go to Joint Constraints > Enable Draw.
//Selection: You can click on a joint in the viewport. The Details Panel will show the specific impulses, current “Broken” state, and the internal settings (Stiffness, Damping) used during that specific physics tick.
//5. Networked Physics Considerations
//If you are testing Networked Physics Resimulation, you can visualize the difference between the Client and Server’s version of a constraint:
//
//Show Server Joints: p.Chaos.Solver.DebugDraw.ShowServer 1
//Show Client Joints: p.Chaos.Solver.DebugDraw.ShowClient 1
//This is vital for identifying “ghost” constraints that exist on one machine but have been eliminated on the other, causing jitter or rubber-banding.
//
//Performance & Best Practices
//Reduce Noise: If the screen is filled with lines, use p.Chaos.DebugDraw.SingleActor 1 to only draw the constraints for the actor you are currently looking at with the camera.
//Avoid Tick: Do not leave these debug commands active during performance profiling, as the debug drawing itself can significantly impact the frame rate.