#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadarJammerActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * 雷达干扰区域Actor：在蓝方目标附近生成半球形干扰区域
 */
UCLASS()
class ARadarJammerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ARadarJammerActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** 设置干扰半径（厘米） */
	void SetJammerRadius(float InRadius);

	/** 获取当前干扰半径 */
	float GetJammerRadius() const { return JammerRadius; }
	
	/** 获取基础干扰半径 */
	float GetBaseRadius() const { return BaseRadius; }

	/** 检查点是否在干扰区域内 */
	bool IsPointInJammerRange(const FVector& Point) const;

	/** 设置是否启用反制（反制后干扰半径会减小） */
	void SetCountermeasureActive(bool bActive);
	
	/** 清除所有反制状态，恢复原始半径 */
	void ClearCountermeasure();

	/** 获取是否启用反制 */
	bool IsCountermeasureActive() const { return bCountermeasureActive; }

	/** 设置反制强度（0-1，影响干扰半径的减小程度） */
	void SetCountermeasureStrength(float Strength) { CountermeasureStrength = FMath::Clamp(Strength, 0.f, 1.f); }

	/** 根据导弹距离更新干扰半径（直接基于距离计算，不使用插值） */
	void UpdateRadiusByMissileDistance(float MissileDistance);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* MeshComponent;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	float JammerRadius = 5000.f; // 默认干扰半径（厘米）
	float BaseRadius = 5000.f; // 基础干扰半径（厘米）
	bool bCountermeasureActive = false;
	float CountermeasureStrength = 0.f; // 反制强度（0-1）
	
	// 反制激活距离：当导弹距离半球表面这个距离时开始缩小
	// 设置为基础半径的倍数，确保在区域外就能看到明显缩小
	static constexpr float CountermeasureActivationDistanceMultiplier = 1.5f; // 激活距离 = BaseRadius * 1.5
	
	float VisualizedRadius = 5000.f; // 当前可视化的半径（用于平滑插值）
	float TargetVisualRadius = 5000.f; // 目标可视化半径
	float LastGeneratedRadius = 0.f; // 上一次生成网格时的半径

	void UpdateVisualization();
	void UpdateVisualizationSmooth(float DeltaTime);
	void GenerateSphereMesh(float Radius);
	void CreateDefaultTranslucentMaterial();
};

