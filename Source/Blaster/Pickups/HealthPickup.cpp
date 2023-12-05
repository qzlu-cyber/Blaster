// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/BuffComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AHealthPickup::AHealthPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedSphere, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedSphere, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UBuffComponent* BuffComponent = BlasterCharacter->GetBuffComponent();
		if (BuffComponent)
		{
			if (BlasterCharacter->GetHealth() < BlasterCharacter->GetMaxHealth()) // 血量不健康时才恢复血量
			{
				BuffComponent->Heal(HealAmount, HealTime);
				
				Destroy();
			}
		}
	}
}

void AHealthPickup::Destroyed()
{
	if (PickEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
	
	Super::Destroyed();
}
