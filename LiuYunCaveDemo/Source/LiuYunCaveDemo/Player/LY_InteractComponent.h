// LY_InteractComponent.h
// 交互组件 —— 挂在玩家身上，负责「找目标」和「执行交互」两件事（依据 03 的 3.12 / 04 的 4.15）。
//
// 它属于哪一层？（见 04_蓝图与C++协作.md 的「三层结构」）
//   第 1 层 C++：本组件就是逻辑层。射线怎么打、范围多大、什么时候换目标，全在 C++。
//   第 2/3 层 蓝图 / 关卡：只负责「长什么样」—— 收到 OnInteractTargetChanged 后播 UI 动画。
//
// 它怎么跟物体打交道？
//   只通过 ILY_Interactable 接口（Mechanics/LY_Interactable.h）。
//   本组件不认识火方碑、宝箱这些具体类型 —— 所以同一套逻辑能复用到所有机关。
//
// 使用时（第 3 步会做）：
//   1. 在 BP_LY_PlayerCharacter 里 Add Component -> LY Interact Component
//   2. 相机不用手动指定：组件会在 BeginPlay 自动找角色身上的 CameraComponent
//
// 三条不要碰的红线：
//   * 用 Timer（默认 0.1s）探测，不要改成每帧 Tick
//   * 射线必须忽略自己（Params.AddIgnoredActor(Owner)），否则第三人称相机先打到自己
//   * 提示文字不在这里：这里只负责「选中」，文字由物体的 GetInteractPrompt() 提供

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "LY_InteractComponent.generated.h"

class UCameraComponent;

/** 交互日志分类：在 Output Log 里过滤 "LogLYInteraction" 就能只看交互相关信息 */
DECLARE_LOG_CATEGORY_EXTERN(LogLYInteraction, Log, All);

/**
 * 交互目标变化时广播。
 * NewTarget 可能为 nullptr —— 表示玩家离开了所有可交互物（UI 应该淡出）。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLY_OnInteractTargetChanged, AActor*, NewTarget);

/**
 * 交互成功时广播（TryInteract 真的执行了目标的 Interact）。
 * 用途：按下 E 的即时反馈 —— 蓝图收到后播「闪一下」。
 * 注意：它不等于「目标丢失」。物体交互后还能不能再交互，由物体自己的 CanInteract 决定。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLY_OnInteractPerformed, AActor*, Target);

/**
 * 玩家交互组件。
 * 每 TraceInterval 秒从相机向前打一条射线，找最近的可交互物，作为「当前交互目标」。
 */
UCLASS(ClassGroup = (LY), meta = (BlueprintSpawnableComponent))
class LIUYUNCAVEDEMO_API ULY_InteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULY_InteractComponent();

	// ---------------- 可调参数 ----------------

	/** 交互距离 (uu)，以玩家为基准。250~350 之间：太小够不到，太大能隔墙交互 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Interact",
		meta = (ClampMin = "50.0", ClampMax = "1000.0"))
	float InteractRange = 300.f;

	/** 探测间隔（秒）。用 Timer 定期探测，不要改成每帧 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Interact",
		meta = (ClampMin = "0.02", ClampMax = "0.5"))
	float TraceInterval = 0.1f;

	/** 射线使用的碰撞通道。M3 用 Visibility；将来建了 LY_Interactable 自定义通道再切 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Interact")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** 射线起点相机。留空则自动用角色身上的 CameraComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Interact")
	TObjectPtr<UCameraComponent> TraceCamera = nullptr;

	// ---------------- 调试 ----------------

	/** 把射线画到屏幕上（绿色 = 有目标，灰色 = 无目标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Interact|Debug")
	bool bDrawDebug = false;

	/** 目标变化时打日志，第 3 步验证用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Interact|Debug")
	bool bLogTargetChanges = true;

	// ---------------- 事件（蓝图绑它更新 UI）----------------

	/** 目标变化时触发；蓝图（玩家）收到后显示 / 隐藏 WBP_LY_InteractPrompt */
	UPROPERTY(BlueprintAssignable, Category = "LY|Interact")
	FLY_OnInteractTargetChanged OnInteractTargetChanged;

	/**
	 * 交互成功时触发；蓝图（玩家）收到后播按下的即时反馈（WBP_LY_InteractPrompt 的 Pulse 动画）。
	 * 广播顺序：本事件排在 RefreshInteractTarget() 之前 —— 这样一次性机关随后的 FadeOut
	 * 会成为最后播放的动画（UMG 里后播的动画会打断前一条，FadeOut 必须是终态）。
	 */
	UPROPERTY(BlueprintAssignable, Category = "LY|Interact")
	FLY_OnInteractPerformed OnInteractPerformed;

	// ---------------- 查询 / 调用 ----------------

	/** 当前交互目标（可能为空） */
	UFUNCTION(BlueprintPure, Category = "LY|Interact")
	AActor* GetCurrentInteractTarget() const { return CurrentInteractTarget.Get(); }

	/** 当前是否锁定了可交互物 */
	UFUNCTION(BlueprintPure, Category = "LY|Interact")
	bool HasInteractTarget() const { return CurrentInteractTarget.IsValid(); }

	/** 按 E 时调用：对当前目标执行 Interact。返回是否真的触发了 */
	UFUNCTION(BlueprintCallable, Category = "LY|Interact")
	bool TryInteract();

	/** 立刻刷新一次目标（不等 Timer）。想主动重算时用 */
	UFUNCTION(BlueprintCallable, Category = "LY|Interact")
	AActor* RefreshInteractTarget();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Timer 回调：探测 + 调试绘制 */
	void HandleTraceTick();

	/** 真正打射线，返回命中的可交互物（可能为空） */
	AActor* DoTrace();

	/** 射线打到了东西、但没成为目标时打一条日志（同一个物体只报一次） */
	void LogRejectedHit(AActor* HitActor, const TCHAR* Reason);

	/** 设置当前目标；仅当变化时广播，并切换双方的高亮 */
	void SetInteractTarget(AActor* NewTarget);

	/** 计算射线起止点（相机优先，无相机则用眼睛视点） */
	void GetTraceSegment(FVector& OutStart, FVector& OutEnd) const;

	/** 定期探测用的 Timer */
	FTimerHandle TraceTimerHandle;

	/** 当前交互目标（弱引用，物体被销毁不会悬空） */
	TWeakObjectPtr<AActor> CurrentInteractTarget;

	/** 最近一次射线打到的 Actor（不管能不能交互）—— 只用于调试上色 */
	TWeakObjectPtr<AActor> LastTraceHit;

	/** 上一次因为「不可交互」报过日志的 Actor，避免每 0.1s 刷屏 */
	TWeakObjectPtr<AActor> LastRejectedLogged;
};