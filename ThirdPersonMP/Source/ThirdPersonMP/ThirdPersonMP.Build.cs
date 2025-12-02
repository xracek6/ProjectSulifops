// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ThirdPersonMP : ModuleRules
{
	public ThirdPersonMP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange([
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		]);

		PrivateDependencyModuleNames.AddRange([]);
		
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

		PublicIncludePaths.AddRange([
			"ThirdPersonMP",
			"ThirdPersonMP/Variant_Platforming",
			"ThirdPersonMP/Variant_Platforming/Animation",
			"ThirdPersonMP/Variant_Combat",
			"ThirdPersonMP/Variant_Combat/AI",
			"ThirdPersonMP/Variant_Combat/Animation",
			"ThirdPersonMP/Variant_Combat/Gameplay",
			"ThirdPersonMP/Variant_Combat/Interfaces",
			"ThirdPersonMP/Variant_Combat/UI",
			"ThirdPersonMP/Variant_SideScrolling",
			"ThirdPersonMP/Variant_SideScrolling/AI",
			"ThirdPersonMP/Variant_SideScrolling/Gameplay",
			"ThirdPersonMP/Variant_SideScrolling/Interfaces",
			"ThirdPersonMP/Variant_SideScrolling/UI"
		]);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
