// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/BlasterTypes/TurnInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Blaster/BlasterTypes/CombatState.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void CalculateAOPitch();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 重写 GetLifetimeReplicatedProps() 函数，设置需要同步的属性
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;
	
	void UpdateHealthHUD();
	void UpdateShieldHUD();
	void UpdateAmmoHUD();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//
	// 角色的移动和旋转
	//
	void MoveForward(const struct FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);

	//  装备 / 丢弃武器
	void EquipWeapon(const FInputActionValue& Value);
	void DropWeapon(const FInputActionValue& Value);

	// 下蹲 / 跳跃
	void Crouching(const FInputActionValue& Value);
	virtual void Jump() override;

	// 瞄准
	void Aiming(const FInputActionValue& Value);

	// 开火
	void Fire(const FInputActionValue& Value);

	// 投掷手榴弹
	void ThrowGrenade(const FInputActionValue& Value);

	// 换弹
	void Reload(const FInputActionValue& Value);

	// 配置 AimOffset
	void AimOffset(float DeltaTime);

	// 处理原地转向的逻辑
	void TurningInPlace(float DeltaTime);
	// SimulateProxy 动画会有卡顿，单独处理其转向逻辑
	void SimulateProxyTurningInPlace();

	// 播放受到攻击时的动画
	void PlayHitReactMontage();
	
public:	
	void SetOverlappingWeapon(class AWeapon* Weapon);

	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	FORCEINLINE bool IsElimmed() const { return bIsElimmed; }

	FORCEINLINE float GetAOYaw() const { return AOYaw; }
	FORCEINLINE float GetAOPitch() const { return AOPitch; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ETurnInPlace GetTurnInPlace() const { return TurnInPlace; }
	FORCEINLINE bool GetRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE class UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE class UBuffComponent* GetBuffComponent() const { return Buff; }
	FORCEINLINE class UStaticMeshComponent* GetGrenadeMeshComponent() const { return GrenadeMeshComponent; }

	void PlayFireWeaponMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();

	FVector GetHitTarget() const;

	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/// 处理角色死亡时的逻辑
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	// server 处理角色死亡时需要单独在 erver 端执行的逻辑，如角色 respawn
	void Elim();

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }

	ECombatState GetCombatState() const;

	void SetDisableGameplay(bool Value) { bDisableGameplay = Value; }
	bool GetDisableGameplay() const { return bDisableGameplay; }

	// 当装备的武器是狙击枪时，在瞄准时显示狙击镜
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowWidget);

private:
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	// 当 RootComponent's position and velocity 发生变化被 replicate 时，会调用该函数
	virtual void OnRep_ReplicatedMovement() override;

	// 当 Shield 或 Health 发生变化被 replicate 时，会调用该函数
	UFUNCTION()
	void OnRep_Shield(float LastShield);
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/// Remote Procedure Calls: 在一台机器上调用某些操作，但在另一台机器上执行
	/// 已经通过 Overlapping 实现了将变量从 server 端 replicate 到 client 端，即从 server 端向 client 端同步数据
	/// 当 client 端需要通知 server 端执行某些操作时（例如拾取物品等），可以使用 RPC
	/// 当 RPC 是从 client 调用并在 server 上执行，client 必须拥有调用 RPC 的 Actor
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon();
	UFUNCTION(Server, Reliable)
	void ServerSwapWeapons();
	UFUNCTION(Server, Reliable)
	void SeverDropWeapon(AWeapon* WeaponToDrop);

	void HideCameraIfCharacterClose();

	/// 处理受到伤害时的逻辑
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
	
	// 角色死亡计时器结束回调函数
	void ElimTimerFinished();

	// Dissolve Effect
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolveTimeline();

	// Pull for any relevant classes and initialize our HUD
	void PollInit();

	void RotateInPlace(float DeltaTime);

	// 生成默认武器
	void SpawnDefaultWeapon();

public:
	/// Hit boxes used for server-side rewind
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Head;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Pelvis;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine_02;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine_03;
	UPROPERTY(EditAnywhere)
	UBoxComponent* UpperArm_L;
	UPROPERTY(EditAnywhere)
	UBoxComponent* UpperArm_R;
	UPROPERTY(EditAnywhere)
	UBoxComponent* LowerArm_L;
	UPROPERTY(EditAnywhere)
	UBoxComponent* LowerArm_R;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Hand_L;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Hand_R;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Thigh_L;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Thigh_R;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Calf_L;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Calf_R;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Foot_L;
	UPROPERTY(EditAnywhere)
	UBoxComponent* Foot_R;
	
private:
	/// PlayerController
	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	/// Grenade
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* GrenadeMeshComponent;
	
	/// Components
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category=Camera)
	UCameraComponent* FollowCamera;
	UPROPERTY(EditAnywhere, Category="HUD", BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UWidgetComponent* OverheadWidget;
	// Actor Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UCombatComponent* Combat;
	UPROPERTY(VisibleAnywhere)
	UBuffComponent* Buff;
	// Dissolve
	UPROPERTY(VisibleAnywhere, Category="Elim")
	UTimelineComponent* DissolveTimeline;

	/// Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	class UInputMappingContext* InputMapping;
	// Move
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	class UInputAction* MoveForwardAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* MoveRightAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* TurnAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* LookUpAction;
	// Equip / Drop Weapon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* PickUpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* DropAction;
	// Crouch
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* CrouchAction;
	// Aiming
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* AimAction;
	// Fire
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* FireAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* ReloadAction;
	// Throw
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* ThrowGrenadeAction;

	// Montages
	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* FireWeaponMontage; // 角色开火时动画
	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ReloadMontage; // 角色换弹时动画
	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* HitReactMontage; // 角色受到攻击时动画
	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ElimMontage; // 角色死亡时动画
	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ThrowGrenadeMontage; // 角色投掷手榴弹动画
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	float MoveSpeed = 600.f;

	/// Default Weapon
	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<AWeapon> DefaultWeaponClass;
	
	/// Weapon Overlapping
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	// Is Aiming
	bool bIsAiming = false;

	// Aim Offset
	float AOYaw; // 静止时的 BaseAimRotation 和 静止后当前的 BaseAimRotation 的 Delta Yaw
	float AOPitch; // 
	FRotator StartAimRotation;

	// Turn In Place
	ETurnInPlace TurnInPlace;
	float InterpAOYaw;
	bool bRotateRootBone; // SimulateProxy 不使用蓝图中的 RotateRootBone 播放动画
	FRotator ProxyRotationLastFrame; // SimulateProxy 上一帧的旋转
	FRotator ProxyRotation; // SimulateProxy 当前的旋转
	float ProxyYaw; // SimulateProxy 的 Yaw
	float TurnThreshold = 10.f; // 角色转身的阈值
	float TimeSinceLastMovementReplication; // 跟踪上一次移动同步的时间，达到一定阈值手动同步移动

	// Hide Camera when Character is close
	UPROPERTY(EditAnywhere, Category="Camera")
	float HideCameraDistance = 200.f;

	/// Player Stats
	UPROPERTY(EditAnywhere, Category="Player Stats")
	float MaxShield = 100.f; // 最大护盾值
	UPROPERTY(ReplicatedUsing=OnRep_Shield, EditAnywhere, Category="Player Stats")
	float Shield = 100.f; // 当前护盾值
	UPROPERTY(EditAnywhere, Category="Player Stats")
	float MaxHealth = 100.f; // 最大血量
	UPROPERTY(ReplicatedUsing=OnRep_Health, EditAnywhere, Category="Player Stats")
	float Health = 100.f; // 当前血量
	bool bIsElimmed = false; // 是否已经死亡
	FTimerHandle ElimTimerHandle; // 死亡后的计时器，用于 respawn
	UPROPERTY(EditDefaultsOnly, Category="Player Stats")
	float ElimDelay = 5.f; // 死亡后的计时器时间
	// Dissolve Effect
	FOnTimelineFloat DissolveTrack;
	UPROPERTY(EditAnywhere)
	class UCurveFloat* DissolveCurve;
	UPROPERTY(VisibleAnywhere, Category="Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; // 在运行中动态变化的材质实例
	UPROPERTY(EditAnywhere, Category="Elim")
	UMaterialInstance* DissolveMaterialInstance; // 在蓝图中配置的材质实例，用于初始化 DynamicDissolveMaterialInstance

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	bool bDisableGameplay = false; // 角色死亡或游戏结束时禁用某些功能
};
