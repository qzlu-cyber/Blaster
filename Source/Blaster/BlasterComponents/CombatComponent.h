// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/HUD/BlasterHUD.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 从准星发射跟踪射线
	void TraceUnderCrosshair(FHitResult& HitResult);

	// 设置绘制准星的 Texture，每帧都要更新
	void SetHUDCrosshairs(float DeltaTime);

public:	
	// 角色拾取武器
	void EquipWeapon(class AWeapon* WeaponToEquip);
	// 角色卸下武器
	void DropWeapon();
	// 角色瞄准
	void Aiming(bool bAiming);
	void Shoot();
	// 角色开火
	void Fire(bool bFire);

private:
	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* LastEquippedWeapon);
	UFUNCTION()
	void OnRep_IsAiming(bool bLastIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void InterpFOV(float DeltaTime);

	/// Automatic fire
	void StartFireTimer();
	// 用于自动开火的计时器回调函数
	void FireTimerFinished();

	// 是否可以开火
	bool CanFire() const;

private:
	UPROPERTY()
	class ABlasterCharacter* Character; // 将 CombatComponent 挂载到的角色
	UPROPERTY()
	class ABlasterPlayerController* PlayerController; // 角色的 PlayerController，用于获取 HUD
	UPROPERTY()
	class ABlasterHUD* HUD; // 角色的 HUD，用于设置准星的 Texture

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon; // 当前装备的武器

	UPROPERTY(ReplicatedUsing=OnRep_IsAiming)
	bool bIsAiming; // 是否在瞄准
	bool bIsFire; // 是否在开火

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	FVector HitTarget; // 击中目标的位置

	/// HUD and Crosshairs
	FHUDPackage HUDPackage;
	float CrosshairVelocityFactor; // 准星扩散速度因子
	float CrosshairInAirFactor; // 准星扩散空中因子
	float CrosshairAimFactor; // 准星扩散瞄准因子
	float CrosshairShootingFactor; // 准星扩散开火因子

	/// Aiming FOV
	float DefaultFOV; // 没有瞄准时的 FOV
	float CurrentFOV; // 当前的 FOV
	float ZoomInterpSpeed = 20.f; // 瞄准时的插值速度

	/// Automatic fire
	FTimerHandle FireTimerHandle;
	bool bCanFire = true; // 连发武器必须等待计时器结束才能再次开火

	friend class ABlasterCharacter;
};
