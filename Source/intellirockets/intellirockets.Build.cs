// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class intellirockets : ModuleRules
{
	public intellirockets(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"UMG",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"NavigationSystem",
				"AIModule",
				"ProceduralMeshComponent"
			}
		);

		// DesktopPlatform 仅在 Editor 配置中可用
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] 
			{ 
				"DesktopPlatform"
			});
		}

		PublicIncludePaths.AddRange(
			new string[] {
				"intellirockets",
				"intellirockets/Core",
				"intellirockets/UI",
				"intellirockets/Actors",
				"intellirockets/Managers"
			}
		);


		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
		
		// 确保Config目录下的JSON文件被打包
		if (Target.Type != TargetType.Editor)
		{
			RuntimeDependencies.Add("$(ProjectDir)/Config/DecisionIndicators.json", StagedFileType.NonUFS);
		}
	}
}
