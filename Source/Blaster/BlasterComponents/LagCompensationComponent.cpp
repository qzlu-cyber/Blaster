#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

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

	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (BlasterCharacter == nullptr || !BlasterCharacter->HasAuthority()) return; // 只在服务器端保存帧信息，进行回滚
	
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
/// @return 回滚结果（是否命中）
FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
                                                 const FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn = HitCharacter == nullptr ||
		BlasterCharacter->GetLagCompensationComponent() == nullptr ||
		BlasterCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||
		BlasterCharacter->GetLagCompensationComponent()->FrameHistory.GetTail() == nullptr;

	if (bReturn) return FServerSideRewindResult();

	FFramePackage FrameToCheck;

	bool bShouldInterpolate = true;

	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestTime = History.GetTail()->GetValue().Time;
	const float NewestTime = History.GetHead()->GetValue().Time;
	if (OldestTime > HitTime) return FServerSideRewindResult(); // 如果最旧的帧的时间大于命中时间，说明太久远了，就不进行回滚
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

	return ConfirmHit(HitCharacter, FrameToCheck, TraceStart, HitLocation);
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

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(ABlasterCharacter* HitCharacter,
	const FFramePackage& FramePackage, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr || HitCharacter->GetMesh()) return FServerSideRewindResult();

	FFramePackage CurrentFramePackage; // 被击中角色当前的帧信息
	CacheBoxInformation(HitCharacter, CurrentFramePackage);
	MoveBoxes(HitCharacter, FramePackage); // 移动被击中角色的 HitBox 到击中时的位置

	/// 重新进行 LineTrace
	FHitResult ConfirmHitResult;
	const FVector TranceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UBoxComponent* HeadBox = HitCharacter->HitBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HitCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 关闭 Mesh 的碰撞，防止 LineTrace 击中 Mesh
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TranceEnd,
			ECC_Visibility
		);
		if (ConfirmHitResult.bBlockingHit) // 击中了头部
		{
			ResetBoxes(HitCharacter, CurrentFramePackage); // 重置 HitBoxes，关闭 HeadBox 的碰撞
			HitCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 打开 Mesh 的碰撞
			
			return FServerSideRewindResult{ true, true };
		}
		else // 没有击中头部，检查是否击中了其他部位的 Box
		{
			// 打开所有 HitBox 的碰撞
			for (auto& BoxPair : HitCharacter->HitBoxes)
			{
				if (BoxPair.Value != nullptr)
				{
					BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					BoxPair.Value->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				}
			}

			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TranceEnd,
				ECC_Visibility
			);
			if (ConfirmHitResult.bBlockingHit) // 击中了某个部位
			{
				ResetBoxes(HitCharacter, CurrentFramePackage); // 重置 HitBoxes，关闭 HeadBox 的碰撞
				HitCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 打开 Mesh 的碰撞
			
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	/// 没有击中任何部位
	ResetBoxes(HitCharacter, CurrentFramePackage); // 重置 HitBoxes，关闭 HeadBox 的碰撞
	HitCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 打开 Mesh 的碰撞
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxInformation(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;

	for (const auto& BoxPair : HitCharacter->HitBoxes)
	{
		if (BoxPair.Value != nullptr)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			OutFramePackage.HitBoxesInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& FramePackage)
{
	if (HitCharacter == nullptr) return;

	for (auto& BoxPair : HitCharacter->HitBoxes)
	{
		if (BoxPair.Value != nullptr)
		{
			const FName& BoxName = BoxPair.Key;
			const FBoxInformation& BoxInfo = FramePackage.HitBoxesInfo[BoxName];

			BoxPair.Value->SetWorldLocation(BoxInfo.Location);
			BoxPair.Value->SetWorldRotation(BoxInfo.Rotation);
		}
	}
}

void ULagCompensationComponent::ResetBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& FramePackage)
{
	if (HitCharacter == nullptr) return;

	for (auto& BoxPair : HitCharacter->HitBoxes)
	{
		if (BoxPair.Value != nullptr)
		{
			const FName& BoxName = BoxPair.Key;
			const FBoxInformation& BoxInfo = FramePackage.HitBoxesInfo[BoxName];

			BoxPair.Value->SetWorldLocation(BoxInfo.Location);
			BoxPair.Value->SetWorldRotation(BoxInfo.Rotation);

			BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::ServerScoreConfirm_Implementation(ABlasterCharacter* HitCharacter,
	AWeapon* DamageCauser, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult ConfirmResult = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	if (BlasterCharacter && DamageCauser && HitCharacter && ConfirmResult.bConfirmedHit)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter, // 被击中的角色
			DamageCauser->GetWeaponDamage(), // 伤害值
			BlasterCharacter->Controller, // 造成伤害的控制器
			DamageCauser, // DamageCause
			UDamageType::StaticClass() // 伤害类型
		);
	}
}
