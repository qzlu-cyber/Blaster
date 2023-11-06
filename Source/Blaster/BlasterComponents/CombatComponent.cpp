// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	// 装配武器
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

	EquippedWeapon->SetOwner(Character); // 设置武器的 owner
}

void UCombatComponent::DropWeapon()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	// 从右手插槽分离武器
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		// 设置武器的位置和旋转
		FVector WeaponLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f; // 适当的位置
		FRotator WeaponRotation = Character->GetActorRotation(); // 适当的旋转

		WeaponLocation.Z = 2.f;
		WeaponRotation.Pitch = -90.f;

		EquippedWeapon->SetActorLocation(WeaponLocation);
		EquippedWeapon->SetActorRotation(WeaponRotation);

		EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	// 卸下武器
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Dropped);

	EquippedWeapon->SetOwner(nullptr);
	EquippedWeapon = nullptr;
}

void UCombatComponent::Aiming(bool bAiming)
{
	bIsAiming = bAiming;
}

