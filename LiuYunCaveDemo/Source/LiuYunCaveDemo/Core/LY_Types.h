// LY_Types.h
// 公共类型定义 —— 全项目共享的枚举 / 结构体（依据 04_蓝图与C++协作.md 的 4.14）。
//
// 为什么单独一个文件，而不写在某个 Actor 的头文件里？
//   因为这些类型被多个系统共用（玩家、机关、谜题、UI 都要认识它）。
//   写在某个 Actor 里会导致别的系统为了拿一个枚举而包含整个 Actor 头文件，容易形成循环依赖。
//
// 维护约定：只往这里加「两个以上系统都要用」的类型。
//   现在只有机关状态；后续里程碑再追加（ELY_GamePhase 见 4.14、FLY_PuzzleResult 见 4.14）。

#pragma once

#include "CoreMinimal.h"
#include "LY_Types.generated.h"

/**
 * 机关通用状态。所有机关（火方碑 / 转盘 / 宝箱 / 光路 / 风场…）共用同一套语义，
 * 这样「谜题总控」不用认识具体机关类型，只问状态就能工作。
 *
 * Dormant   ：还没到能用的时机（例如风场未解锁）。玩家射线不会选中它。
 * Available ：就绪，可以被选中、可以交互。
 * Active    ：已生效（火方碑被点燃、转盘转到位）。
 * Completed ：一次性机关用完了，之后永远不再接受交互。
 *
 * 交互门控规则（见 ALY_MechanicBase::CanInteract_Implementation）：
 *   只有 Available 允许交互 —— 所以「交互后状态切换」和「重复触发不出错」是同一条规则保证的。
 */
UENUM(BlueprintType)
enum class ELY_MechanicState : uint8
{
	Dormant   UMETA(DisplayName = "休眠（不可交互）"),
	Available UMETA(DisplayName = "可交互"),
	Active    UMETA(DisplayName = "已激活"),
	Completed UMETA(DisplayName = "已完成")
};