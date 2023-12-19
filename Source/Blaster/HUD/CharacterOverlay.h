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
	UProgressBar* ShieldBar; // 护盾条
	UPROPERTY(meta=(BindWidget))
	UTextBlock* ShieldText; // 护盾值
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
	UPROPERTY(meta=(BindWidget))
	UTextBlock* SecondaryWeaponAmmoAmount; // 副武器每个弹夹弹药数
	UPROPERTY(meta=(BindWidget))
	UTextBlock* SecondarySlash;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* SecondaryCarriedAmmoAmount; // 副武器携带弹药数
	UPROPERTY(meta=(BindWidget))
	UImage* SecondaryWeaponImage; // 副武器图片
	UPROPERTY(meta=(BindWidget))
	UImage* GrenadeImage; // 手榴弹图片
	UPROPERTY(meta=(BindWidget))
	UTextBlock* GrenadeAmount; // 手榴弹数量
	UPROPERTY(meta=(BindWidget))
	UTextBlock* MatchCountdownText; // 倒计时

	UPROPERTY(meta=(BindWidget))
	UImage* HighPingImage; // 高延迟图片
	UPROPERTY(meta=(BindWidgetAnim), Transient)
	class UWidgetAnimation* HighPingAnimation; // 高延迟动画

	UPROPERTY(meta=(BindWidget))
	UTextBlock* BlueTeamText; // 蓝队文本
	UPROPERTY(meta=(BindWidget))
	UTextBlock* BlueTeamScore; // 蓝队分数
	UPROPERTY(meta=(BindWidget))
	UTextBlock* RedTeamText; // 红队文本
	UPROPERTY(meta=(BindWidget))
	UTextBlock* RedTeamScore; // 红队分数
};
