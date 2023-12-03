// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Blaster/Character/BlasterCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget); // 播放开枪动画、生成弹壳、更新 HUD
	
	// 获取武器的 MuzzleFlash
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取 MuzzleFlash 的位置
		FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = MuzzleFlashTransform.GetLocation();

		TMap<ABlasterCharacter*, int32> HitCharactersMap; // 保存击中的角色和对应的击中次数
		for (uint32 i = 0; i < NumberOfPellets; ++i)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			if (FireHit.bBlockingHit)
			{
				ABlasterCharacter* Character = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (Character && HasAuthority() && FireHit.GetActor() != GetOwner())
				{
					if (HitCharactersMap.Contains(Character)) HitCharactersMap[Character]++;
					else HitCharactersMap.Emplace(Character, 1);
				}

				// 生成击中特效
				if (ImpactEffect)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactEffect,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}

				// 播放击中音效
				if (ImpactSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						GetWorld(),
						ImpactSound,
						FireHit.ImpactPoint,
						.3f
					);
				}
			}
		}

		for (const auto &Pair : HitCharactersMap)
		{
			UGameplayStatics::ApplyDamage(
				Pair.Key,
				Damage * Pair.Value,
				GetOwner()->GetInstigatorController(),
				this,
				UDamageType::StaticClass()
			);
		}
	}
}
