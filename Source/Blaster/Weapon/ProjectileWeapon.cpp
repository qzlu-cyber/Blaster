// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Projectile.h"

#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/ProjectileMovementComponent.h"

/// 是否生成 replicated projectile 取决于几件事
/// 武器本身有一个 bUsedServerSideRewind，但 projectile 也有
/// 1. 在 server 端，且武器的 bUsedServerSideRewind 为 true，且为本地控制的 pawn，生成 replicated 且 bUsedServerSideRewind 为 false 的 projectile
/// 这是因为 LocallyControlled Character on the server 无需使用 rewind，它根据 projectile 的 hit event 直接在 server 端造成伤害并复制给 client 端
/// 2. 在 server 端，且武器的 bUsedServerSideRewind 为 true，但不是本地控制的那么开枪的就是其他 client，生成 non replicated 且 bUsedServerSideRewind 为 false 的 projectile
/// 这是因为此 client 将生成自己的本地 non replicated 但 bUsedServerSideRewind 为 true 的 projectile，之后再向 server 端发送 ServerScoreRequest
/// 3. 在 client 端，且武器的 bUsedServerSideRewind 为 true，且为本地控制的 pawn，生成 non replicated 且 bUsedServerSideRewind 为 true 的 projectile，当该 projectile 在此 client 上命中时，将向 server 端发送 ServerScoreRequest
/// 4. 在 client 端，且武器的 bUsedServerSideRewind 为 true，但不是本地控制的 pawn，生成 non replicated 且 bUsedServerSideRewind 为 false 的 projectile
/// 5. 在 server 端，且武器的 bUsedServerSideRewind 为 false，无论是否是本地控制的 pawn，生成 replicated 且 bUsedServerSideRewind 为 false 的 projectile
/// 6. 在 client 端，且武器的 bUsedServerSideRewind 为 false，无论是否是本地控制的 pawn，client 端不生成 projectile，交由 server 端处理，server 端会生成 replicated 的 projectile 再复制给 client 端

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 只允许服务器端生成子弹，再通过服务器将子弹同步到客户端
	if (!HasAuthority()) return;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	UWorld* World = GetWorld();

	/// 生成子弹
	// 1. 获取子弹的生成位置
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket && World)
	{
		FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		// 设置子弹的生成方向
		FVector ToTarget = HitTarget - MuzzleFlashTransform.GetLocation();
		// 设置子弹的生成参数
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner(); // 将子弹的拥有者设置为武器的拥有者
		SpawnParameters.Instigator = InstigatorPawn; // 将子弹的发起者设置为武器的拥有者
		
		// 2. 生成子弹
		if (ProjectileClass && InstigatorPawn)
		{
			AProjectile* SpawnedProjectile = nullptr;
			if (bUseServerSideRewind) // 如果武器需要使用服务器端回滚
			{
				if (HasAuthority()) // server
				{
					if (InstigatorPawn->IsLocallyControlled()) // 本地控制的 Character
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(
							ProjectileClass,
							MuzzleFlashTransform.GetLocation(),
							ToTarget.Rotation(),
							SpawnParameters
						);
						SpawnedProjectile->bUseServerSideRewind = false;
						SpawnedProjectile->Damage = Damage;
					}
					else
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(
							ServerSideRewindProjectileClass,
							MuzzleFlashTransform.GetLocation(),
							ToTarget.Rotation(),
							SpawnParameters
						);
						SpawnedProjectile->bUseServerSideRewind = false;
					}
				}
				else
				{
					if (InstigatorPawn->IsLocallyControlled()) // 本地控制的 Character
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(
							ServerSideRewindProjectileClass,
							MuzzleFlashTransform.GetLocation(),
							ToTarget.Rotation(),
							SpawnParameters
						);
						SpawnedProjectile->bUseServerSideRewind = true;
						SpawnedProjectile->StartLocation = MuzzleFlashTransform.GetLocation();
						SpawnedProjectile->LaunchVelocity = GetActorForwardVector() * SpawnedProjectile->GetProjectileMovementComponent()->InitialSpeed;
						/// 之前直接利用 projectile 的 Damage 值，但是生成 non replicated 的 projectile 后，此 projectile(DamageCauser) 不会被复制到 server
						/// 因此，必须在确认命中时检查服务器上的武器伤害变量
						SpawnedProjectile->Damage = Damage;
					}
					else
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(
							ServerSideRewindProjectileClass,
							MuzzleFlashTransform.GetLocation(),
							ToTarget.Rotation(),
							SpawnParameters
						);
						SpawnedProjectile->bUseServerSideRewind = false;
					}
				}
			}
			else // 武器不使用服务器端回滚
			{
				if (InstigatorPawn->HasAuthority())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(
						ProjectileClass,
						MuzzleFlashTransform.GetLocation(),
						ToTarget.Rotation(),
						SpawnParameters
					);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
	}
}
