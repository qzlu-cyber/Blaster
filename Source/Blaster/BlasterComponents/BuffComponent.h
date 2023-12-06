// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);

	void SetInitialSpeeds(float WalkSpeed, float CrouchSpeed);
	void SpeedBuff(float WalkBuffSpeed, float CrouchBuffSpeed, float SpeedBuffTime);

private:
	void HealRampUp(float DeltaTime);

	void SpeedTimerFinished();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeed(float WalkSpeed, float CrouchSpeed);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	/// Health Buff
	bool bIsHealing = false; // 是否正在恢复血量
	float HealingRate = 0.f; // 每秒恢复的血量
	float AmountToHeal = 0.f; // 总共需要恢复的血量

	/// Speed Buff
	float InitialWalkSpeed; // 角色初始步行速度
	float InitialCrouchSpeed; // 角色初始下蹲行走速度
	FTimerHandle SpeedTimer;

	friend class ABlasterCharacter;
};
