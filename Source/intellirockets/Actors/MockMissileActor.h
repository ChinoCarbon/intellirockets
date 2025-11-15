#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MockMissileActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class USphereComponent;

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

	/** 初始化导弹逻辑参数与目标 */
	void InitializeMissile(AActor* InTarget, float InLaunchSpeed, float InMaxLifetime);

	/** 设置外观（静态网格 + 基础材质 + 颜色） */
	void SetupAppearance(UStaticMesh* InMesh, UMaterialInterface* InBaseMaterial, const FLinearColor& TintColor);

	/** 手动触发命中，用于外部判定（例如碰撞回调） */
	void TriggerImpact(AActor* OtherActor);

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
};


