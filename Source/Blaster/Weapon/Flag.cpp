// Fill out your copyright notice in the Description page of Project Settings.


#include "Flag.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	PrimaryActorTick.bCanEverTick = false;

	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(FlagMesh);
	GetWeaponMesh()->SetupAttachment(FlagMesh);
	GetWeaponCollision()->SetupAttachment(FlagMesh);
	GetPickupWidget()->SetupAttachment(FlagMesh);
}

void AFlag::OnEquipped()
{
	ShowWeaponPickupWidget(false);  // 拾取后隐藏拾取提示
	// 装备后禁用武器的碰撞，避免碰撞到角色后在 server 端生成重叠事件再次显示拾取提示
	GetWeaponCollision()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFlag::OnDropped()
{
	if (HasAuthority()) GetWeaponCollision()->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 开启碰撞
	FlagMesh->SetSimulatePhysics(true);
	FlagMesh->SetEnableGravity(true);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Block); // 设置碰撞响应
	FlagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 设置忽略 Pawn 的碰撞
	FlagMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 设置忽略 Camera 的碰撞
}

