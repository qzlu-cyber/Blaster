// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Blaster/Character/BlasterCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

#define TRACE_LENGTH 80000.f

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

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		
		// 如果击中了角色
		if (FireHit.bBlockingHit)
		{
			ABlasterCharacter* Character = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (Character && FireHit.GetActor() != GetOwner()) // 不对自己造成伤害
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

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere; // 球心
	const FVector RandomVectorInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius); // 随机散射方向
	const FVector TraceEnd = SphereCenter + RandomVectorInSphere; // TraceEnd 位置
	const FVector ToTraceEnd = TraceEnd - TraceStart;

	// DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	// DrawDebugSphere(GetWorld(), TraceEnd, 4.f, 12, FColor::Orange, true);
	// DrawDebugLine(
	// 	GetWorld(),
	// 	TraceStart,
	// 	FVector(TraceStart + ToTraceEnd * TRACE_LENGTH / ToTraceEnd.Size()),
	// 	FColor::Cyan,
	// 	true);

	return FVector(TraceStart + ToTraceEnd * TRACE_LENGTH / ToTraceEnd.Size());
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;
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
