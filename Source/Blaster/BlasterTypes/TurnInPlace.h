#pragma once

// 用以表示角色的转身动画
UENUM(BlueprintType)
enum class ETurnInPlace : uint8
{
	ETIP_Left UMETA(DisplayName = "Turn Left"),
	ETIP_Right UMETA(DisplayName = "Turn Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),

	ETIP_Max UMETA(DisplayName = "DefaultMax")
};