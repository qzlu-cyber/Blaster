// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/CombatComponent.h"


void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedSphere, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedSphere, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UCombatComponent* CombatComponent = BlasterCharacter->GetCombatComponent();
		if (CombatComponent)
		{
			CombatComponent->PickupAmmo(WeaponType, AmmoAmount);
			Destroy(); // 拾取后销毁
		}
	}
}
