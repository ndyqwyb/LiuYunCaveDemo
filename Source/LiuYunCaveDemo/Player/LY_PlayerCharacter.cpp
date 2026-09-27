// LY_PlayerCharacter.cpp

#include "Player/LY_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

ALY_PlayerCharacter::ALY_PlayerCharacter()
{
	// 构造函数只设默认值，不做任何世界查询 / 资源加载。
	ApplyLYTuning();
}

void ALY_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 蓝图里改过的数值也在开游戏时应用一次，保证编辑器预览与实际手感一致。
	ApplyLYTuning();
}

void ALY_PlayerCharacter::ApplyLYTuning()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = LYMaxWalkSpeed;
		Movement->JumpZVelocity = LYJumpZVelocity;
		Movement->AirControl = LYAirControl;
		Movement->BrakingDecelerationWalking = LYBrakingDecelerationWalking;
	}

	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->TargetArmLength = LYCameraArmLength;
		Boom->SocketOffset = LYCameraSocketOffset;
		Boom->bDoCollisionTest = bLYCameraCollisionTest;
	}
}