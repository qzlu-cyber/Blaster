// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

#define TRACE_LENGTH 80000.f

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector()); // 播放开枪动画、生成弹壳、更新 HUD
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	
	// 获取武器的 MuzzleFlash
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取 MuzzleFlash 的位置
		FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = MuzzleFlashTransform.GetLocation();

		TMap<ABlasterCharacter*, int32> HitCharactersMap; // 保存击中的角色和对应的击中次数
		TMap<ABlasterCharacter*, int32> HeadShotCharactersMap; // 保存击中头部的角色和对应的击中次数
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			if (FireHit.bBlockingHit)
			{
				ABlasterCharacter* Character = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (Character && FireHit.GetActor() != GetOwner())
				{
					bool bIsHeadShot = FireHit.BoneName.ToString() == FName("head");
					if (bIsHeadShot)
					{
						if (HeadShotCharactersMap.Contains(Character)) HeadShotCharactersMap[Character]++;
						else HeadShotCharactersMap.Emplace(Character, 1);
					}
					else
					{
						if (HitCharactersMap.Contains(Character)) HitCharactersMap[Character]++;
						else HitCharactersMap.Emplace(Character, 1);
					}
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

		TArray<ABlasterCharacter*> HitCharacters;
		TMap<ABlasterCharacter*, float> HitCharactersDamageMap; // 保存击中的角色和对应的伤害
		for (auto Pair : HitCharactersMap)
		{
			if (Pair.Key)
			{
				HitCharacters.Emplace(Pair.Key);

				HitCharactersDamageMap.Emplace(Pair.Key, Damage * Pair.Value);
			}
		}
		for (auto Pair : HeadShotCharactersMap)
		{
			if (Pair.Key)
			{
				HitCharacters.Emplace(Pair.Key);

				if (HitCharactersDamageMap.Contains(Pair.Key)) HitCharactersDamageMap[Pair.Key] += HeadShotDamage * Pair.Value; // 如果已经击中该角色身体，则累加头部伤害
				else HitCharactersDamageMap.Emplace(Pair.Key, HeadShotDamage * Pair.Value);
			}
		}
		for (auto Pair : HitCharactersDamageMap) // 对每个被击中的角色施加伤害
		{
			if (Pair.Key)
			{
				// 服务器端无延迟，直接造成伤害；不使用服务器端回滚，由服务器造成伤害
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						Pair.Key,
						Pair.Value,
						GetOwner()->GetInstigatorController(),
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}
		
		if (!HasAuthority() && bUseServerSideRewind)
		{
			BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
			if (BlasterOwnerCharacter)
			{
				BlasterOwnerPlayerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->GetController());
				if (BlasterOwnerPlayerController && BlasterOwnerCharacter->GetLagCompensationComponent() && BlasterOwnerCharacter->IsLocallyControlled())
				{
					BlasterOwnerCharacter->GetLagCompensationComponent()->ShotgunServerScoreRequest(
						HitCharacters,
						Start,
						HitTargets,
						BlasterOwnerPlayerController->GetServerTime() - BlasterOwnerPlayerController->GetSingleTripTime()
					);
				}
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;
	// 获取 MuzzleFlash 的位置
	FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	FVector TraceStart = MuzzleFlashTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere; // 球心
	
	// 随机生成 NumberOfPellets 个散射方向
	for (uint32 i = 0; i < NumberOfPellets; ++i)
	{
		const FVector RandomVectorInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius); // 随机散射方向
		const FVector TraceEnd = SphereCenter + RandomVectorInSphere; // TraceEnd 位置
		const FVector ToTraceLoc = TraceEnd - TraceStart;
		const FVector ToTraceEnd = TraceStart + ToTraceLoc * TRACE_LENGTH / ToTraceLoc.Size();

		HitTargets.Add(ToTraceEnd);
	}
}
