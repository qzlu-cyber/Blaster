// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	class UProgressBar* HealthBar; // 血条
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* HealthText; // 血量
	UPROPERTY(meta=(BindWidget))
	UTextBlock* ScoreAmount; // 击杀数
	UPROPERTY(meta=(BindWidget))
	UTextBlock* DeathAmount; // 死亡数
	UPROPERTY(meta=(BindWidget))
	UTextBlock* WeaponAmmoAmount; // 每个弹夹弹药数
	UPROPERTY(meta=(BindWidget))
	UTextBlock* Slash;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* CarriedAmmoAmount; // 携带弹药数
	UPROPERTY(meta=(BindWidget))
	class UImage* MainWeaponImage; // 主武器图片
};
