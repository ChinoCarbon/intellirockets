#pragma once

#include "CoreMinimal.h"

struct FMissileCountermeasureStats
{
	bool bCountermeasureEnabled = false;
	bool bDetectionLogged = false;
	bool bCountermeasureTriggered = false;
	bool bCountermeasureActivated = false;
	float DetectionTime = -1.f;
	float DetectionDistanceToJammer = -1.f;
	float DetectionBaseRadius = 0.f;
	float DetectionHeightDifference = 0.f;
	float CountermeasureTriggerTime = -1.f;
	float CountermeasureTargetDistance = -1.f;
	float CountermeasureActivationTime = -1.f;
	float CountermeasureActivationDistanceToJammer = -1.f;
	float CountermeasureActivationBaseRadius = 0.f;
	float CountermeasureActivationHeightDifference = 0.f;
	float CountermeasureActivationRadiusReductionPercent = 0.f; // 反制激活时的半径缩减百分比
	float CountermeasureDuration = 0.f; // 干扰持续时间（从激活到离开/命中）
	bool bLostTargetInJammerRange = false; // 是否在干扰区域内失去目标
	bool bEnteredJammerRange = false; // 是否进入过干扰区域
	float JammerRangeExitTime = -1.f; // 离开干扰区域的时间
};

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
	bool bIsSplitChild = false;
	int32 SplitGroupId = INDEX_NONE;
	FMissileCountermeasureStats CountermeasureStats;

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

	int32 HLSplitAttemptCount = 0;
	int32 HLSplitSuccessCount = 0;
	int32 HLSplitChildShotCount = 0;
	int32 HLSplitChildHitCount = 0;
	int32 HLSplitGroupCount = 0;
	float HLSplitGroupUniqueTargetsTotal = 0.f;
	int32 CountermeasureDetectionSamples = 0;
	float CountermeasureAverageDetectionMarginPercent = 0.f;
	int32 CountermeasureActivationSamples = 0;
	float CountermeasureAverageActivationDelay = 0.f;
	float CountermeasureOnTimeRate = 0.f;
	float CountermeasureAverageActivationDistance = 0.f;
	float CountermeasureAverageActivationHeightDiff = 0.f;
	float CountermeasureSuppressionSuccessRate = 0.f; // 干扰抑制成功率
	float CountermeasureAverageRadiusReductionRate = 0.f; // 平均反制半径缩减率
	float CountermeasureCoverageSuccessRate = 0.f; // 干扰覆盖成功率
	float CountermeasureAverageDuration = 0.f; // 平均干扰持续时间
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
