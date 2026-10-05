// LY_Interactable.h
// 交互接口 —— 整个 Demo 使用频率最高的系统（依据 03_机关设计规格.md 的 3.12 / 04 的 4.15）。
//
// 这是什么？
//   一份「合同」。任何物体只要声明实现本接口，就承诺提供下面 4 个能力。
//   玩家射线只认这份合同，不关心对面到底是火方碑、宝箱还是石柱 —— 所以同一套交互逻辑能复用到所有机关。
//
// 谁实现它？
//   * C++ 机关类：class ALY_FirePillar : public ALY_MechanicBase, public ILY_Interactable
//   * 蓝图类：Class Settings -> Interfaces -> Add -> LY Interactable（M3 的测试物就走这条路）
//
// 为什么用 BlueprintNativeEvent？
//   C++ 给每个函数一份「默认实现」，蓝图可以只覆盖自己关心的那个。
//   例：宝箱蓝图只需覆盖 Interact，CanInteract / GetInteractPrompt 用默认值即可。
//
// 必须记住的两个坑：
//   1. 蓝图里的事件名带空格：Can Interact / Interact / Get Interact Prompt / Set Highlighted
//   2. C++ 的默认实现必须叫「函数名 + _Implementation」，否则链接错误
//
// 本文件是纯接口：只有 .h，没有 .cpp。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LY_Interactable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class ULY_Interactable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可交互物体接口。
 * 玩家侧（ULY_InteractComponent）只通过这个接口跟物体打交道，不认识具体机关类型。
 */
class LIUYUNCAVEDEMO_API ILY_Interactable
{
	GENERATED_BODY()

public:
	/**
	 * 现在能不能交互？
	 * 默认 true。可用于：冷却中、已锁定、已完成的一次性机关 等场景。
	 * 组件每次锁定目标时都会问一次，返回 false 就不会成为交互目标。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LY|Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const { return true; }

	/**
	 * 执行交互。按下 IA_LY_Interact 时由玩家调用。
	 * 这是子类最常覆盖的一个：物体在这里做自己的反应（点燃、开箱、旋转…）。
	 * Interactor = 发起交互的 Actor（通常是玩家），有的机关需要它（例如给玩家发东西）。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LY|Interaction")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor) {}

	/**
	 * 交互提示文字，显示在 WBP_LY_InteractPrompt 上，例如「按 E 点燃」。
	 * 注意：不同物体的文字必须不同 —— 由这里返回，不要在蓝图里硬编码。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LY|Interaction")
	FText GetInteractPrompt(AActor* Interactor) const;
	virtual FText GetInteractPrompt_Implementation(AActor* Interactor) const
	{
		return NSLOCTEXT("LY", "DefaultInteractPrompt", "按 E 交互");
	}

	/**
	 * 当前目标是否高亮（描边 / 发光）。
	 * 默认空实现；M3 可以不用，留给后面做「聚焦反馈」。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LY|Interaction")
	void SetHighlighted(bool bHighlighted);
	virtual void SetHighlighted_Implementation(bool bHighlighted) {}
};