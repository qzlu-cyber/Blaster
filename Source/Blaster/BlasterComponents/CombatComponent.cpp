// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 300.f;
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

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 设置角色不跟随移动方向旋转
		Character->bUseControllerRotationYaw = true; // 设置角色跟随控制器的旋转
	}
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
	/// EquipWeapon 只会在 server 端执行，以下代码只会在 server 端执行
	/// 为了使 client 和 server 同步，client 端的设置交由 OnRep_EquippedWeapon() 函数处理
	Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 设置角色不跟随移动方向旋转
	Character->bUseControllerRotationYaw = true; // 设置角色跟随控制器的旋转
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

void UCombatComponent::OnRep_IsAiming(bool bLastIsAiming)
{
	if (Character) Character->GetCharacterMovement()->MaxWalkSpeed = bLastIsAiming ? BaseWalkSpeed : AimWalkSpeed;
}

void UCombatComponent::Aiming(bool bAiming)
{
	bIsAiming = bAiming;

	if (Character) Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}