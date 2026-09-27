// Copyright Epic Games, Inc. All Rights Reserved.

#include "LiuYunCaveDemoGameMode.h"
#include "UObject/ConstructorHelpers.h"

ALiuYunCaveDemoGameMode::ALiuYunCaveDemoGameMode()
{
	// 指定默认玩家 Pawn：项目自己的角色蓝图 BP_LY_ThirdPersonCharacter。
	// 该蓝图父类是 C++ 的 ALY_PlayerCharacter，手感参数在 LY|Player 分类里调。
	//
	// 注意：这是 C++ 里硬编码的资产路径，不会跟着编辑器里的「移动 / 改名」自动更新。
	//       以后这个蓝图换了文件夹或改了名字，要回来同步改这里。

	static ConstructorHelpers::FClassFinder<APawn> LYPlayerClass(
		TEXT("/Game/Characters/Player/Blueprints/BP_LY_PlayerCharacter"));
	if (LYPlayerClass.Succeeded())
	{
		DefaultPawnClass = LYPlayerClass.Class;
	}
}