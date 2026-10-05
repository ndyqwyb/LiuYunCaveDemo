// LY_PlayerCharacter.cpp

#include "Player/LY_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/LY_InteractComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

ALY_PlayerCharacter::ALY_PlayerCharacter()
{
	// 交互组件：默认子对象，挂在玩家身上，随角色一起生成 / 销毁。
	// 相机不用手动指定 —— 组件会在 BeginPlay 自动找角色身上的 CameraComponent。
	InteractComponent = CreateDefaultSubobject<ULY_InteractComponent>(TEXT("InteractComponent"));

	// 构造函数只设默认值，不做任何世界查询 / 资源加载。
	ApplyLYTuning();
}

void ALY_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 蓝图里改过的数值也在开游戏时应用一次，保证编辑器预览与实际手感一致。
	ApplyLYTuning();
}

void ALY_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 父类先绑移动 / 跳跃 / 镜头，这里再补交互，互不覆盖。
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// InteractAction 是蓝图上指定的资产；没指定就不绑，不报错。
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(
				InteractAction,
				ETriggerEvent::Started,
				this,
				&ALY_PlayerCharacter::OnInteractPressed);
		}
	}
}

void ALY_PlayerCharacter::OnInteractPressed()
{
	// 只负责「扣扳机」：当前有没有目标、能不能交互，全由组件判断。
	if (InteractComponent)
	{
		InteractComponent->TryInteract();
	}
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