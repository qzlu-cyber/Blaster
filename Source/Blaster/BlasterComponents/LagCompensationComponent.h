#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()
	
	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxesInfo;
};

USTRUCT()
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bConfirmedHit; // 是否命中

	UPROPERTY()
	bool bHitHead; // 是否命中头部
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(Server, Reliable)
	void ServerScoreConfirm(
		class ABlasterCharacter* HitCharacter,
		class AWeapon* DamageCauser,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

protected:
	void SaveFramePackage(FFramePackage& FramePackage);
	void SaveFramePackage();

	FServerSideRewindResult ServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

	FServerSideRewindResult ConfirmHit(
		ABlasterCharacter* HitCharacter,
		const FFramePackage& FramePackage,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation
	);
	void CacheBoxInformation(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& FramePackage);
	void ResetBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& FramePackage);

private:
	UPROPERTY()
	ABlasterCharacter* BlasterCharacter;
	UPROPERTY()
	class ABlasterPlayerController* PlayerController;

	TDoubleLinkedList<FFramePackage> FrameHistory; // 用双向链表来存储历史帧
	UPROPERTY()
	float MaxRecordTime = 4.f; // 最大记录时间

	friend ABlasterCharacter;
};
