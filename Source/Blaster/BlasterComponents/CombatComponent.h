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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 角色拾取武器
	void EquipWeapon(class AWeapon* WeaponToEquip);
	// 角色卸下武器
	void DropWeapon();

private:
	class ABlasterCharacter* Character; // 将 CombatComponent 挂载到的角色
	AWeapon* EquippedWeapon; // 当前装备的武器

	friend class ABlasterCharacter;
};
