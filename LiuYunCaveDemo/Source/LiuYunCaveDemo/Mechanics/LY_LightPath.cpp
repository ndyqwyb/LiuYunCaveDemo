// LY_LightPath.cpp
// 光桥的实现。这里一行美术代码都没有 —— 长什么样全在 BP_LY_LightPath 里。
//
// 一条时间线（旋转桥会这样调它）：
//   Retract(RetractDuration)  ->  收到 OnRetractFinished  ->  Rotating  ->  GrowIn(GrowDuration)
// 本类只负责「什么时候算收完 / 长完」和「碰撞什么时候开」，不负责转盘。

#include "Mechanics/LY_LightPath.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

ALY_LightPath::ALY_LightPath()
{
	// 机关类一律不 Tick，要等时间就用 Timer。
	PrimaryActorTick.bCanEverTick = false;

	// 纯单机 Demo，不需要复制。
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 站立碰撞：一个 Box 覆盖整条桥。它只是「能不能踩」，跟视觉上的擦除完全解耦。
	// 尺寸在关卡实例里调（Extent 默认给个占位值，别当成最终尺寸）。
	WalkableCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WalkableCollision"));
	WalkableCollision->SetupAttachment(SceneRoot);
	WalkableCollision->SetBoxExtent(FVector(200.f, 80.f, 10.f));
	WalkableCollision->SetCollisionProfileName(TEXT("BlockAll"));
	WalkableCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ALY_LightPath::BeginPlay()
{
	Super::BeginPlay();

	// 开局按 bStartShown 对齐一次：碰撞先定好，再让蓝图把 Wipe 摆到 0 或 1。
	// 用 Duration = 0 调钩子，蓝图那边要走「瞬间置终态」的分支，别跑 Timeline。
	bShownTarget = bStartShown;
	ApplyWalkable(bShownTarget && bWalkableWhenShown);

	if (bStartShown)
	{
		OnGrowVisuals(0.f);
	}
	else
	{
		OnRetractVisuals(0.f);
	}
}

void ALY_LightPath::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 关卡切换 / Actor 销毁时把还挂着的 Timer 清掉，避免回调打到已经死掉的 Actor 上。
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ALY_LightPath::GrowIn(float Duration)
{
	StartTransition(/*bGrow=*/true, Duration);
}

void ALY_LightPath::Retract(float Duration)
{
	StartTransition(/*bGrow=*/false, Duration);
}

void ALY_LightPath::StartTransition(bool bGrow, float Duration)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 已经在目标状态、又没在跑动画 -> 不用重播（否则会先闪一下再动）。
	// 典型场景：开局就是隐藏的那一段，第一次收回不该先亮一下。
	if (!bBusy && bShownTarget == bGrow)
	{
		Duration = 0.f;
	}

	// 重入保护：上一个 Timer 直接丢掉，新的时长重新计时。
	World->GetTimerManager().ClearTimer(TransitionTimer);

	bBusy = true;
	bShownTarget = bGrow;
	bTransitionIsGrow = bGrow;
	ActiveDuration = Duration;

	// 碰撞规则（先逻辑后表现，和基类一致）：
	//   收回 = 调用瞬间就断开，不能让人站在正在消失的桥上；
	//   生长 = 等长完再打开（见 HandleTransitionFinished）。
	if (!bGrow)
	{
		ApplyWalkable(false);
	}

	if (bDebugLightPath)
	{
		UE_LOG(LogTemp, Log, TEXT("[LY|LightPath] %s %s duration=%.2f"),
			*GetName(), bGrow ? TEXT("Grow") : TEXT("Retract"), Duration);
	}

	// 表现交给蓝图：Wipe 0->1 或 1->0。
	if (bGrow)
	{
		OnGrowVisuals(Duration);
	}
	else
	{
		OnRetractVisuals(Duration);
	}

	if (Duration <= KINDA_SMALL_NUMBER)
	{
		// 瞬间完成：直接走收尾（会广播事件），不必等下一帧。
		HandleTransitionFinished();
		return;
	}

	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &ALY_LightPath::HandleTransitionFinished);
	World->GetTimerManager().SetTimer(TransitionTimer, Delegate, Duration, /*bLoop=*/false);
}

void ALY_LightPath::HandleTransitionFinished()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TransitionTimer);
	}

	const bool bWasGrow = bTransitionIsGrow;
	bBusy = false;
	ActiveDuration = 0.f;

	// 生长完成才允许踩。
	if (bWasGrow)
	{
		ApplyWalkable(bWalkableWhenShown);
	}

	if (bDebugLightPath)
	{
		UE_LOG(LogTemp, Log, TEXT("[LY|LightPath] %s %s finished"),
			*GetName(), bWasGrow ? TEXT("Grow") : TEXT("Retract"));
	}

	// 先逻辑事件，再让蓝图自己做收尾表现（例如关掉粒子的发射器）。
	if (bWasGrow)
	{
		OnGrowFinished.Broadcast();
	}
	else
	{
		OnRetractFinished.Broadcast();
	}
}

void ALY_LightPath::SetWalkable(bool bNewWalkable)
{
	ApplyWalkable(bNewWalkable);
}

void ALY_LightPath::ApplyWalkable(bool bNewWalkable)
{
	if (!WalkableCollision)
	{
		return;
	}

	WalkableCollision->SetCollisionEnabled(
		bNewWalkable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

bool ALY_LightPath::IsWalkable() const
{
	return WalkableCollision && WalkableCollision->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
}

