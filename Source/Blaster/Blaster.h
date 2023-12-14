// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/// 自定义碰撞频道
#define ECC_SkeletaMesh ECollisionChannel::ECC_GameTraceChannel1 // 用于武器的碰撞检测，碰撞体为 SkeletaMesh 而非 CapsuleComponent
#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel2 // 用于 ServerSideRewind 时武器的碰撞检测，碰撞体为 HitBoxes