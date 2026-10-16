// LY_FirePillar.cpp
// 火方碑的实现（依据 03 的 3.2 / 3.13）。
//
// 这个文件里没有一行美术代码，这是刻意的：
//   逻辑（亮没亮、能不能交互）在 C++；表现（材质、粒子、灯光、声音）在蓝图。
//   想改「看起来什么样」-> 改 BP_LY_FirePillar，不要改这里。

#include "Mechanics/LY_FirePillar.h"

ALY_FirePillar::ALY_FirePillar()
{
	// 交互提示文字由基类的 GetInteractPrompt() 返回，这里只换文案。
	InteractPrompt = NSLOCTEXT("LY", "LY_LightPillarPrompt", "按 E 点燃");
}

void ALY_FirePillar::BeginPlay()
{
	Super::BeginPlay();	// 基类在这里按 InitialState 设好机关状态

	// 封印：只管门控，不改机关状态 —— 「被罩住」和「没点燃」是两件事。
	bIsSealed = bStartSealed;
	OnSealedVisuals(bIsSealed);

	if (bStartLit)
	{
		// 开局就亮的方碑：走一次完整的 SetLit（含广播 + 状态同步），别绕过它直接改 bIsLit
		SetLit(true);
	}
	else
	{
		// 开局暗态：也要调一次表现钩子，让蓝图把材质/火核/灯光摆到暗态（否则蓝图里配的默认值可能不是暗态）
		OnLitVisuals(false);
	}
}

void ALY_FirePillar::SetLit(bool bNewLit)
{
	// 状态没变直接返回：这是「重复触发不出错」的第一道防线（第二道是 CanInteract 门控）
	if (bIsLit == bNewLit)
	{
		return;
	}

	bIsLit = bNewLit;

	// 同步机关状态：亮 = Active，暗 = Available。
	// 好处：CanInteract 的门控规则不用重写，状态一变交互能力自动跟着变。
	//
	// 注意：基类的 bOneShot 只锁 Completed 状态。方碑目前只在 Active/Available 之间切换，
	// 不会被锁死；以后若做成一靠近就 Completed 的一次性方碑，记得同步检查这里。
	SetMechanicState(bIsLit ? ELY_MechanicState::Active : ELY_MechanicState::Available);

	// 顺序和基类一致：先广播（逻辑层），再表现（美术层）
	OnLitChanged.Broadcast(bIsLit);
	OnLitVisuals(bIsLit);
}

void ALY_FirePillar::ToggleLit()
{
	SetLit(!bIsLit);
}

void ALY_FirePillar::SetSealed(bool bNewSealed)
{
	if (bIsSealed == bNewSealed)
	{
		return;
	}

	bIsSealed = bNewSealed;

	OnSealedChanged.Broadcast(bIsSealed);
	OnSealedVisuals(bIsSealed);
}

bool ALY_FirePillar::CanInteract_Implementation(AActor* Interactor) const
{
	// 封印中：一律不可交互（规格 3.26 —— 罩子存在期间 CanInteract 返回 false）
	if (bIsSealed)
	{
		return false;
	}

	// 否则沿用基类规则：只有 Available（未点燃且可用）才能交互
	return Super::CanInteract_Implementation(Interactor);
}

void ALY_FirePillar::Interact_Implementation(AActor* Interactor)
{
	// 方碑的默认语义就是「点亮」。
	// 记忆谜题里若要「再按一次熄灭」，由谜题总控调 ToggleLit()，方碑自己不猜意图。
	SetLit(true);
}