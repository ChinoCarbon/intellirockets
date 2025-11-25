#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MockMissileActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class USphereComponent;
class ARadarJammerActor;

/**
 * 简易导弹占位 Actor：使用静态网格表示，并通过 Tick 追踪目标。
 * 命中或超时都会自动销毁，并发出事件供外部更新状态。
 */
UCLASS()
class AMockMissileActor : public AActor
{
	GENERATED_BODY()

public:
		DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMissileImpactSignature, AMockMissileActor* /*Missile*/, AActor* /*HitTarget*/);
		DECLARE_MULTICAST_DELEGATE_OneParam(FOnMissileExpiredSignature, AMockMissileActor* /*Missile*/);

	AMockMissileActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 初始化导弹逻辑参数与目标（目标可以为nullptr，导弹会在视野内自动搜索） */
	void InitializeMissile(AActor* InTarget, float InLaunchSpeed, float InMaxLifetime);
	
	/** 设置导弹的算法配置，用于决定是否启用反制/分裂等功能 */
	void SetAlgorithmConfig(const TArray<FString>& AlgorithmNames, const TArray<FString>& PrototypeNames);

	/** 设置当前分裂层级（限制再次分裂） */
	void SetSplitGeneration(int32 Generation);

	/** 设置电磁干扰场景标志，调整飞行高度 */
	void SetInterferenceMode(bool bActive);

	int32 GetSplitGroupId() const;
	bool IsSplitChild() const;
	bool IsCountermeasureEnabled() const { return bCountermeasureEnabled; }
	
	/** 获取反制统计数据（供ScenarioMenuSubsystem使用） */
	void GetCountermeasureStats(struct FMissileCountermeasureStats& OutStats) const;

	/** 设置外观（静态网格 + 基础材质 + 颜色） */
	void SetupAppearance(UStaticMesh* InMesh, UMaterialInterface* InBaseMaterial, const FLinearColor& TintColor);

	/** 手动触发命中，用于外部判定（例如碰撞回调） */
	void TriggerImpact(AActor* OtherActor);

	/** 启用或禁用躲避对抗分系统效果（根据分系统配置） */
	void SetEvasionSubsystemEnabled(bool bEnabled);

	/** 将当前导弹配置为拦截导弹，指定目标导弹与速度 */
	void ConfigureInterceptorRole(AMockMissileActor* TargetMissile, float InterceptorSpeed, float InMaxLifetime);

	/** 返回是否为拦截导弹 */
	bool IsInterceptor() const { return bIsInterceptor; }

	/** 获取拦截导弹当前锁定的目标导弹（仅拦截导弹有效） */
	AMockMissileActor* GetInterceptorTarget() const { return InterceptorTargetMissile.Get(); }

	/** 被拦截导弹命中时调用 */
	void HandleInterceptedByEnemy(AMockMissileActor* Interceptor);

	/** 命中事件：会在命中目标或接近距离内触发 */
	FOnMissileImpactSignature OnImpact;

	/** 超时 / 提前销毁事件 */
	FOnMissileExpiredSignature OnExpired;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateBallistic(float DeltaSeconds);
	void HandleLifetime(float DeltaSeconds);
	void HandleImpact(AActor* HitActor);
	void UpdateTrail();

private:
	UPROPERTY()
	USceneComponent* RootSceneComponent;
	UPROPERTY()
	USphereComponent* CollisionComponent;

	UPROPERTY()
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	UPointLightComponent* LightComponent;

	TWeakObjectPtr<AActor> TargetActor;

	float Speed = 3000.f;
	float AscentSpeed = 2000.f;
	float AscentHeight = 2000.f;
	float MaxLifetime = 30.f;
	float ElapsedLifetime = 0.f;

	bool bHasImpacted = false;
		bool bExpiredNotified = false;
	bool bAscending = true;
	
	// 抛物线轨迹参数
	float Gravity = 980.f; // 重力加速度 (cm/s²)
	FVector InitialVelocity; // 初始速度向量
	FVector LaunchLocation; // 发射位置（开始抛物线时的位置）
	float TrajectoryStartTime = 0.f; // 开始抛物线轨迹的时间

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial = nullptr;

	FVector Velocity = FVector::ZeroVector;
	FVector StartLocation = FVector::ZeroVector;
	FVector CachedTargetLocation = FVector::ZeroVector;
	FVector LastTrailLocation = FVector::ZeroVector;
	bool bTrailActive = false;

	float TrailPointSpacing = 120.f;
	float TrailLifetime = 20.f;
	float TrailThickness = 8.f;
	float MinAscentHeight = 2400.f;
	float MaxAscentHeight = 5200.f;
	float MinLaunchSpeed = 1500.f;
	float MaxLaunchSpeed = 6000.f;

	void UpdateAscent(float DeltaSeconds);
	void BeginHoming();
	void UpdateHoming(float DeltaSeconds);
	void UpdateBallisticStraightLine(float DeltaSeconds);
	void SetFixedSplitTarget(const FVector& Location);
	void SetSplitGroupId(int32 InGroupId) { SplitGroupId = InGroupId; }
	
	// 目标搜索相关
	bool IsTargetInView(AActor* Candidate) const;
	AActor* FindTargetInView() const;
	void SearchAndLockTarget();
	
	// 视野参数
	float ViewDistance = 50000.f; // 视野距离（厘米）
	float ViewAngle = 120.f; // 视野角度（度）- 增加到120度以覆盖左右两侧目标
	float TargetSearchInterval = 0.1f; // 目标搜索间隔（秒）
	float LastTargetSearchTime = 0.f; // 上次搜索目标的时间
	
	// 雷达干扰相关
	bool bCountermeasureEnabled = false; // 是否启用了反制功能（根据算法配置决定）
	bool bInJammerRange = false; // 是否在干扰区域内
	bool bCountermeasureActive = false; // 是否启用反制
	float CountermeasureActivationTime = -1.f; // 反制激活时间
	float LatestCountermeasureTime = -1.f; // 最晚需要反制的时间
	float LatestCountermeasureDistance = -1.f; // 最晚需要反制时与目标的距离
	bool bShouldUseCountermeasure = false; // 是否需要使用反制
	bool bElectromagneticInterferenceActive = false; // 是否处于电磁干扰测试环境
	bool bJammerDetectionLogged = false;
	float JammerDetectionTime = -1.f;
	float JammerDetectionDistance = -1.f;
	float JammerDetectionBaseRadius = 0.f;
	float JammerDetectionHeightDifference = 0.f;
	float CountermeasureActivationDistanceToJammer = -1.f;
	float CountermeasureActivationBaseRadius = 0.f;
	float CountermeasureActivationHeightDifference = 0.f;
	mutable bool bCountermeasureStatsSubmitted = false;

	// HL 分配算法相关
	bool bHLAllocationEnabled = false; // 是否启用 HL 分配算法效果
	bool bHasSplit = false; // 是否已经执行过分裂
	int32 SplitGeneration = 0; // 当前分裂层级
	static constexpr int32 MaxSplitGeneration = 1; // 允许分裂的最大层级
	bool bUseFixedSplitTarget = false;
	FVector FixedSplitTargetLocation = FVector::ZeroVector;
	int32 SplitGroupId = INDEX_NONE;
	bool bIsSplitChild = false;
	
	// 轨迹优化算法相关
	bool bTrajectoryOptimizationEnabled = false; // 是否启用轨迹优化算法（绕过干扰区域）
	FVector AvoidanceWaypoint = FVector::ZeroVector; // 绕过路径的航点
	bool bHasAvoidanceWaypoint = false; // 是否有有效的绕过航点
	float TrajectoryOptimizationUpdateInterval = 0.1f; // 轨迹优化更新间隔（秒），避免每帧都更新
	float LastTrajectoryOptimizationUpdate = 0.f; // 上次轨迹优化更新时间
	
	// 干扰检测和反制
	bool IsInJammerRange() const;
	void UpdateJammerDetection();
	void ActivateCountermeasure();
	void ClearJammerCountermeasure(); // 清除干扰区域的反制状态
	void LogCountermeasureStats() const;
	void SubmitCountermeasureStats() const;
	void TrySplitForClusterTargets(AActor* HitActor);
	void AddCollisionIgnoreActor(AActor* ActorToIgnore);
	
	// 轨迹优化：检测并绕过干扰区域
	bool CheckPathForJammers(const FVector& Start, const FVector& End, ARadarJammerActor*& OutBlockingJammer) const;
	FVector CalculateAvoidanceWaypoint(const FVector& Start, const FVector& Target, ARadarJammerActor* Jammer) const;
	void UpdateTrajectoryOptimization();
	bool GetNearestJammerInfo(ARadarJammerActor*& OutJammer, float& OutDistance, float& OutBaseRadius, float& OutHeightDifference) const;

	// 拦截导弹与躲避对抗
	void UpdateInterceptorBehavior(float DeltaSeconds);
	void UpdateInterceptorAwareness(float DeltaSeconds);
	void StartEvasiveManeuver(const FVector& ThreatDirection);
	void StopEvasiveManeuver();

	bool bEvasiveSubsystemEnabled = false;
	bool bPerformingEvasiveManeuver = false;
	float EvasiveTimeRemaining = 0.f;
	float EvasionCooldown = 0.f;
	FVector CurrentEvasiveDirection = FVector::ZeroVector;
	float EvasiveDirectionFlipTimer = 0.f;
	float EvasiveDirectionFlipInterval = 0.35f;
	float EvasiveSpeedBoostTime = 0.f;
	float EvasiveSpeedBoostMultiplier = 1.45f;

	bool bIsInterceptor = false;
	TWeakObjectPtr<AMockMissileActor> InterceptorTargetMissile;
};


