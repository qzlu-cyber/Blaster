// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();

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

	// 拾取 / 丢弃武器
	void PickUp(const FInputActionValue& Value);
	void Drop(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 重写 GetLifetimeReplicatedProps() 函数，设置需要同步的属性
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

	void SetOverlappingWeapon(class AWeapon* Weapon);

private:
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	/// Remote Procedure Calls: 在一台机器上调用某些操作，但在另一台机器上执行
	/// 已经通过 Overlapping 实现了将变量从 server 端 replicate 到 client 端，即从 server 端向 client 端同步数据
	/// 当 client 端需要通知 server 端执行某些操作时（例如拾取物品等），可以使用 RPC
	UFUNCTION(Server, Reliable)
	void ServerPickUp();
	UFUNCTION(Server, Reliable)
	void SeverDrop();

private:
	/// Components
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class UCameraComponent* FollowCamera;
	UPROPERTY(EditAnywhere, Category="HUD", BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UWidgetComponent* OverheadWidget;
	// Actor Components
	class UCombatComponent* Combat;

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
	// Pick Up / Drop
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* PickUpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	UInputAction* DropAction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	float MoveSpeed = 600.f;

	/// Weapon Overlapping
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;
};
