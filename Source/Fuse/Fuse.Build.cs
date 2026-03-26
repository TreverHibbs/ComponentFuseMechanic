// Copyright Epic Games, Inc. All Rights Reserved.

//Below are my current goals. The bigger picture is that I want a totk
//system that used chaos mover character's and the modular vehicle system and
//a fuse hand system that is fully networked.
//TODO look into animated the wheels so that only the wheel bone moves and not
//the axel
//TODO set up chaos mover character
//  TODO set up input event binding system with caching
//  TODO set up input for camera rotation
//  TODO set up and test networking
//  TODO look into why physics collision doesn't seem to function properly
//TODO implement simple as possible dynamic modular vehicle building system
//using chaos move character as base
//TODO set up basic dynamic interaction activation system for modular vehicle

using UnrealBuildTool;

public class Fuse : ModuleRules
{
	public Fuse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{ "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "Mover"
			});
	}
}