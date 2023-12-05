// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"

#include "Blaster/Character/BlasterCharacter.h"


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

