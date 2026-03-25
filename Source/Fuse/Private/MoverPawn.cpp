// Fill out your copyright notice in the Description page of Project Settings.


#include "MoverPawn.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
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

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(this))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>())
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
	UInputMappingContext* MappingContext = InputMappingContext.Get();
	auto MoveAction = MappingContext->GetMapping(0).Action;
	const APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedPlayerInput* EnhancedInput = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	FInputActionValue MoveActionValue = EnhancedInput->GetActionValue(MoveAction);
	//CharacterInputs.SetMoveInput(FInputActionValue, MoveActionValue);
}

// Called every frame
void AMoverPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AMoverPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
