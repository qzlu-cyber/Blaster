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
	FORCEINLINE class USphereComponent* GetWeaponCollision() const { return WeaponCollision; }

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	virtual void Fire(const FVector& HitTarget);

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

private:
	UFUNCTION()
	void OnRep_WeaponState();

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

	/// Aiming FOV
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	float ZoomedFOV = 45.f;
	UPROPERTY(EditAnywhere, Category="Weapon Properties")
	float ZoomInterpSpeed = 20.f;
};
