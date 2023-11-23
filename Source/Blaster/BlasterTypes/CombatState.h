#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"), // 未占用
	ECS_Reloading UMETA(DisplayName = "Reloading"), // 换弹

	ECS_MAX UMETA(DisplayName = "DefaultMax")
};