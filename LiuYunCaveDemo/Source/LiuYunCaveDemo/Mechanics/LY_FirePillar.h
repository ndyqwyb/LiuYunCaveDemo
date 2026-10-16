// LY_FirePillar.h
// 火元素方碑 —— 本项目第一个「完整机关闭环」：交互 -> 状态改变 -> 广播 -> 表现更新。
// 依据：03_机关设计规格.md 的 3.2（规格）/ 3.13（分工）；04 的 4.17 只是最小骨架，以 3.2/3.13 为准。
//
// 视觉方案已定 B2：镂空石柱 + 中段一颗悬浮的发光火核（不是整根柱子发光）。
//
// 本类 / 基类 / 蓝图，各管什么？
//   * ALY_MechanicBase（父类）：状态机、状态广播、交互接口的默认实现
//   * 本类：方碑特有的三个数据（MechIndex / bIsLit / bIsSealed）+ 三组事件 + 封印门控
//   * BP_LY_FirePillar（蓝图）：只做表现 —— 切亮暗材质实例、显隐火核、播/停火焰粒子、开关点光、播声音、显隐封印罩
//
// 两条纪律：
//   1. 蓝图里绝对不要再写「点燃」逻辑。蓝图只响应 OnLitVisuals / OnSealedVisuals。
//   2. SetLit / SetSealed 内部：先广播逻辑事件，再调表现钩子（顺序和基类保持一致）。

#pragma once

#include "CoreMinimal.h"
#include "Mechanics/LY_MechanicBase.h"
#include "LY_FirePillar.generated.h"

/** 亮 / 暗变化时广播（订阅者自己知道自己订的是哪一根方碑） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLY_OnPillarLitChanged, bool, bIsLit);

/** 封印罩出现 / 解除时广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLY_OnPillarSealedChanged, bool, bIsSealed);

/**
 * 火元素方碑。谜题的基本单元。
 * 交互：走近 -> 准星对准 -> 提示「按 E 点燃」-> 按 E 点亮（规格 3.2 的 A1 方案）。
 */
UCLASS(Blueprintable)
class LIUYUNCAVEDEMO_API ALY_FirePillar : public ALY_MechanicBase
{
	GENERATED_BODY()

public:
	ALY_FirePillar();

	// ---------------- 关卡里配置的参数 ----------------

	/**
	 * 谜题里的编号（1 起）。关卡实例里逐个配置。
	 * 记忆顺序谜题靠它判断「玩家点的顺序对不对」，所以同一组方碑里必须唯一。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LY|FirePillar")
	int32 MechIndex = 1;

	/** 开局就是亮的？给「一开始就点燃」的方碑用 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LY|FirePillar")
	bool bStartLit = false;

	/** 开局就带封印罩？给需要「先解封印才能点」的方碑用 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LY|FirePillar")
	bool bStartSealed = false;

	// ---------------- 查询 ----------------

	/** 现在亮着吗 */
	UFUNCTION(BlueprintPure, Category = "LY|FirePillar")
	bool IsLit() const { return bIsLit; }

	/** 现在被封印罩罩着吗 */
	UFUNCTION(BlueprintPure, Category = "LY|FirePillar")
	bool IsSealed() const { return bIsSealed; }

	// ---------------- 操作（逻辑入口，蓝图和谜题总控都调它们）----------------

	/**
	 * 点亮 / 熄灭。状态没变就直接返回（不会重复播表现）。
	 * 同时把机关状态同步成 Active（亮）/ Available（暗），交互门控自动跟着变。
	 */
	UFUNCTION(BlueprintCallable, Category = "LY|FirePillar")
	void SetLit(bool bNewLit);

	/** 亮 -> 暗 / 暗 -> 亮。谜题要「再按一次熄灭」时由谜题总控调用 */
	UFUNCTION(BlueprintCallable, Category = "LY|FirePillar")
	void ToggleLit();

	/** 开关封印。只影响「能不能交互」，不改变机关状态 */
	UFUNCTION(BlueprintCallable, Category = "LY|FirePillar")
	void SetSealed(bool bNewSealed);

	// ---------------- 事件（逻辑层监听）----------------

	/** 亮暗变化：谜题总控统计亮了几根、UI 刷新用 */
	UPROPERTY(BlueprintAssignable, Category = "LY|FirePillar")
	FLY_OnPillarLitChanged OnLitChanged;

	/** 封印变化：宿主蓝图开关罩子、播解锁音效用 */
	UPROPERTY(BlueprintAssignable, Category = "LY|FirePillar")
	FLY_OnPillarSealedChanged OnSealedChanged;

	// ---------------- 表现钩子（蓝图实现，C++ 不写美术代码）----------------

	/** 蓝图在这里：切材质实例、显隐火核、播/停火焰、开关点光、播声音 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LY|FirePillar")
	void OnLitVisuals(bool bNewLit);

	/** 蓝图在这里：显隐封印罩（M6 再接也行）、播封印出现/解除音效 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LY|FirePillar")
	void OnSealedVisuals(bool bNewSealed);

	// ---------------- 交互（覆盖基类的两个）----------------

	/** 封印中一律不可交互；否则沿用基类规则（只有 Available 可交互） */
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** 默认语义：按 E 点亮 */
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual void BeginPlay() override;

	/** 运行时状态：亮没亮 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LY|FirePillar")
	bool bIsLit = false;

	/** 运行时状态：是否被封印 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LY|FirePillar")
	bool bIsSealed = false;
};