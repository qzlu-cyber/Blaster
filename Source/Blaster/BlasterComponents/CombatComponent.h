// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

public:	
	// 角色拾取武器
	void EquipWeapon(class AWeapon* WeaponToEquip);
	// 角色卸下武器
	void DropWeapon();
	// 角色瞄准
	void Aiming(bool bAiming);

private:
	class ABlasterCharacter* Character; // 将 CombatComponent 挂载到的角色

	UPROPERTY(Replicated)
	AWeapon* EquippedWeapon; // 当前装备的武器

	UPROPERTY(Replicated)
	bool bIsAiming; // 是否在瞄准

	friend class ABlasterCharacter;
};
