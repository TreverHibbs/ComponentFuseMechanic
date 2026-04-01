// Copyright Epic Games, Inc. All Rights Reserved.


using UnrealBuildTool;
using UnrealBuildTool.Rules;

public class Fuse : ModuleRules
{
	public Fuse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "Mover",
			"ChaosMover", "GameplayAbilities", "GameplayTags", "GameplayTAsks" 
		});
	}
}

//Below are my current goals. The bigger picture is that I want a totk
//system that used chaos mover character's and the modular vehicle system and
//a fuse hand system that is fully networked.
//TODO look into animated the wheels so that only the wheel bone moves and not
//the axel
//TODO implement simple as possible dynamic modular vehicle building system
//  TODO learn about and decide if GAS is what I should use for the ultra hand ability
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
//  look into next. it is my next TODO. according the the UE AI fields can be replicated
//  that is exciting and I think it is true.
//TODO set up boilerplate code for GAS
//TODO set up basic dynamic interaction activation system for modular vehicle

//NOTES
//
// So after reading the docs on gameplay ablities and the replectation of those abilities
// I think this is what I need to do. I need to create an ability for the activation
// of a fuse grab. This ability should be initiated by the server after the client requrests
// said initiation with a button press. The ability should then be replicated during its
// execution with the easiest option which is just the intantiate per execution option.
// ok I should now create a hello world of this.
// TODO I will create a C++ ability that just prints out hello world.
// Before doing the above though I need to grant the ability to the pawn
// on the server.
// TODO grant the fuse ability to my pawn on the server