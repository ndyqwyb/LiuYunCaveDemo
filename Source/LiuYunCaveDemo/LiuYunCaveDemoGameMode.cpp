// Copyright Epic Games, Inc. All Rights Reserved.

#include "LiuYunCaveDemoGameMode.h"
#include "LiuYunCaveDemoCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALiuYunCaveDemoGameMode::ALiuYunCaveDemoGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
