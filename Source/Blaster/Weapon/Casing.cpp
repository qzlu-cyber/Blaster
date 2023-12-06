// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&ACasing::DestroyTimerFinished,
		DestroyDelay
	);

	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectImpulse);
}

void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACasing::DestroyTimerFinished()
{
	if (ShellSound) UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	
	Destroy();
}
