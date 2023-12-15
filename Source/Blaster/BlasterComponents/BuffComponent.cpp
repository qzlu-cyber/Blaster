// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldHealRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bIsHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal = HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bIsHealing || Character == nullptr || Character->IsElimmed()) return; // 如果没有在恢复血量，或者角色为空，或者角色已经被淘汰，就不再恢复血量

	const float HealThisFrame = HealingRate * DeltaTime; // 每帧恢复的血量
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	Character->UpdateHealthHUD();
	AmountToHeal -= HealThisFrame; // 每帧减去已经恢复的血量

	// 如果待恢复的血量小于等于 0，或者角色的血量已经达到最大值，就停止恢复血量
	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bIsHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldHeal(float ShieldHealAmount, float HealingTime)
{
	bIsShieldHealing = true;
	ShieldHealingRate = ShieldHealAmount / HealingTime;
	AmountToShieldHeal = ShieldHealAmount;
}

void UBuffComponent::ShieldHealRampUp(float DeltaTime)
{
	if (!bIsShieldHealing || Character == nullptr || Character->IsElimmed()) return;

	const float ShieldHealThisFrame = ShieldHealingRate * DeltaTime;
	Character->SetShield(FMath::Clamp(Character->GetShield() + ShieldHealThisFrame, 0.f, Character->GetMaxShield()));
	Character->UpdateShieldHUD();
	AmountToShieldHeal -= ShieldHealThisFrame;

	if (AmountToShieldHeal <= 0.f || Character->GetShield() == Character->GetMaxShield())
	{
		bIsShieldHealing = false;
		AmountToShieldHeal = 0.f;
	}
}

void UBuffComponent::SetInitialSpeeds(float WalkSpeed, float CrouchSpeed)
{
	InitialWalkSpeed = WalkSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SpeedBuff(float WalkBuffSpeed, float CrouchBuffSpeed, float SpeedBuffTime)
{
	if (Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		SpeedTimer,
		this,
		&UBuffComponent::SpeedTimerFinished,
		SpeedBuffTime
	);

	if (Character->GetCharacterMovement()) // 只在 server 端生效
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = WalkBuffSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchBuffSpeed;
	}

	// server 端调用 多播 RPC 在所有 client 上执行速度 buff
	MulticastSpeed(WalkBuffSpeed, CrouchBuffSpeed);
}

void UBuffComponent::SpeedTimerFinished()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;
	
	Character->GetCharacterMovement()->MaxWalkSpeed = InitialWalkSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

	// 计时器结束后 server 端调用 多播 RPC 在所有client 上恢复初始速度
	MulticastSpeed(InitialWalkSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastSpeed_Implementation(float WalkSpeed, float CrouchSpeed)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}
