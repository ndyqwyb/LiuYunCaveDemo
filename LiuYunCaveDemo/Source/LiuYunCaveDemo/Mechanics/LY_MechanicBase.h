// LY_MechanicBase.h
// 机关基类 —— 所有机关的公共父类，只负责「状态 + 事件」，不含任何美术资源。
// 依据：03_机关设计规格.md 的 3.11 / 04_蓝图与C++协作.md 的 4.16。
//
// 它属于三层结构的哪一层？（04 的 4.3）
//   第 1 层 C++（本文件）：状态机、状态变化事件、交互接口的默认实现
//   第 2 层 蓝图子类（BP_LY_*）：只实现 OnStateChangedVisuals() —— 换材质 / 显隐火核 / 播粒子 / 播音效
//   第 3 层 关卡实例：覆盖 InitialState、交互提示文字等参数
//
// 三条纪律（04 的 4.16.1，违反会痛不欲生）：
//   1. bCanEverTick = false —— 机关不需要每帧更新，要定时就用 Timer
//   2. SetMechanicState() 里先 Broadcast 再调 OnStateChangedVisuals —— 逻辑永远先于表现
//   3. OnStateChangedVisuals() 只读状态，绝不改状态 —— 表现层永远不决定状态
//
// 为什么要基类？
//   后面所有机关（旋转桥 / 宝箱 / 风场 / 封印罩）都是「交互 -> 改状态 -> 广播 -> 表现更新」这条链的变体。
//   把这条链写在基类里，M4 做扎实，M5~M7 会快很多。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LY_Types.h"
#include "Mechanics/LY_Interactable.h"
#include "LY_MechanicBase.generated.h"

class USceneComponent;

// 前置声明：委托的参数类型就是本类的指针，而本类还没声明完，
// 所以必须先让编译器认识这个名字，DECLARE 宏才能通过。
class ALY_MechanicBase;

/**
 * 机关状态变化时广播给所有监听者（蓝图 Bind 这个事件就能收到）。
 * 用途：谜题总控统计「几个方碑亮了」、UI 刷新、音效触发等。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLY_OnMechanicStateChanged,
	ALY_MechanicBase*, Mechanic, ELY_MechanicState, NewState);

/**
 * 所有机关的公共父类。只负责：状态 + 事件。
 *
 * Abstract 是刻意的：基类没有任何视觉，出现在关卡放置列表里只会被误用，
 * 所以强制你必须派生（蓝图子类 BP_LY_XXX）。你在编辑器里找不到能直接摆的 ALY_MechanicBase，这是正常的。
 */
UCLASS(Abstract, Blueprintable)
class LIUYUNCAVEDEMO_API ALY_MechanicBase : public AActor, public ILY_Interactable
{
	GENERATED_BODY()

public:
	ALY_MechanicBase();

	// ---------------- 状态 ----------------

	/** 当前状态。谜题总控用它判断机关是否到位，不要自己去猜 bIsLit 之类的子类变量 */
	UFUNCTION(BlueprintPure, Category = "LY|Mechanic")
	ELY_MechanicState GetMechanicState() const { return MechanicState; }

	/** 是否已经生效 */
	UFUNCTION(BlueprintPure, Category = "LY|Mechanic")
	bool IsMechanicActive() const { return MechanicState == ELY_MechanicState::Active; }

	/**
	 * 改状态。只有状态真的变了才会广播；蓝图子类不要重写这个函数。
	 * 一次性机关（bOneShot）进入 Completed 之后就不再允许离开 Completed。
	 */
	UFUNCTION(BlueprintCallable, Category = "LY|Mechanic")
	void SetMechanicState(ELY_MechanicState NewState);

	/**
	 * 便捷方法：Available -> Active。
	 * 如果某个一次性机关需要「用完之后永久锁死」的语义，子类可以改调 SetMechanicState(Completed)。
	 */
	UFUNCTION(BlueprintCallable, Category = "LY|Mechanic")
	void SetActive();

	// ---------------- 事件 / 表现钩子 ----------------

	/** 状态变化事件：逻辑层监听（谜题总控、UI） */
	UPROPERTY(BlueprintAssignable, Category = "LY|Mechanic")
	FLY_OnMechanicStateChanged OnMechanicStateChanged;

	/**
	 * 表现层钩子：由蓝图子类实现（换材质、显隐、粒子、音效、灯光）。
	 * C++ 不写任何美术代码；基类不实现它，派生类不实现也不会报错（只是没有表现）。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LY|Mechanic")
	void OnStateChangedVisuals(ELY_MechanicState NewState);

	// ---------------- 交互（ILY_Interactable 的默认实现）----------------
	// 注意：这三个是接口函数，只做 override，不要加 UFUNCTION，否则反射冲突。

	/** 默认规则：只有 Available 才能被交互。=== 交互门控的唯一入口 === */
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** 默认行为：置为 Active。子类按自己的机关语义重写（例如火方碑改 bIsLit） */
	virtual void Interact_Implementation(AActor* Interactor) override;

	/** 返回 InteractPrompt 里的文字（例：「按 E 点燃」） */
	virtual FText GetInteractPrompt_Implementation(AActor* Interactor) const override;

protected:
	virtual void BeginPlay() override;

	/**
	 * 根组件。基类没有任何网格，但需要一个根，蓝图子类加的网格/粒子/灯光才有地方挂。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LY|Mechanic")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 初始状态：关卡/蓝图实例里可改（有的机关一开始就是 Active） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LY|Mechanic")
	ELY_MechanicState InitialState = ELY_MechanicState::Available;

	/** 是否一次性机关。true = 进入 Completed 后永久锁死 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LY|Mechanic")
	bool bOneShot = true;

	/** 交互提示文字。由 GetInteractPrompt() 返回，不要在蓝图里硬编码 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LY|Mechanic")
	FText InteractPrompt = NSLOCTEXT("LY", "DefaultMechanicPrompt", "按 E 交互");

	/** 运行时状态。构造后是 Dormant，BeginPlay 时才按 InitialState 生效 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LY|Mechanic")
	ELY_MechanicState MechanicState = ELY_MechanicState::Dormant;
};