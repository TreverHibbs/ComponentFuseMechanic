// Fill out your copyright notice in the Description page of Project Settings.


#include "MoverPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "EnhancedPlayerInput.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"

// Sets default values
AMoverPawn::AMoverPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CharacterMotionComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("CharacterMotionComponent"));
}

// Called when the game starts or when spawned
void AMoverPawn::BeginPlay()
{
	Super::BeginPlay();
	CharacterMotionComponent->InputProducer = this;

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
		const FVector RotatedMoveInputXAxis = ControlRotationQuat.RotateVector(CachedMoveInputIntent.XAxisVector);
		const FVector RotatedMoveInputYAxis = ControlRotationQuat.RotateVector(CachedMoveInputIntent.YAxisVector);
		const FVector RotatedMoveInput = RotatedMoveInputXAxis + RotatedMoveInputYAxis;
		CharacterInputs.SetMoveInput(CharacterInputs.GetMoveInputType(), RotatedMoveInput);
		CachedMoveInputIntent = FVector::ZeroVector;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Move Action is null on the mover pawn, can't get user input"));
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
		//Input->BindAction(LookInputAction, ETriggerEvent::Completed, this, &AMoverExamplesCharacter::OnLookCompleted);
		//Input->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AMoverExamplesCharacter::OnJumpStarted);
		//Input->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &AMoverExamplesCharacter::OnJumpReleased);
		//Input->BindAction(FlyInputAction, ETriggerEvent::Triggered, this, &AMoverExamplesCharacter::OnFlyTriggered);
	}
}
