// LY_MechanicBase.cpp
// 机关基类实现 —— 状态机 + 广播 + 交互默认行为（依据 03 的 3.11 / 04 的 4.16.1）。
//
// 读这个文件时先记住一件事：
//   这里一个美术资源都没有，也不该有。所有「长什么样」的东西都在蓝图子类的 OnStateChangedVisuals 里。

#include "Mechanics/LY_MechanicBase.h"
#include "Components/SceneComponent.h"

ALY_MechanicBase::ALY_MechanicBase()
{
	// 纪律 1：机关不需要每帧更新。要定时就用 Timer（例如火方碑的 0.25s 过渡交给蓝图动画）。
	PrimaryActorTick.bCanEverTick = false;

	// Demo 是纯单机，关掉网络复制，省掉一堆无意义的开销。
	bReplicates = false;

	// 一个空根组件：让蓝图子类挂的网格 / 粒子 / 灯光有个共同的父节点。
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ALY_MechanicBase::BeginPlay()
{
	Super::BeginPlay();

	// 用 InitialState 覆盖构造时的 Dormant。
	// 注意：如果 InitialState 不是 Dormant，这里会立刻广播一次 —— 那个时刻其他 Actor 可能还没开始 BeginPlay。
	// 所以监听者（后面的谜题总控）不要只依赖事件，开局要主动 GetMechanicState() 读一次当前状态。
	SetMechanicState(InitialState);
}

void ALY_MechanicBase::SetMechanicState(ELY_MechanicState NewState)
{
	// 1) 状态没变就直接返回 —— 保证「重复触发不出错」，也不会重复播表现。
	if (MechanicState == NewState)
	{
		return;
	}

	// 2) 一次性机关进入 Completed 之后永久锁死，不允许再改回来。
	if (bOneShot && MechanicState == ELY_MechanicState::Completed)
	{
		return;
	}

	MechanicState = NewState;

	// 3) 纪律 2：先广播（逻辑层先收到），再调表现钩子（美术层后处理）。
	OnMechanicStateChanged.Broadcast(this, NewState);
	OnStateChangedVisuals(NewState);
}

void ALY_MechanicBase::SetActive()
{
	SetMechanicState(ELY_MechanicState::Active);
}

bool ALY_MechanicBase::CanInteract_Implementation(AActor* Interactor) const
{
	// 交互门控的唯一入口：只有 Available 能被选中。
	// 这一条 if 同时保证了两件事：
	//   * 交互后状态变成 Active/Completed -> 立刻不可再次交互（重复按 E 不会出错）
	//   * Dormant（未解锁）的机关玩家碰不到
	// 子类如果需要附加条件（例如火方碑的封印罩），在这里 or 上自己的判断。
	return MechanicState == ELY_MechanicState::Available;
}

void ALY_MechanicBase::Interact_Implementation(AActor* Interactor)
{
	// 默认行为：交互一次就算「已生效」。子类通常重写（火方碑会去改 bIsLit）。
	SetActive();
}

FText ALY_MechanicBase::GetInteractPrompt_Implementation(AActor* Interactor) const
{
	return InteractPrompt;
}