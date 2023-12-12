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

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);

		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time; // 计算最新和最旧的帧之间的时间差
		while (HistoryLength > MaxRecordTime) // 如果时间差大于最大记录时间，就删除最旧的帧
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time; // 重新计算时间差
		}

		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);

		FrameHistory.AddHead(ThisFrame);
	}
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

/// @brief 服务器端回滚
/// @param HitCharacter 被命中的角色
/// @param TraceStart LineTrace 起点
/// @param HitLocation 命中位置
/// @param HitTime 命中的时间
void ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
                                                 const FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn = HitCharacter == nullptr ||
		BlasterCharacter->GetLagCompensationComponent() == nullptr ||
		BlasterCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||
		BlasterCharacter->GetLagCompensationComponent()->FrameHistory.GetTail() == nullptr;

	if (bReturn) return;

	FFramePackage FrameToCheck;

	bool bShouldInterpolate = true;

	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestTime = History.GetTail()->GetValue().Time;
	const float NewestTime = History.GetHead()->GetValue().Time;
	if (OldestTime > HitTime) return; // 如果最旧的帧的时间大于命中时间，说明太久远了，就不进行回滚
	if (OldestTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	/// 击中时间不处于边界
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* YoungerNode = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* OlderNode = History.GetHead();
	while (OlderNode->GetValue().Time > HitTime) // 循环找到击中帧的前后两帧，再通过插值得到击中时间时的帧信息
	{
		// 时间倒退，直到 OlderTime < HitTime < YoungerTime
		if (OlderNode->GetNextNode() == nullptr) break;
		OlderNode = OlderNode->GetNextNode();
		if (OlderNode->GetValue().Time > HitTime) YoungerNode = OlderNode;
	}

	if (OlderNode->GetValue().Time == HitTime)
	{
		FrameToCheck = OlderNode->GetValue();
		bShouldInterpolate = false;
	}

	if (!bShouldInterpolate)
	{
		FrameToCheck = InterpBetweenFrames(
			OlderNode->GetValue(),
			YoungerNode->GetValue(),
			HitTime
		);
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame,
	float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;
	for (auto& YoungerPair : YoungerFrame.HitBoxesInfo)
	{
		const FName& BoxName = YoungerPair.Key;
		const FBoxInformation& YoungerBoxInfo = YoungerPair.Value;
		const FBoxInformation& OlderBoxInfo = OlderFrame.HitBoxesInfo[BoxName];

		FBoxInformation InterpBoxInfo;
		/// FMath::VInterpTo 插值移动 DeltaMove 距离，DeltaMove = Distance * FMath::Clamp(DeltaTime * InterpSpeed, 0, 1)
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBoxInfo.Location, YoungerBoxInfo.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBoxInfo.Rotation, YoungerBoxInfo.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBoxInfo.BoxExtent;

		InterpFramePackage.HitBoxesInfo.Add(BoxName, InterpBoxInfo);
	}

	return InterpFramePackage;
}
