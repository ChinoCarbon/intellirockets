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

