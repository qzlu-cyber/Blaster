#include "BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameModes/BlasterGameMode.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerStarts/TeamPlayerStart.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadDisplay"));
	OverheadWidget->SetupAttachment(RootComponent);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh()); // 将 spring arm 绑定到 mesh 上。角色蹲下时 capsule 会缩小，但是 mesh 不会缩小，所以要绑定到 mesh 上
	CameraBoom->TargetArmLength = 600.f; // 设置 spring arm 的长度
	CameraBoom->bUsePawnControlRotation = true; // 设置 spring arm 的旋转跟随 pawn 的旋转

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // 设置 camera 不跟随 pawn 的旋转，而是跟随 spring arm 的旋转

	bUseControllerRotationYaw = false; // 设置 pawn 不跟随控制器的旋转
	GetCharacterMovement()->bOrientRotationToMovement = true; // 设置 pawn 的旋转跟随移动方向

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); // 设置为 replicated, 使得该组件在 server 端和 client 端同步

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	GetMovementComponent()->NavAgentProps.bCanCrouch = true; // 设置角色可以蹲下

	GetMesh()->SetCollisionObjectType(ECC_SkeletaMesh); // 设置 mesh 的碰撞类型为 SkeletalMesh
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f); // 设置角色旋转速度

	GrenadeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	GrenadeMeshComponent->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	GrenadeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TurnInPlace = ETurnInPlace::ETIP_NotTurning; // 初始化转身动画状态

	NetUpdateFrequency = 66.f; // 设置网络同步频率，将此 actor 进行每秒复制的频率
	MinNetUpdateFrequency = 33.f; // 设置网络同步频率，复制属性很少发生变化时的节流率

	/// Hit boxes for server-side rewind 
	Head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	Head->SetupAttachment(GetMesh(), FName("head"));
	HitBoxes.Add(FName("head"), Head); // 将 head 添加到 HitBoxes 中

	Pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	Pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitBoxes.Add(FName("pelvis"), Pelvis);

	Spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	Spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitBoxes.Add(FName("spine_02"), Spine_02);

	Spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	Spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitBoxes.Add(FName("spine_03"), Spine_03);

	UpperArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	UpperArm_L->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitBoxes.Add(FName("upperarm_l"), UpperArm_L);

	UpperArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	UpperArm_R->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitBoxes.Add(FName("upperarm_r"), UpperArm_R);

	LowerArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	LowerArm_L->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitBoxes.Add(FName("lowerarm_l"), LowerArm_L);

	LowerArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	LowerArm_R->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitBoxes.Add(FName("lowerarm_r"), LowerArm_R);

	Hand_L = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	Hand_L->SetupAttachment(GetMesh(), FName("hand_l"));
	HitBoxes.Add(FName("hand_l"), Hand_L);

	Hand_R = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	Hand_R->SetupAttachment(GetMesh(), FName("hand_r"));
	HitBoxes.Add(FName("hand_r"), Hand_R);

	Thigh_L = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	Thigh_L->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitBoxes.Add(FName("thigh_l"), Thigh_L);

	Thigh_R = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	Thigh_R->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitBoxes.Add(FName("thigh_r"), Thigh_R);

	Calf_L = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	Calf_L->SetupAttachment(GetMesh(), FName("calf_l"));
	HitBoxes.Add(FName("calf_l"), Calf_L);

	Calf_R = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	Calf_R->SetupAttachment(GetMesh(), FName("calf_r"));
	HitBoxes.Add(FName("calf_r"), Calf_R);

	Foot_L = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	Foot_L->SetupAttachment(GetMesh(), FName("foot_l"));
	HitBoxes.Add(FName("foot_l"), Foot_L);

	Foot_R = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	Foot_R->SetupAttachment(GetMesh(), FName("foot_r"));
	HitBoxes.Add(FName("foot_r"), Foot_R);

	// 为所有 HitBox 设置 CollisionChannel
	for (const auto HitBoxPair : HitBoxes)
	{
		if (HitBoxPair.Value)
		{
			HitBoxPair.Value->SetCollisionObjectType(ECC_HitBox);
			HitBoxPair.Value->SetCollisionResponseToAllChannels(ECR_Ignore);
			HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block); // 只开启 ECC_HitBox 通道的碰撞
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 禁用碰撞。目的只是用作滞后补偿组件在 rewind 时的定位
		}
	}
}

// 注册需要同步的属性
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly); // 只在 owner 端同步
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat) Combat->Character = this;

	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
	}

	if (LagCompensation)
	{
		LagCompensation->BlasterCharacter = this;
		if (Controller) LagCompensation->PlayerController = Cast<ABlasterPlayerController>(Controller);
	}
}

void ABlasterCharacter::SetMeshByTeam(ETeam Team)
{
	if (GetMesh() == nullptr || BlueTeamMesh == nullptr || RedTeamMesh == nullptr) return;

	switch (Team)
	{
		case ETeam::ET_NoTeam:
		case ETeam::ET_BlueTeam:
			GetMesh()->SetSkeletalMesh(BlueTeamMesh);
			break;
		case ETeam::ET_RedTeam:
			GetMesh()->SetSkeletalMesh(RedTeamMesh);
			break;
		default: break;
	}
}

void ABlasterCharacter::SetPlayerStart()
{
	if (HasAuthority() && BlasterPlayerState && BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(
			this,
			ATeamPlayerStart::StaticClass(),
			PlayerStarts
		);

		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamPlayerStart = Cast<ATeamPlayerStart>(Start);
			if (TeamPlayerStart && TeamPlayerStart->GetTeam() == BlasterPlayerState->GetTeam()) TeamPlayerStarts.Emplace(TeamPlayerStart);
		}

		if (TeamPlayerStarts.Num() > 0)
		{
			ATeamPlayerStart* PlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(
				PlayerStart->GetActorLocation(),
				PlayerStart->GetActorRotation()
			);
		}
	}
}

void ABlasterCharacter::UpdateHealthHUD()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController) BlasterPlayerController->SetBlasterHealthHUD(Health, MaxHealth); // 设置 HUD 的血条
}

void ABlasterCharacter::UpdateShieldHUD()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController) BlasterPlayerController->SetBlasterShieldHUD(Shield, MaxShield); // 设置 HUD 的护盾条
}

void ABlasterCharacter::UpdateAmmoHUD()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetCarriedAmmoHUD(Combat->CarriedWeaponAmmo);
		BlasterPlayerController->SetWeaponAmmoHUD(Combat->EquippedWeapon->GetAmmo());
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (GrenadeMeshComponent) GrenadeMeshComponent->SetVisibility(false); // 游戏开始隐藏 GrenadeMesh，只在投掷时显示

	UpdateShieldHUD(); // 初始化护盾条。仍然需要调用它，因为在 BeginPlay 时，并非所有的 HUD 元素都有效，OnProcess() 就无法设置
	UpdateHealthHUD(); // 初始化血条。仍然需要调用它，因为在 BeginPlay 时，并非所有的 HUD 元素都有效，OnProcess() 就无法设置

	UpdateAmmoHUD(); // 初始化弹药 HUD

	if (Combat) Combat->UpdateGrenades(); // 初始化手雷数量

	// 由 server 端统一处理受到伤害的逻辑
	if (HasAuthority()) OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage); // 注册受到伤害时的回调函数
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);

	HideCameraIfCharacterClose();
	// 初始化角色的状态，但 PlayerState 在第一帧中无效，通过对其进行轮询在其有效后再初始化角色的状态
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (Combat && Combat->bIsHoldingTheFlag)
	{
		UE_LOG(LogTemp, Warning, TEXT("bIsHoldingTheFlag Test"))
		TurnInPlace = ETurnInPlace::ETIP_NotTurning; // 禁止角色转身动画
		bRotateRootBone = false; // 禁止角色根骨骼旋转
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;

		return;
	}
	if (bDisableGameplay)
	{
		TurnInPlace = ETurnInPlace::ETIP_NotTurning; // 禁止角色转身动画
		bRotateRootBone = false; // 禁止角色根骨骼旋转
		bUseControllerRotationYaw = false;

		return;
	}
	// 如果不是 SimulatedProxy 且是受控制的角色，执行 AimOffset
	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled()) AimOffset(DeltaTime);
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f) OnRep_ReplicatedMovement();

		CalculateAOPitch();
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !bIsElimmed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		
		if (Combat) ServerEquipWeapon(StartingWeapon);
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDeath(0);

			SetMeshByTeam(BlasterPlayerState->GetTeam());

			SetPlayerStart();

			SpawnDefaultWeapon(); // 根据队伍设置不同的 SkeletalMesh 后生成默认武器

			// 如果当前角色依然属于 TopScoringPlayers，Respawn 后即生成 Crown
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState)) MulticastGainedTheLead();
		}
	}
}

void ABlasterCharacter::CalculateAOPitch()
{
	AOPitch = GetBaseAimRotation().Pitch;
	/// AOPitch 会在网络同步中压缩为 unsinged 类型，导致负数符号丢失
	/// 对 AOPitch 进行修正，使得其范围为 [-90, 90]
	if (AOPitch > 90.f && !IsLocallyControlled())
	{
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AOPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AOPitch);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && !Combat->EquippedWeapon) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();

	const bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		
		StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AOYaw = 0.f;

		bUseControllerRotationYaw = true;
	}
	// 只有角色静止时才会使用 AimOffset
	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartAimRotation);
		AOYaw = DeltaAimRotation.Yaw;
		
		if (TurnInPlace == ETurnInPlace::ETIP_NotTurning) InterpAOYaw = AOYaw;

		/// 实现角色根骨骼的旋转
		// 设置角色跟随控制器旋转，当角色旋转时，保持根骨不动，直到旋转到 (-)90 度，这时根骨会向右(向左)旋转
		// 在动画蓝图中，使用 RotateRootBone 将角色根骨骼转回去
		// 所以要做的就是改变 AOYaw 的值，当旋转时，可以让该值趋近于零(TurningInPlace 插值)，根骨骼就会根据控制旋转
		// AOYaw 用来模拟角色上半身的目标偏移。通过改变 AOYaw 的值，可以控制角色上半身的旋转
		// 又由于角色跟随控制器旋转，当 AOYaw 的值为 0 时，角色的根骨骼会转回到角色的方向
		// 插值使旋转更加平滑
		bUseControllerRotationYaw = true;

		TurningInPlace(DeltaTime);
	}

	CalculateAOPitch();
}

void ABlasterCharacter::TurningInPlace(float DeltaTime)
{
	if (AOYaw > 90.f) TurnInPlace = ETurnInPlace::ETIP_Right;
	else if (AOYaw < -90.f) TurnInPlace = ETurnInPlace::ETIP_Left;

	// 如果角色正在转身，将角色的旋转设置为插值后的旋转
	if (TurnInPlace != ETurnInPlace::ETIP_NotTurning)
	{
		InterpAOYaw = FMath::FInterpTo(InterpAOYaw, 0.f, DeltaTime, 6.f); // 插值计算角色旋转的目标值
		AOYaw = InterpAOYaw; // 将角色的旋转设置为插值后的旋转

		// 转动角度足够小，表示转身已经完成
		if (FMath::Abs(AOYaw) < 15.f)
		{
			TurnInPlace = ETurnInPlace::ETIP_NotTurning;
			StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 更新 StartAimRotation 用于下一次转身
		}
	}
}

void ABlasterCharacter::SimulateProxyTurningInPlace()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	bRotateRootBone = false;

	// 当开始移动时如果没有把 TurnInPlace 设置为 NotTurning，会出现角色滑行的问题
	// 消除快速转动角色并移动后出现滑行的问题
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();
	if (Speed > 0.f)
	{
		TurnInPlace = ETurnInPlace::ETIP_NotTurning;
		
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold) TurnInPlace = ETurnInPlace::ETIP_Right;
		else if (ProxyYaw < -TurnThreshold) TurnInPlace = ETurnInPlace::ETIP_Left;
		else TurnInPlace = ETurnInPlace::ETIP_NotTurning;

		return;
	}
	
	TurnInPlace = ETurnInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	// 设置只在 owner 端执行
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < HideCameraDistance)
	{
		GetMesh()->SetVisibility(false);

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
	}
	else
	{
		GetMesh()->SetVisibility(true);

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
	}
}

// 只在 server 端执行，所以 更新血条 和 播放受到攻击时的动画 操作放到 OnRep_Health 中
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	if (bIsElimmed || BlasterGameMode == nullptr) return; // 如果角色已经被淘汰，不再受到伤害

	Damage = BlasterGameMode->CalculateDamage(InstigatedBy, Controller, Damage); // 计算伤害

	float DamageToHealth = Damage; // 真伤
	if (Shield > 0.f)
	{
		if (Shield >= Damage) // 护盾值大于伤害值，此时护盾吸收所有伤害
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield); // 更新护盾
			DamageToHealth = 0.f;
		}
		else // 护盾值不够承受所有伤害，吸收一部分，剩余的对生命值造成伤害
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, MaxHealth); // 更新真伤
			Shield = 0.f;
		}
	}
	
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth); // 更新血量

	UpdateShieldHUD(); // 更新护盾条
	UpdateHealthHUD(); // 更新血条
	PlayHitReactMontage(); // 播放受到攻击时的动画

	if (Health == 0.f)
	{
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			if (BlasterPlayerController)
			{
				ABlasterPlayerController* AttackerPlayerController = Cast<ABlasterPlayerController>(InstigatedBy);
				BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerPlayerController);
			}
		}
	}
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(InputMapping, 100);
		}
	}
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		if (MoveForwardAction)
		{
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::MoveForward);
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Completed, this, &ABlasterCharacter::MoveForward);
		}
		if (MoveRightAction)
		{
			EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::MoveRight);
			EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Completed, this, &ABlasterCharacter::MoveRight);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Jump);
		}
		if (TurnAction)
		{
			EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Turn);
			EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Turn);
		}
		if (LookUpAction)
		{
			EnhancedInputComponent->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::LookUp);
			EnhancedInputComponent->BindAction(LookUpAction, ETriggerEvent::Completed, this, &ABlasterCharacter::LookUp);
		}
		if (PickUpAction) // 绑定拾取事件
		{
			EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Completed, this, &ABlasterCharacter::EquipWeapon);
		}
		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Completed, this, &ABlasterCharacter::DropWeapon);
		}
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Crouching);
		}
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::Aiming);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Aiming);
		}
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::Fire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Fire);
		}
		if (ThrowGrenadeAction)
		{
			EnhancedInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::ThrowGrenade);
			EnhancedInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Completed, this, &ABlasterCharacter::ThrowGrenade);
		}
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Reload);
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Reload);
		}
	}

}

void ABlasterCharacter::MoveForward(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	
	FVector2D MovementVector = Value.Get<FVector2D>(); // 获取输入的移动向量

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation(); // 获取控制器的旋转，角色的前进方向为控制器的前向向量
		const FRotator YawRotation(0, Rotation.Yaw, 0); // 保留 Yaw 轴的旋转，将其他轴的旋转置为 0
		
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); // 获取旋转矩阵以得到前向向量
		MovementVector.X *= MoveSpeed * UGameplayStatics::GetTimeSeconds(GetWorld()); // 乘以时间，使得移动速度不受帧率影响
		AddMovementInput(ForwardDirection, MovementVector.X); // 移动
	}
}

void ABlasterCharacter::MoveRight(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	FVector2D MovementVector = Value.Get<FVector2D>(); // 获取输入的移动向量

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation(); // 获取控制器的旋转
		const FRotator YawRotation(0, Rotation.Yaw, 0); // 保留 Yaw 轴的旋转，将其他轴的旋转置为 0
		
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y); // 获取旋转矩阵以得到前向向量
		MovementVector.Y *= MoveSpeed * UGameplayStatics::GetTimeSeconds(GetWorld()); // 乘以时间，使得移动速度不受帧率影响
		AddMovementInput(ForwardDirection, MovementVector.Y); // 移动
	}
}

void ABlasterCharacter::Turn(const FInputActionValue& Value)
{
	AddControllerYawInput(Value.GetMagnitude());
}

void ABlasterCharacter::LookUp(const FInputActionValue& Value)
{
	AddControllerPitchInput(Value.GetMagnitude());
}

void ABlasterCharacter::Crouching(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	if (Combat && Combat->bIsHoldingTheFlag) return;

	if (bIsCrouched) UnCrouch();
	else Crouch();
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;

	if (Combat && Combat->bIsHoldingTheFlag) return;

	if (bIsCrouched) UnCrouch();
	else Super::Jump();
}

void ABlasterCharacter::Aiming(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	
	if (Value.GetMagnitude() > 0.f) bIsAiming = true;
	else bIsAiming = false;

	if (Combat)
	{
		if (Combat->bIsHoldingTheFlag) return;

		Combat->Aiming(bIsAiming);
	}
}

void ABlasterCharacter::Fire(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		if (Combat->bIsHoldingTheFlag) return;

		// 开火和战斗有关，交给 CombatComponent 处理
		if (Value.GetMagnitude() > 0.f) Combat->Fire(true);
		else Combat->Fire(false);
	}
}

void ABlasterCharacter::ThrowGrenade(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		if (Combat->bIsHoldingTheFlag) return;
		
		if (Value.GetMagnitude() > 0.f) Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::Reload(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		if (Combat->bIsHoldingTheFlag) return;
		
		Combat->Reload();
	}
}

void ABlasterCharacter::PlayFireWeaponMontage(bool bAiming)
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName Section = bAiming ? FName("Rifle_Hip") : FName("Rifle_Aim"); // 根据是否瞄准跳转到对应的 Section
		AnimInstance->Montage_JumpToSection(Section);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName Section;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
			case EWeaponTypes::EWT_AssaultRifle:
				Section = FName("Rifle");
				break;
			case EWeaponTypes::EWT_RocketLauncher:
				Section = FName("Rifle");
				break;
			case EWeaponTypes::EWT_Pistol:
				Section = FName("Pistol");
				break;
			case EWeaponTypes::EWT_SubmachineGun:
				Section = FName("Rifle");
				break;
			case EWeaponTypes::EWT_Shotgun:
				Section = FName("Shotgun");
				break;
			case EWeaponTypes::EWT_SniperRifle:
				Section = FName("Rifle");
				break;
			case EWeaponTypes::EWT_GrenadeLauncher:
				Section = FName("Rifle");
				break;
			default: break;
		}
		AnimInstance->Montage_JumpToSection(Section);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && ElimMontage) AnimInstance->Montage_Play(ElimMontage);
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && ThrowGrenadeMontage) AnimInstance->Montage_Play(ThrowGrenadeMontage);
}

void ABlasterCharacter::PlaySwapWeaponsMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && SwapWeaponsMontage) AnimInstance->Montage_Play(SwapWeaponsMontage);
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName Section("FromFront");
		AnimInstance->Montage_JumpToSection(Section);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (BlasterGameMode && !bLeftGame) BlasterGameMode->PlayerRespawn(this, Controller);
	if (bLeftGame && IsLocallyControlled()) OnLeftGameDelegate.Broadcast();
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance) DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
}

void ABlasterCharacter::StartDissolveTimeline()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial); // 绑定回调函数，当 timeline play 时调用，timeline play 需要绑定 curve
	if (DissolveTimeline && DissolveCurve)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack); // 将 curve 添加到 timeline 中，并告知 timeline 在播放时去 DissolveTrack 中查找回调函数
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	// 丢掉或销毁武器
	if (Combat)
	{
		if (Combat->bIsHoldingTheFlag && Combat->Flag) Combat->DropWeapon(Combat->Flag);
		if (Combat->EquippedWeapon)
		{
			if (!Combat->EquippedWeapon->bDestroyWeapon) Combat->DropWeapon(Combat->EquippedWeapon);
			else Combat->EquippedWeapon->Destroy();
		}
		if (Combat->SecondaryWeapon)
		{
			if (!Combat->SecondaryWeapon->bDestroyWeapon) Combat->DropWeapon(Combat->SecondaryWeapon);
			else Combat->SecondaryWeapon->Destroy();
		}
	}
	
	MulticastElim(bPlayerLeftGame);
}

// server 端调用 ReceiveDamage()，ReceiveDamage 调用多播 RPC，在所有 client 端执行
void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	
	// 角色死亡，设置武器弹药归零
	if (BlasterPlayerController) BlasterPlayerController->SetWeaponAmmoHUD(0);
	
	bIsElimmed = true;
	PlayElimMontage();

	// 角色消融效果
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this); // 创建动态材质实例
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance); // 设置 mesh 的材质

		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.5f); // 设置材质的 Dissolve 参数
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f); // 设置材质的 Glow 参数
	}
	
	StartDissolveTimeline(); // 启动 timeline
	
	// 隐藏狙击镜
	if (IsLocallyControlled() && Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_SniperRifle && bIsAiming)
		ShowSniperScopeWidget(false);

	// 禁止角色移动和旋转
	GetCharacterMovement()->DisableMovement(); // 禁止移动
	GetCharacterMovement()->StopMovementImmediately(); // 立即停止移动，同时禁止角色跟随鼠标旋转
	// 禁止响应某些输入
	bDisableGameplay = true;
	// 禁用碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrenadeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 销毁 Crown
	if (CrownSystemComponent) CrownSystemComponent->DestroyComponent();
	
	// 重生
	GetWorldTimerManager().SetTimer(ElimTimerHandle, this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

// 不能在 client 端直接调用拾取函数，要先在 sever 端进行验证，统一由 server 端调用
void ABlasterCharacter::EquipWeapon(const FInputActionValue& Value)
{
	if (bIsElimmed) return;
	
	if (Combat)
	{
		if (Combat->bIsHoldingTheFlag) return;
		
		if (OverlappingWeapon) ServerEquipWeapon(OverlappingWeapon); // 无论是 server 还是 client 端，调用 server RPC 都会在 server 上执行
		else if (Combat->EquippedWeapon && Combat->SecondaryWeapon)
		{
			if (Combat->CombatState == ECombatState::ECS_Unoccupied)
			{
				ServerSwapWeapons();
				if (!HasAuthority()) PlaySwapWeaponsMontage();
			}
		}
	}
}

void ABlasterCharacter::DropWeapon(const FInputActionValue& Value)
{
	if (Combat && Combat->bIsHoldingTheFlag && Combat->Flag) // 如果拿着 Flag，按下后首先丢掉 Flag
	{
		SeverDropWeapon(Combat->Flag);

		return;
	}
	
	if (Combat && Combat->EquippedWeapon) SeverDropWeapon(Combat->EquippedWeapon);
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon) OverlappingWeapon->ShowWeaponPickupWidget(false);
	
	OverlappingWeapon = Weapon;

	// rep notify 只在 client 端调用，在 server 端显示 pickup widget
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon) OverlappingWeapon->ShowWeaponPickupWidget(true);
	}
}

// 当 OverlappingWeapon 被 sever 端 replicate 到指定的 client 端时调用
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon) OverlappingWeapon->ShowWeaponPickupWidget(true);
	if (LastWeapon) LastWeapon->ShowWeaponPickupWidget(false);
}

// 仅在 movement component 的位置和速度发生变化时才会调用，一旦停止移动 SimulateProxyTurningInPlace() 就不会被调用
void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimulateProxyTurningInPlace();

	TimeSinceLastMovementReplication = 0.f; // 重置时间
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateShieldHUD(); // 更新护盾条

	if (LastShield > Shield) PlayHitReactMontage();
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHealthHUD(); // 更新血条
	
	if (LastHealth > Health) PlayHitReactMontage(); // 播放受到攻击时的动画
}

// 当 Client 调用 RPC 时， Server 端实际执行的操作
void ABlasterCharacter::ServerEquipWeapon_Implementation(AWeapon* WeaponToEquip)
{
	/// OverlappingWeapon 只复制给了 owner，所以只有 owner 可以拾取武器
	/// 其他 client 端的 OverlappingWeapon 为 nullptr，所以不能拾取武器
	if (Combat) Combat->EquipWeapon(WeaponToEquip);
}

void ABlasterCharacter::ServerSwapWeapons_Implementation()
{
	if (Combat) Combat->SwapWeapons();
}

void ABlasterCharacter::SeverDropWeapon_Implementation(AWeapon* WeaponToDrop)
{
	if (Combat) Combat->DropWeapon(WeaponToDrop);
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterGameMode && BlasterPlayerState) BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;

	if (CrownSystemComponent == nullptr)
	{
		CrownSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetMesh()->GetBoneLocation("head") + FVector(0.f, 0.f, 30.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (CrownSystemComponent) CrownSystemComponent->Activate();
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownSystemComponent) CrownSystemComponent->DestroyComponent();
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	// Equip Weapon 只在 server 端执行，要设置 EquippedWeapon 为 replicated
	return (Combat && Combat->EquippedWeapon);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (!Combat || !Combat->EquippedWeapon) return nullptr;
	return Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return (Combat && Combat->bIsAiming);
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if (Combat) return Combat->bIsHoldingTheFlag;
	return false;
}

void ABlasterCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (Combat == nullptr) return;

	Combat->bIsHoldingTheFlag = bHolding;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (!Combat) return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (!Combat) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterPlayerState) return BlasterPlayerState->GetTeam();

	return ETeam::ET_NoTeam;
}
