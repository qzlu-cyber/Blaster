// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"

#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& HitResult)
{
	APawn* FiringPawn = GetInstigator(); // 获取发射者
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			TArray<AActor*> IgnoreActors;
			IgnoreActors.Emplace(GetOwner()); // 不对发射者造成伤害
			// 造成伤害
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // WorldContextObject
				Damage, // BaseDamage
				5.f, // MinimumDamageRadius
				GetActorLocation(), // Origin
				200.f, // DamageInnerRadius
				500.f, // DamageOuterRadius
				1.f, // DamageFalloff, 线性
				UDamageType::StaticClass(), // DamageTypeClass
				IgnoreActors, // IgnoreActors
				this, // DamageCauser
				FiringController // InstigatedByController
			);
		}
	}
	
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, HitResult);
}
