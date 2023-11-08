// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

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
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false; // 设置 camera 不跟随 pawn 的旋转，而是跟随 spring arm 的旋转

	bUseControllerRotationYaw = false; // 设置 pawn 不跟随控制器的旋转
	GetCharacterMovement()->bOrientRotationToMovement = true; // 设置 pawn 的旋转跟随移动方向

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); // 设置为 replicated, 使得该组件在 server 端和 client 端同步

	GetMovementComponent()->NavAgentProps.bCanCrouch = true; // 设置角色可以蹲下
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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

		bUseControllerRotationYaw = false; // 角色静止时，不跟随控制器的旋转，只使用瞄准旋转角度来控制 AOYaw，而不受控制器旋转的影响
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

bool ABlasterCharacter::IsWeaponEquipped() const
{
	// Equip Weapon 只在 server 端执行，要设置 EquippedWeapon 为 replicated
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() const
{
	return (Combat && Combat->bIsAiming);
}

