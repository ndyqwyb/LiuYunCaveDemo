// LY_LightPath.h
// 光桥（光路）—— 一段「可收回 / 可生长」的发光桥面，是旋转桥的组成单元。
// 依据：03_机关设计规格.md 的 3.3；2026-10 用户修正（见 DevLog M5）。
//
// 为什么它不继承 ALY_MechanicBase？
//   它不是「机关」：不能被玩家交互，也没有 Available / Active 那套语义。
//   它只是旋转桥手里的一根「会伸缩的桥」。继承基类要背一堆用不上的东西。
//
// 三层分工（本项目的固定套路）：
//   * 本类（C++）：谁先收、收多久、什么时候收完、碰撞何时开关 —— 纯逻辑，零美术资源
//   * BP_LY_LightPath（蓝图）：在 OnGrowVisuals / OnRetractVisuals 里跑 Timeline，把材质参数 Wipe 从 0 推到 1（或反过来）
//   * M_LY_LightBridge（材质）：读 Wipe，沿桥长把桥擦出来 / 擦没
//
// ★ 关键约定：材质参数 Wipe 的方向
//   近端（靠转盘一端）= u 0，远端（远离转盘一端）= u 1。
//   Wipe = 0 全隐藏，Wipe = 1 全显示。
//   于是：收回 = Wipe 1->0 -> 远端先灭、近端最后灭；
//         生长 = Wipe 0->1 -> 近端先亮、远端最后亮。
//   方向自动正确，不需要把桥拆成很多段做可见性。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LY_LightPath.generated.h"

class UBoxComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLY_OnLightPathGrown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLY_OnLightPathRetracted);

/**
 * 一段光桥。旋转桥按「朝向 -> 桥清单」驱动它：收回 -> 转盘旋转 -> 再生长。
 *
 * 它自己不知道转盘在哪、朝向是什么 —— 那些是 ALY_RotatingBridge 的事。
 * 它只回答三个问题：开始长 / 开始收 / 好了没有。
 *
 * 注意：C++ 用 Timer 计时，不用 Tick。机关类一帧都不该 tick。
 */
UCLASS(Blueprintable)
class LIUYUNCAVEDEMO_API ALY_LightPath : public AActor
{
	GENERATED_BODY()

public:
	ALY_LightPath();

	// ---------------- 操作（旋转桥 / 测试物调用）----------------

	/**
	 * 生长：Wipe 0 -> 1，Duration 秒。
	 * 碰撞在「生长完成」时才打开 —— 半截的桥不能踩。
	 * Duration <= 0 表示瞬间到终态（蓝图里要按 0 处理，别去跑 Timeline）。
	 */
	UFUNCTION(BlueprintCallable, Category = "LY|LightPath")
	void GrowIn(float Duration);

	/**
	 * 收回：Wipe 1 -> 0，Duration 秒。
	 * 碰撞在「调用瞬间」就关掉 —— 不能让人站在正在消失的桥上。
	 * Duration <= 0 表示瞬间到终态。
	 */
	UFUNCTION(BlueprintCallable, Category = "LY|LightPath")
	void Retract(float Duration);

	/** 直接设置碰撞（一般不用手动调，GrowIn / Retract 会自己管） */
	UFUNCTION(BlueprintCallable, Category = "LY|LightPath")
	void SetWalkable(bool bNewWalkable);

	// ---------------- 查询 ----------------

	/** 正在收 / 正在长？（旋转桥靠它防止重入） */
	UFUNCTION(BlueprintPure, Category = "LY|LightPath")
	bool IsBusy() const { return bBusy; }

	/** 目标状态：true = 应该显示（已经长完或正在长），false = 应该隐藏 */
	UFUNCTION(BlueprintPure, Category = "LY|LightPath")
	bool IsShownTarget() const { return bShownTarget; }

	/** 当前这一段动画的时长（秒）；不在动画中时为 0 */
	UFUNCTION(BlueprintPure, Category = "LY|LightPath")
	float GetTransitionDuration() const { return bBusy ? ActiveDuration : 0.f; }

	/** 现在能不能踩（蓝图做调试显示时可读） */
	UFUNCTION(BlueprintPure, Category = "LY|LightPath")
	bool IsWalkable() const;

	// ---------------- 事件（逻辑层监听）----------------

	/** 生长完成（碰撞已打开） */
	UPROPERTY(BlueprintAssignable, Category = "LY|LightPath")
	FLY_OnLightPathGrown OnGrowFinished;

	/** 收回完成（视觉上已经全灭） */
	UPROPERTY(BlueprintAssignable, Category = "LY|LightPath")
	FLY_OnLightPathRetracted OnRetractFinished;

	// ---------------- 表现钩子（蓝图实现，C++ 不写美术代码）----------------

	/**
	 * 蓝图在这里：把 Wipe 从 0 推到 1，时长 Duration 秒。
	 * 做法：Timeline 固定长度 L，SetPlayRate(L / Duration) 后 PlayFromStart；
	 *       Duration <= 0 时直接把 Wipe 设为 1，不跑 Timeline。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LY|LightPath")
	void OnGrowVisuals(float Duration);

	/**
	 * 蓝图在这里：把 Wipe 从 1 推回 0，时长 Duration 秒。
	 * Duration <= 0 时直接把 Wipe 设为 0。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LY|LightPath")
	void OnRetractVisuals(float Duration);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 空根组件：BP 的桥网格挂在这下面，桥的本地 X 轴 = 桥长方向（u 的基准） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LY|LightPath")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 * 可站立碰撞：一个 Box 覆盖整条桥。
	 * 尺寸由关卡里的实例调（直桥够用；以后弯桥再换）。不要塞进网格里当复杂碰撞。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LY|LightPath")
	TObjectPtr<UBoxComponent> WalkableCollision;

	/** 开局是显示状态还是收回状态（关卡实例里按需改） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|LightPath")
	bool bStartShown = true;

	/** 长完以后是否能踩。默认 true；纯装饰的桥段可以关掉 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|LightPath")
	bool bWalkableWhenShown = true;

	/** 打开会在输出日志里打每一次 收回/生长 的起止，排查用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|LightPath")
	bool bDebugLightPath = false;

private:
	void StartTransition(bool bGrow, float Duration);
	void HandleTransitionFinished();
	void ApplyWalkable(bool bNewWalkable);

	FTimerHandle TransitionTimer;

	bool bBusy = false;
	bool bShownTarget = true;
	bool bTransitionIsGrow = false;
	float ActiveDuration = 0.f;
};

