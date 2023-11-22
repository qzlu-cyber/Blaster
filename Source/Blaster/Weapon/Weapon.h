// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ShowWeaponPickupWidget(bool bShowWidget);

	void SetWeaponState(EWeaponState NewState);
	void SetAmmoHUD();
	FORCEINLINE class USphereComponent* GetWeaponCollision() const { return WeaponCollision; }

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	virtual void Fire(const FVector& HitTarget);

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool GetAutomaticFire() const { return bAutomaticFire; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsEmptyAmmo() const { return Ammo <= 0; }

	virtual void OnRep_Owner() override;

	class ABlasterPlayerController* GetBlasterOwnerPlayerController();

private:
	UFUNCTION()
	void OnRep_WeaponState();

	UFUNCTION()
	void OnRep_Ammo();
	// server 端执行减少子弹及更新 AmmoHUD
	void SpendRound();

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

private:
	UPROPERTY(VisibleAnywhere, Category="Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;
	UPROPERTY(VisibleAnywhere, Category="Weapon Properties")
	USphereComponent* WeaponCollision;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState, VisibleAnywhere, Category="Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HUD", meta=(AllowPrivateAccess="true"))
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	class UAnimationAsset* FireAnimation; // 武器开火时动画

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass; // 弹壳类

	// 用于更新 AmmoHUD
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerPlayerController;

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
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Ammo)
	int32 Ammo; // 每梭子子弹数
	UPROPERTY(EditAnywhere)
	int32 MaxAmmoCapacity; // 所有子弹数
};
