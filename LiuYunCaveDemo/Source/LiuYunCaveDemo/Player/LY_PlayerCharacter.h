// LY_PlayerCharacter.h
// 玩家角色基类 —— 继承 UE 第三人称模板角色 ALiuYunCaveDemoCharacter。
//
// 分工（见 04_蓝图与C++协作.md 的「三层结构」）：
//   * 这里只放「可调参数」+「把参数应用到角色上的逻辑」，不写任何美术表现代码。
//   * 模型、动画蓝图、输入资产（IMC_LY_Default / IA_LY_*）都挂在蓝图 BP_LY_PlayerCharacter 上。
//
// 调手感时改哪里？
//   改本文件的 LY* 变量（蓝图 Details 面板 -> LY|Player 分类）。
//   不要直接改 Character Movement 组件上的 Max Walk Speed 等值：开游戏时会被 LY* 覆盖。

#pragma once

#include "CoreMinimal.h"
#include "LiuYunCaveDemoCharacter.h"
#include "LY_PlayerCharacter.generated.h"

/**
 * 玩家角色基类。
 * 移动 / 跳跃 / 镜头的默认值参考 02_关卡考据与设计.md 的 2.2 UE5 尺度规范。
 */
UCLASS(Blueprintable)
class ALY_PlayerCharacter : public ALiuYunCaveDemoCharacter
{
	GENERATED_BODY()

public:
	ALY_PlayerCharacter();

	/** 把下面这些 LY* 参数写到 CharacterMovement / SpringArm 上 */
	void ApplyLYTuning();

protected:
	virtual void BeginPlay() override;

public:
	// ---------------- 移动手感 ----------------

	/** 行走速度 (uu/s)。2.2 尺度表建议 300~400，原神约 400 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Movement",
		meta = (ClampMin = "100.0", ClampMax = "900.0"))
	float LYMaxWalkSpeed = 400.f;

	/** 起跳初速 (uu/s)。越大跳越高：400 约跳 100uu，600 约跳 180uu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Movement",
		meta = (ClampMin = "0.0", ClampMax = "1500.0"))
	float LYJumpZVelocity = 600.f;

	/** 空中控制度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Movement",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LYAirControl = 0.35f;

	/** 落地后的减速度 (uu/s^2)，越大停得越干脆 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Movement",
		meta = (ClampMin = "0.0"))
	float LYBrakingDecelerationWalking = 2000.f;

	// ---------------- 镜头 ----------------

	/** 第三人称相机臂长 (uu)。2.2 尺度表建议 300~400 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Camera",
		meta = (ClampMin = "100.0", ClampMax = "1200.0"))
	float LYCameraArmLength = 400.f;

	/** 相机相对角色的偏移：抬高一点更容易看到前方地面 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Camera")
	FVector LYCameraSocketOffset = FVector(0.f, 0.f, 60.f);

	/** 相机是否做碰撞检测（撞墙时自动拉近，防止穿墙） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Player|Camera")
	bool bLYCameraCollisionTest = true;
};