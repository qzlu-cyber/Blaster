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

	UPROPERTY()
	class ABlasterCharacter* Character; // 用于标识是哪个角色的帧包
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

USTRUCT()
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadHits; // 头部被击中的次数

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyHits; // 身体被击中的次数
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
	void ServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& LaunchVelocity,
		float HitTime
	);

protected:
	void SaveFramePackage(FFramePackage& FramePackage);
	void SaveFramePackage();

	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime);
	FServerSideRewindResult ServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);
	FServerSideRewindResult ProjectileServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& LaunchVelocity,
		float HitTime
	);

	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

	FServerSideRewindResult ConfirmHit(
		ABlasterCharacter* HitCharacter,
		const FFramePackage& FramePackage,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation
	);
	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
	);
	FServerSideRewindResult ProjectileConfirmHit(
		ABlasterCharacter* HitCharacter,
		const FFramePackage& FramePackage,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& LaunchVelocity
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
