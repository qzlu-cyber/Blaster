// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagZone.h"
#include "Blaster/Weapon//Flag.h"
#include "Blaster/GameModes/CaptureTheFlagGameMode.h"

#include "Components/SphereComponent.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));\
	SetRootComponent(SphereComponent);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereOverlap);
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFlag* OverlappingFlag = Cast<AFlag>(OtherActor);
	if (OverlappingFlag && OverlappingFlag->GetTeam() != Team) // 敌方旗帜到本方旗帜区域，计算得分
	{
		// 只在 server 端能得到 GameMode
		ACaptureTheFlagGameMode* GameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>();
		if (GameMode) GameMode->FlagCaptured(OverlappingFlag, this);

		OverlappingFlag->ResetFlag();
	}
}
