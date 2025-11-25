#include "Actors/MockMissileActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Systems/ScenarioMenuSubsystem.h"
#include "Systems/ScenarioTestMetrics.h"
#include "Actors/RadarJammerActor.h"
#include "EngineUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"

AMockMissileActor::AMockMissileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("MissileCollision"));
	CollisionComponent->InitSphereRadius(120.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetMobility(EComponentMobility::Movable);

	SetRootComponent(CollisionComponent);

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MissileRoot"));
	RootSceneComponent->SetupAttachment(CollisionComponent);
	RootSceneComponent->SetMobility(EComponentMobility::Movable);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	MeshComponent->SetupAttachment(RootSceneComponent);

	if (MeshComponent)
	{
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComponent->SetGenerateOverlapEvents(true);
		MeshComponent->SetEnableGravity(false);
		MeshComponent->SetSimulatePhysics(false);
	}

	LightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("MissileGlow"));
	if (LightComponent)
	{
		LightComponent->SetupAttachment(MeshComponent);
		LightComponent->SetIntensity(8000.f);
		LightComponent->SetAttenuationRadius(400.f);
		LightComponent->SetLightColor(FLinearColor(1.f, 0.2f, 0.05f));
		LightComponent->bUseInverseSquaredFalloff = false;
		LightComponent->SetMobility(EComponentMobility::Movable);
	}
}

void AMockMissileActor::BeginPlay()
{
	Super::BeginPlay();

	LastTrailLocation = GetActorLocation();
	bTrailActive = true;
}

void AMockMissileActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!bExpiredNotified)
	{
		bExpiredNotified = true;
		OnExpired.Broadcast(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AMockMissileActor::InitializeMissile(AActor* InTarget, float InLaunchSpeed, float InMaxLifetime)
{
	// 允许目标为nullptr，导弹会在视野内自动搜索目标
	TargetActor = InTarget;
	const float ClampedSpeed = FMath::Clamp(InLaunchSpeed, MinLaunchSpeed, MaxLaunchSpeed);
	Speed = ClampedSpeed;
	AscentSpeed = ClampedSpeed * 0.5f;
	MaxLifetime = InMaxLifetime;
	ElapsedLifetime = 0.f;
	bHasImpacted = false;
	bExpiredNotified = false;
	StartLocation = GetActorLocation();
	LastTargetSearchTime = 0.f;
	bInJammerRange = false;
	bCountermeasureActive = false;
	bShouldUseCountermeasure = false;
	CountermeasureActivationTime = -1.f;
	LatestCountermeasureTime = -1.f;
	LatestCountermeasureDistance = -1.f;
	bJammerDetectionLogged = false;
	JammerDetectionTime = -1.f;
	JammerDetectionDistance = -1.f;
	JammerDetectionBaseRadius = 0.f;
	JammerDetectionHeightDifference = 0.f;
	CountermeasureActivationDistanceToJammer = -1.f;
	CountermeasureActivationBaseRadius = 0.f;
	CountermeasureActivationHeightDifference = 0.f;
	bCountermeasureStatsSubmitted = false;
	bCountermeasureEnabled = false;
	bHLAllocationEnabled = false;
	// 注意：不要重置 bHasSplit，因为分裂的导弹需要保持这个状态
	// bHasSplit = false; // 注释掉，避免重置分裂状态
	if (SplitGeneration == 0) // 只有非分裂导弹才重置
	{
		bHasSplit = false;
	}
	// SplitGeneration 保持原值，不要重置

	// 初始化反制相关状态
	bCountermeasureEnabled = false;  // 默认不启用，需要根据算法配置设置
	bInJammerRange = false;
	bCountermeasureActive = false;
	bShouldUseCountermeasure = false;
	CountermeasureActivationTime = -1.f;
	LatestCountermeasureTime = -1.f;
	LatestCountermeasureDistance = -1.f;
	bIsSplitChild = false;
	SplitGroupId = INDEX_NONE;
	bUseFixedSplitTarget = false;
	FixedSplitTargetLocation = FVector::ZeroVector;
	
	// 如果提供了目标，缓存其位置；否则使用默认方向
	if (TargetActor.IsValid())
	{
		CachedTargetLocation = TargetActor->GetActorLocation();
	}
	else
	{
		// 没有初始目标，使用发射方向
		CachedTargetLocation = StartLocation + GetActorForwardVector() * 4000.f;
	}
	
	// 检查是否是沙漠地图
	bool bIsDesertMap = false;
	if (UWorld* World = GetWorld())
	{
		FString MapName = World->GetMapName();
		MapName.ToLowerInline();
		// 检查地图名称中是否包含"沙漠"或"desert"
		if (MapName.Contains(TEXT("沙漠")) || MapName.Contains(TEXT("desert")))
		{
			bIsDesertMap = true;
			UE_LOG(LogTemp, Log, TEXT("Missile: Detected desert map: %s"), *World->GetMapName());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Missile: Non-desert map: %s, reducing ascent height by half"), *World->GetMapName());
		}
	}
	
	// 根据目标距离动态计算上升高度
	// 对于近处目标（小于5000厘米），使用较小的上升高度或直接开始制导
	// 对于远处目标，使用较大的上升高度
	const float TargetDistance = FVector::Dist(StartLocation, CachedTargetLocation);
	float BaseAscentHeight = 0.f;
	
	if (TargetDistance < 3000.f)
	{
		// 非常近的目标（小于30米），直接开始制导，不上升
		BaseAscentHeight = 0.f;
		bAscending = false;
	}
	else if (TargetDistance < 10000.f)
	{
		// 近处目标（30-100米），使用较小的上升高度（500-1500厘米）
		BaseAscentHeight = FMath::Lerp(500.f, 1500.f, (TargetDistance - 3000.f) / 7000.f);
		bAscending = true;
	}
	else if (TargetDistance < 50000.f)
	{
		// 中距离目标（100-500米），使用中等上升高度（1500-3000厘米）
		BaseAscentHeight = FMath::Lerp(1500.f, 3000.f, (TargetDistance - 10000.f) / 40000.f);
		bAscending = true;
	}
	else
	{
		// 远距离目标（大于500米），使用较大的上升高度（3000-5000厘米）
		BaseAscentHeight = FMath::Lerp(3000.f, 5000.f, FMath::Clamp((TargetDistance - 50000.f) / 100000.f, 0.f, 1.f));
		bAscending = true;
	}
	
	// 如果不是沙漠地图，上升高度降低到原来的1/4（进一步降低）
	if (!bIsDesertMap && BaseAscentHeight > 0.f)
	{
		AscentHeight = BaseAscentHeight * 0.25f;
	}
	else
	{
		AscentHeight = BaseAscentHeight;
	}
	
	// 如果不需要上升，直接开始制导
	if (!bAscending)
	{
		Velocity = FVector::ZeroVector;
		BeginHoming();
	}
	else
	{
		Velocity = FVector::UpVector * AscentSpeed;
		SetActorRotation(Velocity.ToOrientationRotator());
	}
	
	LastTrailLocation = StartLocation;
	bTrailActive = true;
}

void AMockMissileActor::SetAlgorithmConfig(const TArray<FString>& AlgorithmNames, const TArray<FString>& PrototypeNames)
{
	static const TArray<FString> CountermeasureAlgorithmNames = {
		TEXT("干扰对抗算法"),
		TEXT("抗干扰识别"),
		TEXT("频谱分析与自适应选择")
	};

	bool bDetectedCountermeasure = false;
	bool bDetectedHLAllocation = false;
	bool bDetectedTrajectoryOptimization = false;

	for (const FString& AlgorithmName : AlgorithmNames)
	{
		for (const FString& CountermeasureName : CountermeasureAlgorithmNames)
		{
			if (AlgorithmName.Contains(CountermeasureName) || CountermeasureName.Contains(AlgorithmName))
			{
				bDetectedCountermeasure = true;
			}
		}

		if (AlgorithmName.Contains(TEXT("HL分配算法")))
		{
			bDetectedHLAllocation = true;
		}
		
		// 匹配"轨迹优化算法"或"轨迹规划算法"
		if (AlgorithmName.Contains(TEXT("轨迹优化算法")) || AlgorithmName.Contains(TEXT("轨迹规划算法")))
		{
			bDetectedTrajectoryOptimization = true;
		}
	}

	bCountermeasureEnabled = bDetectedCountermeasure;
	bHLAllocationEnabled = bDetectedHLAllocation && (SplitGeneration < MaxSplitGeneration);
	bTrajectoryOptimizationEnabled = bDetectedTrajectoryOptimization;

	if (bCountermeasureEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 启用反制功能（检测到干扰对抗算法）"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 未选择干扰对抗算法，反制功能已禁用"), *GetName());
	}

	if (bHLAllocationEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 启用 HL 分配算法效果"), *GetName());
	}
	
	if (bTrajectoryOptimizationEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 启用轨迹优化算法（绕过干扰区域）"), *GetName());
	}

	bool bDetectedEvasionSubsystem = false;
	for (const FString& PrototypeName : PrototypeNames)
	{
		if (PrototypeName.Contains(TEXT("躲避对抗")))
		{
			bDetectedEvasionSubsystem = true;
			break;
		}
	}

	SetEvasionSubsystemEnabled(bDetectedEvasionSubsystem);
}

void AMockMissileActor::SetEvasionSubsystemEnabled(bool bEnabled)
{
	bEvasiveSubsystemEnabled = bEnabled;
	if (bEvasiveSubsystemEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 启用躲避对抗分系统：将主动规避拦截导弹"), *GetName());
	}
	else
	{
		bPerformingEvasiveManeuver = false;
		EvasiveTimeRemaining = 0.f;
		CurrentEvasiveDirection = FVector::ZeroVector;
	}
}

void AMockMissileActor::SetSplitGeneration(int32 Generation)
{
	SplitGeneration = FMath::Max(0, Generation);
	bIsSplitChild = (SplitGeneration > 0);
	// 如果分裂层级 > 0，说明这是分裂生成的导弹，应该使用直线飞行
	if (SplitGeneration > 0)
	{
		bHasSplit = true;
		bAscending = false; // 确保不上升
	}
	if (SplitGeneration >= MaxSplitGeneration)
	{
		bHLAllocationEnabled = false;
	}
}

void AMockMissileActor::SetInterferenceMode(bool bActive)
{
	bElectromagneticInterferenceActive = bActive;
	if (bElectromagneticInterferenceActive)
	{
		AscentHeight = FMath::Clamp(AscentHeight, 0.f, 1200.f); // Limit ascent height
		AscentSpeed = FMath::Min(AscentSpeed, Speed * 0.35f); // Reduce ascent speed
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 电磁干扰模式：限制上升高度为 %.1f"), *GetName(), AscentHeight);
	}
}

void AMockMissileActor::SetupAppearance(UStaticMesh* InMesh, UMaterialInterface* InBaseMaterial, const FLinearColor& TintColor)
{
	if (MeshComponent)
	{
		if (InMesh)
		{
			MeshComponent->SetStaticMesh(InMesh);
			MeshComponent->SetRelativeScale3D(FVector(0.55f));
			MeshComponent->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		}

		if (InBaseMaterial)
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(InBaseMaterial, this);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TintColor);
				MeshComponent->SetMaterial(0, DynamicMaterial);
			}
			else
			{
				MeshComponent->SetMaterial(0, InBaseMaterial);
			}
		}
	}
}

void AMockMissileActor::TriggerImpact(AActor* OtherActor)
{
	HandleImpact(OtherActor);
}

void AMockMissileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HandleLifetime(DeltaSeconds);

	if (!IsPendingKillPending() && !bHasImpacted)
	{
		if (bIsInterceptor)
		{
			UpdateInterceptorBehavior(DeltaSeconds);
		}
		else
		{
				// 更新干扰检测
			UpdateJammerDetection();
			
			// 如果启用了轨迹优化，更新绕过路径（限制更新频率，避免频繁摆动）
			if (bTrajectoryOptimizationEnabled)
			{
				if (ElapsedLifetime - LastTrajectoryOptimizationUpdate >= TrajectoryOptimizationUpdateInterval)
				{
					UpdateTrajectoryOptimization();
					LastTrajectoryOptimizationUpdate = ElapsedLifetime;
				}
			}

			if (bEvasiveSubsystemEnabled)
			{
				UpdateInterceptorAwareness(DeltaSeconds);
			}
			
			// 定期搜索目标（如果还没有目标，或者当前目标消失）
			// 如果在干扰区域内，不搜索目标（失去目标）
			// 注意：轨迹优化算法不会失去目标，而是绕过干扰区域
			if ((!bInJammerRange || bTrajectoryOptimizationEnabled) && ElapsedLifetime - LastTargetSearchTime >= TargetSearchInterval)
			{
				SearchAndLockTarget();
				LastTargetSearchTime = ElapsedLifetime;
				
				// 打印目标状态日志
				if (TargetActor.IsValid())
				{
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 目标已锁定: %s (位置: %s, 距离: %.2f)"), 
						*GetName(), 
						*TargetActor->GetName(), 
						*TargetActor->GetActorLocation().ToString(), 
						FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation()));
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 当前无目标，正在搜索视野内的目标..."), *GetName());
				}
			}
			else if (bInJammerRange && !bTrajectoryOptimizationEnabled)
			{
				// 在干扰区域内，清除目标（轨迹优化算法不会失去目标，而是绕过）
				if (TargetActor.IsValid())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 进入干扰区域，失去目标: %s"), 
						*GetName(), *TargetActor->GetName());
					TargetActor = nullptr;
				}
			}
			
			// 如果已经分裂或使用固定分裂目标，使用直线飞行（不再追踪）
			if (bHasSplit || bUseFixedSplitTarget)
			{
				UpdateBallisticStraightLine(DeltaSeconds);
			}
			else if (bAscending)
			{
				UpdateAscent(DeltaSeconds);
			}
			else
			{
				UpdateHoming(DeltaSeconds);
			}
		}
	}

	UpdateTrail();
}

void AMockMissileActor::HandleLifetime(float DeltaSeconds)
{
	ElapsedLifetime += DeltaSeconds;
	if (ElapsedLifetime >= MaxLifetime)
	{
		if (!bExpiredNotified)
		{
			bExpiredNotified = true;
			// 清除干扰区域的反制状态（导弹消失后恢复）
			ClearJammerCountermeasure();
			// 输出反制统计信息
			LogCountermeasureStats();
			OnExpired.Broadcast(this);
		}
		Destroy();
	}
}

void AMockMissileActor::UpdateAscent(float DeltaSeconds)
{
	// 如果上升高度为0，直接开始制导
	if (AscentHeight <= 0.f)
	{
		BeginHoming();
		UpdateHoming(DeltaSeconds);
		return;
	}
	
	const FVector CurrentLocation = GetActorLocation();
	const float DeltaHeight = CurrentLocation.Z - StartLocation.Z;
	if (DeltaHeight >= AscentHeight)
	{
		BeginHoming();
		UpdateHoming(DeltaSeconds);
		return;
	}

	const FVector Step = FVector::UpVector * AscentSpeed * DeltaSeconds;
	FHitResult Hit;
	AddActorWorldOffset(Step, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit.GetActor());
		return;
	}
	
	// 在上升过程中，如果目标很近，提前开始制导
	if (TargetActor.IsValid())
	{
		const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetActor->GetActorLocation());
		// 如果目标距离小于上升高度的2倍，提前开始制导
		if (DistanceToTarget < AscentHeight * 2.f && DeltaHeight >= AscentHeight * 0.3f)
		{
			BeginHoming();
			UpdateHoming(DeltaSeconds);
			return;
		}
	}
}

void AMockMissileActor::BeginHoming()
{
	bAscending = false;
	
	// 如果没有目标，尝试搜索一个
	if (!TargetActor.IsValid())
	{
		SearchAndLockTarget();
	}
	
	// 如果仍然没有目标，使用当前方向继续飞行
	if (!TargetActor.IsValid())
	{
		CachedTargetLocation = GetActorLocation() + GetActorForwardVector() * 4000.f;
	}
	else
	{
		CachedTargetLocation = TargetActor->GetActorLocation();
	}
	
	// 计算抛物线轨迹的初始速度
	LaunchLocation = GetActorLocation();
	TrajectoryStartTime = ElapsedLifetime;
	
	const FVector ToTarget = CachedTargetLocation - LaunchLocation;
	const float HorizontalDistance = FVector2D(ToTarget.X, ToTarget.Y).Size();
	const float VerticalDistance = ToTarget.Z;
	
	// 计算抛物线轨迹所需的速度和角度
	// 使用弹道学公式：考虑重力的抛物线轨迹
	// 目标：找到合适的发射角度，使得导弹能够到达目标
	
	// 计算所需的时间（基于水平距离和速度）
	// 使用更合理的估算：考虑垂直距离的影响
	const float TotalDistance = ToTarget.Size();
	// 对于近处目标（小于5000厘米），使用更短的估算时间，避免速度过大
	const float EstimatedTime = TotalDistance < 5000.f 
		? FMath::Max(0.2f, TotalDistance / Speed)  // 近处目标：最小0.2秒
		: FMath::Max(0.5f, TotalDistance / Speed); // 远处目标：最小0.5秒
	
	// 计算垂直方向的初始速度（考虑重力）
	// 公式：h = v0y * t - 0.5 * g * t^2
	// 解出：v0y = (h + 0.5 * g * t^2) / t
	// 如果目标是向下的，需要额外的向上速度来克服重力
	const float GravityEffect = 0.5f * Gravity * EstimatedTime * EstimatedTime;
	const float VerticalVelocity = (VerticalDistance + GravityEffect) / EstimatedTime;
	
	// 计算水平方向的初始速度
	const float HorizontalVelocity = HorizontalDistance / EstimatedTime;
	
	// 确保速度不会太小或太大
	const float MinVelocity = Speed * 0.3f;
	const float MaxVelocity = Speed * 1.5f;
	
	// 构建初始速度向量
	FVector HorizontalDirection = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
	if (HorizontalDirection.IsNearlyZero())
	{
		HorizontalDirection = GetActorForwardVector();
		HorizontalDirection.Z = 0.f;
		HorizontalDirection.Normalize();
		if (HorizontalDirection.IsNearlyZero())
		{
			HorizontalDirection = FVector(1.f, 0.f, 0.f);
		}
	}
	
	// 限制速度在合理范围内
	const float ClampedHorizontalVel = FMath::Clamp(HorizontalVelocity, MinVelocity, MaxVelocity);
	const float ClampedVerticalVel = FMath::Clamp(VerticalVelocity, -MaxVelocity, MaxVelocity);
	
	InitialVelocity = HorizontalDirection * ClampedHorizontalVel + FVector::UpVector * ClampedVerticalVel;
	Velocity = InitialVelocity;
	
	// 设置导弹朝向
	FVector LookDirection = Velocity.GetSafeNormal();
	if (!LookDirection.IsNearlyZero())
	{
		SetActorRotation(LookDirection.Rotation());
	}
}

void AMockMissileActor::UpdateHoming(float DeltaSeconds)
{
	const FVector CurrentLocation = GetActorLocation();
	
	// 如果启用了轨迹优化且有绕过航点，优先使用绕过航点（强制绕过，不追踪目标）
	if (bTrajectoryOptimizationEnabled && bHasAvoidanceWaypoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Missile %s] ===== 使用绕过航点飞行 ===== 当前位置=%s, 绕过航点=%s"), 
			*GetName(), *CurrentLocation.ToString(), *AvoidanceWaypoint.ToString());
		
		// 检查是否已经到达绕过航点
		const float DistanceToWaypoint = FVector::Dist(CurrentLocation, AvoidanceWaypoint);
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 距离绕过航点: %.2f 厘米 (%.2f 米)"), 
			*GetName(), DistanceToWaypoint, DistanceToWaypoint / 100.f);
		
		if (DistanceToWaypoint < 2000.f) // 距离航点小于20米，认为已到达
		{
			// 清除绕过航点，继续朝向目标
			bHasAvoidanceWaypoint = false;
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] 已绕过干扰区域，继续朝向目标"), *GetName());
		}
		else
		{
			// 强制使用绕过航点作为目标，直接飞向绕过航点（不追踪实际目标）
			FVector ToWaypoint = AvoidanceWaypoint - CurrentLocation;
			FVector Direction = ToWaypoint.GetSafeNormal();
			if (Direction.IsNearlyZero())
			{
				Direction = GetActorForwardVector();
			}
			
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] 飞向绕过航点: 方向=%s, 步长=%.2f"), 
				*GetName(), *Direction.ToString(), Speed * DeltaSeconds);
			
			// 直接朝向绕过航点飞行
			const FVector Step = Direction * Speed * DeltaSeconds;
			FHitResult Hit;
			AddActorWorldOffset(Step, true, &Hit);
			if (Hit.IsValidBlockingHit())
			{
				HandleImpact(Hit.GetActor());
				return;
			}
			SetActorRotation(Direction.Rotation());
			
			// 检查移动后是否还在干扰区域内
			const FVector NewLocation = GetActorLocation();
			if (bInJammerRange)
			{
				UE_LOG(LogTemp, Error, TEXT("[Missile %s] 警告：移动后仍在干扰区域内！新位置=%s"), 
					*GetName(), *NewLocation.ToString());
			}
			
			// 不检查目标距离，专注于绕过
			return;
		}
	}
	else if (bTrajectoryOptimizationEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 轨迹优化已启用但无绕过航点 (bHasAvoidanceWaypoint=%d)"), 
			*GetName(), bHasAvoidanceWaypoint ? 1 : 0);
	}
	
	// 更新目标位置（只有在没有绕过航点时才更新）
	if (TargetActor.IsValid())
	{
		CachedTargetLocation = TargetActor->GetActorLocation();
	}
	else if (CachedTargetLocation.IsNearlyZero())
	{
		// 如果没有目标且缓存位置无效，使用当前方向
		CachedTargetLocation = GetActorLocation() + GetActorForwardVector() * 4000.f;
	}
	
	// 优先检查到实际目标Actor的距离（更准确）
	if (TargetActor.IsValid())
	{
		const float DistanceToActualTarget = FVector::Dist(CurrentLocation, TargetActor->GetActorLocation());
		// 增加命中距离阈值到500厘米（5米），对于近处目标更友好
		if (DistanceToActualTarget <= 500.f)
		{
			HandleImpact(TargetActor.Get());
			return;
		}
	}

	const FVector ToTarget = CachedTargetLocation - CurrentLocation;
	const float DistanceToTarget = ToTarget.Size();
	
	// HL分配算法：当导弹接近目标时（距离100米=10000cm）触发分裂
	if (bHLAllocationEnabled && !bHasSplit && TargetActor.IsValid())
	{
		const float DistanceToActualTarget = FVector::Dist(CurrentLocation, TargetActor->GetActorLocation());
		if (DistanceToActualTarget < 10000.f) // 100米
		{
			TrySplitForClusterTargets(TargetActor.Get());
		}
	}
	
	// 如果已经分裂，使用直线飞行（不再追踪）
	if (bHasSplit)
	{
		UpdateBallisticStraightLine(DeltaSeconds);
		return;
	}
	
	// 对于近处目标（小于5000厘米），使用更直接的追踪方式
	const bool bIsCloseTarget = DistanceToTarget < 5000.f;
	
	// 使用抛物线轨迹：考虑重力影响
	// 位置公式：P(t) = P0 + V0*t + 0.5*G*t^2
	// 其中 P0 是初始位置，V0 是初始速度，G 是重力加速度，t 是时间
	
	const float TrajectoryTime = ElapsedLifetime - TrajectoryStartTime;
	
	// 计算重力影响下的位置
	FVector GravityAcceleration = FVector(0.f, 0.f, -Gravity); // 重力向下
	FVector PredictedLocation = LaunchLocation + InitialVelocity * TrajectoryTime + 0.5f * GravityAcceleration * TrajectoryTime * TrajectoryTime;
	
	// 计算速度（考虑重力影响）
	Velocity = InitialVelocity + GravityAcceleration * TrajectoryTime;
	
	// 如果目标移动，调整方向以更好地追踪目标
	FVector ToTargetFromPredicted = CachedTargetLocation - PredictedLocation;
	// 对于近处目标，使用更大的修正因子，让导弹能更好地追踪
	const float CorrectionFactor = bIsCloseTarget ? 0.3f : 0.1f;
	FVector NewLocation = PredictedLocation + ToTargetFromPredicted * CorrectionFactor;
	
	// 移动导弹（限制步长，避免瞬移）
	const FVector Step = NewLocation - CurrentLocation;
	const float BaseMaxStepSize = Speed * DeltaSeconds * 1.5f; // 限制单帧最大移动距离
	constexpr float EvasiveBoostReferenceTime = 0.9f;
	float SpeedBoostFactor = 1.f;
	if (bPerformingEvasiveManeuver && EvasiveSpeedBoostTime > 0.f)
	{
		const float BoostAlpha = FMath::Clamp(EvasiveSpeedBoostTime / EvasiveBoostReferenceTime, 0.f, 1.f);
		SpeedBoostFactor = FMath::Lerp(1.f, EvasiveSpeedBoostMultiplier, BoostAlpha);
	}
	const float MaxStepSize = BaseMaxStepSize * SpeedBoostFactor;
	const float StepSize = Step.Size();
	FVector FinalStep = StepSize > MaxStepSize ? Step.GetSafeNormal() * MaxStepSize : Step;

	if (bPerformingEvasiveManeuver && !CurrentEvasiveDirection.IsNearlyZero())
	{
		const FVector EvasiveOffset = CurrentEvasiveDirection.GetSafeNormal() * Speed * DeltaSeconds * 0.85f;
		FinalStep += EvasiveOffset;

		if (SpeedBoostFactor > 1.f)
		{
			const FVector ForwardBoost = GetActorForwardVector().GetSafeNormal() * BaseMaxStepSize * (SpeedBoostFactor - 1.f);
			FinalStep += ForwardBoost;
		}

		const float AdjustedSize = FinalStep.Size();
		const float MaxAllowed = MaxStepSize * 1.35f;
		if (AdjustedSize > MaxAllowed && AdjustedSize > KINDA_SMALL_NUMBER)
		{
			FinalStep = FinalStep.GetSafeNormal() * MaxAllowed;
		}
	}
	FHitResult Hit;
	AddActorWorldOffset(FinalStep, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit.GetActor());
		return;
	}

	// 更新导弹朝向
	FVector LookDirection;
	if (bIsCloseTarget)
	{
		// 对于近处目标，更直接地朝向目标
		LookDirection = ToTarget.GetSafeNormal();
	}
	else
	{
		// 对于远处目标，朝向速度方向
		LookDirection = Velocity.GetSafeNormal();
	}
	
	if (!LookDirection.IsNearlyZero())
	{
		SetActorRotation(LookDirection.Rotation());
	}

	// 再次检查到实际目标的距离（在移动后）
	if (TargetActor.IsValid())
	{
		const float DistanceSq = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation());
		if (DistanceSq <= FMath::Square(500.f)) // 增加到500厘米
		{
			HandleImpact(TargetActor.Get());
		}
	}
}

void AMockMissileActor::HandleImpact(AActor* HitActor)
{
	if (bHasImpacted)
	{
		return;
	}

	if (SplitGroupId != INDEX_NONE)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
				{
					const FString HitName = (HitActor ? HitActor->GetName() : FString());
					Subsystem->RegisterHLSplitGroupHit(SplitGroupId, HitName);
					if (bIsSplitChild)
					{
						Subsystem->RegisterHLSplitChildHit();
					}
				}
			}
		}
	}

	// 注意：HL分配算法的分裂逻辑已经在UpdateHoming中处理（接近目标时分裂）
	// 这里不再调用TrySplitForClusterTargets，因为应该在命中前就分裂

	bHasImpacted = true;
	
	// 记录命中时是否正在被干扰
	UE_LOG(LogTemp, Log, TEXT("[Missile %s] 命中目标 %s，命中时是否正在被干扰: %s"), 
		*GetName(), 
		HitActor ? *HitActor->GetName() : TEXT("未知"),
		bInJammerRange ? TEXT("是") : TEXT("否"));
	
	// 清除干扰区域的反制状态（导弹消失后恢复）
	ClearJammerCountermeasure();
	
	// 输出反制统计信息
	LogCountermeasureStats();

	if (bTrailActive)
	{
		if (UWorld* World = GetWorld())
		{
			const FColor TrailColor = bIsInterceptor ? FColor(80, 160, 255) : FColor::Red;
			DrawDebugLine(World, LastTrailLocation, GetActorLocation(), TrailColor, true, TrailLifetime, 0, TrailThickness);
		}
		bTrailActive = false;
	}

	OnImpact.Broadcast(this, HitActor);
	if (!bExpiredNotified)
	{
		bExpiredNotified = true;
		OnExpired.Broadcast(this);
	}

	if (!IsPendingKillPending())
	{
		Destroy();
	}
}

void AMockMissileActor::UpdateTrail()
{
	if (!bTrailActive)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (FVector::DistSquared(CurrentLocation, LastTrailLocation) < FMath::Square(TrailPointSpacing))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const FColor TrailColor = bIsInterceptor ? FColor(80, 160, 255) : FColor::Red;
		DrawDebugLine(World, LastTrailLocation, CurrentLocation, TrailColor, true, TrailLifetime, 0, TrailThickness);
	}

	LastTrailLocation = CurrentLocation;
}

bool AMockMissileActor::IsTargetInView(AActor* Candidate) const
{
	if (!Candidate || Candidate->IsPendingKillPending())
	{
		return false;
	}

	const FVector MissileLocation = GetActorLocation();
	const FVector MissileForward = GetActorForwardVector();
	const FVector ToTarget = Candidate->GetActorLocation() - MissileLocation;
	const float Distance = ToTarget.Size();

	// 检查距离
	if (Distance > ViewDistance || Distance < 100.f)
	{
		return false;
	}

	// 检查角度（视野锥）
	const FVector ToTargetNormalized = ToTarget.GetSafeNormal();
	const float DotProduct = FVector::DotProduct(MissileForward, ToTargetNormalized);
	const float ViewAngleRad = FMath::DegreesToRadians(ViewAngle);
	const float CosViewAngle = FMath::Cos(ViewAngleRad * 0.5f);
	
	if (DotProduct < CosViewAngle)
	{
		return false; // 不在视野角度内
	}

	// 检查是否有遮挡（射线检测）
	if (UWorld* World = GetWorld())
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(Candidate);
		QueryParams.bTraceComplex = false;

		const FVector Start = MissileLocation;
		const FVector End = Candidate->GetActorLocation();
		
		if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			// 如果射线被遮挡，检查是否遮挡物就是目标本身（允许）
			if (HitResult.GetActor() != Candidate)
			{
				return false; // 被其他物体遮挡
			}
		}
	}

	return true; // 在视野内且无遮挡
}

AActor* AMockMissileActor::FindTargetInView() const
{
	// 通过GameInstance获取ScenarioMenuSubsystem，访问蓝方单位列表
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
			{
				// 获取所有有效的蓝方单位
				TArray<AActor*> BlueUnits;
				Subsystem->GetActiveBlueUnits(BlueUnits);
				
				// 遍历所有候选目标，找到视野内的最近目标
				AActor* BestTarget = nullptr;
				float BestDistance = MAX_FLT;
				
				for (AActor* Candidate : BlueUnits)
				{
					if (IsTargetInView(Candidate))
					{
						const float Distance = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
						if (Distance < BestDistance)
						{
							BestTarget = Candidate;
							BestDistance = Distance;
						}
					}
				}
				
				return BestTarget;
			}
		}
	}
	
	return nullptr;
}

void AMockMissileActor::SearchAndLockTarget()
{
	// 如果当前目标有效且在视野内，保持锁定
	if (TargetActor.IsValid())
	{
		if (IsTargetInView(TargetActor.Get()))
		{
			// 目标仍然在视野内，更新缓存位置
			CachedTargetLocation = TargetActor->GetActorLocation();
			return;
		}
		// 目标不在视野内或已失效，清除目标
		TargetActor = nullptr;
	}

	// 搜索视野内的新目标
	AActor* NewTarget = FindTargetInView();
	if (NewTarget)
	{
		// 如果切换了目标，打印日志
		if (!TargetActor.IsValid() || TargetActor.Get() != NewTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] 锁定新目标: %s (位置: %s, 距离: %.2f)"), 
				*GetName(), 
				*NewTarget->GetName(), 
				*NewTarget->GetActorLocation().ToString(), 
				FVector::Dist(GetActorLocation(), NewTarget->GetActorLocation()));
		}
		
		TargetActor = NewTarget;
		CachedTargetLocation = NewTarget->GetActorLocation();
		
		// 如果已经开始制导，重新计算轨迹（从当前位置开始新的抛物线）
		if (!bAscending)
		{
			// 重新设置轨迹起点和时间，以便计算新的抛物线
			LaunchLocation = GetActorLocation();
			TrajectoryStartTime = ElapsedLifetime;
			BeginHoming();
		}
	}
	else if (!bAscending)
	{
		// 没有找到目标，但已经开始制导，继续按当前轨迹飞行
		// 使用当前方向作为目标方向
		if (CachedTargetLocation.IsNearlyZero() || FVector::DistSquared(GetActorLocation(), CachedTargetLocation) < 10000.f)
		{
			CachedTargetLocation = GetActorLocation() + GetActorForwardVector() * 4000.f;
		}
		
		// 如果之前有目标但现在丢失了，打印日志
		if (TargetActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 目标 %s 已丢失（不在视野内或已销毁），继续搜索..."), 
				*GetName(), 
				*TargetActor->GetName());
		}
	}
}

bool AMockMissileActor::IsInJammerRange() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
			{
				// 获取所有雷达干扰区域
				FVector MissileLocation = GetActorLocation();
				TArray<ARadarJammerActor*> Jammers;
				Subsystem->GetActiveRadarJammers(Jammers);
				
				for (ARadarJammerActor* Jammer : Jammers)
				{
					if (Jammer && Jammer->IsPointInJammerRange(MissileLocation))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

void AMockMissileActor::UpdateJammerDetection()
{
	bool bWasInRange = bInJammerRange;
	bInJammerRange = IsInJammerRange();

	ARadarJammerActor* NearestJammer = nullptr;
	float NearestDistance = 0.f;
	float NearestBaseRadius = 0.f;
	float NearestHeightDiff = 0.f;
	const bool bHasNearestJammer = GetNearestJammerInfo(NearestJammer, NearestDistance, NearestBaseRadius, NearestHeightDiff);

	if (bHasNearestJammer && NearestJammer && !bJammerDetectionLogged)
	{
		const float DetectionRadius = NearestJammer->GetDetectionRadius();
		if (NearestDistance <= DetectionRadius)
		{
			bJammerDetectionLogged = true;
			JammerDetectionTime = ElapsedLifetime;
			JammerDetectionDistance = NearestDistance;
			JammerDetectionBaseRadius = NearestBaseRadius;
			JammerDetectionHeightDifference = NearestHeightDiff;
		}
	}

	if (!bCountermeasureEnabled)
	{
		// 如果启用了轨迹优化，不会失去目标，而是绕过干扰区域
		if (bTrajectoryOptimizationEnabled)
		{
			if (bInJammerRange && !bWasInRange)
			{
				UE_LOG(LogTemp, Log, TEXT("[Missile %s] 进入雷达干扰区域（轨迹优化：将绕过）"), *GetName());
			}
			else if (!bInJammerRange && bWasInRange)
			{
				UE_LOG(LogTemp, Log, TEXT("[Missile %s] 离开雷达干扰区域（轨迹优化）"), *GetName());
			}
			return; // 轨迹优化算法不会失去目标
		}
		
		// 未启用轨迹优化和反制，进入干扰区域会失去目标
		if (bInJammerRange && !bWasInRange)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 进入雷达干扰区域，失去目标锁定（未启用反制功能）"), *GetName());
			TargetActor = nullptr;
		}
		else if (!bInJammerRange && bWasInRange)
		{
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] 离开雷达干扰区域（未启用反制功能）"), *GetName());
		}
		return;
	}

	// 如果进入干扰区域
	if (bInJammerRange && !bWasInRange)
	{
		if (!bJammerDetectionLogged && bHasNearestJammer && NearestJammer)
		{
			bJammerDetectionLogged = true;
			JammerDetectionTime = ElapsedLifetime;
			JammerDetectionDistance = NearestDistance;
			JammerDetectionBaseRadius = NearestBaseRadius;
			JammerDetectionHeightDifference = NearestHeightDiff;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 进入雷达干扰区域，失去目标锁定"), *GetName());
		
		// 记录是否需要反制（如果还没有激活反制）
		// 在清除目标之前记录距离
		if (!bCountermeasureActive)
		{
			bShouldUseCountermeasure = true;
			if (LatestCountermeasureTime < 0.f)
			{
				LatestCountermeasureTime = ElapsedLifetime;
				if (TargetActor.IsValid())
				{
					LatestCountermeasureDistance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
				}
				else
				{
					LatestCountermeasureDistance = -1.f;
				}
			}
		}
		
		// 自动反制逻辑：一进入干扰区域就自动开启反制
		if (!bCountermeasureActive)
		{
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] 检测到干扰信号，自动开启反制系统"), *GetName());
			ActivateCountermeasure();
		}
		
		// 清除目标
		TargetActor = nullptr;
	}
	// 如果离开干扰区域
	else if (!bInJammerRange && bWasInRange)
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 离开雷达干扰区域，重新搜索目标"), *GetName());
		// 离开干扰区域时，清除该干扰区域的反制状态
		ClearJammerCountermeasure();
	}
	
	// 如果启用了反制，无论导弹是否在干扰区域内，只要接近干扰区域就更新半径
	if (bCountermeasureActive)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
				{
					FVector MissileLocation = GetActorLocation();
					TArray<ARadarJammerActor*> Jammers;
					Subsystem->GetActiveRadarJammers(Jammers);
					
					for (ARadarJammerActor* Jammer : Jammers)
					{
						if (Jammer)
						{
							const float LocalDistanceToJammer = FVector::Dist(MissileLocation, Jammer->GetActorLocation());
							const float ActivationThreshold = Jammer->GetDetectionRadius();
							
							if (LocalDistanceToJammer < ActivationThreshold)
							{
								Jammer->UpdateRadiusByMissileDistance(LocalDistanceToJammer);
							}
							else
							{
								Jammer->ClearCountermeasure();
							}
						}
					}
				}
			}
		}
	}
}

void AMockMissileActor::ActivateCountermeasure()
{
	if (!bCountermeasureActive)
	{
		bCountermeasureActive = true;
		ARadarJammerActor* NearestJammer = nullptr;
	float NearestDistanceToJammer = -1.f;
		float BaseRadius = 0.f;
		float HeightDifference = 0.f;
	if (GetNearestJammerInfo(NearestJammer, NearestDistanceToJammer, BaseRadius, HeightDifference))
		{
		CountermeasureActivationDistanceToJammer = NearestDistanceToJammer;
			CountermeasureActivationBaseRadius = BaseRadius;
			CountermeasureActivationHeightDifference = HeightDifference;

			if (!bJammerDetectionLogged)
			{
				bJammerDetectionLogged = true;
				JammerDetectionTime = ElapsedLifetime;
			JammerDetectionDistance = NearestDistanceToJammer;
				JammerDetectionBaseRadius = BaseRadius;
				JammerDetectionHeightDifference = HeightDifference;
			}
		}
		CountermeasureActivationTime = ElapsedLifetime;
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 激活反制系统，时间: %.2f秒"), *GetName(), CountermeasureActivationTime);
		
		// 更新所有干扰区域的反制状态
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
				{
					FVector MissileLocation = GetActorLocation();
					TArray<ARadarJammerActor*> Jammers;
					Subsystem->GetActiveRadarJammers(Jammers);
					
					for (ARadarJammerActor* Jammer : Jammers)
					{
						if (Jammer)
						{
							// 计算导弹到干扰器中心的距离
							float DistanceToJammer = FVector::Dist(MissileLocation, Jammer->GetActorLocation());
							
							// 直接根据距离更新半径
							Jammer->UpdateRadiusByMissileDistance(DistanceToJammer);
						}
					}
				}
			}
		}
	}
}

void AMockMissileActor::TrySplitForClusterTargets(AActor* HitActor)
{
	if (!bHLAllocationEnabled || bHasSplit || SplitGeneration >= MaxSplitGeneration)
	{
		return;
	}

	AActor* ReferenceActor = HitActor ? HitActor : TargetActor.Get();
	if (!ReferenceActor)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	Subsystem->RegisterHLSplitAttempt();

	TArray<AActor*> BlueUnits;
	Subsystem->GetActiveBlueUnits(BlueUnits);

	const FVector ImpactLocation = ReferenceActor->GetActorLocation();
	const float ClusterRadius = 8000.f;
	const int32 MaxTotalMissiles = 4;
	const int32 AdditionalSlots = MaxTotalMissiles - 1;

	TArray<AActor*> CandidateTargets;
	for (AActor* Candidate : BlueUnits)
	{
		if (!Candidate || Candidate == ReferenceActor)
		{
			continue;
		}

		if (FVector::Dist(ImpactLocation, Candidate->GetActorLocation()) <= ClusterRadius)
		{
			CandidateTargets.Add(Candidate);
		}
	}

	if (CandidateTargets.Num() == 0)
	{
		return;
	}

	struct FTargetDistanceSorter
	{
		FVector RefLocation;
		bool operator()(const AActor& A, const AActor& B) const
		{
			return FVector::DistSquared(RefLocation, A.GetActorLocation()) < FVector::DistSquared(RefLocation, B.GetActorLocation());
		}
	};

	CandidateTargets.Sort(FTargetDistanceSorter{ ImpactLocation });

	const int32 SpawnCount = FMath::Min(AdditionalSlots, CandidateTargets.Num());
	if (SpawnCount <= 0)
	{
		return;
	}

	bHasSplit = true;

	const FVector ParentForward = GetActorForwardVector().GetSafeNormal().IsNearlyZero() ? FVector::ForwardVector : GetActorForwardVector().GetSafeNormal();
	FVector ParentRight = FVector::CrossProduct(ParentForward, FVector::UpVector).GetSafeNormal();
	if (ParentRight.IsNearlyZero())
	{
		ParentRight = FVector::RightVector;
	}
	const float BaseForwardOffset = 150.f;
	const float LateralSpacing = 200.f;
	const float VerticalOffset = 60.f;

	const int32 AssignedTargets = 1 + SpawnCount;
	const int32 NewSplitGroupId = Subsystem->RegisterHLSplitSuccess(AssignedTargets);
	SetSplitGroupId(NewSplitGroupId);
	Subsystem->UpdateMissileSplitMeta(this, false, NewSplitGroupId);

	TArray<AMockMissileActor*> SpawnedChildren;
	SpawnedChildren.Reserve(SpawnCount);

	for (int32 i = 0; i < SpawnCount; ++i)
	{
		AActor* Candidate = CandidateTargets[i];
		if (!Candidate)
		{
			continue;
		}

		const float SpreadIndex = (SpawnCount > 1) ? (i - (SpawnCount - 1) * 0.5f) : 0.f;
		const FVector SpawnLocation = GetActorLocation()
			+ ParentForward * (BaseForwardOffset + i * 40.f)
			+ ParentRight * (SpreadIndex * LateralSpacing)
			+ FVector::UpVector * VerticalOffset;

		const FVector Direction = (Candidate->GetActorLocation() - SpawnLocation).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			continue;
		}

		const FRotator SpawnRotation = Direction.Rotation();
		if (AMockMissileActor* NewMissile = Subsystem->SpawnMissile(World, Candidate, false, &SpawnLocation, &SpawnRotation))
		{
			NewMissile->SetSplitGeneration(SplitGeneration + 1);
			NewMissile->SetSplitGroupId(NewSplitGroupId);
			NewMissile->SetFixedSplitTarget(Candidate->GetActorLocation());
			Subsystem->RegisterHLSplitChildSpawn();
			Subsystem->UpdateMissileSplitMeta(NewMissile, true, NewSplitGroupId);
			SpawnedChildren.Add(NewMissile);
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] HL 分配：生成分裂导弹 -> %s (直线飞行模式)"), *GetName(), *Candidate->GetName());
		}
	}

	for (AMockMissileActor* Child : SpawnedChildren)
	{
		if (!Child)
		{
			continue;
		}
		Child->AddCollisionIgnoreActor(this);
		AddCollisionIgnoreActor(Child);

		for (AMockMissileActor* OtherChild : SpawnedChildren)
		{
			if (!OtherChild || Child == OtherChild)
			{
				continue;
			}
			Child->AddCollisionIgnoreActor(OtherChild);
		}
	}
}

void AMockMissileActor::AddCollisionIgnoreActor(AActor* ActorToIgnore)
{
	if (CollisionComponent && ActorToIgnore)
	{
		CollisionComponent->IgnoreActorWhenMoving(ActorToIgnore, true);
	}
}

void AMockMissileActor::ClearJammerCountermeasure()
{
	// 清除所有干扰区域的反制状态，恢复原始半径
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
			{
				TArray<ARadarJammerActor*> Jammers;
				Subsystem->GetActiveRadarJammers(Jammers);
				
				for (ARadarJammerActor* Jammer : Jammers)
				{
					if (Jammer)
					{
						Jammer->ClearCountermeasure();
					}
				}
			}
		}
	}
}

void AMockMissileActor::LogCountermeasureStats() const
{
	// 如果未启用反制功能，不记录统计信息
	if (!bCountermeasureEnabled)
	{
		SubmitCountermeasureStats();
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("========== 导弹反制统计 [%s] =========="), *GetName());
	UE_LOG(LogTemp, Log, TEXT("是否需要开启反制: %s"), bShouldUseCountermeasure ? TEXT("是") : TEXT("否"));
	
	if (LatestCountermeasureTime >= 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("最晚需要反制的时间: %.2f秒"), LatestCountermeasureTime);
		if (LatestCountermeasureDistance >= 0.f)
		{
			UE_LOG(LogTemp, Log, TEXT("最晚需要反制时与目标的距离: %.2f厘米 (%.2f米)"), 
				LatestCountermeasureDistance, LatestCountermeasureDistance / 100.f);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("最晚需要反制的时间: 未记录"));
		UE_LOG(LogTemp, Log, TEXT("最晚需要反制时与目标的距离: 未记录"));
	}
	
	if (bCountermeasureActive)
	{
		UE_LOG(LogTemp, Log, TEXT("反制已激活: 是，激活时间: %.2f秒"), CountermeasureActivationTime);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("反制已激活: 否"));
	}
	
	UE_LOG(LogTemp, Log, TEXT("=========================================="));
	SubmitCountermeasureStats();
}

void AMockMissileActor::SubmitCountermeasureStats() const
{
	if (bCountermeasureStatsSubmitted)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
			{
				FMissileCountermeasureStats Stats;
				Stats.bCountermeasureEnabled = bCountermeasureEnabled;
				Stats.bDetectionLogged = bJammerDetectionLogged;
				Stats.DetectionTime = JammerDetectionTime;
				Stats.DetectionDistanceToJammer = JammerDetectionDistance;
				Stats.DetectionBaseRadius = JammerDetectionBaseRadius;
				Stats.DetectionHeightDifference = JammerDetectionHeightDifference;
				Stats.bCountermeasureTriggered = bShouldUseCountermeasure;
				Stats.CountermeasureTriggerTime = LatestCountermeasureTime;
				Stats.CountermeasureTargetDistance = LatestCountermeasureDistance;
				Stats.bCountermeasureActivated = (CountermeasureActivationTime >= 0.f);
				Stats.CountermeasureActivationTime = CountermeasureActivationTime;
				Stats.CountermeasureActivationDistanceToJammer = CountermeasureActivationDistanceToJammer;
				Stats.CountermeasureActivationBaseRadius = CountermeasureActivationBaseRadius;
				Stats.CountermeasureActivationHeightDifference = CountermeasureActivationHeightDifference;

				Subsystem->UpdateMissileCountermeasureStats(const_cast<AMockMissileActor*>(this), Stats);
			}
		}
	}

	bCountermeasureStatsSubmitted = true;
}

void AMockMissileActor::UpdateBallisticStraightLine(float DeltaSeconds)
{
	const FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = CachedTargetLocation; // Default to cached target

	if (bUseFixedSplitTarget)
	{
		TargetLocation = FixedSplitTargetLocation;
	}
	else if (TargetActor.IsValid())
	{
		TargetLocation = TargetActor->GetActorLocation();
	}

	FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector(); // Fallback to current forward if no clear direction
	}

	const FVector Step = Direction * Speed * DeltaSeconds;

	FHitResult Hit;
	AddActorWorldOffset(Step, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit.GetActor());
		return;
	}
	SetActorRotation(Direction.Rotation());
	
	// 检查是否命中目标
	if (TargetActor.IsValid())
	{
		const float DistanceSq = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation());
		if (DistanceSq <= FMath::Square(500.f)) // 500厘米命中距离
		{
			HandleImpact(TargetActor.Get());
		}
	}
}

void AMockMissileActor::SetFixedSplitTarget(const FVector& Location)
{
	bUseFixedSplitTarget = true;
	FixedSplitTargetLocation = Location;
	// 不清除TargetActor，保留引用以便UpdateBallisticStraightLine可以计算方向
}

bool AMockMissileActor::CheckPathForJammers(const FVector& Start, const FVector& End, ARadarJammerActor*& OutBlockingJammer) const
{
	OutBlockingJammer = nullptr;
	
	if (UWorld* World = const_cast<AMockMissileActor*>(this)->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
			{
				TArray<ARadarJammerActor*> Jammers;
				Subsystem->GetActiveRadarJammers(Jammers);
				
				UE_LOG(LogTemp, Log, TEXT("[Missile %s] CheckPathForJammers: 起点=%s, 终点=%s, 干扰器数量=%d"), 
					*GetName(), *Start.ToString(), *End.ToString(), Jammers.Num());
				
				const FVector PathDirection = (End - Start).GetSafeNormal();
				const float PathLength = FVector::Dist(Start, End);
				
				// 沿着路径采样多个点，检查是否与干扰区域相交
				// 增加采样密度，确保能提前检测到干扰区域
				const int32 NumSamples = FMath::Max(10, FMath::CeilToInt(PathLength / 1000.f)); // 每10米采样一次，更密集
				
				for (ARadarJammerActor* Jammer : Jammers)
				{
					if (!Jammer)
					{
						continue;
					}
					
					const FVector JammerCenter = Jammer->GetActorLocation();
					const float JammerRadius = Jammer->GetBaseRadius();
					
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 检查干扰器 %s: 中心=%s, 半径=%.2f"), 
						*GetName(), *Jammer->GetName(), *JammerCenter.ToString(), JammerRadius);
					
					// 首先检查起点和终点是否在干扰区域内（快速检查）
					bool bStartInRange = Jammer->IsPointInJammerRange(Start);
					bool bEndInRange = Jammer->IsPointInJammerRange(End);
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 起点在干扰区域内: %d, 终点在干扰区域内: %d"), 
						*GetName(), bStartInRange ? 1 : 0, bEndInRange ? 1 : 0);
					
					if (bStartInRange || bEndInRange)
					{
						OutBlockingJammer = Jammer;
						UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 路径起点或终点在干扰区域内，需要绕过！"), *GetName());
						return true;
					}
					
					// 计算从干扰器中心到路径的最近点（更精确的检测）
					FVector ToJammer = JammerCenter - Start;
					float ProjectionLength = FVector::DotProduct(ToJammer, PathDirection);
					// 限制投影长度在路径范围内
					ProjectionLength = FMath::Clamp(ProjectionLength, 0.f, PathLength);
					FVector ClosestPointOnPath = Start + PathDirection * ProjectionLength;
					
					// 计算从干扰器中心到路径的距离
					FVector ToClosestPoint = ClosestPointOnPath - JammerCenter;
					float DistanceToPath = ToClosestPoint.Size();
					
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 路径最近点=%s, 距离干扰器中心=%.2f, 干扰器半径=%.2f"), 
						*GetName(), *ClosestPointOnPath.ToString(), DistanceToPath, JammerRadius);
					
					// 如果路径距离干扰器中心太近（在干扰区域内或接近干扰区域），需要绕过
					// 使用 BaseRadius 来判断，因为这是干扰区域的实际范围
					// 只在路径非常接近干扰区域边缘时（比如距离边缘小于500cm）才开始绕过
					const float SafetyMargin = 500.f; // 安全边距：500厘米（5米），只在非常接近边缘时才开始绕
					const float Threshold = JammerRadius + SafetyMargin;
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 距离阈值=%.2f (半径=%.2f + 安全边距=%.2f)"), 
						*GetName(), Threshold, JammerRadius, SafetyMargin);
					
					if (DistanceToPath < Threshold)
					{
						OutBlockingJammer = Jammer;
						UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 路径距离干扰区域太近 (%.2f < %.2f)，需要绕过！"), 
							*GetName(), DistanceToPath, Threshold);
						return true;
					}
					
					// 也检查路径上的采样点（作为备用检测）
					for (int32 i = 0; i <= NumSamples; ++i)
					{
						const float T = static_cast<float>(i) / static_cast<float>(NumSamples);
						const FVector SamplePoint = Start + PathDirection * (PathLength * T);
						
						// 检查采样点是否在干扰区域内
						if (Jammer->IsPointInJammerRange(SamplePoint))
						{
							OutBlockingJammer = Jammer;
							UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 采样点 %d/%d (%.2f%%) 在干扰区域内，需要绕过！"), 
								*GetName(), i, NumSamples, T * 100.f);
							return true;
						}
						
						// 也检查采样点到干扰器中心的距离，如果太近也认为需要绕过
						const float DistanceToJammer = FVector::Dist(SamplePoint, JammerCenter);
						const float SampleSafetyMargin = 500.f; // 采样点安全边距：500厘米（5米）
						if (DistanceToJammer < JammerRadius + SampleSafetyMargin) // 只在非常接近边缘时才开始绕
						{
							OutBlockingJammer = Jammer;
							UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 采样点 %d/%d 距离干扰器太近 (%.2f < %.2f)，需要绕过！"), 
								*GetName(), i, NumSamples, DistanceToJammer, JammerRadius + SampleSafetyMargin);
							return true;
						}
					}
				}
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[Missile %s] 路径未经过干扰区域"), *GetName());
	return false;
}

FVector AMockMissileActor::CalculateAvoidanceWaypoint(const FVector& Start, const FVector& Target, ARadarJammerActor* Jammer) const
{
	if (!Jammer)
	{
		return Target;
	}
	
	const FVector JammerCenter = Jammer->GetActorLocation();
	const float JammerRadius = Jammer->GetBaseRadius();
	
	// 计算从起点到目标的向量
	FVector ToTarget = (Target - Start).GetSafeNormal();
	if (ToTarget.IsNearlyZero())
	{
		return Target;
	}
	
	// 计算从干扰器中心到路径的最近点
	FVector ToJammer = JammerCenter - Start;
	float ProjectionLength = FVector::DotProduct(ToJammer, ToTarget);
	
	// 限制投影长度在合理范围内（在起点和目标之间）
	float PathLength = FVector::Dist(Start, Target);
	ProjectionLength = FMath::Clamp(ProjectionLength, 0.f, PathLength);
	
	FVector ClosestPointOnPath = Start + ToTarget * ProjectionLength;
	
	// 计算从干扰器中心到路径的距离
	FVector ToClosestPoint = ClosestPointOnPath - JammerCenter;
	float DistanceToPath = ToClosestPoint.Size();
	
	// 计算绕过方向：垂直于路径方向，远离干扰器中心
	FVector PerpendicularDirection = FVector::CrossProduct(ToTarget, FVector::UpVector).GetSafeNormal();
	if (PerpendicularDirection.IsNearlyZero())
	{
		// 如果路径是垂直的，使用水平方向
		FVector Horizontal = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
		if (Horizontal.IsNearlyZero())
		{
			Horizontal = FVector(1.f, 0.f, 0.f);
		}
		PerpendicularDirection = FVector::CrossProduct(Horizontal, FVector::UpVector).GetSafeNormal();
	}
	
	// 确定绕过方向：选择远离干扰器中心的方向
	FVector ToJammerFromClosest = (JammerCenter - ClosestPointOnPath).GetSafeNormal();
	if (ToJammerFromClosest.IsNearlyZero())
	{
		ToJammerFromClosest = (JammerCenter - Start).GetSafeNormal();
	}
	
	float DotProduct = FVector::DotProduct(PerpendicularDirection, ToJammerFromClosest);
	
	// 如果垂直方向指向干扰器，则反向（确保远离干扰器）
	if (DotProduct > 0.f)
	{
		PerpendicularDirection = -PerpendicularDirection;
	}
	
	// 计算绕过航点：在干扰器侧面，距离边缘一定安全距离
	// 绕过航点应该在路径的最近点附近，但远离干扰器中心
	// 绕过距离：半径 + 500cm，确保绕过但不会太远
	const float AvoidanceDistance = JammerRadius + 500.f; // 绕过距离：半径 + 500厘米
	FVector AvoidancePoint = ClosestPointOnPath + PerpendicularDirection * AvoidanceDistance;
	
	// 如果起点在干扰区域内，绕过航点应该在起点和干扰器之间
	if (Jammer->IsPointInJammerRange(Start))
	{
		// 从起点向外计算绕过航点
		FVector FromStartToJammer = (JammerCenter - Start).GetSafeNormal();
		if (!FromStartToJammer.IsNearlyZero())
		{
			// 绕过航点应该在起点外侧，远离干扰器
			AvoidancePoint = Start + FromStartToJammer * (-AvoidanceDistance);
		}
	}
	
	// 确保绕过点的Z坐标在合理范围内（在起点和目标之间）
	AvoidancePoint.Z = FMath::Lerp(Start.Z, Target.Z, 0.5f);
	
	UE_LOG(LogTemp, Log, TEXT("[Missile] 计算绕过航点: Start=%s, Target=%s, JammerCenter=%s, AvoidancePoint=%s, DistanceToPath=%.2f, JammerRadius=%.2f"), 
		*Start.ToString(), *Target.ToString(), *JammerCenter.ToString(), *AvoidancePoint.ToString(), DistanceToPath, JammerRadius);
	
	return AvoidancePoint;
}

void AMockMissileActor::UpdateTrajectoryOptimization()
{
	if (!bTrajectoryOptimizationEnabled)
	{
		if (bHasAvoidanceWaypoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 轨迹优化已禁用，清除绕过航点"), *GetName());
			bHasAvoidanceWaypoint = false;
		}
		return;
	}
	
	if (!TargetActor.IsValid())
	{
		if (bHasAvoidanceWaypoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 无目标，清除绕过航点"), *GetName());
			bHasAvoidanceWaypoint = false;
		}
		return;
	}
	
	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	
	// 减少日志输出频率，避免日志过多
	static float LastLogTime = 0.f;
	if (ElapsedLifetime - LastLogTime > 1.0f) // 每秒最多输出一次日志
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] UpdateTrajectoryOptimization: 当前位置=%s, 目标位置=%s, bInJammerRange=%d, bHasAvoidanceWaypoint=%d"), 
			*GetName(), *CurrentLocation.ToString(), *TargetLocation.ToString(), bInJammerRange ? 1 : 0, bHasAvoidanceWaypoint ? 1 : 0);
		LastLogTime = ElapsedLifetime;
	}
	
	// 如果导弹已经误入干扰区域，停止绕飞，清除绕过航点，失去目标
	if (bInJammerRange)
	{
		UE_LOG(LogTemp, Error, TEXT("[Missile %s] 已误入干扰区域！停止绕飞，失去目标"), *GetName());
		
		// 清除绕过航点
		bHasAvoidanceWaypoint = false;
		AvoidanceWaypoint = FVector::ZeroVector;
		
		// 失去目标
		if (TargetActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 在干扰区域内，失去目标: %s"), 
				*GetName(), *TargetActor->GetName());
			TargetActor = nullptr;
		}
		
		return;
	}
	
	// 如果已经有绕过航点，检查是否仍然需要
	if (bHasAvoidanceWaypoint)
	{
		// 检查当前路径是否仍然经过干扰区域
		ARadarJammerActor* BlockingJammer = nullptr;
		bool bPathBlocked = CheckPathForJammers(CurrentLocation, TargetLocation, BlockingJammer);
		
		if (bPathBlocked)
		{
			// 仍然需要绕过，更新绕过航点（但只在位置变化较大时才更新，避免频繁摆动）
			if (BlockingJammer)
			{
				FVector NewAvoidanceWaypoint = CalculateAvoidanceWaypoint(CurrentLocation, TargetLocation, BlockingJammer);
				// 如果新航点与旧航点距离较远（超过1000cm），才更新，避免频繁微调导致摆动
				float DistanceToOldWaypoint = FVector::Dist(NewAvoidanceWaypoint, AvoidanceWaypoint);
				if (DistanceToOldWaypoint > 1000.f)
				{
					AvoidanceWaypoint = NewAvoidanceWaypoint;
					UE_LOG(LogTemp, Log, TEXT("[Missile %s] 更新绕过航点: %s (距离旧航点: %.2f)"), 
						*GetName(), *AvoidanceWaypoint.ToString(), DistanceToOldWaypoint);
				}
			}
		}
		else
		{
			// 路径已经不再经过干扰区域，清除绕过航点
			bHasAvoidanceWaypoint = false;
			UE_LOG(LogTemp, Log, TEXT("[Missile %s] 路径已避开干扰区域，清除绕过航点"), *GetName());
		}
	}
	else
	{
		// 检查路径是否经过干扰区域（提前检测，在进入干扰区域之前）
		ARadarJammerActor* BlockingJammer = nullptr;
		bool bPathBlocked = CheckPathForJammers(CurrentLocation, TargetLocation, BlockingJammer);
		
		if (bPathBlocked)
		{
			// 路径经过干扰区域，提前计算绕过航点（在进入干扰区域之前就开始绕过）
			if (BlockingJammer)
			{
				AvoidanceWaypoint = CalculateAvoidanceWaypoint(CurrentLocation, TargetLocation, BlockingJammer);
				bHasAvoidanceWaypoint = true;
				UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 提前检测到路径将经过干扰区域，计算绕过航点避免进入: %s"), 
					*GetName(), *AvoidanceWaypoint.ToString());
			}
		}
	}
}

bool AMockMissileActor::GetNearestJammerInfo(ARadarJammerActor*& OutJammer, float& OutDistance, float& OutBaseRadius, float& OutHeightDifference) const
{
	OutJammer = nullptr;
	OutDistance = 0.f;
	OutBaseRadius = 0.f;
	OutHeightDifference = 0.f;

	if (UWorld* World = const_cast<AMockMissileActor*>(this)->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
			{
				TArray<ARadarJammerActor*> Jammers;
				Subsystem->GetActiveRadarJammers(Jammers);

				const FVector MissileLocation = GetActorLocation();
				float BestDistance = TNumericLimits<float>::Max();

				for (ARadarJammerActor* Jammer : Jammers)
				{
					if (!Jammer)
					{
						continue;
					}

					const float Distance = FVector::Dist(MissileLocation, Jammer->GetActorLocation());
					if (Distance < BestDistance)
					{
						BestDistance = Distance;
						OutJammer = Jammer;
						OutDistance = Distance;
						OutBaseRadius = Jammer->GetBaseRadius();
						OutHeightDifference = MissileLocation.Z - Jammer->GetActorLocation().Z;
					}
				}

				return OutJammer != nullptr;
			}
		}
	}

	return false;
}

void AMockMissileActor::ConfigureInterceptorRole(AMockMissileActor* TargetMissile, float InterceptorSpeed, float InMaxLifetime)
{
	bIsInterceptor = true;
	InterceptorTargetMissile = TargetMissile;
	TargetActor = TargetMissile;

	Speed = InterceptorSpeed;
	AscentSpeed = InterceptorSpeed;
	MaxLifetime = InMaxLifetime;
	bAscending = false;
	AscentHeight = 0.f;
	bCountermeasureEnabled = false;
	bTrajectoryOptimizationEnabled = false;
	bHasAvoidanceWaypoint = false;
	bHLAllocationEnabled = false;
	bUseFixedSplitTarget = false;
	bEvasiveSubsystemEnabled = false;
	bPerformingEvasiveManeuver = false;
	EvasiveTimeRemaining = 0.f;
	CurrentEvasiveDirection = FVector::ZeroVector;

	UE_LOG(LogTemp, Log, TEXT("[Missile %s] 配置为拦截导弹，目标=%s，速度=%.1f"), 
		*GetName(),
		TargetMissile ? *TargetMissile->GetName() : TEXT("未知"),
		Speed);
}

void AMockMissileActor::UpdateInterceptorBehavior(float DeltaSeconds)
{
	if (bHasImpacted)
	{
		return;
	}

	AActor* Target = TargetActor.Get();
	if (!Target || Target->IsPendingKillPending())
	{
		UE_LOG(LogTemp, Log, TEXT("[Missile %s] 拦截目标失效，拦截导弹自毁"), *GetName());
		TriggerImpact(nullptr);
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();
	FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	const float StepSize = Speed * DeltaSeconds * 0.95f;
	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = Direction.Rotation();
	const float MaxTurnRate = 90.f; // degrees per second
	const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaSeconds, MaxTurnRate);
	const FVector MoveDirection = NewRotation.Vector();
	FHitResult Hit;
	AddActorWorldOffset(MoveDirection * StepSize, true, &Hit);
	SetActorRotation(NewRotation);

	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit.GetActor());
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetLocation);
	if (DistanceToTarget <= 400.f)
	{
		TriggerImpact(Target);
	}
}

void AMockMissileActor::HandleInterceptedByEnemy(AMockMissileActor* Interceptor)
{
	if (bHasImpacted)
	{
		return;
	}

	bHasImpacted = true;

	UE_LOG(LogTemp, Warning, TEXT("[Missile %s] 被拦截导弹 %s 击毁，任务失败"), 
		*GetName(),
		Interceptor ? *Interceptor->GetName() : TEXT("未知"));

	ClearJammerCountermeasure();
	LogCountermeasureStats();

	if (bTrailActive)
	{
		if (UWorld* World = GetWorld())
		{
			const FColor TrailColor = bIsInterceptor ? FColor(80, 160, 255) : FColor::Red;
			DrawDebugLine(World, LastTrailLocation, GetActorLocation(), TrailColor, true, TrailLifetime, 0, TrailThickness);
		}
		bTrailActive = false;
	}

	OnImpact.Broadcast(this, Interceptor);
	if (!bExpiredNotified)
	{
		bExpiredNotified = true;
		OnExpired.Broadcast(this);
	}

	if (!IsPendingKillPending())
	{
		Destroy();
	}
}

void AMockMissileActor::UpdateInterceptorAwareness(float DeltaSeconds)
{
	EvasionCooldown = FMath::Max(0.f, EvasionCooldown - DeltaSeconds);
	if (EvasiveSpeedBoostTime > 0.f)
	{
		EvasiveSpeedBoostTime = FMath::Max(0.f, EvasiveSpeedBoostTime - DeltaSeconds);
	}

	if (bPerformingEvasiveManeuver)
	{
		EvasiveTimeRemaining -= DeltaSeconds;
		if (EvasiveTimeRemaining <= 0.f)
		{
			StopEvasiveManeuver();
		}
		else if (!CurrentEvasiveDirection.IsNearlyZero())
		{
			EvasiveDirectionFlipTimer -= DeltaSeconds;
			if (EvasiveDirectionFlipTimer <= 0.f)
			{
				CurrentEvasiveDirection = -CurrentEvasiveDirection;
				EvasiveDirectionFlipTimer = EvasiveDirectionFlipInterval;
				UE_LOG(LogTemp, Log, TEXT("[Missile %s] 躲避动作方向反转 -> %s"),
					*GetName(), *CurrentEvasiveDirection.ToString());
			}
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UScenarioMenuSubsystem* Subsystem = GameInstance->GetSubsystem<UScenarioMenuSubsystem>())
		{
			TArray<AMockMissileActor*> Interceptors;
			Subsystem->GetActiveInterceptorMissiles(Interceptors);

			AMockMissileActor* ClosestThreat = nullptr;
			float ClosestDistance = TNumericLimits<float>::Max();

			for (AMockMissileActor* Candidate : Interceptors)
			{
				if (!Candidate || Candidate == this)
				{
					continue;
				}

				if (!Candidate->IsInterceptor() || Candidate->GetInterceptorTarget() != this)
				{
					continue;
				}

				const float Distance = FVector::Dist(Candidate->GetActorLocation(), GetActorLocation());
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestThreat = Candidate;
				}
			}

			const float TriggerDistance = 15000.f;
			const float ReleaseDistance = 22000.f;

			if (ClosestThreat && ClosestDistance <= TriggerDistance && EvasionCooldown <= 0.f)
			{
				const FVector ThreatDirection = (ClosestThreat->GetActorLocation() - GetActorLocation()).GetSafeNormal();
				StartEvasiveManeuver(ThreatDirection);
			}
			else if ((!ClosestThreat || ClosestDistance > ReleaseDistance) && bPerformingEvasiveManeuver)
			{
				StopEvasiveManeuver();
			}
		}
	}
}

void AMockMissileActor::StartEvasiveManeuver(const FVector& ThreatDirection)
{
	FVector Perpendicular = FVector::CrossProduct(ThreatDirection, FVector::UpVector);
	if (Perpendicular.IsNearlyZero())
	{
		Perpendicular = FVector::CrossProduct(ThreatDirection, FVector::RightVector);
	}

	if (Perpendicular.IsNearlyZero())
	{
		return;
	}

	const float DirectionSign = FMath::RandBool() ? 1.f : -1.f;
	const bool bThreatFromAbove = ThreatDirection.Z < -0.2f;
	const bool bCurrentlyDiving = Velocity.Z < -200.f;
	FVector CombinedDirection = Perpendicular.GetSafeNormal() * DirectionSign;

	if (bThreatFromAbove || bCurrentlyDiving)
	{
		const float VerticalBias = bCurrentlyDiving ? -0.9f : -0.6f;
		CombinedDirection += FVector(0.f, 0.f, VerticalBias);
	}

	if (CombinedDirection.IsNearlyZero())
	{
		CombinedDirection = Perpendicular.GetSafeNormal() * DirectionSign;
	}

	CurrentEvasiveDirection = CombinedDirection.GetSafeNormal();
	bPerformingEvasiveManeuver = true;
	EvasiveTimeRemaining = 1.8f;
	EvasionCooldown = 2.5f;
	EvasiveDirectionFlipTimer = EvasiveDirectionFlipInterval;
	EvasiveSpeedBoostTime = 1.2f;

	UE_LOG(LogTemp, Log, TEXT("[Missile %s] 触发躲避动作，方向=%s"), 
		*GetName(),
		*CurrentEvasiveDirection.ToString());
}

void AMockMissileActor::StopEvasiveManeuver()
{
	if (!bPerformingEvasiveManeuver)
	{
		return;
	}

	bPerformingEvasiveManeuver = false;
	EvasiveTimeRemaining = 0.f;
	CurrentEvasiveDirection = FVector::ZeroVector;
	EvasiveDirectionFlipTimer = 0.f;
	EvasiveSpeedBoostTime = 0.f;

	UE_LOG(LogTemp, Log, TEXT("[Missile %s] 结束躲避动作"), *GetName());
}

int32 AMockMissileActor::GetSplitGroupId() const
{
	return SplitGroupId;
}

bool AMockMissileActor::IsSplitChild() const
{
	return bIsSplitChild;
}

void AMockMissileActor::GetCountermeasureStats(FMissileCountermeasureStats& OutStats) const
{
	OutStats.bCountermeasureEnabled = bCountermeasureEnabled;
	OutStats.bDetectionLogged = bJammerDetectionLogged;
	OutStats.bCountermeasureActivated = bCountermeasureActive;
	OutStats.DetectionTime = JammerDetectionTime;
	OutStats.DetectionDistanceToJammer = JammerDetectionDistance;
	OutStats.DetectionBaseRadius = JammerDetectionBaseRadius;
	OutStats.DetectionHeightDifference = JammerDetectionHeightDifference;
	OutStats.CountermeasureActivationTime = CountermeasureActivationTime;
	OutStats.CountermeasureActivationDistanceToJammer = CountermeasureActivationDistanceToJammer;
	OutStats.CountermeasureActivationBaseRadius = CountermeasureActivationBaseRadius;
	OutStats.CountermeasureActivationHeightDifference = CountermeasureActivationHeightDifference;
	OutStats.bEnteredJammerRange = bInJammerRange || (CountermeasureActivationTime >= 0.f);
	
	// 计算反制激活时的半径缩减率（如果已激活）
	if (bCountermeasureActive && CountermeasureActivationTime >= 0.f && CountermeasureActivationBaseRadius > KINDA_SMALL_NUMBER)
	{
		const float BaseRadius = CountermeasureActivationBaseRadius;
		const float DistanceToJammer = CountermeasureActivationDistanceToJammer;
		const float ActivationDistance = BaseRadius * 1.5f; // CountermeasureActivationDistanceMultiplier
		const float ActivationThreshold = BaseRadius + ActivationDistance;
		
		if (DistanceToJammer >= 0.f && DistanceToJammer < ActivationThreshold)
		{
			// 计算归一化距离
			float NormalizedDistance = (ActivationThreshold - DistanceToJammer) / ActivationDistance;
			NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.f, 1.f);
			
			// 使用立方衰减公式计算缩减后的半径
			float NormalizedDistanceCubed = NormalizedDistance * NormalizedDistance * NormalizedDistance;
			float ReductionFactor = FMath::Lerp(1.f, 0.05f, NormalizedDistanceCubed);
			float CurrentRadius = BaseRadius * ReductionFactor;
			
			// 计算缩减百分比
			OutStats.CountermeasureActivationRadiusReductionPercent = (1.f - (CurrentRadius / BaseRadius)) * 100.f;
		}
	}
}

