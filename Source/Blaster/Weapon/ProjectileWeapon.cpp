// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Projectile.h"

#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	/// 生成子弹
	// 1. 获取子弹的生成位置
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// 2. 生成子弹
		if (ProjectileClass)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				// 设置子弹的生成方向
				FVector ToTarget = HitTarget - MuzzleFlashTransform.GetLocation();
				// 设置子弹的生成参数
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.Owner = GetOwner(); // 将子弹的拥有者设置为武器的拥有者
				SpawnParameters.Instigator = InstigatorPawn; // 将子弹的发起者设置为武器的拥有者
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					MuzzleFlashTransform.GetLocation(),
					ToTarget.Rotation(),
					SpawnParameters
				);
			}
		}
	}
}
