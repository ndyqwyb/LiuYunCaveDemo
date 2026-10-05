// LY_InteractComponent.cpp

#include "Player/LY_InteractComponent.h"
#include "Mechanics/LY_Interactable.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogLYInteraction);

ULY_InteractComponent::ULY_InteractComponent()
{
	// ★ 不用 Tick：探测交给 Timer
	PrimaryComponentTick.bCanEverTick = false;
}

// ------------------------------------------------------------------ 生命周期

void ULY_InteractComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1) 解析射线起点相机：优先用显式指定的，否则自动找角色身上的 CameraComponent
	if (!TraceCamera)
	{
		if (AActor* Owner = GetOwner())
		{
			TraceCamera = Owner->FindComponentByClass<UCameraComponent>();
		}
	}

	if (bLogTargetChanges)
	{
		UE_LOG(LogLYInteraction, Log, TEXT("[交互] 组件启动：Owner=%s, 相机=%s, 范围=%.0f, 间隔=%.2f"),
			*GetNameSafe(GetOwner()),
			TraceCamera ? *TraceCamera->GetName() : TEXT("<无，改用眼睛视点>"),
			InteractRange, TraceInterval);
	}

	// 2) 用 Timer 定期探测（不用 Tick，省性能）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TraceTimerHandle,
			this,
			&ULY_InteractComponent::HandleTraceTick,
			TraceInterval,
			/*bLoop=*/true,
			/*FirstDelay=*/0.f);
	}
}

void ULY_InteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraceTimerHandle);
	}

	// 清掉当前目标的高亮，并广播一次「无目标」，避免 UI 残留
	SetInteractTarget(nullptr);

	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------ 探测

void ULY_InteractComponent::HandleTraceTick()
{
	AActor* NewTarget = RefreshInteractTarget();

	if (bDrawDebug)
	{
		FVector Start, End;
		GetTraceSegment(Start, End);

		// 绿 = 锁定成功；黄 = 打到了东西但不能交互；灰 = 什么都没打到
		FColor LineColor = FColor::Silver;
		if (NewTarget)
		{
			LineColor = FColor::Green;
		}
		else if (LastTraceHit.IsValid())
		{
			LineColor = FColor::Yellow;
		}

		DrawDebugLine(GetWorld(), Start, End, LineColor, /*bPersistent=*/false, TraceInterval, /*DepthPriority=*/0, /*Thickness=*/1.5f);
	}
}

AActor* ULY_InteractComponent::RefreshInteractTarget()
{
	AActor* NewTarget = DoTrace();
	SetInteractTarget(NewTarget);
	return NewTarget;
}

AActor* ULY_InteractComponent::DoTrace()
{
	LastTraceHit = nullptr;

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return nullptr;
	}

	FVector Start, End;
	GetTraceSegment(Start, End);

	// ★ 忽略自己：否则第三人称相机的位置会先打到自己
	FCollisionQueryParams Params(FName(TEXT("LYInteractTrace")), /*bTraceComplex=*/false, Owner);
	Params.AddIgnoredActor(Owner);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	LastTraceHit = HitActor;

	if (!HitActor || !HitActor->Implements<ULY_Interactable>())
	{
		LogRejectedHit(HitActor, TEXT("没有实现 LY_Interactable 接口"));
		return nullptr;
	}

	// 目标自己说了算：现在能不能交互（冷却中 / 已完成的一次性机关会返回 false）
	if (!ILY_Interactable::Execute_CanInteract(HitActor, Owner))
	{
		LogRejectedHit(HitActor, TEXT("CanInteract 返回 false"));
		return nullptr;
	}

	return HitActor;
}

void ULY_InteractComponent::GetTraceSegment(FVector& OutStart, FVector& OutEnd) const
{
	const AActor* Owner = GetOwner();

	if (TraceCamera)
	{
		OutStart = TraceCamera->GetComponentLocation();

		// ★ 关键：射线从相机出发，但「交互距离」以玩家为基准。
		//   弹簧臂让相机落在角色身后（默认 400uu），若只走 InteractRange，
		//   射线连角色都够不到。所以长度 = 相机到角色的距离 + InteractRange。
		float TraceLength = InteractRange;
		if (Owner)
		{
			TraceLength += FVector::Dist(OutStart, Owner->GetActorLocation());
		}

		OutEnd = OutStart + TraceCamera->GetForwardVector() * TraceLength;
		return;
	}

	// 兜底：没有相机就用角色的眼睛视点（这里距离天然以角色为基准，不需要加偏移）
	FVector EyeLocation = FVector::ZeroVector;
	FRotator EyeRotation = FRotator::ZeroRotator;

	if (Owner)
	{
		Owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	}

	OutStart = EyeLocation;
	OutEnd = EyeLocation + EyeRotation.Vector() * InteractRange;
}

void ULY_InteractComponent::SetInteractTarget(AActor* NewTarget)
{
	AActor* OldTarget = CurrentInteractTarget.Get();
	if (OldTarget == NewTarget)
	{
		return;
	}

	// 旧目标取消高亮，新目标点亮高亮（SetHighlighted 默认是空实现，不实现也不会报错）
	if (OldTarget && OldTarget->Implements<ULY_Interactable>())
	{
		ILY_Interactable::Execute_SetHighlighted(OldTarget, false);
	}
	if (NewTarget && NewTarget->Implements<ULY_Interactable>())
	{
		ILY_Interactable::Execute_SetHighlighted(NewTarget, true);
	}

	CurrentInteractTarget = NewTarget;

	if (bLogTargetChanges)
	{
		if (NewTarget)
		{
			UE_LOG(LogLYInteraction, Log, TEXT("[交互] 锁定目标: %s"), *NewTarget->GetName());
		}
		else
		{
			UE_LOG(LogLYInteraction, Log, TEXT("[交互] 目标丢失（离开范围或不可交互）"));
		}
	}

	// ★ 逻辑先响，再由蓝图更新 UI
	OnInteractTargetChanged.Broadcast(NewTarget);
}

// ------------------------------------------------------------------ 交互

bool ULY_InteractComponent::TryInteract()
{
	AActor* Target = CurrentInteractTarget.Get();
	AActor* Interactor = GetOwner();

	if (!Target || !Interactor)
	{
		return false;
	}

	if (!Target->Implements<ULY_Interactable>())
	{
		return false;
	}

	// 二次确认：目标可能在两次探测之间变了状态
	if (!ILY_Interactable::Execute_CanInteract(Target, Interactor))
	{
		return false;
	}

	ILY_Interactable::Execute_Interact(Target, Interactor);

	UE_LOG(LogLYInteraction, Log, TEXT("[交互] 已对 %s 执行 Interact"), *Target->GetName());

	// ★ 顺序硬要求：先广播「交互成功」，再重算目标。
	//   一次性机关重算后会变成不可交互 → 广播 nullptr → 蓝图播 FadeOut。
	//   UMG 里后播的动画会打断前一条，FadeOut 必须最后播，否则提示会半透明地卡在屏幕上。
	OnInteractPerformed.Broadcast(Target);

	// 交互后立刻重算：一次性机关完成后应该马上从准星里消失
	RefreshInteractTarget();

	return true;
}

// ------------------------------------------------------------------ 调试

void ULY_InteractComponent::LogRejectedHit(AActor* HitActor, const TCHAR* Reason)
{
	// 同一个物体只报一次，避免每 0.1s 刷屏
	if (!bLogTargetChanges || !HitActor || LastRejectedLogged.Get() == HitActor)
	{
		return;
	}
	LastRejectedLogged = HitActor;

	UE_LOG(LogLYInteraction, Warning, TEXT("[交互] 射线打在 %s 上，但没成为目标：%s"), *HitActor->GetName(), Reason);
}
