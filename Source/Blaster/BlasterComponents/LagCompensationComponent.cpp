#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

#include "Components/BoxComponent.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& FramePackage)
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		FramePackage.Time = GetWorld()->GetTimeSeconds();
		for (const auto& BoxPair : BlasterCharacter->HitBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			FramePackage.HitBoxesInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}
