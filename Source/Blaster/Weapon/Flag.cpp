// Fill out your copyright notice in the Description page of Project Settings.


#include "Flag.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	PrimaryActorTick.bCanEverTick = false;

	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	SetRootComponent(FlagMesh);
	GetWeaponMesh()->SetupAttachment(FlagMesh);
	GetWeaponCollision()->SetupAttachment(FlagMesh);
	GetPickupWidget()->SetupAttachment(FlagMesh);
}

