#pragma once

#include "CoreMinimal.h"

struct FMissileTestRecord
{
	double LaunchTimeSeconds = 0.0;
	bool bAutoFire = false;
	TWeakObjectPtr<AActor> TargetActor;
	FString TargetName;
	FVector LaunchLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	float InitialDistance = 0.f;

	double ImpactTimeSeconds = 0.0;
	bool bImpactRegistered = false;
	bool bDirectHit = false;
	int32 DestroyedCount = 0;
	bool bExpired = false;
	bool bTargetDestroyed = false;

	float GetFlightDuration() const
	{
		return (bImpactRegistered || bExpired) ? static_cast<float>(ImpactTimeSeconds - LaunchTimeSeconds) : 0.f;
	}
};

struct FMissileTestSummary
{
	int32 TotalShots = 0;
	int32 ManualShots = 0;
	int32 AutoShots = 0;
	int32 Hits = 0;
	int32 DirectHits = 0;
	int32 AoEHits = 0;
	int32 Misses = 0;

	float HitRate = 0.f;
	float DirectHitRate = 0.f;
	float AverageFlightTime = 0.f;
	float AverageLaunchDistance = 0.f;
	float AverageDestroyedPerHit = 0.f;
	float AverageAutoLaunchInterval = 0.f;
	float SessionDuration = 0.f;
};

struct FIndicatorEvaluationResult
{
	FString IndicatorId;
	FString DisplayName;
	FString StatusText;
	FString ValueText;
	FString TargetText;
	FString RemarkText;
	FLinearColor StatusColor = FLinearColor::Gray;
	bool bHasData = false;
	bool bPass = false;
};

// 轻量级：感知算法-运行期统计（用于 9.1.1 指标计算）
struct FPerceptionRuntimeStats
{
	// 目标检测
	int32 NumDetectionsCorrect = 0;
	int32 NumFalsePositives = 0;
	int32 NumFalseNegatives = 0;

	// 识别速度（累计识别时长，计数）
	double TotalRecognitionTimeSeconds = 0.0;
	int32 NumRecognitionSamples = 0;

	// 环境适应性（不同条件下的识别率样本）
	int32 SamplesLightTotal = 0;
	int32 SamplesLightCorrect = 0;
	int32 SamplesWeatherTotal = 0;
	int32 SamplesWeatherCorrect = 0;

	// 多目标跟踪
	int32 MaxSimultaneousTracksObserved = 0;
	double TotalTrackingErrorMeters = 0.0;
	int32 NumTrackingErrorSamples = 0;

	// 抗干扰精度
	int32 SamplesJamLightTotal = 0;
	int32 SamplesJamLightCorrect = 0;
	int32 SamplesJamMediumTotal = 0;
	int32 SamplesJamMediumCorrect = 0;

	// 频谱监控
	int32 SamplesInterferenceDetectedTotal = 0;
	int32 SamplesInterferenceDetectedCorrect = 0;
	int32 SamplesFrequencyAdjustTotal = 0;
	int32 SamplesFrequencyAdjustSuccess = 0;

	// 信号恢复
	double TotalRecoveryTimeSeconds = 0.0;
	int32 NumRecoverySamples = 0;

	// 关键部位识别
	int32 SamplesKeypartTotal = 0;
	int32 SamplesKeypartCorrect = 0;

	// 热源跟踪
	double TotalHeatTrackErrorMeters = 0.0;
	int32 NumHeatTrackSamples = 0;
};
