// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadePickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

void AGrenadePickup::OnSphereOverlap(UPrimitiveComponent* OverlappedSphere, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedSphere, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UCombatComponent* CombatComponent = BlasterCharacter->GetCombatComponent();
		if (CombatComponent)
		{
			if (CombatComponent->GetGrenades() < CombatComponent->GetMaxGrenades())
			{
				CombatComponent->SetGrenades(FMath::Clamp(CombatComponent->GetGrenades() + GrenadeAmount, 0, CombatComponent->GetMaxGrenades()));
				CombatComponent->UpdateGrenades();
				
				Destroy();
			}
		}
	}
}
