// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/BlasterTypes/CombatState.h"

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
	// 角色投掷手榴弹
	void ThrowGrenade();
	// 角色换弹
	void Reload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void PlayEquipWeaponSound();
	void UpdateCarriedAmmo();
	void UpdateGrenades();

	void ShowAttachedGrenade(bool bShow);

	FORCEINLINE int32 GetGrenades() const { return Grenades; }

private:
	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* LastEquippedWeapon);
	UFUNCTION()
	void OnRep_IsAiming(bool bLastIsAiming);
	UFUNCTION()
	void OnRep_CarriedWeaponAmmo();
	UFUNCTION()
	void OnRep_CombatState();
	UFUNCTION()
	void OnRep_Grenades();
	
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(Server, Reliable)
	void ServerReload();
	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void InterpFOV(float DeltaTime);

	/// Automatic fire
	void StartFireTimer();
	// 用于自动开火的计时器回调函数
	void FireTimerFinished();

	// 是否可以开火
	bool CanFire() const;

	// server 端初始化武器携带的弹药数
	void InitializeCarriedAmmoMap();

	// 处理换弹逻辑
	void HandleReload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	// 计算换弹数量
	int32 AmountToReload();
	// 更新各种弹药数
	void UpdateAmmos();
	// 更新霰弹枪弹药数
	void UpdateShotgunAmmo();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

private:
	UPROPERTY()
	class ABlasterCharacter* Character; // 将 CombatComponent 挂载到的角色
	UPROPERTY()
	class ABlasterPlayerController* PlayerController; // 角色的 PlayerController，用于获取 HUD
	UPROPERTY()
	class ABlasterHUD* HUD; // 角色的 HUD，用于设置准星的 Texture

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied; // 角色的战斗状态

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

	/// Weapons Stats
	UPROPERTY(ReplicatedUsing=OnRep_CarriedWeaponAmmo)
	int32 CarriedWeaponAmmo; // 当前装备的武器携带的弹药数
	TMap<EWeaponTypes, int32> CarriedAmmoMap; // 每个角色每种武器携带的弹药数，只允许在 server 端设置
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 150; // Assault Rifle 起始弹药数
	UPROPERTY(EditAnywhere)
	int32 StartingRLAmmo = 5; // Rocket Launcher 起始弹药数
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 5; // 手枪起始弹药数
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 150; // SMG 起始弹药数
	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 50; // Shotgun 起始弹药数
	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 10; // Sniper Rifle 起始弹药数
	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeAmmo = 5; // Grenade Launcher 起始弹药数
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Grenades)
	int32 Grenades = 2; // 当前拥有的手雷数
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 2; // 最多可携带的手雷数
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	friend class ABlasterCharacter;
};
