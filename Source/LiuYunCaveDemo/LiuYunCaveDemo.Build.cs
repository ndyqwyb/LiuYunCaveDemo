// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LiuYunCaveDemo : ModuleRules
{
	public LiuYunCaveDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 把模块根目录加入头文件搜索路径，这样在子文件夹里可以写
		//   #include "Player/LY_PlayerCharacter.h"
		//   #include "LiuYunCaveDemoCharacter.h"
		// 而不必写一长串 ../../ 相对路径。
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}