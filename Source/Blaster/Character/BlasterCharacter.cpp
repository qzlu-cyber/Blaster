// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Blaster.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
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

	GetMovementComponent()->NavAgentProps.bCanCrouch = true; // 设置角色可以蹲下

	GetMesh()->SetCollisionObjectType(ECC_SkeletaMesh); // 设置 mesh 的碰撞类型为 SkeletalMesh
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f); // 设置角色旋转速度

	TurnInPlace = ETurnInPlace::ETIP_NotTurning; // 初始化转身动画状态

	NetUpdateFrequency = 66.f; // 设置网络同步频率，将此 actor 进行每秒复制的频率
	MinNetUpdateFrequency = 33.f; // 设置网络同步频率，复制属性很少发生变化时的节流率
}

// 注册需要同步的属性
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly); // 只在 owner 端同步
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat) Combat->Character = this;
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
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
		StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AOYaw = 0.f;

		bUseControllerRotationYaw = true;
	}
	// 只有角色静止时才会使用 AimOffset
	if (Speed == 0.f && !bIsInAir)
	{
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

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	// 设置只在 owner 端执行
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < HideCameraDistance)
	{
		GetMesh()->SetVisibility(false);

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
	}
	else
	{
		GetMesh()->SetVisibility(true);

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
	}
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
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Aiming);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Aiming);
		}
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::Fire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Fire);
		}
	}

}

void ABlasterCharacter::MoveForward(const FInputActionValue& Value)
{
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
	if (bIsCrouched) UnCrouch();
	else Crouch();
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched) UnCrouch();
	else Super::Jump();
}

void ABlasterCharacter::Aiming(const FInputActionValue& Value)
{
	if (Value.GetMagnitude() > 0.f)
	{
		if (HasAuthority()) Combat->Aiming(true);
		else ServerAiming(true);
	}
	else
	{
		if (HasAuthority()) Combat->Aiming(false);
		else ServerAiming(false);
	}
}

void ABlasterCharacter::Fire(const FInputActionValue& Value)
{
	if (Combat)
	{
		// 开火和战斗有关，交给 CombatComponent 处理
		if (Value.GetMagnitude() > 0.f) Combat->Fire(true);
		else Combat->Fire(false);
	}
}

void ABlasterCharacter::PlayFireWeaponMontage(bool bAiming)
{
	if (!Combat && !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName Section = bAiming ? FName("Rifle_Hip") : FName("Rifle_Aim"); // 根据是否瞄准跳转到对应的 Section
		AnimInstance->Montage_JumpToSection(Section);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat && !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 得到角色的动画实例
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName Section("FromFront");
		AnimInstance->Montage_JumpToSection(Section);
	}
}

// 不能在 client 端直接调用拾取函数，要先在 sever 端进行验证，统一由 server 端调用
void ABlasterCharacter::EquipWeapon(const FInputActionValue& Value)
{
	if (Combat)
	{
		if (HasAuthority()) Combat->EquipWeapon(OverlappingWeapon); // 如果是 server 端，直接拾取
		else ServerEquipWeapon(); // 如果是 client 端，调用 RPC
	}
}

void ABlasterCharacter::DropWeapon(const FInputActionValue& Value)
{
	if (Combat)
	{
		if (HasAuthority()) Combat->DropWeapon();
		else SeverDropWeapon();
	}
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

// 当 Client 调用 RPC 时， Server 端实际执行的操作
void ABlasterCharacter::ServerEquipWeapon_Implementation()
{
	/// OverlappingWeapon 只复制给了 owner，所以只有 owner 可以拾取武器
	/// 其他 client 端的 OverlappingWeapon 为 nullptr，所以不能拾取武器
	if (Combat) Combat->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::SeverDropWeapon_Implementation()
{
	if (Combat) Combat->DropWeapon();
}

void ABlasterCharacter::ServerAiming_Implementation(bool bIsAiming)
{
	if (Combat) Combat->Aiming(bIsAiming);
}

void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	// Equip Weapon 只在 server 端执行，要设置 EquippedWeapon 为 replicated
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() const
{
	return (Combat && Combat->bIsAiming);
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (!Combat) return FVector();
	return Combat->HitTarget;
}

