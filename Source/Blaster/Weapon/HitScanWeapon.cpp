// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Blaster/Character/BlasterCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"


void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 获取武器的 MuzzleFlash
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取 MuzzleFlash 的位置
		FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		FVector Start = MuzzleFlashTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;

		UWorld* World = GetWorld();
		if (World)
		{
			FHitResult HitResult;
			
			World->LineTraceSingleByChannel(
				HitResult,
				Start,
				End,
				ECC_Visibility
			);

			FVector BeamEnd = End;
			// 如果击中了角色
			if (HitResult.bBlockingHit)
			{
				BeamEnd = HitResult.ImpactPoint;
				
				ABlasterCharacter* Character = Cast<ABlasterCharacter>(HitResult.GetActor());
				if (Character)
				{
					// 只有服务器端才能造成伤害
					if (HasAuthority())
					{
						UGameplayStatics::ApplyDamage(
							Character,
							Damage,
							GetOwner()->GetInstigatorController(),
							this,
							UDamageType::StaticClass()
						);
					}

					// 生成击中特效
					if (ImpactEffect)
					{
						UGameplayStatics::SpawnEmitterAtLocation(
							World,
							ImpactEffect,
							HitResult.ImpactPoint,
							HitResult.ImpactNormal.Rotation()
						);
					}
				}
			}

			// 无论是否击中都播放 BeafEffect
			if (BeamEffect)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					BeamEffect,
					MuzzleFlashTransform
				);

				if (Beam) Beam->SetVectorParameter(FName(TEXT("Target")), BeamEnd);
			}
		}
	}
}
