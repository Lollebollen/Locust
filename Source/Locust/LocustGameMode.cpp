// Copyright Epic Games, Inc. All Rights Reserved.

#include "LocustGameMode.h"
#include "LocustCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALocustGameMode::ALocustGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
