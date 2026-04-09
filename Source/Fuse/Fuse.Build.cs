// Copyright Epic Games, Inc. All Rights Reserved.


using UnrealBuildTool;

public class Fuse : ModuleRules
{
	public Fuse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "Mover",
			"ChaosMover", "GameplayAbilities", "GameplayTags", "GameplayTasks", "Chaos", "ChaosSolverEngine",
			"ChaosCore", "PhysicsCore"
		});

		SetupIrisSupport(Target);
	}
}

//Below are my current goals. The bigger picture is that I want a totk
//system that used chaos mover character's and the modular vehicle system and
//a fuse hand system that is fully networked.
//TODO look into animated the wheels so that only the wheel bone moves and not
//the axel
//TODO implement simple as possible dynamic modular vehicle building system
//
//  So far with my gas exploration I think this is how the system would work for the ultra hand.
//  The ultra hand will be an ability that can be activated by a keybind from the player characters.
//  This ability will shoot a ray to detect an object that can be grabbed. I guess I would use (tags
//  to tag items that can be grabbed). An effect would be triggered to try and grab the object if
//  such an object is found by the ray cast. If it is not a effect will trigger to inform the player
//  that they did not grab something maybe. Or the input will just be ignored.
//  If the grab is indead successfull the player and the object will enter some kind of grabbed state.
//  This state will be implemented in the logic with tags.
//  The tags will be replicated using the replicated effects and tags system in gas.
//  If the server does not think that the object should be grabbed but the client did think
//  it should be grabbed then I will use the rollback gameplay effect support to rollback
//  whatever changed on the client between the grab effect execution and the server correction.
//  This means that tags will have to be updated and potentially physics objects will have
//  to move back to where they came. Moving back physics objects sounds hard so I think a better
//  solution would be to just not allow the completion of the grabbed state until the server
//  says it is ok.
//  Once the grabbed state the logic for controlling the object will use the networked physics
//  component, which will have to be placed on the grabbed object, to marshal the movement of the
//  character to move the object.
//  Actually the last stated method is probably not right.
//  The actual solution might be utilizing chaos physics fields to move the object. However
//  I am not sure if the chaos physics fields can be networked. I think this is what I should
//  look into next. it is my next.  according the the UE AI fields can be replicated
//  that is exciting and I think it is true.

//NOTES
//
// So after reading the docs on gameplay ablities and the replectation of those abilities
// I think this is what I need to do. I need to create an ability for the activation
// of a fuse grab. This ability should be initiated by the server after the client requrests
// said initiation with a button press. The ability should then be replicated during its
// execution with the easiest option which is just the intantiate per execution option.
// ok I should now create a hello world of this.
// Before doing the above though I need to grant the ability to the pawn
// on the server.
//
// notes 4/2/2026
// I have set up a hello world for the fuse ability and now I am thinking about implementing
// the corresponding grab effect of the ability. This means that I will need to 
// trace from the camera and see if it hits an object that can be grabbed. If it does
// I then need to create some physics interaction that implements a ultra hand type grab.
// I am thinking I am going to use a physics constraint for this.
// I think I will use the gameplay ability system for this. The fuse ability will
// create a grabbed effect on the grabbed object and a grabbing effect on the player's
// pawn. I think the grabbed effect will create the physics constraint and when the grabbed
// effect is gone that contraint will disapear. I think this is what I will try to implement
// at first.
// TODO implement the stuff I describe above