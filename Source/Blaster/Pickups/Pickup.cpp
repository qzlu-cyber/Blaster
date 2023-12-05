// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "Blaster/Weapon/WeaponTypes.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(50.f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 只检测碰撞
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 只检测 Pawn

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	PickupMesh->SetupAttachment(OverlapSphere);
	PickupMesh->SetRenderCustomDepth(true);
	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	PickupMesh->MarkRenderStateDirty();
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereOverlap);
}

void APickup::OnSphereOverlap(UPrimitiveComponent* OverlappedSphere, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickup::Destroyed()
{
	Super::Destroyed();

	if (PickupSound) UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
}
