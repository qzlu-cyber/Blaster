// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "WeaponTypes.h"
#include "Blaster/BlasterTypes/Team.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	virtual void OnEquipped();
	virtual void OnDropped();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ShowWeaponPickupWidget(bool bShowWidget);

	void SetWeaponState(EWeaponState NewState);
	void SetAmmoHUD();
	
	FORCEINLINE class USphereComponent* GetWeaponCollision() const { return WeaponCollision; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE class UWidgetComponent* GetPickupWidget() const { return PickupWidget; }

	virtual void Fire(const FVector& HitTarget);

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool GetAutomaticFire() const { return bAutomaticFire; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsEmptyAmmo() const { return Ammo <= 0; }
	FORCEINLINE bool IsFullAmmo() const { return Ammo == MaxAmmoCapacity; }
	FORCEINLINE bool IsUseScatter() const { return bUseScatter; }
	FORCEINLINE EWeaponTypes GetWeaponType() const { return WeaponType; }
	FORCEINLINE EFireType GetWeaponFireType() const { return FireType; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMaxAmmoCapacity() const { return MaxAmmoCapacity; }
	FORCEINLINE float GetWeaponDamage() const { return Damage; }
	FORCEINLINE float GetWeaponHeadShotDamage() const { return HeadShotDamage; }

	virtual void OnRep_Owner() override;

	class ABlasterPlayerController* GetBlasterOwnerPlayerController();

	// 装弹
	void AddAmmo(int32 AmmoToAdd);

	FVector TraceEndWithScatter(const FVector& HitTarget);

private:
	UFUNCTION()
	void OnRep_WeaponState();
	
	void SpendRound();

	void OnWeaponStateSet();
	
	void OnEquippedSecondary();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

public:
	/// 绘制准星的材质
	UPROPERTY(EditAnywhere, Category="Crosshire")
	class UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category="Crosshire")
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category="Crosshire")
	UTexture2D* CrosshairsBottom;
	UPROPERTY(EditAnywhere, Category="Crosshire")
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category="Crosshire")
	UTexture2D* CrosshairsRight;

	// 装备武器音效
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	bool bDestroyWeapon = false; // 设置角色默认武器在死亡后自动销毁

protected:
	// 用于更新 AmmoHUD
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerPlayerController;
	
	/// Trace end with scatter 
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f; // 武器伤害
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f; // 武器爆头伤害

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false; // 是否使用服务器端回滚。当 ping 太高时自动禁用，需要在 server 和 client 同步

	UPROPERTY(EditAnywhere)
	ETeam Team;

private:
	UPROPERTY(VisibleAnywhere, Category="Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;
	UPROPERTY(VisibleAnywhere, Category="Weapon Properties")
	USphereComponent* WeaponCollision;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState, VisibleAnywhere, Category="Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HUD", meta=(AllowPrivateAccess="true"))
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	UAnimationAsset* FireAnimation; // 武器开火时动画

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass; // 弹壳类

	// 武器类型
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	EWeaponTypes WeaponType;
	// 武器开火类型
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	EFireType FireType;

	/// Aiming FOV
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	float ZoomedFOV = 45.f;
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	float ZoomInterpSpeed = 20.f;

	/// Automatic fire
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	bool bAutomaticFire = true; // 是否是连发武器
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	float FireDelay = 0.15f; // 连发武器的开火间隔

	/// 弹药
	UPROPERTY(EditAnywhere)
	int32 Ammo; // 每梭子当前剩余子弹数
	UPROPERTY(EditAnywhere)
	int32 MaxAmmoCapacity; // 每梭子子弹数
	// server 端还没处理的 Ammo 更新请求的序列
	// 在 SpendRound 中增加，在 ClientUpdateAmmo 中减少
	int32 Sequence = 0;
};
