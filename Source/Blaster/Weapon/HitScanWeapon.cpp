// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController//BlasterPlayerController.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

#define TRACE_LENGTH 80000.f

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	// 获取武器的 MuzzleFlash
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		// 获取 MuzzleFlash 的位置
		FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		FVector Start = MuzzleFlashTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		
		// 如果击中了角色
		if (FireHit.bBlockingHit)
		{
			ABlasterCharacter* Character = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (Character && FireHit.GetActor() != GetOwner()) // 不对自己造成伤害
			{
				// 服务器端无延迟，直接造成伤害；不使用服务器端回滚，由服务器造成伤害
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						Character,
						Damage,
						GetOwner()->GetInstigatorController(),
						this,
						UDamageType::StaticClass()
					);
				}
				if (!HasAuthority() && bUseServerSideRewind)
				{
					BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
					if (BlasterOwnerCharacter) BlasterOwnerPlayerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
					if (BlasterOwnerPlayerController && BlasterOwnerCharacter->GetLagCompensationComponent() && BlasterOwnerCharacter->IsLocallyControlled())
					{
						BlasterOwnerCharacter->GetLagCompensationComponent()->ServerScoreRequest(
							Character,
							this,
							Start,
							HitTarget,
							BlasterOwnerPlayerController->GetServerTime() - BlasterOwnerPlayerController->GetSingleTripTime() // 此时服务器时间 - RPC 单程时间 = 发射时服务器的时间
						);
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
						FireHit.ImpactPoint
					);
				}
			}
		}

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				MuzzleFlashTransform
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit) BeamEnd = OutHit.ImpactPoint;

		// 无论是否击中都播放 BeamEffect
		if (BeamEffect)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamEffect,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);

			if (Beam) Beam->SetVectorParameter(FName(TEXT("Target")), BeamEnd);
		}

		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}
