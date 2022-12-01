#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied State"),
	ECS_Reloading UMETA(DisplayName = "Reloading State"),
	ECS_Throwing UMETA(DisplayName = "Throwing State"),
	ECS_MAX UMETA(DisplayName = "Max")
};
