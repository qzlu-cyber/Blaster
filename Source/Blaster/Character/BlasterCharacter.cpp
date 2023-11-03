// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

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
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly); // 只在 owner 端同步
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

void ABlasterCharacter::SetWeaponOverlapping(AWeapon* Weapon)
{
	if (OverlappingWeapon) OverlappingWeapon->ShowWeaponPickupWidget(false);
	
	OverlappingWeapon = Weapon;

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon) OverlappingWeapon->ShowWeaponPickupWidget(true);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon) OverlappingWeapon->ShowWeaponPickupWidget(true);
	if (LastWeapon) LastWeapon->ShowWeaponPickupWidget(false);
}
