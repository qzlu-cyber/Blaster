// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/BlasterTypes/TurnInPlace.h"

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

	UPROPERTY()
	class AWeapon* EquippedWeapon; // 当前装备的武器

	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	ETurnInPlace TurnInPlace; // 转身动画
	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	bool bRotateRootBone; // 是否旋转角色的根骨骼，SimulateProxy 不使用旋转角色的根骨骼来处理动画

	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	FRotator RightHandRotation; // 右手的旋转，用于将武器的枪口旋转到准星的位置

	UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
	bool bIsLocallyControlled; // 是否是本地控制的角色

	UPROPERTY(BlueprintReadOnly, Category="Player Stats", meta=(AllowPrivateAccess="true"))
	bool bIsElimmed; // 角色是否死亡，用于播放动画

	UPROPERTY(BlueprintReadOnly, Category="Player Stats", meta=(AllowPrivateAccess="true"))
	bool bUseFabrik; // 换弹、投掷手雷时禁用固定左手的 IK

	UPROPERTY(BlueprintReadOnly, Category="Player Stats", meta=(AllowPrivateAccess="true"))
	bool bTransformRightHand; // 换弹、投掷手雷时禁用右手变换

	UPROPERTY(BlueprintReadOnly, Category="Player Stats", meta=(AllowPrivateAccess="true"))
	bool bUseAimOffset; // 换弹、投掷手雷时禁用 Aim Offset
};
