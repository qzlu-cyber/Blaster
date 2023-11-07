// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner()); // 获取角色
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!BlasterCharacter) BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	if (!BlasterCharacter) return;

	FVector Velocity = BlasterCharacter->GetVelocity(); // 获取角色速度
	Velocity.Z = 0.f; // 只考虑水平速度
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bIsAiming = BlasterCharacter->IsAiming();

	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation(); // 获取角色的瞄准方向。 global rotation
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity()); // 获取角色的移动方向。 local rotation
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation); // 计算移动方向和瞄准方向的偏移量
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f); // 插值计算角色旋转的变化量，使得动画更加平滑
	YawOffset = DeltaRotation.Yaw;

	CharacterLastFrameRotation = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation(); // 获取角色的旋转
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterLastFrameRotation); // 计算角色旋转的变化量
	const float Target = Delta.Yaw / DeltaSeconds; // 缩放变化量，得到角色旋转的目标值
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f); // 插值计算角色旋转的目标值
	Lean = FMath::Clamp(Interp, -90.f, 90.f); // 限制角色旋转的范围

	AOYaw = BlasterCharacter->GetAOYaw();
	AOPitch = BlasterCharacter->GetAOPitch();
}
