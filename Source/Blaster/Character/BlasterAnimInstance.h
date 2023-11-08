// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BlasterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(BlueprintReadOnly, Category=Character, meta=(AllowPrivateAccess="true"))
	class ABlasterCharacter* BlasterCharacter; // 角色
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	float Speed; // 移动速度
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	bool bIsInAir; // 是否在空中
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	bool bIsAccelerating; // 是否在按下移动键
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	bool bIsCrouched; // 是否蹲下
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bWeaponEquipped; // 是否装备武器
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bIsAiming; // 是否在瞄准
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	float YawOffset; // 相对于摄像机方向，角色前进方向的偏移量
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	float Lean; // 角色倾斜
	FRotator DeltaRotation; // 角色旋转的变化量
	FRotator CharacterLastFrameRotation; // 角色上一帧的旋转
	FRotator CharacterRotation; // 角色当前的旋转

	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	float AOYaw; // Aim Offset Yaw
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	float AOPitch; // Aim Offset Pitch

	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	FTransform LeftHandTransform; // Fabrik 手臂 IK 的目标位置

	class AWeapon* EquippedWeapon; // 当前装备的武器
};
