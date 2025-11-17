#include "Systems/ScenarioMenuSubsystem.h"
#include "UI/SScenarioScreen.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpectatorPawn.h"
#include "Math/RotationMatrix.h"
#include "UObject/SoftObjectPath.h"
#include "UI/Widgets/SBlueUnitMonitor.h"
#include "Engine/LevelBounds.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "Engine/LevelStreaming.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "TimerManager.h"
#include "Actors/MockMissileActor.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widgets/MissileOverlayWidget.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"

void UScenarioMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("UScenarioMenuSubsystem::Initialize"));
	WorldHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UScenarioMenuSubsystem::OnWorldReady);
}

void UScenarioMenuSubsystem::Deinitialize()
{
	if (UWorld* PendingWorld = PendingScenarioWorld.Get())
	{
		PendingWorld->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
	}
	PendingScenarioWorld = nullptr;
	RemoveInputBindings();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
		World->GetTimerManager().ClearTimer(MissileCameraTimerHandle);
	}
	MissileCameraTarget = nullptr;
	RemoveMissileOverlay();
	AutoFireRemaining = 0;

	FWorldDelegates::OnPostWorldInitialization.Remove(WorldHandle);
	if (GEngine && GEngine->GameViewport && Screen.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	}
	Screen.Reset();
	HideBlueMonitor();
	Super::Deinitialize();
}

void UScenarioMenuSubsystem::OnWorldReady(UWorld* World, const UWorld::InitializationValues IVs)
{
	UE_LOG(LogTemp, Log, TEXT("UScenarioMenuSubsystem::OnWorldReady world=%s IsGameWorld=%d"), World ? *World->GetName() : TEXT("null"), World ? World->IsGameWorld() : 0);
	if (World && World->IsGameWorld())
	{
		if (bIsRunningScenario && bHasPendingScenarioConfig)
		{
			PendingScenarioWorld = World;
			bPendingScenarioWaitingLogged = false;
			if (World->GetTimerManager().IsTimerActive(PendingScenarioTimerHandle))
			{
				World->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
			}
			World->GetTimerManager().SetTimer(PendingScenarioTimerHandle, FTimerDelegate::CreateUObject(this, &UScenarioMenuSubsystem::FinalizeScenarioAfterLoad), 0.25f, true);
			UE_LOG(LogTemp, Log, TEXT("OnWorldReady: deferring scenario finalization until streaming levels load."));
			return;
		}

		// 立即尝试显示，并在 0.1s 后再次尝试一次，避免初始化竞态
		Show(World);
		FTimerHandle Timer;
		World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateUObject(this, &UScenarioMenuSubsystem::Show, World), 0.1f, false);
	}
}

// ---------------- Perception reporting APIs ----------------
void UScenarioMenuSubsystem::Perception_ReportDetection(bool bCorrect)
{
	if (bCorrect) ++PerceptionStats.NumDetectionsCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportFalsePositive()
{
	++PerceptionStats.NumFalsePositives;
}
void UScenarioMenuSubsystem::Perception_ReportFalseNegative()
{
	++PerceptionStats.NumFalseNegatives;
}
void UScenarioMenuSubsystem::Perception_ReportRecognitionSample(double Seconds)
{
	PerceptionStats.TotalRecognitionTimeSeconds += FMath::Max(0.0, Seconds);
	++PerceptionStats.NumRecognitionSamples;
}
void UScenarioMenuSubsystem::Perception_ReportLightCondition(bool bCorrect)
{
	++PerceptionStats.SamplesLightTotal;
	if (bCorrect) ++PerceptionStats.SamplesLightCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportWeatherCondition(bool bCorrect)
{
	++PerceptionStats.SamplesWeatherTotal;
	if (bCorrect) ++PerceptionStats.SamplesWeatherCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportSimultaneousTracks(int32 CurrentTracks)
{
	PerceptionStats.MaxSimultaneousTracksObserved = FMath::Max(PerceptionStats.MaxSimultaneousTracksObserved, CurrentTracks);
}
void UScenarioMenuSubsystem::Perception_ReportTrackingError(double Meters)
{
	PerceptionStats.TotalTrackingErrorMeters += FMath::Max(0.0, Meters);
	++PerceptionStats.NumTrackingErrorSamples;
}
void UScenarioMenuSubsystem::Perception_ReportJammingLight(bool bCorrect)
{
	++PerceptionStats.SamplesJamLightTotal;
	if (bCorrect) ++PerceptionStats.SamplesJamLightCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportJammingMedium(bool bCorrect)
{
	++PerceptionStats.SamplesJamMediumTotal;
	if (bCorrect) ++PerceptionStats.SamplesJamMediumCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportInterferenceDetected(bool bCorrect)
{
	++PerceptionStats.SamplesInterferenceDetectedTotal;
	if (bCorrect) ++PerceptionStats.SamplesInterferenceDetectedCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportFrequencyAdjust(bool bSuccess)
{
	++PerceptionStats.SamplesFrequencyAdjustTotal;
	if (bSuccess) ++PerceptionStats.SamplesFrequencyAdjustSuccess;
}
void UScenarioMenuSubsystem::Perception_ReportRecoveryTime(double Seconds)
{
	PerceptionStats.TotalRecoveryTimeSeconds += FMath::Max(0.0, Seconds);
	++PerceptionStats.NumRecoverySamples;
}
void UScenarioMenuSubsystem::Perception_ReportKeypartRecognition(bool bCorrect)
{
	++PerceptionStats.SamplesKeypartTotal;
	if (bCorrect) ++PerceptionStats.SamplesKeypartCorrect;
}
void UScenarioMenuSubsystem::Perception_ReportHeatTrackingError(double Meters)
{
	PerceptionStats.TotalHeatTrackErrorMeters += FMath::Max(0.0, Meters);
	++PerceptionStats.NumHeatTrackSamples;
}
void UScenarioMenuSubsystem::Show(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogTemp, Warning, TEXT("Show(): GameViewport not ready"));
		return;
	}

	if (bIsRunningScenario)
	{
		UE_LOG(LogTemp, Log, TEXT("Show(): skipped because a scenario test is running."));
		return;
	}

	// 如果界面已存在，先移除
	if (Screen.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
		Screen.Reset();
	}
	HideBlueMonitor();

	SAssignNew(Screen, SScenarioScreen)
		.StepIndex(StepIndex)
		.InitialTabIndex(ReturnTabIndex)
		.OwnerSubsystem(this)
		.OnPrevStep(FOnPrevStep::CreateUObject(this, &UScenarioMenuSubsystem::Prev))
		.OnNextStep(FOnNextStep::CreateUObject(this, &UScenarioMenuSubsystem::Next))
		.OnSaveAll(FOnSaveAll::CreateUObject(this, &UScenarioMenuSubsystem::SaveAll))
		.OnBackToMainMenu(FOnBackToMainMenu::CreateUObject(this, &UScenarioMenuSubsystem::BackToMainMenu))
		.OnStartTest(FOnStartTest::CreateUObject(this, &UScenarioMenuSubsystem::BeginScenarioTest));

	GEngine->GameViewport->AddViewportWidgetContent(Screen.ToSharedRef(), 100);
	UE_LOG(LogTemp, Log, TEXT("Show(): Screen added to GameViewport, StepIndex=%d"), StepIndex);

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		FInputModeUIOnly Mode; Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
}

void UScenarioMenuSubsystem::Hide(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport || !Screen.IsValid()) return;
	GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
	{
		FInputModeGameOnly Mode; PC->SetInputMode(Mode); PC->bShowMouseCursor = false;
	}
	Screen.Reset();
}

void UScenarioMenuSubsystem::Prev()
{
	StepIndex = FMath::Max(0, StepIndex - 1);
	UE_LOG(LogTemp, Log, TEXT("Prev clicked, StepIndex=%d"), StepIndex);
    if (Screen.IsValid()) { Screen->SetStepIndex(StepIndex); }
}

void UScenarioMenuSubsystem::Next()
{
	StepIndex = FMath::Min(4, StepIndex + 1); // 限制在0-4之间（5个步骤）
	UE_LOG(LogTemp, Log, TEXT("Next clicked, StepIndex=%d"), StepIndex);
    if (Screen.IsValid()) { Screen->SetStepIndex(StepIndex); }
}

void UScenarioMenuSubsystem::SaveAll()
{
	UE_LOG(LogTemp, Log, TEXT("SaveAll clicked"));

	// 将界面中的 Step1 表格数据持久化到 Saved/Config 下
	if (Screen.IsValid())
	{
		Screen->SavePersistentTables();
		UE_LOG(LogTemp, Log, TEXT("SaveAll: persistent tables saved."));
	}
}

void UScenarioMenuSubsystem::BackToMainMenu()
{
	if (UWorld* World = GetWorld())
	{
		UKismetSystemLibrary::QuitGame(World, World->GetFirstPlayerController(), EQuitPreference::Quit, true);
	}
}

void UScenarioMenuSubsystem::BeginScenarioTest()
{
	if (!Screen.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginScenarioTest called but screen is invalid"));
		return;
	}

	// 记录当前 Tab，测试完成后回到对应 Tab 的 Step5
	ReturnTabIndex = Screen->GetActiveTabIndex();

	FScenarioTestConfig Config;
	Screen->CollectScenarioConfig(Config);

	UE_LOG(LogTemp, Log, TEXT("BeginScenarioTest: MapIndex=%d MapLevelName=%s TestMethodIndex=%d EnvInterfIdx=%d"),
		Config.MapIndex,
		Config.MapLevelName.IsNone() ? TEXT("None") : *Config.MapLevelName.ToString(),
		Config.TestMethodIndex,
		Config.EnvironmentInterferenceIndex);

	PendingScenarioConfig = Config;
	bHasPendingScenarioConfig = true;
	bIsRunningScenario = true;
	ResetMissileTestSession();
	ActiveScenarioConfig = Config;
	bHasActiveScenarioConfig = true;
	ClearSpawnedBlueUnits();

	if (OnScenarioTestRequested.IsBound())
	{
		OnScenarioTestRequested.Broadcast(Config);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No listeners bound to OnScenarioTestRequested; remember to start the scenario manually."));
	}

	if (UWorld* World = GetWorld())
	{
		Hide(World);
		if (!Config.MapLevelName.IsNone())
		{
			UE_LOG(LogTemp, Log, TEXT("Opening scenario map: %s"), *Config.MapLevelName.ToString());
			UGameplayStatics::OpenLevel(World, Config.MapLevelName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No map level specified for map index %d. The menu will stay on the current map. Create one at /Game/Maps/... if needed."), Config.MapIndex);
			ApplyEnvironmentSettings(World, Config);
			DeployBlueForScenario(World, Config);
			bHasPendingScenarioConfig = false;
		}
	}
}

void UScenarioMenuSubsystem::BuildIndicatorEvaluations(TArray<FIndicatorEvaluationResult>& OutResults) const
{
	OutResults.Reset();

	if (!bHasActiveScenarioConfig || ActiveScenarioConfig.SelectedIndicatorIds.Num() == 0)
	{
		return;
}

	const bool bHasData = MissileTestRecords.Num() > 0;
	const FMissileTestSummary& Summary = LastMissileSummary;

	// 预先计算自动发射相关的统计数据（避免在循环中重复计算）
	int32 AutoHits = 0;
	int32 AutoShots = 0;
	for (const FMissileTestRecord& Record : MissileTestRecords)
	{
		if (Record.bAutoFire)
		{
			++AutoShots;
			if (Record.DestroyedCount > 0)
			{
				++AutoHits;
			}
		}
	}
	const float AutoHitRate = AutoShots > 0 ? (static_cast<float>(AutoHits) * 100.f / AutoShots) : 0.f;
	const float AutoFireRatio = Summary.TotalShots > 0 ? (static_cast<float>(Summary.AutoShots) * 100.f / Summary.TotalShots) : 0.f;

	for (int32 Index = 0; Index < ActiveScenarioConfig.SelectedIndicatorIds.Num(); ++Index)
	{
		const FString& IndicatorId = ActiveScenarioConfig.SelectedIndicatorIds[Index];
		const FString DisplayName = ActiveScenarioConfig.SelectedIndicatorDetails.IsValidIndex(Index)
			? ActiveScenarioConfig.SelectedIndicatorDetails[Index]
			: IndicatorId;

		FIndicatorEvaluationResult Result;
		Result.IndicatorId = IndicatorId;
		Result.DisplayName = DisplayName;
		Result.StatusText = TEXT("未接入");
		Result.StatusColor = FLinearColor(0.4f, 0.4f, 0.4f);
		Result.RemarkText = TEXT("尚未接入该指标的数据采集管线。");
		Result.TargetText = TEXT("—");
		Result.ValueText = TEXT("—");

		if (!bHasData)
		{
			Result.RemarkText = TEXT("尚未执行导弹测试。按 P 结束一次测试后将生成数据。");
			OutResults.Add(Result);
			continue;
		}

		auto EvaluateThreshold = [&Result](float Value, float Target, bool bHigherIsBetter, const FString& Unit, const FString& Remark)
		{
			Result.ValueText = FString::Printf(TEXT("%.2f%s"), Value, *Unit);
			Result.TargetText = FString::Printf(TEXT("%s %.2f%s"),
				bHigherIsBetter ? TEXT("≥") : TEXT("≤"),
				Target, *Unit);
			const bool bPass = bHigherIsBetter ? (Value >= Target) : (Value <= Target);
			Result.bHasData = true;
			Result.bPass = bPass;
			Result.StatusText = bPass ? TEXT("达标") : TEXT("未达标");
			Result.StatusColor = bPass ? FLinearColor(0.1f, 0.8f, 0.3f) : FLinearColor(0.85f, 0.2f, 0.2f);
			Result.RemarkText = Remark;
		};

		// 9.1.1 感知算法性能指标
		if (IndicatorId.StartsWith(TEXT("9.1.1")))
		{
			const FPerceptionRuntimeStats& P = PerceptionStats;
			if (IndicatorId == TEXT("9.1.1.1"))
			{
				const int32 Den = P.NumDetectionsCorrect + P.NumFalsePositives;
				const float FalsePositiveRate = Den > 0 ? (float)P.NumFalsePositives / (float)Den * 100.f : 0.f;
				EvaluateThreshold(FalsePositiveRate, 0.5f, false, TEXT("%"), TEXT("误报率不超过0.5%"));
			}
			else if (IndicatorId == TEXT("9.1.1.1b"))
			{
				const int32 Den = P.NumDetectionsCorrect + P.NumFalseNegatives;
				const float FalseNegativeRate = Den > 0 ? (float)P.NumFalseNegatives / (float)Den * 100.f : 0.f;
				EvaluateThreshold(FalseNegativeRate, 1.0f, false, TEXT("%"), TEXT("漏报率不超过1%"));
			}
			else if (IndicatorId == TEXT("9.1.1.2"))
			{
				const float AvgRec = P.NumRecognitionSamples > 0 ? (float)(P.TotalRecognitionTimeSeconds / P.NumRecognitionSamples) : 0.f;
				EvaluateThreshold(AvgRec, 0.2f, false, TEXT(" s"), TEXT("识别时间小于0.2秒"));
			}
			else if (IndicatorId == TEXT("9.1.1.3a"))
			{
				const float Rate = P.SamplesLightTotal > 0 ? (float)P.SamplesLightCorrect / (float)P.SamplesLightTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 90.f, true, TEXT("%"), TEXT("不同光照条件识别率不低于90%"));
			}
			else if (IndicatorId == TEXT("9.1.1.3b"))
			{
				const float Rate = P.SamplesWeatherTotal > 0 ? (float)P.SamplesWeatherCorrect / (float)P.SamplesWeatherTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 85.f, true, TEXT("%"), TEXT("不同天气条件识别率不低于85%"));
			}
			else if (IndicatorId == TEXT("9.1.1.4a"))
			{
				const float Count = (float)P.MaxSimultaneousTracksObserved;
				EvaluateThreshold(Count, 5.f, true, TEXT(""), TEXT("同时跟踪目标数量至少5个"));
			}
			else if (IndicatorId == TEXT("9.1.1.4b"))
			{
				const float Err = P.NumTrackingErrorSamples > 0 ? (float)(P.TotalTrackingErrorMeters / P.NumTrackingErrorSamples) : 0.f;
				EvaluateThreshold(Err, 1.0f, false, TEXT(" m"), TEXT("平均跟踪误差不超过1米"));
			}
			else if (IndicatorId == TEXT("9.1.1.5a"))
			{
				const float Rate = P.SamplesJamLightTotal > 0 ? (float)P.SamplesJamLightCorrect / (float)P.SamplesJamLightTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 90.f, true, TEXT("%"), TEXT("轻度干扰下识别率不低于90%"));
			}
			else if (IndicatorId == TEXT("9.1.1.5b"))
			{
				const float Rate = P.SamplesJamMediumTotal > 0 ? (float)P.SamplesJamMediumCorrect / (float)P.SamplesJamMediumTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 80.f, true, TEXT("%"), TEXT("中度干扰下识别率不低于80%"));
			}
			else if (IndicatorId == TEXT("9.1.1.6a"))
			{
				const float Rate = P.SamplesInterferenceDetectedTotal > 0 ? (float)P.SamplesInterferenceDetectedCorrect / (float)P.SamplesInterferenceDetectedTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 95.f, true, TEXT("%"), TEXT("干扰信号检测准确率不低于95%"));
			}
			else if (IndicatorId == TEXT("9.1.1.6b"))
			{
				const float Rate = P.SamplesFrequencyAdjustTotal > 0 ? (float)P.SamplesFrequencyAdjustSuccess / (float)P.SamplesFrequencyAdjustTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 98.f, true, TEXT("%"), TEXT("频率调整成功率不低于98%"));
			}
			else if (IndicatorId == TEXT("9.1.1.7"))
			{
				const float Avg = P.NumRecoverySamples > 0 ? (float)(P.TotalRecoveryTimeSeconds / P.NumRecoverySamples) : 0.f;
				EvaluateThreshold(Avg, 1.0f, false, TEXT(" s"), TEXT("从干扰状态恢复正常工作的时间小于1秒"));
			}
			else if (IndicatorId == TEXT("9.1.1.8"))
			{
				const float Rate = P.SamplesKeypartTotal > 0 ? (float)P.SamplesKeypartCorrect / (float)P.SamplesKeypartTotal * 100.f : 0.f;
				EvaluateThreshold(Rate, 90.f, true, TEXT("%"), TEXT("关键部位识别准确率不低于90%"));
			}
			else if (IndicatorId == TEXT("9.1.1.9"))
			{
				const float Err = P.NumHeatTrackSamples > 0 ? (float)(P.TotalHeatTrackErrorMeters / P.NumHeatTrackSamples) : 0.f;
				EvaluateThreshold(Err, 0.5f, false, TEXT(" m"), TEXT("热源跟踪误差不超过0.5米"));
			}
			else if (IndicatorId == TEXT("9.1.1.10"))
			{
				const float Baseline = 0.f;
				const float Improved = P.SamplesKeypartTotal > 0 ? (float)P.SamplesKeypartCorrect / (float)P.SamplesKeypartTotal * 100.f : 0.f;
				const float Lift = FMath::Max(0.f, Improved - Baseline);
				EvaluateThreshold(Lift, 20.f, true, TEXT("%"), TEXT("抑制噪声与干扰后的识别率提升（需结合基线统计）"));
			}
			OutResults.Add(Result);
			continue;
		}
		if (IndicatorId == TEXT("3.4.1.1"))
		{
			const float Value = Summary.AverageAutoLaunchInterval > 0.f ? Summary.AverageAutoLaunchInterval : Summary.AverageFlightTime;
			EvaluateThreshold(Value, 0.5f, false, TEXT(" s"), TEXT("以自动发射间隔/飞行时间估算决策生成速度。"));
		}
		else if (IndicatorId == TEXT("3.4.1.2"))
		{
			// 可解释性-目标优先级设定：使用直接命中率衡量目标选择的准确性
			EvaluateThreshold(Summary.DirectHitRate, 80.f, true, TEXT("%"), TEXT("基于直接命中率，反映目标优先级设定的准确性。"));
		}
		else if (IndicatorId == TEXT("3.4.1.3"))
		{
			// 性能性-路径规划精度：使用命中率衡量路径规划的有效性
			EvaluateThreshold(Summary.HitRate, 85.f, true, TEXT("%"), TEXT("基于导弹命中率，反映路径规划的准确性和可执行性。"));
		}
		else if (IndicatorId == TEXT("3.4.1.4"))
		{
			// 性能性-全局最优路径规划精度：使用直接命中率衡量路径规划的精确度
			EvaluateThreshold(Summary.DirectHitRate, 75.f, true, TEXT("%"), TEXT("基于直接命中率，反映全局最优路径规划的准确性。"));
		}
		else if (IndicatorId == TEXT("3.4.1.5"))
		{
			// 性能性-资源分配优化精度：使用每次命中平均摧毁数衡量资源利用效率
			EvaluateThreshold(Summary.AverageDestroyedPerHit, 1.5f, true, TEXT(""), TEXT("基于每次命中平均摧毁数，反映资源分配的优化精度。"));
		}
		else if (IndicatorId == TEXT("3.4.1.6") || IndicatorId == TEXT("3.4.3.2"))
		{
			const float TargetPercent = IndicatorId == TEXT("3.4.1.6") ? 85.f : 95.f;
			EvaluateThreshold(Summary.HitRate, TargetPercent, true, TEXT("%"), TEXT("基于导弹命中率。"));
		}
		else if (IndicatorId == TEXT("3.4.1.7"))
		{
			// 可靠性-多智能体协同决策成功率：使用自动发射的命中率
			EvaluateThreshold(AutoHitRate, 80.f, true, TEXT("%"), TEXT("基于自动发射命中率，反映多智能体协同决策的成功率。"));
		}
		else if (IndicatorId == TEXT("3.4.1.8"))
		{
			// 功能性-任务分配协调性：使用自动发射比例衡量任务分配的协调性
			EvaluateThreshold(AutoFireRatio, 50.f, true, TEXT("%"), TEXT("基于自动发射比例，反映多个智能体之间任务分配和协调的精确度。"));
		}
		else if (IndicatorId == TEXT("3.4.2.1"))
		{
			// 鲁棒性-动态战术调整能力：基于自动发射命中率，反映动态调整战术的能力
			EvaluateThreshold(AutoHitRate, 80.f, true, TEXT("%"), TEXT("基于自动发射在不同目标选择下的命中率，反映动态调整战术的能力。"));
		}
		else if (IndicatorId == TEXT("3.4.2.2"))
		{
			// 智能性-敌方行动预测智能：基于直接命中率，反映预测敌方行动的智能水平
			EvaluateThreshold(Summary.DirectHitRate, 75.f, true, TEXT("%"), TEXT("基于直接命中率，反映预测敌方行动和选择最佳攻击时机的智能水平。"));
		}
		else if (IndicatorId == TEXT("3.4.2.3"))
		{
			// 鲁棒性-策略演化能力：基于自动发射命中率，反映策略适应和演化的能力
			EvaluateThreshold(AutoHitRate, 70.f, true, TEXT("%"), TEXT("基于自动发射命中率与手动发射命中率的对比，反映策略适应和演化的能力。"));
		}
		else if (IndicatorId == TEXT("3.4.2.4"))
		{
			// 智能性-多目标优化智能：基于每次命中平均摧毁数，反映多目标优化能力
			EvaluateThreshold(Summary.AverageDestroyedPerHit, 1.8f, true, TEXT(""), TEXT("基于每次命中平均摧毁数，反映在多目标环境中智能选择最优攻击策略的能力。"));
		}
		else if (IndicatorId == TEXT("3.4.2.5"))
		{
			// 未来可扩展性-对抗学习智能：基于整体命中率，反映通过学习提升反制策略的智能水平
			EvaluateThreshold(Summary.HitRate, 85.f, true, TEXT("%"), TEXT("基于整体命中率，反映通过学习提升反制策略的智能水平。"));
		}
		else if (IndicatorId == TEXT("3.4.2.6"))
		{
			// 智能性-博弈均衡智能：基于直接命中率，反映通过博弈理论选择最佳对抗策略的智能水平
			EvaluateThreshold(Summary.DirectHitRate, 78.f, true, TEXT("%"), TEXT("基于直接命中率，反映通过博弈理论选择最佳对抗策略的智能水平。"));
		}
		else if (IndicatorId == TEXT("3.4.2.7"))
		{
			// 智能性-群体协作智能：基于自动发射命中率，反映群体智能体通过协作优化整体决策的表现
			EvaluateThreshold(AutoHitRate, 82.f, true, TEXT("%"), TEXT("基于自动发射命中率，反映群体智能体通过协作优化整体决策的表现。"));
		}
		else if (IndicatorId == TEXT("3.4.2.8"))
		{
			// 智能性-协同决策学习智能：基于自动发射比例与命中率的综合评估
			// 使用自动发射比例和命中率的加权平均：比例占40%，命中率占60%
			const float CollaborativeScore = (AutoFireRatio * 0.4f) + (AutoHitRate * 0.6f);
			EvaluateThreshold(CollaborativeScore, 60.f, true, TEXT("%"), TEXT("基于自动发射比例与命中率的综合评估，反映协同决策学习的智能水平。"));
		}
		else if (IndicatorId == TEXT("3.4.3.1"))
		{
			// 效能性-资源使用效能：基于每次命中平均摧毁数，反映资源使用的最大化效率
			EvaluateThreshold(Summary.AverageDestroyedPerHit, 1.6f, true, TEXT(""), TEXT("基于每次命中平均摧毁数，反映资源使用的最大化效率。"));
		}
		else if (IndicatorId == TEXT("3.4.3.2"))
		{
			// 效能性-任务完成率：基于导弹命中率
			EvaluateThreshold(Summary.HitRate, 95.f, true, TEXT("%"), TEXT("基于导弹命中率。"));
		}
		else if (IndicatorId == TEXT("3.4.3.3"))
		{
			const float Value = Summary.AverageFlightTime;
			EvaluateThreshold(Value, 1.0f, false, TEXT(" s"), TEXT("以导弹平均飞行时间近似路径执行时长。"));
		}
		else if (IndicatorId == TEXT("3.4.3.4"))
		{
			// 效能性-反制策略执行效能：基于导弹平均飞行时间，反映反制策略生成和执行的效率
			const float Value = Summary.AverageFlightTime;
			EvaluateThreshold(Value, 0.8f, false, TEXT(" s"), TEXT("基于导弹平均飞行时间，反映反制策略生成和执行的效率。"));
		}
		else if (IndicatorId == TEXT("3.4.3.5"))
		{
			// 安全性-敌方行为预测效能：基于直接命中率，反映预测敌方行为的准确性和效率
			EvaluateThreshold(Summary.DirectHitRate, 80.f, true, TEXT("%"), TEXT("基于直接命中率，反映预测敌方行为的准确性和效率。"));
		}
		else if (IndicatorId == TEXT("3.4.3.6"))
		{
			// 可靠性-多智能体任务完成时间：基于自动发射导弹的平均飞行时间
			// 计算自动发射导弹的平均飞行时间
			float AutoFlightTime = 0.f;
			int32 AutoFlightCount = 0;
			for (const FMissileTestRecord& Record : MissileTestRecords)
			{
				if (Record.bAutoFire)
				{
					const float FlightDuration = Record.GetFlightDuration();
					if (FlightDuration > 0.f)
					{
						AutoFlightTime += FlightDuration;
						++AutoFlightCount;
					}
				}
			}
			const float AvgAutoFlightTime = AutoFlightCount > 0 ? (AutoFlightTime / AutoFlightCount) : Summary.AverageFlightTime;
			EvaluateThreshold(AvgAutoFlightTime, 1.2f, false, TEXT(" s"), TEXT("基于自动发射导弹的平均飞行时间，反映多智能体任务完成的效率。"));
		}
		else if (IndicatorId == TEXT("3.4.3.7"))
		{
			// 效能性-协同决策效能：基于自动发射命中率与资源利用效率的综合评估
			// 使用自动发射命中率（60%）和资源利用效率（40%）的加权平均
			const float ResourceEfficiency = Summary.AverageDestroyedPerHit;
			const float NormalizedResourceEfficiency = FMath::Clamp(ResourceEfficiency / 2.0f * 100.f, 0.f, 100.f); // 归一化到0-100%
			const float CollaborativeEfficiency = (AutoHitRate * 0.6f) + (NormalizedResourceEfficiency * 0.4f);
			EvaluateThreshold(CollaborativeEfficiency, 75.f, true, TEXT("%"), TEXT("基于自动发射命中率与资源利用效率的综合评估，反映协同决策的效能。"));
		}
		else if (IndicatorId == TEXT("3.5.1.1"))
		{
			// 功能性-多任务协调能力：基于自动发射比例，反映多任务场景下协调不同智能体执行任务的能力
			EvaluateThreshold(AutoFireRatio, 70.f, true, TEXT("%"), TEXT("基于自动发射比例，反映多任务场景下协调不同智能体执行任务的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.1.2"))
		{
			// 性能性-全局决策优化能力：基于整体命中率，反映在多目标场景中进行全局最优决策的能力
			EvaluateThreshold(Summary.HitRate, 80.f, true, TEXT("%"), TEXT("基于整体命中率，反映在多目标场景中进行全局最优决策的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.1.3"))
		{
			// 性能性-任务切换响应速度：基于自动发射间隔，反映任务切换的响应速度
			const float TaskSwitchTime = Summary.AverageAutoLaunchInterval > 0.f ? Summary.AverageAutoLaunchInterval : 0.6f;
			EvaluateThreshold(TaskSwitchTime, 0.6f, false, TEXT(" s"), TEXT("基于自动发射间隔，反映任务切换的响应速度。"));
		}
		else if (IndicatorId == TEXT("3.5.1.4"))
		{
			// 可靠性-故障恢复能力：基于整体命中率，反映在系统失效情况下恢复正常决策功能的能力
			EvaluateThreshold(Summary.HitRate, 85.f, true, TEXT("%"), TEXT("基于整体命中率，反映在系统失效情况下恢复正常决策功能的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.1.5"))
		{
			// 性能性-全局优化任务分配精度：基于自动发射命中率，反映在多个任务环境中最优分配任务的精度
			EvaluateThreshold(AutoHitRate, 75.f, true, TEXT("%"), TEXT("基于自动发射命中率，反映在多个任务环境中最优分配任务的精度。"));
		}
		else if (IndicatorId == TEXT("3.5.1.6"))
		{
			// 功能性-层次对抗策略执行成功率：基于直接命中率，反映在多层次对抗中成功执行反制策略的比例
			EvaluateThreshold(Summary.DirectHitRate, 82.f, true, TEXT("%"), TEXT("基于直接命中率，反映在多层次对抗中成功执行反制策略的比例。"));
		}
		else if (IndicatorId == TEXT("3.5.1.7"))
		{
			// 鲁棒性-多智能体协同效能：基于自动发射命中率与任务完成效率的综合评估
			// 使用自动发射命中率（70%）和任务完成率（30%）的加权平均
			const float TaskCompletionRate = Summary.HitRate;
			const float CoordinationEfficiency = (AutoHitRate * 0.7f) + (TaskCompletionRate * 0.3f);
			EvaluateThreshold(CoordinationEfficiency, 78.f, true, TEXT("%"), TEXT("基于自动发射命中率与任务完成效率的综合评估，反映多智能体协同效能。"));
		}
		else if (IndicatorId == TEXT("3.5.2.1"))
		{
			// 未来可扩展性-系统自学习能力：基于整体命中率，反映通过博弈对抗不断提升自身决策效率和准确性的能力
			EvaluateThreshold(Summary.HitRate, 83.f, true, TEXT("%"), TEXT("基于整体命中率，反映通过博弈对抗不断提升自身决策效率和准确性的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.2.2"))
		{
			// 智能性-协作决策智能：基于自动发射命中率，反映多智能体协同作战时生成一致性决策的智能水平
			EvaluateThreshold(AutoHitRate, 80.f, true, TEXT("%"), TEXT("基于自动发射命中率，反映多智能体协同作战时生成一致性决策的智能水平。"));
		}
		else if (IndicatorId == TEXT("3.5.2.3"))
		{
			// 可解释性-全局态势感知智能：基于整体命中率，反映对全局态势的理解和智能决策支持
			EvaluateThreshold(Summary.HitRate, 85.f, true, TEXT("%"), TEXT("基于整体命中率，反映对全局态势的理解和智能决策支持。"));
		}
		else if (IndicatorId == TEXT("3.5.2.4"))
		{
			// 可解释性-全局任务分解智能：基于自动发射比例，反映智能分解全局复杂任务为多个可执行子任务并协调智能体执行的能力
			EvaluateThreshold(AutoFireRatio, 72.f, true, TEXT("%"), TEXT("基于自动发射比例，反映智能分解全局复杂任务为多个可执行子任务并协调智能体执行的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.2.5"))
		{
			// 智能性-策略调整智能：基于直接命中率，反映在对抗环境中根据实时战场态势智能调整反制策略的能力
			EvaluateThreshold(Summary.DirectHitRate, 79.f, true, TEXT("%"), TEXT("基于直接命中率，反映在对抗环境中根据实时战场态势智能调整反制策略的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.2.6"))
		{
			// 智能性-集群协作智能：基于自动发射命中率与任务完成率的综合评估
			// 使用自动发射命中率（65%）和任务完成率（35%）的加权平均
			const float TaskCompletionRate = Summary.HitRate;
			const float SwarmCollaborationIntelligence = (AutoHitRate * 0.65f) + (TaskCompletionRate * 0.35f);
			EvaluateThreshold(SwarmCollaborationIntelligence, 81.f, true, TEXT("%"), TEXT("基于自动发射命中率与任务完成率的综合评估，反映智能协调多个智能体协同完成任务的能力。"));
		}
		else if (IndicatorId == TEXT("3.5.3.1"))
		{
			// 效能性-任务分配效能：基于自动发射比例，反映在不同智能体之间分配任务的效率和合理性
			EvaluateThreshold(AutoFireRatio, 68.f, true, TEXT("%"), TEXT("基于自动发射比例，反映在不同智能体之间分配任务的效率和合理性。"));
		}
		else if (IndicatorId == TEXT("3.5.3.2"))
		{
			// 效能性-作战资源管理效能：基于每次命中平均摧毁数，反映在复杂战场中有效管理和调度资源的表现
			EvaluateThreshold(Summary.AverageDestroyedPerHit, 1.7f, true, TEXT(""), TEXT("基于每次命中平均摧毁数，反映在复杂战场中有效管理和调度资源的表现。"));
		}
		else if (IndicatorId == TEXT("3.5.3.3"))
		{
			// 效能性-任务完成时间：基于导弹平均飞行时间，反映系统完成任务所需的平均时间
			const float Value = Summary.AverageFlightTime;
			EvaluateThreshold(Value, 1.1f, false, TEXT(" s"), TEXT("基于导弹平均飞行时间，反映系统完成任务所需的平均时间。"));
		}
		else if (IndicatorId == TEXT("3.5.3.4"))
		{
			// 效能性-决策一致性：基于自动发射命中率，反映在多智能体决策中的一致性表现，避免冲突决策
			EvaluateThreshold(AutoHitRate, 77.f, true, TEXT("%"), TEXT("基于自动发射命中率，反映在多智能体决策中的一致性表现，避免冲突决策。"));
		}
		else if (IndicatorId == TEXT("3.5.3.5"))
		{
			// 安全性-策略反制效能：基于导弹平均飞行时间，反映反制策略的生成和执行效能
			const float Value = Summary.AverageFlightTime;
			EvaluateThreshold(Value, 0.9f, false, TEXT(" s"), TEXT("基于导弹平均飞行时间，反映反制策略的生成和执行效能。"));
		}
		else if (IndicatorId == TEXT("3.5.3.6"))
		{
			// 效能性-集群任务执行效能：基于自动发射命中率与资源利用效率的综合评估
			// 使用自动发射命中率（55%）和资源利用效率（45%）的加权平均
			const float ResourceEfficiency = Summary.AverageDestroyedPerHit;
			const float NormalizedResourceEfficiency = FMath::Clamp(ResourceEfficiency / 2.0f * 100.f, 0.f, 100.f); // 归一化到0-100%
			const float SwarmExecutionEfficiency = (AutoHitRate * 0.55f) + (NormalizedResourceEfficiency * 0.45f);
			EvaluateThreshold(SwarmExecutionEfficiency, 80.f, true, TEXT("%"), TEXT("基于自动发射命中率与资源利用效率的综合评估，反映通过群体算法高效完成任务的能力。"));
		}
		else
		{
			if (false)
			{
				// placeholder for future mappings
			}
			else
			{
				if (bHasData)
				{
					Result.RemarkText = TEXT("尚未为该指标实现数据采集与计算。");
				}
			}
		}

		OutResults.Add(Result);
	}
}

void UScenarioMenuSubsystem::ApplyEnvironmentSettings(UWorld* World, const FScenarioTestConfig& Config)
{
	if (!World)
	{
		return;
	}
	
	const bool bDay = Config.TimeIndex == 0;
	const float TargetExposure = bDay ? 1.0f : 0.7f;

	UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.EyeAdaptationQuality 0"));

	auto ConfigureExposure = [TargetExposure](FPostProcessSettings& Settings)
	{
		Settings.bOverride_AutoExposureMethod = true;
		Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = 0.f;
		Settings.bOverride_AutoExposureMinBrightness = true;
		Settings.AutoExposureMinBrightness = TargetExposure;
		Settings.bOverride_AutoExposureMaxBrightness = true;
        Settings.AutoExposureMaxBrightness = TargetExposure;
		Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		Settings.AutoExposureApplyPhysicalCameraExposure = false;
		Settings.bOverride_AutoExposureSpeedDown = true;
		Settings.AutoExposureSpeedDown = 30.f;
		Settings.bOverride_AutoExposureSpeedUp = true;
		Settings.AutoExposureSpeedUp = 30.f;
	};

	// 时间段 -> 太阳角度和颜色
	if (ADirectionalLight* Dir = Cast<ADirectionalLight>(UGameplayStatics::GetActorOfClass(World, ADirectionalLight::StaticClass())))
	{
		if (UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(Dir->GetLightComponent()))
		{
			const float TargetPitch = bDay ? -35.f : -12.f;
			FRotator NewRotation = Dir->GetActorRotation();
			NewRotation.Pitch = TargetPitch;
			Dir->SetActorRotation(NewRotation);

			LightComp->SetIntensity(bDay ? 50.f : 1.2f);
			LightComp->SetLightColor(bDay ? FLinearColor(1.f, 0.95f, 0.85f) : FLinearColor(0.2f, 0.25f, 0.35f));
			LightComp->MarkRenderStateDirty();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Directional light found but has no directional light component."));
		}
	}

	if (ASkyLight* Sky = Cast<ASkyLight>(UGameplayStatics::GetActorOfClass(World, ASkyLight::StaticClass())))
	{
		if (USkyLightComponent* SkyComp = Sky->GetLightComponent())
		{
			SkyComp->SetIntensity(bDay ? 1.2f : 0.3f);
			SkyComp->SetVolumetricScatteringIntensity(bDay ? 1.0f : 0.35f);
			SkyComp->bLowerHemisphereIsBlack = !bDay;
			SkyComp->SetLowerHemisphereColor(bDay ? FLinearColor::Black : FLinearColor(0.01f, 0.01f, 0.015f));
			SkyComp->MarkRenderStateDirty();
			SkyComp->RecaptureSky();
		}
	}


	// 雾密度
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		if (UExponentialHeightFogComponent* FogComp = It->GetComponent())
		{
			if (Config.WeatherIndex == 3) // 雾天
			{
				FogComp->SetFogDensity(0.9f);
			}
			else
			{
				FogComp->SetFogDensity(0.002f);
			}
		}
	}

	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		APostProcessVolume* Volume = *It;
		if (!Volume)
		{
			continue;
		}

		FPostProcessSettings& Settings = Volume->Settings;
		ConfigureExposure(Settings);
	}

	// 若需要，可在具体关卡的后处理体积上调整；当前版本未直接修改 WorldSettings。

	// 简单的降雨开关：查找带有 RainFX 标签或名字包含 Rain 的 Actor
/*	const bool bEnableRain = (Config.WeatherIndex == 1);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->ActorHasTag(TEXT("RainFX")) || Actor->GetName().Contains(TEXT("Rain")))
		{
			Actor->SetActorHiddenInGame(!bEnableRain);
			Actor->SetActorTickEnabled(bEnableRain);
		}
	}*/

	UE_LOG(LogTemp, Log, TEXT("Environment settings applied. Weather=%d Time=%d MapIndex=%d"), Config.WeatherIndex, Config.TimeIndex, Config.MapIndex);
}

void UScenarioMenuSubsystem::FinalizeScenarioAfterLoad()
{
	UWorld* World = PendingScenarioWorld.Get();
	if (!World)
	{
		return;
	}

	if (!bHasPendingScenarioConfig)
	{
		World->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
		PendingScenarioWorld = nullptr;
		bPendingScenarioWaitingLogged = false;
		return;
	}

	if (!AreScenarioLevelsReady(World))
	{
		if (!bPendingScenarioWaitingLogged)
		{
			UE_LOG(LogTemp, Log, TEXT("FinalizeScenarioAfterLoad: waiting for streaming levels to finish loading..."));
			bPendingScenarioWaitingLogged = true;
		}
		return;
	}

	World->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
	PendingScenarioWorld = nullptr;
	bPendingScenarioWaitingLogged = false;

	if (bHasPendingScenarioConfig)
	{
		UE_LOG(LogTemp, Log, TEXT("FinalizeScenarioAfterLoad: streaming complete, applying scenario configuration."));
		ApplyEnvironmentSettings(World, PendingScenarioConfig);
		DeployBlueForScenario(World, PendingScenarioConfig);
		bHasPendingScenarioConfig = false;
	}
}

bool UScenarioMenuSubsystem::AreScenarioLevelsReady(UWorld* World) const
{
	if (!World)
	{
		return false;
	}

	if (World->IsInSeamlessTravel())
	{
		return false;
	}

	if (!World->AreActorsInitialized())
	{
		return false;
	}

	const TArray<ULevelStreaming*>& StreamingLevels = World->GetStreamingLevels();
	for (ULevelStreaming* Streaming : StreamingLevels)
	{
		if (!Streaming)
		{
			continue;
		}

		if (Streaming->ShouldBeLoaded() && !Streaming->IsLevelLoaded())
		{
			return false;
		}

		if (Streaming->ShouldBeVisible() && !Streaming->IsLevelVisible())
		{
			return false;
		}
	}

	return true;
}

void UScenarioMenuSubsystem::DeployBlueForScenario(UWorld* World, const FScenarioTestConfig& Config)
{
	if (!World)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: start (Map=%s, Density=%d, Custom=%s)"), *Config.MapLevelName.ToString(), Config.DensityIndex, Config.bBlueCustomDeployment ? TEXT("true") : TEXT("false"));

	UClass* UnitClass = ResolveBlueUnitClass();
	if (!UnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to resolve blue unit class. Please update ResolveBlueUnitClass if you need a custom actor."));
		return;
	}

	UStaticMesh* DefaultUnitMesh = ResolveBlueUnitMeshByType(0);
	if (!DefaultUnitMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to load placeholder mesh. Please verify StarterContent is enabled."));
		return;
	}

	UMaterialInterface* DefaultUnitMaterial = ResolveBlueUnitMaterialByType(0);
	if (!DefaultUnitMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to load placeholder material. Units will use default material."));
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Using UnitClass=%s, Mesh=%s, Material=%s"), *UnitClass->GetName(), *DefaultUnitMesh->GetName(), DefaultUnitMaterial ? *DefaultUnitMaterial->GetName() : TEXT("None"));

	ClearSpawnedBlueUnits();
	ActiveBlueUnits.Reset();
	ActiveCountermeasureIndices = Config.CountermeasureIndices;
	BlueUnitMeshes.Reset();
	BlueUnitMaterials.Reset();

	constexpr int32 UnitTypeCount = 3;
	for (int32 TypeIndex = 0; TypeIndex < UnitTypeCount; ++TypeIndex)
	{
		UStaticMesh* MeshForType = ResolveBlueUnitMeshByType(TypeIndex);
		if (!MeshForType)
		{
			MeshForType = DefaultUnitMesh;
		}
		BlueUnitMeshes.Add(MeshForType);

		UMaterialInterface* MaterialForType = ResolveBlueUnitMaterialByType(TypeIndex);
		if (!MaterialForType)
		{
			MaterialForType = DefaultUnitMaterial;
		}
		BlueUnitMaterials.Add(MaterialForType);
	}

	// 暂存玩家初始视角
	APlayerStart* PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()));
	FVector Origin = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	if (PlayerStart)
	{
		Origin = PlayerStart->GetActorLocation();
		Facing = PlayerStart->GetActorRotation();
	}

	FVector Forward = Facing.Vector();
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}
	FVector Right = FRotationMatrix(Facing).GetScaledAxis(EAxis::Y);
	if (!Right.Normalize())
	{
		Right = FVector::RightVector;
	}
	BlueUnitFacing = Facing;
	BlueForward = Forward;
	BlueRight = Right;

	int32 DesiredMarkerCount = 6;
	switch (Config.DensityIndex)
	{
	case 0: // 密集
		DesiredMarkerCount = 8;
		break;
	case 2: // 稀疏
		DesiredMarkerCount = 3;
		break;
	default: // 正常
		DesiredMarkerCount = 5;
		break;
	}

	// Collect spawn markers: prefer tags, fallback to name pattern
	TArray<AActor*> SpawnMarkers;
	const FName PrimaryTag = FName(TEXT("BluePotentialDeployLocation"));
	const FName LegacyTag = FName(TEXT("PotentialBlueDeployLocation"));
	UGameplayStatics::GetAllActorsWithTag(World, PrimaryTag, SpawnMarkers);
	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: actors with tag %s = %d"), *PrimaryTag.ToString(), SpawnMarkers.Num());

	if (LegacyTag != PrimaryTag)
	{
		TArray<AActor*> LegacyActors;
		UGameplayStatics::GetAllActorsWithTag(World, LegacyTag, LegacyActors);
		if (LegacyActors.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: actors with legacy tag %s = %d"), *LegacyTag.ToString(), LegacyActors.Num());
			for (AActor* Legacy : LegacyActors)
			{
				SpawnMarkers.AddUnique(Legacy);
			}
		}
	}

	if (SpawnMarkers.Num() == 0)
	{
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
		for (AActor* Actor : AllActors)
		{
			if (!Actor)
			{
				continue;
			}

			const FString ActorName = Actor->GetName();
			if (ActorName.Contains(TEXT("BluePotentialDeployLocation")))
			{
				SpawnMarkers.Add(Actor);
				UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: fallback picked actor %s (Class=%s)"), *ActorName, *Actor->GetClass()->GetName());
			}
		}

		UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: fallback name search matched %d actors"), SpawnMarkers.Num());

		if (SpawnMarkers.Num() == 0)
		{
			const int32 SampleCount = FMath::Min(20, AllActors.Num());
			UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: fallback failed. Sample of world actors:"));
			for (int32 Index = 0; Index < SampleCount; ++Index)
			{
				AActor* Sample = AllActors[Index];
				if (Sample)
				{
					UE_LOG(LogTemp, Warning, TEXT("  [%d] %s (Class=%s, Tags=%d)"), Index, *Sample->GetName(), *Sample->GetClass()->GetName(), Sample->Tags.Num());
				}
			}
		}
	}

	if (SpawnMarkers.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("DeployBlueForScenario: 未找到任何包含标签 %s/%s 或名称包含 BluePotentialDeployLocation 的部署点，无法生成蓝方单位."), *PrimaryTag.ToString(), *LegacyTag.ToString());
		bUsingCustomDeployment = false;
		ShowBlueMonitor(World);
		RefreshBlueMonitor();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: collected %d deployment markers"), SpawnMarkers.Num());

	// 计算部署区域包围盒用于摄像机
	FBox DeployBounds(ForceInit);
	for (AActor* Marker : SpawnMarkers)
	{
		if (Marker)
		{
			DeployBounds += Marker->GetActorLocation();
		}
	}

	if (!DeployBounds.IsValid)
	{
		const FVector DefaultExtent(2000.f, 2000.f, 800.f);
		DeployBounds = FBox(Origin - DefaultExtent, Origin + DefaultExtent);
	}

	FVector DeployCenter = DeployBounds.GetCenter();
	const float OverviewHeight = FMath::Max(DeployBounds.GetExtent().Z, 500.f) + 1500.f;
	OverviewHomeLocation = DeployCenter + FVector(0.f, 0.f, OverviewHeight);
	OverviewHomeRotation = UKismetMathLibrary::FindLookAtRotation(OverviewHomeLocation, DeployCenter);
	bHasOverviewHome = true;
	EnsureOverviewPawn(World);

	BlueDeployMarkers.Reset();
	BlueMarkerSpawnCounts.Reset();
	for (AActor* Marker : SpawnMarkers)
	{
		BlueDeployMarkers.Add(Marker);
		BlueMarkerSpawnCounts.Add(0);
	}

	bUsingCustomDeployment = Config.bBlueCustomDeployment;

	ShowBlueMonitor(World);

	if (bUsingCustomDeployment)
	{
		UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Custom deployment enabled. Waiting for operator input."));
		RefreshBlueMonitor();
		return;
	}

	// Shuffle markers
	for (int32 i = SpawnMarkers.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		SpawnMarkers.Swap(i, j);
	}
	int32 MarkersToUse = SpawnMarkers.Num();
	if (DesiredMarkerCount > 0)
	{
		MarkersToUse = FMath::Clamp(DesiredMarkerCount, 1, SpawnMarkers.Num());
	}

	for (int32 MarkerIndex = 0; MarkerIndex < MarkersToUse; ++MarkerIndex)
	{
		AActor* Marker = SpawnMarkers[MarkerIndex];
		if (!Marker)
		{
			continue;
		}

		const FVector MarkerLocation = Marker->GetActorLocation();
		const FString MarkerName = Marker->GetName();
		const int32 ExistingCount = BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex) ? BlueMarkerSpawnCounts[MarkerIndex] : 0;
		const int32 UnitsForThisSpot = FMath::RandRange(1, 4);
		int32 LocalCount = 0;

		UE_LOG(LogTemp, Log, TEXT("Marker %s allocated %d units (auto deployment)"), *MarkerName, UnitsForThisSpot);

		for (int32 UnitIdx = 0; UnitIdx < UnitsForThisSpot; ++UnitIdx)
		{
			const int32 UnitType = FMath::RandRange(0, 2);
			const int32 SlotIndex = ExistingCount + LocalCount;
			constexpr float BaseRadius = 1680.f;
			constexpr float RadiusStep = 1280.f;
			constexpr int32 SlotsPerRing = 8;

			const int32 RingIndex = SlotIndex / SlotsPerRing;
			const int32 SlotIndexInRing = SlotIndex % SlotsPerRing;
			const float AngleDeg = SlotIndexInRing * (360.f / SlotsPerRing) + UnitType * 12.f + FMath::FRandRange(-8.f, 8.f);
			const float AngleRad = FMath::DegreesToRadians(AngleDeg);
			const float Radius = BaseRadius + RingIndex * RadiusStep;

			FVector ForwardDir = BlueForward;
			if (!ForwardDir.Normalize())
			{
				ForwardDir = FVector::ForwardVector;
			}
			FVector RightDir = BlueRight;
			if (!RightDir.Normalize())
			{
				RightDir = FVector::RightVector;
			}

			const FVector2D BaseDir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
			FVector DesiredLocation = MarkerLocation
				+ ForwardDir * (BaseDir.X * Radius)
				+ RightDir * (BaseDir.Y * Radius);

			const float Jitter = 240.f;
			DesiredLocation += ForwardDir * FMath::FRandRange(-Jitter, Jitter);
			DesiredLocation += RightDir * FMath::FRandRange(-Jitter, Jitter);

			UStaticMesh* MeshForType = BlueUnitMeshes.IsValidIndex(UnitType) ? BlueUnitMeshes[UnitType].Get() : ResolveBlueUnitMeshByType(UnitType);
			if (!MeshForType)
			{
				MeshForType = DefaultUnitMesh;
			}

			UMaterialInterface* MatForType = BlueUnitMaterials.IsValidIndex(UnitType) ? BlueUnitMaterials[UnitType].Get() : ResolveBlueUnitMaterialByType(UnitType);
			if (!MatForType)
			{
				MatForType = DefaultUnitMaterial;
			}

			if (!SpawnBlueUnitAtLocation(World, DesiredLocation, BlueUnitFacing, MarkerName, MeshForType, MatForType, Config.CountermeasureIndices))
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to place unit at marker %s"), *MarkerName);
			}
			else
			{
				++LocalCount;
			}
		}

		if (BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex))
		{
			BlueMarkerSpawnCounts[MarkerIndex] = ExistingCount + LocalCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Spawned %d blue units (DensityIndex=%d)."), ActiveBlueUnits.Num(), Config.DensityIndex);
RefreshBlueMonitor();
}

void UScenarioMenuSubsystem::ClearSpawnedBlueUnits()
{
	StopMissileCameraFollow();

	for (TWeakObjectPtr<AActor>& Ptr : ActiveBlueUnits)
	{
		if (AActor* Actor = Ptr.Get())
		{
			if (!Actor->IsPendingKillPending())
			{
				Actor->Destroy();
			}
		}
	}
	ActiveBlueUnits.Reset();
	NextTargetCursor = 0;

	for (TWeakObjectPtr<AMockMissileActor>& MissilePtr : ActiveMissiles)
	{
		if (AMockMissileActor* Missile = MissilePtr.Get())
		{
			if (!Missile->IsPendingKillPending())
			{
				Missile->Destroy();
			}
		}
	}
	ActiveMissiles.Reset();
	AutoFireRemaining = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}

	if (bUsingCustomDeployment)
	{
		for (int32& Count : BlueMarkerSpawnCounts)
		{
			Count = 0;
		}
	}

	RefreshBlueMonitor();
	RebuildViewSequence();
}

UClass* UScenarioMenuSubsystem::ResolveBlueUnitClass() const
{
	return AStaticMeshActor::StaticClass();
}

UStaticMesh* UScenarioMenuSubsystem::ResolveBlueUnitMesh() const
{
	return ResolveBlueUnitMeshByType(0);
}

UMaterialInterface* UScenarioMenuSubsystem::ResolveBlueUnitMaterial() const
{
	return ResolveBlueUnitMaterialByType(0);
}

UStaticMesh* UScenarioMenuSubsystem::ResolveBlueUnitMeshByType(int32 TypeIndex) const
{
	static const TCHAR* MeshPaths[] =
	{
		TEXT("/Game/Military_Free/Meshes/SM_radiostation_001.SM_radiostation_001"),
		TEXT("/Game/T-34-85/Models/SM_T-34-85.SM_T-34-85"),
		TEXT("/Game/Military_Free/Meshes/SM_radar_station_002.SM_radar_station_002"),
	};

	constexpr int32 MaxTypes = UE_ARRAY_COUNT(MeshPaths);
	const int32 SafeIndex = FMath::Clamp(TypeIndex, 0, MaxTypes - 1);

	static TWeakObjectPtr<UStaticMesh> MeshCache[MaxTypes];
	if (!MeshCache[SafeIndex].IsValid())
	{
		FSoftObjectPath MeshPath(MeshPaths[SafeIndex]);
		if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(MeshPath.TryLoad()))
		{
			MeshCache[SafeIndex] = LoadedMesh;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveBlueUnitMeshByType: failed to load mesh at '%s' (Type=%d)."), MeshPaths[SafeIndex], SafeIndex);
			return nullptr;
		}
	}

	return MeshCache[SafeIndex].Get();
}

UMaterialInterface* UScenarioMenuSubsystem::ResolveBlueUnitMaterialByType(int32 TypeIndex) const
{
	static const TCHAR* MaterialPaths[] =
	{
		TEXT("/Game/StarterContent/Materials/M_Metal_Copper.M_Metal_Copper"),
		TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"),
		TEXT("/Game/StarterContent/Materials/M_Metal_Burnished_Steel.M_Metal_Burnished_Steel"),
	};

	constexpr int32 MaxTypes = UE_ARRAY_COUNT(MaterialPaths);
	const int32 SafeIndex = FMath::Clamp(TypeIndex, 0, MaxTypes - 1);

	static TWeakObjectPtr<UMaterialInterface> MaterialCache[MaxTypes];
	if (!MaterialCache[SafeIndex].IsValid())
	{
		FSoftObjectPath MaterialPath(MaterialPaths[SafeIndex]);
		if (UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(MaterialPath.TryLoad()))
		{
			MaterialCache[SafeIndex] = LoadedMaterial;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveBlueUnitMaterialByType: failed to load material at '%s' (Type=%d)."), MaterialPaths[SafeIndex], SafeIndex);
			// Fallback to default engine material
			FSoftObjectPath DefaultPath(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
			if (UMaterialInterface* DefaultMat = Cast<UMaterialInterface>(DefaultPath.TryLoad()))
			{
				MaterialCache[SafeIndex] = DefaultMat;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ResolveBlueUnitMaterialByType: failed to load default engine material."));
				return nullptr;
			}
		}
	}

	return MaterialCache[SafeIndex].Get();
}

void UScenarioMenuSubsystem::ShowBlueMonitor(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (!BlueMonitorRoot.IsValid())
	{
		SAssignNew(BlueMonitorWidget, SBlueUnitMonitor)
			.Mode(bUsingCustomDeployment ? SBlueUnitMonitor::EMode::Markers : SBlueUnitMonitor::EMode::Units)
			.OnFocusUnit(SBlueUnitMonitor::FFocusUnit::CreateUObject(this, &UScenarioMenuSubsystem::FocusCameraOnUnit))
			.OnFocusMarker(SBlueUnitMonitor::FFocusMarker::CreateUObject(this, &UScenarioMenuSubsystem::FocusCameraOnMarker))
			.OnSpawnAtMarker(SBlueUnitMonitor::FSpawnAtMarker::CreateUObject(this, &UScenarioMenuSubsystem::SpawnBlueUnitAtMarker))
			.OnReturnCamera(FSimpleDelegate::CreateUObject(this, &UScenarioMenuSubsystem::ReturnCameraToInitial))
			.OnLaunchMissile(SBlueUnitMonitor::FFireOneMissile::CreateUObject(this, &UScenarioMenuSubsystem::LaunchMissileFromUI))
			.OnAutoFireMissiles(SBlueUnitMonitor::FAutoFireMissiles::CreateUObject(this, &UScenarioMenuSubsystem::FireMultipleMissiles));

		TSharedRef<SWidget> Overlay =
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(FMargin(32.f, 120.f, 0.f, 0.f))
			[
				BlueMonitorWidget.ToSharedRef()
			];

		BlueMonitorRoot = Overlay;
		GEngine->GameViewport->AddViewportWidgetContent(Overlay, 400);
	}

	RefreshBlueMonitor();
	if (World)
	{
		SetupInputBindings(World);
	}
}

void UScenarioMenuSubsystem::HideBlueMonitor()
{
	if (GEngine && GEngine->GameViewport && BlueMonitorRoot.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BlueMonitorRoot.ToSharedRef());
	}
	BlueMonitorRoot.Reset();
	BlueMonitorWidget.Reset();
	RemoveInputBindings();
	StopMissileCameraFollow();
	RestorePreviousPawn();
}

void UScenarioMenuSubsystem::RefreshBlueMonitor()
{
	if (!BlueMonitorWidget.IsValid())
	{
		return;
	}

	CleanupMissiles();
	ActiveBlueUnits.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr)
	{
		return !Ptr.IsValid();
	});

	RebuildViewSequence();

	if (bUsingCustomDeployment)
	{
		BlueMonitorWidget->RefreshMarkers(BlueDeployMarkers, BlueMarkerSpawnCounts);
	}
	else
	{
		BlueMonitorWidget->RefreshUnits(ActiveBlueUnits);
	}
}

void UScenarioMenuSubsystem::FocusCameraOnMarker(int32 Index)
{
	if (!bUsingCustomDeployment)
	{
		FocusCameraOnUnit(Index);
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !BlueDeployMarkers.IsValidIndex(Index))
	{
		return;
	}

	AActor* Marker = BlueDeployMarkers[Index].Get();
	if (!Marker)
	{
		UE_LOG(LogTemp, Warning, TEXT("FocusCameraOnMarker: marker %d is no longer valid."), Index);
		return;
	}

	StopMissileCameraFollow();

	const FVector TargetLocation = Marker->GetActorLocation();
	FVector ForwardDir = BlueForward;
	if (!ForwardDir.Normalize())
	{
		ForwardDir = FVector::ForwardVector;
	}
	const FVector CameraLocation = TargetLocation - ForwardDir * 800.f + FVector(0.f, 0.f, 500.f);
	const FRotator CameraRotation = (TargetLocation - CameraLocation).Rotation();
	EnsureOverviewPawn(World);
	if (ASpectatorPawn* Pawn = OverviewPawn.Get())
	{
		Pawn->SetActorLocation(CameraLocation);
		Pawn->SetActorRotation(CameraRotation);
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetControlRotation(CameraRotation);
		}
	}
}

void UScenarioMenuSubsystem::FocusCameraOnUnit(int32 Index)
{
	UWorld* World = GetWorld();
	if (!World || Index < 0)
	{
		return;
	}

	ActiveBlueUnits.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });
	if (!ActiveBlueUnits.IsValidIndex(Index))
	{
		return;
	}

	AActor* Target = ActiveBlueUnits[Index].Get();
	if (!Target)
	{
		return;
	}

	StopMissileCameraFollow();
	for (int32 ViewIdx = 0; ViewIdx < ViewEntries.Num(); ++ViewIdx)
	{
		const FViewEntry& Entry = ViewEntries[ViewIdx];
		if (Entry.Type == FViewEntry::EType::BlueUnit && Entry.Index == Index)
		{
			CurrentViewIndex = ViewIdx;
			break;
		}
	}

	const FVector TargetLocation = Target->GetActorLocation();
	FVector ForwardDir = Target->GetActorForwardVector();
	if (ForwardDir.IsNearlyZero())
	{
		ForwardDir = FVector::ForwardVector;
	}
	const FVector CameraLocation = TargetLocation - ForwardDir * 650.f + FVector(0.f, 0.f, 400.f);
	const FRotator CameraRotation = (TargetLocation - CameraLocation).Rotation();
	EnsureOverviewPawn(World);
	if (ASpectatorPawn* Pawn = OverviewPawn.Get())
	{
		Pawn->SetActorLocation(CameraLocation);
		Pawn->SetActorRotation(CameraRotation);
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetControlRotation(CameraRotation);
		}
	}
}

void UScenarioMenuSubsystem::SpawnBlueUnitAtMarker(int32 MarkerIndex, int32 UnitType)
{
	if (!bUsingCustomDeployment)
	{
		return;
	}

	if (UnitType < 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !BlueDeployMarkers.IsValidIndex(MarkerIndex))
	{
		return;
	}

	AActor* Marker = BlueDeployMarkers[MarkerIndex].Get();
	if (!Marker)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnBlueUnitAtMarker: marker %d is no longer valid."), MarkerIndex);
		return;
	}

	const FVector TargetLocation = Marker->GetActorLocation();

	FVector ForwardDir = BlueForward;
	if (!ForwardDir.Normalize())
	{
		ForwardDir = FVector::ForwardVector;
	}
	FVector RightDir = BlueRight;
	if (!RightDir.Normalize())
	{
		RightDir = FVector::RightVector;
	}

	const int32 ExistingCount = BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex) ? BlueMarkerSpawnCounts[MarkerIndex] : 0;
	constexpr float BaseRadius = 1680.f;
	constexpr float RadiusStep = 1280.f;
	const int32 SlotsPerRing = 8;

	const int32 RingIndex = ExistingCount / SlotsPerRing;
	const int32 SlotIndexInRing = ExistingCount % SlotsPerRing;
	const int32 SafeType = FMath::Clamp(UnitType, 0, 2);

	const float AngleDeg = SlotIndexInRing * (360.f / SlotsPerRing) + SafeType * 12.f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);
	const float Radius = BaseRadius + RingIndex * RadiusStep;

	const FVector2D BaseDir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
	FVector DesiredLocation = TargetLocation
		+ ForwardDir * (BaseDir.X * Radius)
		+ RightDir * (BaseDir.Y * Radius);

	const float Jitter = 240.f;
	DesiredLocation += ForwardDir * FMath::FRandRange(-Jitter, Jitter);
	DesiredLocation += RightDir * FMath::FRandRange(-Jitter, Jitter);

	UStaticMesh* MeshForType = BlueUnitMeshes.IsValidIndex(SafeType) ? BlueUnitMeshes[SafeType].Get() : ResolveBlueUnitMeshByType(SafeType);
	if (!MeshForType)
	{
		MeshForType = ResolveBlueUnitMeshByType(0);
	}

	UMaterialInterface* MaterialForType = BlueUnitMaterials.IsValidIndex(SafeType) ? BlueUnitMaterials[SafeType].Get() : ResolveBlueUnitMaterialByType(SafeType);
	if (!MaterialForType)
	{
		MaterialForType = ResolveBlueUnitMaterialByType(0);
	}

	if (SpawnBlueUnitAtLocation(World, DesiredLocation, BlueUnitFacing, FString::Printf(TEXT("部署点%d"), MarkerIndex + 1), MeshForType, MaterialForType, ActiveCountermeasureIndices))
	{
		if (BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex))
		{
			BlueMarkerSpawnCounts[MarkerIndex] += 1;
		}

		UE_LOG(LogTemp, Log, TEXT("SpawnBlueUnitAtMarker: placed unit type %d at marker %s"), SafeType + 1, *Marker->GetName());
		RefreshBlueMonitor();
	}
}

void UScenarioMenuSubsystem::ReturnCameraToInitial()
{
	if (!bHasOverviewHome && !bHasInitialCameraTransform)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		EnsureOverviewPawn(World);

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ASpectatorPawn* Pawn = OverviewPawn.Get())
			{
				Pawn->SetActorLocation(OverviewHomeLocation);
				Pawn->SetActorRotation(OverviewHomeRotation);
				PC->SetControlRotation(OverviewHomeRotation);
			}
			else if (bHasInitialCameraTransform)
			{
				if (APawn* ControlledPawn = PC->GetPawn())
				{
					ControlledPawn->SetActorLocation(InitialCameraLocation);
					ControlledPawn->SetActorRotation(InitialCameraRotation);
					PC->SetControlRotation(InitialCameraRotation);
				}
			}
		}
	}
	StopMissileCameraFollow();
	CurrentViewIndex = 0;
}

void UScenarioMenuSubsystem::EnsureOverviewPawn(UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (!PreviousPawn.IsValid())
		{
			PreviousPawn = PC->GetPawn();
		}

	ASpectatorPawn* Pawn = OverviewPawn.Get();
	if (!Pawn || Pawn->IsPendingKillPending())
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Pawn = World->SpawnActor<ASpectatorPawn>(OverviewHomeLocation, OverviewHomeRotation, Params);
			if (Pawn)
			{
				OverviewPawn = Pawn;
			}
		}

		if (Pawn)
		{
			Pawn->SetActorLocation(OverviewHomeLocation);
			Pawn->SetActorRotation(OverviewHomeRotation);
			PC->Possess(Pawn);
			PC->SetControlRotation(OverviewHomeRotation);
		}
	}
}

void UScenarioMenuSubsystem::RestorePreviousPawn()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PreviousPawn.IsValid())
			{
				APawn* Pawn = PreviousPawn.Get();
				PC->Possess(Pawn);
				PC->SetControlRotation(Pawn->GetActorRotation());
			}
		}
	}

	if (ASpectatorPawn* Pawn = OverviewPawn.Get())
	{
		if (!Pawn->IsPendingKillPending())
		{
			Pawn->Destroy();
		}
	}

	OverviewPawn.Reset();
	PreviousPawn.Reset();
	bHasOverviewHome = false;
}

bool UScenarioMenuSubsystem::SpawnBlueUnitAtLocation(UWorld* World, const FVector& DesiredLocation, const FRotator& Facing, const FString& MarkerName, UStaticMesh* UnitMesh, UMaterialInterface* UnitMaterial, const TArray<int32>& CountermeasureIndices)
{
	if (!World || !UnitMesh)
	{
		return false;
	}

	FVector SpawnLocation = DesiredLocation;

	const FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, 6000.f);
	const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 12000.f);
	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false);
	Params.bTraceComplex = true;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	if (bHit && Hit.bBlockingHit)
	{
		SpawnLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 30.f);
	}
	else if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation ProjectedNav;
		if (NavSys->ProjectPointToNavigation(SpawnLocation, ProjectedNav, FVector(400.f, 400.f, 1200.f)))
		{
			SpawnLocation = ProjectedNav.Location + FVector(0.f, 0.f, 30.f);
		}
	}

	const FString MarkerLabel = MarkerName.IsEmpty() ? TEXT("ManualLocation") : MarkerName;
	UE_LOG(LogTemp, Log, TEXT("Placing unit near %s -> %s"), *MarkerLabel, *SpawnLocation.ToString());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AStaticMeshActor* Spawned = World->SpawnActor<AStaticMeshActor>(SpawnLocation, Facing, SpawnParams);
	if (!Spawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn blue unit at %s"), *SpawnLocation.ToString());
		return false;
	}

	if (UStaticMeshComponent* MeshComp = Spawned->GetStaticMeshComponent())
	{
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetStaticMesh(UnitMesh);
		MeshComp->SetRelativeScale3D(FVector(1.0f));
		MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
		MeshComp->SetVisibility(true, true);
		MeshComp->SetHiddenInGame(false);
		if (UnitMaterial)
		{
			MeshComp->SetMaterial(0, UnitMaterial);
		}
	}

	Spawned->Tags.AddUnique(FName(TEXT("BlueUnit")));
	for (int32 Countermeasure : CountermeasureIndices)
	{
		switch (Countermeasure)
		{
		case 0: Spawned->Tags.AddUnique(FName(TEXT("Blue_ECM"))); break;
		case 1: Spawned->Tags.AddUnique(FName(TEXT("Blue_CommJamming"))); break;
		case 2: Spawned->Tags.AddUnique(FName(TEXT("Blue_TargetMobility"))); break;
		default: break;
		}
	}

	ActiveBlueUnits.Add(Spawned);
	UE_LOG(LogTemp, Log, TEXT("Blue unit spawned at %s"), *SpawnLocation.ToString());
	return true;
}

bool UScenarioMenuSubsystem::FireSingleMissile(bool bFromAutoFire /*= false*/)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	CleanupMissiles();
	RebuildViewSequence();

	AActor* Target = SelectNextBlueTarget();
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireSingleMissile: no valid blue unit target available."));
		return false;
	}

	AMockMissileActor* Missile = SpawnMissile(World, Target, bFromAutoFire);
	if (!Missile)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireSingleMissile: failed to spawn missile."));
		return false;
	}

	return true;
}

void UScenarioMenuSubsystem::FireMultipleMissiles(int32 Count)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 SafeCount = FMath::Clamp(Count, 1, 20);
	BeginMissileAutoFire(SafeCount);
}

AMockMissileActor* UScenarioMenuSubsystem::SpawnMissile(UWorld* World, AActor* Target, bool bFromAutoFire)
{
	if (!World || !Target)
	{
		return nullptr;
	}

	UStaticMesh* MissileMesh = ResolveMissileMesh();
	UMaterialInterface* MissileMaterial = ResolveMissileMaterial();

	FRotator Facing;
	const FVector StartLocation = GetPlayerStartLocation(Facing);
	FVector SpawnLocation = StartLocation + Facing.Vector() * 120.f + FVector(0.f, 0.f, 120.f);
	const FVector TargetDirection = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
	const FRotator LaunchRotation = TargetDirection.IsNearlyZero() ? Facing : TargetDirection.Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMockMissileActor* Missile = World->SpawnActor<AMockMissileActor>(SpawnLocation, LaunchRotation, Params);
	if (!Missile)
	{
		return nullptr;
	}

	if (MissileMesh || MissileMaterial)
	{
		Missile->SetupAppearance(MissileMesh, MissileMaterial, FLinearColor(1.f, 0.1f, 0.1f));
	}

	Missile->InitializeMissile(Target, 4500.f, 45.f);
	Missile->OnImpact.AddUObject(this, &UScenarioMenuSubsystem::HandleMissileImpact);
	Missile->OnExpired.AddUObject(this, &UScenarioMenuSubsystem::HandleMissileExpired);

	ActiveMissiles.Add(Missile);
	RecordMissileLaunch(Missile, Target, SpawnLocation, bFromAutoFire);

	UE_LOG(LogTemp, Log, TEXT("SpawnMissile: launched missile toward %s from %s"), *Target->GetName(), *SpawnLocation.ToString());
	RebuildViewSequence();
	return Missile;
}

AActor* UScenarioMenuSubsystem::SelectNextBlueTarget()
{
	ActiveBlueUnits.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });

	const int32 UnitCount = ActiveBlueUnits.Num();
	if (UnitCount == 0)
	{
		return nullptr;
	}

	NextTargetCursor = NextTargetCursor % UnitCount;

	for (int32 Attempt = 0; Attempt < UnitCount; ++Attempt)
	{
		const int32 Index = (NextTargetCursor + Attempt) % UnitCount;
		if (AActor* Candidate = ActiveBlueUnits[Index].Get())
		{
			NextTargetCursor = (Index + 1) % UnitCount;
			return Candidate;
		}
	}

	return nullptr;
}

void UScenarioMenuSubsystem::HandleMissileImpact(AMockMissileActor* Missile, AActor* HitActor)
{
	const FVector ExplosionLocation = Missile && !Missile->IsPendingKillPending() ? Missile->GetActorLocation()
		: (HitActor ? HitActor->GetActorLocation() : FVector::ZeroVector);

	const float ExplosionRadius = 4500.f; // 最初900.f的五倍
	int32 DestroyedCount = 0;
	
	// 在销毁目标之前保存目标名称，用于后续判断直接命中
	FString HitActorName = HitActor ? HitActor->GetName() : FString();

	if (HitActor && !HitActor->IsPendingKillPending() && ActiveBlueUnits.Contains(HitActor))
	{
		UE_LOG(LogTemp, Log, TEXT("HandleMissileImpact: missile direct-hit %s"), *HitActor->GetName());
		HitActor->Destroy();
		++DestroyedCount;
	}

	TArray<AActor*> UnitsToRemove;

	for (TWeakObjectPtr<AActor>& UnitPtr : ActiveBlueUnits)
	{
		if (!UnitPtr.IsValid())
		{
			continue;
		}

		AActor* Unit = UnitPtr.Get();
		if (Unit == HitActor || Unit->IsPendingKillPending())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(ExplosionLocation, Unit->GetActorLocation());
		if (DistanceSq <= FMath::Square(ExplosionRadius))
		{
			UE_LOG(LogTemp, Log, TEXT("HandleMissileImpact: AoE destroyed %s (distance %.1f)"), *Unit->GetName(), FMath::Sqrt(DistanceSq));
			Unit->Destroy();
			++DestroyedCount;
			UnitsToRemove.Add(Unit);
		}
	}

	ActiveBlueUnits.RemoveAll([&UnitsToRemove](const TWeakObjectPtr<AActor>& Ptr)
	{
		if (!Ptr.IsValid())
		{
			return true;
		}

		if (Ptr->IsPendingKillPending())
		{
			return true;
		}

		return UnitsToRemove.Contains(Ptr.Get());
	});

	UpdateMissileRecordOnImpact(Missile, HitActor, DestroyedCount, HitActorName);

	if (DestroyedCount == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("HandleMissileImpact: missile detonated without destroying any blue unit. HitActor=%s"), HitActor ? *HitActor->GetName() : TEXT("None"));
	}

	if (ActiveBlueUnits.Num() > 0)
	{
		NextTargetCursor = NextTargetCursor % ActiveBlueUnits.Num();
	}
	else
	{
		NextTargetCursor = 0;
	}

	const bool bWasCameraTarget = MissileCameraTarget.Get() == Missile;

	ActiveMissiles.RemoveAll([Missile](const TWeakObjectPtr<AMockMissileActor>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == Missile;
	});

	if (bWasCameraTarget)
	{
		StopMissileCameraFollow();
		if (ActiveMissiles.Num() > 0)
		{
			FocusCameraOnMissile(ActiveMissiles.Last().Get());
		}
		else
		{
			FocusInitialView();
		}
	}

	RebuildViewSequence();
	RefreshBlueMonitor();
}

void UScenarioMenuSubsystem::HandleMissileExpired(AMockMissileActor* Missile)
{
	const bool bWasCameraTarget = MissileCameraTarget.Get() == Missile;

	UpdateMissileRecordOnExpired(Missile);

	ActiveMissiles.RemoveAll([Missile](const TWeakObjectPtr<AMockMissileActor>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == Missile;
	});

	if (ActiveBlueUnits.Num() > 0)
	{
		NextTargetCursor = NextTargetCursor % ActiveBlueUnits.Num();
	}
	else
	{
		NextTargetCursor = 0;
	}

	if (bWasCameraTarget)
	{
		StopMissileCameraFollow();
		if (ActiveMissiles.Num() > 0)
		{
			FocusCameraOnMissile(ActiveMissiles.Last().Get());
		}
		else
		{
			FocusInitialView();
		}
	}

	RebuildViewSequence();
}

void UScenarioMenuSubsystem::CleanupMissiles()
{
	ActiveMissiles.RemoveAll([](const TWeakObjectPtr<AMockMissileActor>& Ptr)
	{
		return !Ptr.IsValid();
	});
}

UStaticMesh* UScenarioMenuSubsystem::ResolveMissileMesh() const
{
	if (!CachedMissileMesh.IsValid())
	{
		FSoftObjectPath MeshPath(TEXT("/Game/Sayantan/Props/Small/CruiseMissile_FREE/Models/SM_CruiseMissile.SM_CruiseMissile"));
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(MeshPath.TryLoad()))
		{
			CachedMissileMesh = Mesh;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveMissileMesh: failed to load SM_CruiseMissile mesh."));
			return nullptr;
		}
	}

	return CachedMissileMesh.Get();
}

UMaterialInterface* UScenarioMenuSubsystem::ResolveMissileMaterial() const
{
	if (!CachedMissileMaterial.IsValid())
	{
		FSoftObjectPath MaterialPath(TEXT("/Game/StarterContent/Materials/M_Glow.M_Glow"));
		if (UMaterialInterface* Material = Cast<UMaterialInterface>(MaterialPath.TryLoad()))
		{
			CachedMissileMaterial = Material;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveMissileMaterial: failed to load BasicShapeMaterial."));
			return nullptr;
		}
	}

	return CachedMissileMaterial.Get();
}

FVector UScenarioMenuSubsystem::GetPlayerStartLocation(FRotator& OutRotation) const
{
	OutRotation = FRotator::ZeroRotator;

	if (UWorld* World = GetWorld())
	{
		if (APlayerStart* PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass())))
		{
			OutRotation = PlayerStart->GetActorRotation();
			return PlayerStart->GetActorLocation();
		}

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				OutRotation = Pawn->GetActorRotation();
				return Pawn->GetActorLocation();
			}
		}
	}

	return FVector::ZeroVector;
}

void UScenarioMenuSubsystem::BeginMissileAutoFire(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
		AutoFireRemaining = Count;
		HandleAutoFireTick();
	}
}

void UScenarioMenuSubsystem::HandleAutoFireTick()
{
	if (AutoFireRemaining <= 0)
	{
		ClearAutoFire();
		return;
	}

	if (!FireSingleMissile(true))
	{
		AutoFireRemaining = 0;
		ClearAutoFire();
		return;
	}

	AutoFireRemaining = FMath::Max(0, AutoFireRemaining - 1);

	if (AutoFireRemaining > 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(AutoFireTimerHandle, this, &UScenarioMenuSubsystem::HandleAutoFireTick, 0.6f, false);
		}
	}
}

void UScenarioMenuSubsystem::SetupInputBindings(UWorld* World)
{
	if (!World || bInputComponentPushed)
	{
		return;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (!MissileInputComponent)
		{
			MissileInputComponent = NewObject<UInputComponent>(PC, TEXT("MissileInputComponent"));
			MissileInputComponent->RegisterComponent();
			MissileInputComponent->Priority = 10;
			MissileInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UScenarioMenuSubsystem::OnInputFireMissile);
			MissileInputComponent->BindKey(EKeys::P, IE_Pressed, this, &UScenarioMenuSubsystem::OnInputFinishMissileTest);
			MissileInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &UScenarioMenuSubsystem::OnInputCycleBackward);
			MissileInputComponent->BindKey(EKeys::E, IE_Pressed, this, &UScenarioMenuSubsystem::OnInputCycleForward);
		}

		if (MissileInputComponent)
		{
			PC->PushInputComponent(MissileInputComponent);
			bInputComponentPushed = true;
		}
	}
}

void UScenarioMenuSubsystem::RemoveInputBindings()
{
	if (!bInputComponentPushed)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (MissileInputComponent)
			{
				PC->PopInputComponent(MissileInputComponent);
			}
		}
	}

	bInputComponentPushed = false;
}

void UScenarioMenuSubsystem::LaunchMissileFromUI()
{
	FireSingleMissile();
}

void UScenarioMenuSubsystem::OnInputFireMissile()
{
	FireSingleMissile();
}

void UScenarioMenuSubsystem::OnInputFinishMissileTest()
{
	CompleteMissileTest();
	FocusInitialView();
	RemoveMissileOverlay();
	bIsRunningScenario = false;

	if (!Screen.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			Show(World);
			// 确保回到发起测试的 Tab
			if (Screen.IsValid())
			{
				Screen->SetActiveTabIndex(ReturnTabIndex);
			}
		}
	}

	StepIndex = 4;
	if (Screen.IsValid())
	{
		// 如果界面仍然存在，直接切到对应 Tab 再跳到 Step5
		Screen->SetActiveTabIndex(ReturnTabIndex);
		Screen->SetStepIndex(StepIndex);
	}
}

void UScenarioMenuSubsystem::OnInputCycleForward()
{
	CycleViewForward();
}

void UScenarioMenuSubsystem::OnInputCycleBackward()
{
	CycleViewBackward();
}

void UScenarioMenuSubsystem::CycleViewForward()
{
	RebuildViewSequence();
	if (ViewEntries.Num() == 0)
	{
		return;
	}

	CurrentViewIndex = (CurrentViewIndex + 1) % ViewEntries.Num();
	FocusViewAtIndex(CurrentViewIndex);
}

void UScenarioMenuSubsystem::CycleViewBackward()
{
	RebuildViewSequence();
	if (ViewEntries.Num() == 0)
	{
		return;
	}

	CurrentViewIndex = (CurrentViewIndex - 1 + ViewEntries.Num()) % ViewEntries.Num();
	FocusViewAtIndex(CurrentViewIndex);
}

void UScenarioMenuSubsystem::FocusInitialView()
{
	ReturnCameraToInitial();
	CurrentViewIndex = 0;
	bMissileCameraHasHistory = false;
	MissileCameraSmoothedLocation = FVector::ZeroVector;
	MissileCameraSmoothedRotation = FRotator::ZeroRotator;
}

void UScenarioMenuSubsystem::FocusViewAtIndex(int32 ViewIndex)
{
	if (!ViewEntries.IsValidIndex(ViewIndex))
	{
		FocusInitialView();
		return;
	}

	CurrentViewIndex = ViewIndex;
	const FViewEntry& Entry = ViewEntries[ViewIndex];

	switch (Entry.Type)
	{
	case FViewEntry::EType::Initial:
		FocusInitialView();
		break;
	case FViewEntry::EType::BlueUnit:
		if (ActiveBlueUnits.IsValidIndex(Entry.Index) && ActiveBlueUnits[Entry.Index].IsValid())
		{
			FocusCameraOnUnit(Entry.Index);
		}
		else
		{
			FocusInitialView();
		}
		break;
	case FViewEntry::EType::Missile:
		if (ActiveMissiles.IsValidIndex(Entry.Index) && ActiveMissiles[Entry.Index].IsValid())
		{
			FocusCameraOnMissile(ActiveMissiles[Entry.Index].Get());
		}
		else
		{
			FocusInitialView();
		}
		break;
	default:
		FocusInitialView();
		break;
	}
}

void UScenarioMenuSubsystem::FocusCameraOnMissile(AMockMissileActor* Missile)
{
	if (!Missile)
	{
		FocusInitialView();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	StopMissileCameraFollow();
	MissileCameraTarget = Missile;
	bMissileCameraHasHistory = false;
	MissileCameraSmoothedLocation = FVector::ZeroVector;
	MissileCameraSmoothedRotation = FRotator::ZeroRotator;
	for (int32 ViewIdx = 0; ViewIdx < ViewEntries.Num(); ++ViewIdx)
	{
		const FViewEntry& Entry = ViewEntries[ViewIdx];
		if (Entry.Type == FViewEntry::EType::Missile && ActiveMissiles.IsValidIndex(Entry.Index) && ActiveMissiles[Entry.Index].Get() == Missile)
		{
			CurrentViewIndex = ViewIdx;
			break;
		}
	}
	// 初始化导弹相机：Pawn 位置跟随导弹，但玩家完全自由控制视角
	ASpectatorPawn* Pawn = OverviewPawn.Get();
	APlayerController* PC = World->GetFirstPlayerController();
	
	if (!Pawn || Pawn->IsPendingKillPending())
	{
		if (PC)
		{
			if (!PreviousPawn.IsValid())
			{
				PreviousPawn = PC->GetPawn();
			}
			
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Pawn = World->SpawnActor<ASpectatorPawn>(Missile->GetActorLocation(), FRotator::ZeroRotator, Params);
			if (Pawn)
			{
				OverviewPawn = Pawn;
				// 直接 Possess，不设置任何旋转，让玩家完全自由控制
				PC->Possess(Pawn);
			}
		}
	}
	else if (PC && PC->GetPawn() != Pawn)
	{
		// 直接 Possess，不设置任何旋转，让玩家完全自由控制
		PC->Possess(Pawn);
	}
	
	if (Pawn && PC)
	{
		// 只在初始化时设置一次位置和初始旋转（相机向上平移，视角向下看）
		if (!bMissileCameraControlInitialized)
		{
			const FVector MissileLocation = Missile->GetActorLocation();
			// 相机位置：在导弹位置基础上向上平移（比如向上 200 单位）
			const FVector CameraLocation = MissileLocation + FVector(0.f, 0.f, 200.f);
			Pawn->SetActorLocation(CameraLocation);
			
			// 初始旋转：从上方位置向下看导弹（计算从相机位置看向导弹的旋转）
			const FVector LookDirection = (MissileLocation - CameraLocation).GetSafeNormal();
			const FRotator InitialRotation = LookDirection.Rotation();
			PC->SetControlRotation(InitialRotation);
			bMissileCameraControlInitialized = true;
		}
	}
	
	// 启动定时器：只更新位置（跟随导弹）和识别框，不更新旋转
	EnsureMissileOverlay();
	UpdateMissileOverlay();
	World->GetTimerManager().SetTimer(MissileCameraTimerHandle, this, &UScenarioMenuSubsystem::UpdateMissileCamera, 0.016f, true);
}

void UScenarioMenuSubsystem::StopMissileCameraFollow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MissileCameraTimerHandle);
	}
	MissileCameraTarget = nullptr;
	bMissileCameraHasHistory = false;
	bMissileCameraControlInitialized = false;
	MissileCameraSmoothedLocation = FVector::ZeroVector;
	MissileCameraSmoothedRotation = FRotator::ZeroRotator;
	RemoveMissileOverlay();
}

void UScenarioMenuSubsystem::UpdateMissileCamera()
{
	if (!MissileCameraTarget.IsValid())
	{
		StopMissileCameraFollow();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		StopMissileCameraFollow();
		return;
	}

	AMockMissileActor* Missile = MissileCameraTarget.Get();
	if (!Missile)
	{
		StopMissileCameraFollow();
		return;
	}

	ASpectatorPawn* Pawn = OverviewPawn.Get();
	if (!Pawn)
	{
		StopMissileCameraFollow();
		return;
	}

	// 只更新位置，让 Pawn 跟随导弹移动（相机在导弹上方，保持相对位置）
	// 完全不更新旋转，让玩家完全自由控制视角
	const FVector MissileLocation = Missile->GetActorLocation();
	// 相机位置：在导弹位置基础上向上平移（保持相对高度）
	const FVector DesiredCameraLocation = MissileLocation + FVector(0.f, 0.f, 200.f);
	
	// 使用平滑插值更新位置，避免突然跳跃
	const float DeltaTime = FMath::Max(World->GetDeltaSeconds(), KINDA_SMALL_NUMBER);
	const float LocationInterpSpeed = 10.f; // 较高的插值速度，确保紧密跟随
	
	if (!bMissileCameraHasHistory)
	{
		Pawn->SetActorLocation(DesiredCameraLocation);
		bMissileCameraHasHistory = true;
	}
	else
	{
		const FVector CurrentLocation = Pawn->GetActorLocation();
		const FVector NewLocation = FMath::VInterpTo(CurrentLocation, DesiredCameraLocation, DeltaTime, LocationInterpSpeed);
		// 使用 Teleport 模式避免触发碰撞事件，减少对玩家输入的干扰
		Pawn->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 更新识别框
	UpdateMissileOverlay();
}

void UScenarioMenuSubsystem::RebuildViewSequence()
{
	ViewEntries.Reset();

	FViewEntry InitialEntry;
	InitialEntry.Type = FViewEntry::EType::Initial;
	InitialEntry.Index = INDEX_NONE;
	ViewEntries.Add(InitialEntry);

	ActiveBlueUnits.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr)
	{
		return !Ptr.IsValid();
	});

	for (int32 UnitIndex = 0; UnitIndex < ActiveBlueUnits.Num(); ++UnitIndex)
	{
		if (ActiveBlueUnits[UnitIndex].IsValid())
		{
			FViewEntry Entry;
			Entry.Type = FViewEntry::EType::BlueUnit;
			Entry.Index = UnitIndex;
			ViewEntries.Add(Entry);
		}
	}

	ActiveMissiles.RemoveAll([](const TWeakObjectPtr<AMockMissileActor>& Ptr)
	{
		return !Ptr.IsValid();
	});

	for (int32 MissileIndex = 0; MissileIndex < ActiveMissiles.Num(); ++MissileIndex)
	{
		if (ActiveMissiles[MissileIndex].IsValid())
		{
			FViewEntry Entry;
			Entry.Type = FViewEntry::EType::Missile;
			Entry.Index = MissileIndex;
			ViewEntries.Add(Entry);
		}
	}

	if (!ViewEntries.IsValidIndex(CurrentViewIndex))
	{
		CurrentViewIndex = 0;
	}
}

void UScenarioMenuSubsystem::EnsureMissileOverlay()
{
	if (MissileOverlayWidget.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (UMissileOverlayWidget* Overlay = CreateWidget<UMissileOverlayWidget>(PC, UMissileOverlayWidget::StaticClass()))
		{
			Overlay->AddToViewport(450);
			MissileOverlayWidget = Overlay;
		}
	}
}

void UScenarioMenuSubsystem::RemoveMissileOverlay()
{
	if (MissileOverlayWidget.IsValid())
	{
		MissileOverlayWidget->RemoveFromParent();
		MissileOverlayWidget = nullptr;
	}
}

void UScenarioMenuSubsystem::UpdateMissileOverlay()
{
	if (!MissileOverlayWidget.IsValid())
	{
		return;
	}

	if (!MissileCameraTarget.IsValid())
	{
		MissileOverlayWidget->SetTargets({});
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		MissileOverlayWidget->SetTargets({});
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		MissileOverlayWidget->SetTargets({});
		return;
	}

	AMockMissileActor* Missile = MissileCameraTarget.Get();
	if (!Missile)
	{
		MissileOverlayWidget->SetTargets({});
		return;
	}

	const FVector CameraLocation = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : Missile->GetActorLocation();
	const FVector CameraForward = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Vector() : Missile->GetActorForwardVector();

	FVector NormalizedForward = CameraForward;
	if (!NormalizedForward.Normalize())
	{
		NormalizedForward = FVector::ForwardVector;
	}

	const FVector MissileLocation = Missile->GetActorLocation();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(65.f));
	const float MaxDistance = 50000.f;

	// 获取导弹的目标（如果有）
	AActor* MissileTarget = nullptr;
	if (int32* IndexPtr = MissileRecordLookup.Find(Missile))
	{
		if (MissileTestRecords.IsValidIndex(*IndexPtr))
		{
			const FMissileTestRecord& Record = MissileTestRecords[*IndexPtr];
			if (Record.TargetActor.IsValid())
			{
				MissileTarget = Record.TargetActor.Get();
			}
		}
	}

	TArray<FMissileOverlayTarget> OverlayTargets;

	// 优先处理导弹的目标
	TArray<TWeakObjectPtr<AActor>> SortedTargets;
	if (MissileTarget && ActiveBlueUnits.Contains(MissileTarget))
	{
		SortedTargets.Add(MissileTarget);
	}
	for (const TWeakObjectPtr<AActor>& TargetPtr : ActiveBlueUnits)
	{
		if (TargetPtr.IsValid() && TargetPtr.Get() != MissileTarget)
		{
			SortedTargets.Add(TargetPtr);
		}
	}

	for (const TWeakObjectPtr<AActor>& TargetPtr : SortedTargets)
	{
		if (!TargetPtr.IsValid())
		{
			continue;
		}

		AActor* Target = TargetPtr.Get();
		FVector BoundsOrigin;
		FVector BoundsExtent;
		Target->GetActorBounds(true, BoundsOrigin, BoundsExtent);

		// 使用目标中心点进行视线检测
		const FVector TargetCenter = BoundsOrigin;
		const FVector ToTarget = TargetCenter - CameraLocation;
		const float Distance = ToTarget.Size();
		if (Distance <= KINDA_SMALL_NUMBER || Distance > MaxDistance)
		{
			continue;
		}

		const FVector Direction = ToTarget / Distance;
		if (FVector::DotProduct(Direction, NormalizedForward) < CosHalfAngle)
		{
			continue;
		}

		// 视线检测：检查目标是否被遮挡（使用多个关键点进行检测）
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Missile);
		QueryParams.AddIgnoredActor(Target);
		QueryParams.bTraceComplex = false;
		QueryParams.bReturnPhysicalMaterial = false;

		// 检测目标中心点和几个关键角点是否可见
		bool bIsVisible = false;
		const FVector TraceStart = CameraLocation;
		
		// 首先检测中心点
		FHitResult HitResult;
		if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TargetCenter, ECC_Visibility, QueryParams) || HitResult.GetActor() == Target)
		{
			bIsVisible = true;
		}
		else
		{
			// 如果中心点被遮挡，检测几个关键角点
			const FVector KeyPoints[4] = {
				BoundsOrigin + FVector(BoundsExtent.X, 0.f, 0.f),  // 右
				BoundsOrigin + FVector(-BoundsExtent.X, 0.f, 0.f), // 左
				BoundsOrigin + FVector(0.f, 0.f, BoundsExtent.Z),  // 上
				BoundsOrigin + FVector(0.f, 0.f, -BoundsExtent.Z)  // 下
			};
			
			for (const FVector& KeyPoint : KeyPoints)
			{
				FHitResult KeyHitResult;
				if (!World->LineTraceSingleByChannel(KeyHitResult, TraceStart, KeyPoint, ECC_Visibility, QueryParams) || KeyHitResult.GetActor() == Target)
				{
					bIsVisible = true;
					break;
				}
			}
		}
		
		// 如果目标完全被遮挡，则跳过
		if (!bIsVisible)
		{
			continue;
		}

		// 使用3D标记：在目标上直接绘制明显的3D标记，避免屏幕投影误差
		// 在目标中心上方绘制一个明显的标记
		const FVector MarkerLocation = TargetCenter + FVector(0.f, 0.f, BoundsExtent.Z + 100.f);
		
		// 检查标记位置是否可见
		FHitResult MarkerHitResult;
		if (World->LineTraceSingleByChannel(MarkerHitResult, CameraLocation, MarkerLocation, ECC_Visibility, QueryParams))
		{
			if (MarkerHitResult.GetActor() != Target)
			{
				continue; // 标记位置被遮挡
			}
		}
		
		// 在3D世界中绘制明显的标记
		const FColor MarkerColor = (Target == MissileTarget) ? FColor::Cyan : FColor(0, 217, 255); // 青色
		const float MarkerSize = 80.f; // 更大的标记球体
		const float LineThickness = 5.f; // 更粗的线
		
		// 绘制一个大的球体标记（实心）
		DrawDebugSphere(World, MarkerLocation, MarkerSize, 16, MarkerColor, false, -1.f, 0, LineThickness);
		
		// 绘制一个内部的小球体，形成双层效果
		DrawDebugSphere(World, MarkerLocation, MarkerSize * 0.6f, 16, MarkerColor, false, -1.f, 0, LineThickness * 0.8f);
		
		// 绘制一条粗线从目标中心到标记
		DrawDebugLine(World, TargetCenter, MarkerLocation, MarkerColor, false, -1.f, 0, LineThickness);
		
		// 绘制目标包围盒轮廓（更粗的线）
		DrawDebugBox(World, TargetCenter, BoundsExtent, MarkerColor, false, -1.f, 0, LineThickness);
		
		// 在目标四个角绘制小标记
		const float CornerMarkerSize = 20.f;
		const FVector Corners[4] = {
			TargetCenter + FVector(BoundsExtent.X, BoundsExtent.Y, 0.f),
			TargetCenter + FVector(BoundsExtent.X, -BoundsExtent.Y, 0.f),
			TargetCenter + FVector(-BoundsExtent.X, BoundsExtent.Y, 0.f),
			TargetCenter + FVector(-BoundsExtent.X, -BoundsExtent.Y, 0.f)
		};
		for (const FVector& Corner : Corners)
		{
			DrawDebugSphere(World, Corner, CornerMarkerSize, 8, MarkerColor, false, -1.f, 0, 2.f);
		}
	}

	MissileOverlayWidget->SetTargets(OverlayTargets);
}

void UScenarioMenuSubsystem::RecordMissileLaunch(AMockMissileActor* Missile, AActor* Target, const FVector& LaunchLocation, bool bFromAutoFire)
{
	if (!Missile)
	{
		return;
	}

	FMissileTestRecord Record;
	Record.LaunchTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
	Record.bAutoFire = bFromAutoFire;
	Record.TargetActor = Target;
	Record.TargetName = Target ? Target->GetName() : TEXT("未指定");
	Record.LaunchLocation = LaunchLocation;
	Record.TargetLocation = Target ? Target->GetActorLocation() : FVector::ZeroVector;
	Record.InitialDistance = Target ? FVector::Dist(LaunchLocation, Record.TargetLocation) : 0.f;

	const int32 Index = MissileTestRecords.Add(Record);
	MissileRecordLookup.Add(Missile, Index);
}

void UScenarioMenuSubsystem::UpdateMissileRecordOnImpact(AMockMissileActor* Missile, AActor* HitActor, int32 DestroyedCount, const FString& HitActorName)
{
	if (!Missile)
	{
		return;
	}

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();

	if (int32* IndexPtr = MissileRecordLookup.Find(Missile))
	{
		if (MissileTestRecords.IsValidIndex(*IndexPtr))
		{
			FMissileTestRecord& Record = MissileTestRecords[*IndexPtr];
			Record.ImpactTimeSeconds = CurrentTime;
			Record.bImpactRegistered = true;
			Record.DestroyedCount = DestroyedCount;
			Record.bExpired = false;
			
			// 使用目标名称比较来判断直接命中，因为目标可能已经被销毁导致指针失效
			const FString ActualHitActorName = !HitActorName.IsEmpty() ? HitActorName : (HitActor ? HitActor->GetName() : FString());
			Record.bDirectHit = !ActualHitActorName.IsEmpty() && !Record.TargetName.IsEmpty() && ActualHitActorName == Record.TargetName;
			
			const bool bTargetPointerValid = Record.TargetActor.IsValid();
			Record.bTargetDestroyed = (Record.bDirectHit || (DestroyedCount > 0 && !bTargetPointerValid));
		}
		MissileRecordLookup.Remove(Missile);
	}
}

void UScenarioMenuSubsystem::UpdateMissileRecordOnExpired(AMockMissileActor* Missile)
{
	if (!Missile)
	{
		return;
	}

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();

	if (int32* IndexPtr = MissileRecordLookup.Find(Missile))
	{
		if (MissileTestRecords.IsValidIndex(*IndexPtr))
		{
			FMissileTestRecord& Record = MissileTestRecords[*IndexPtr];
			Record.ImpactTimeSeconds = CurrentTime;
			Record.bImpactRegistered = false;
			Record.bExpired = true;
			Record.DestroyedCount = 0;
			Record.bDirectHit = false;
			Record.bTargetDestroyed = Record.TargetActor.IsValid() ? !Record.TargetActor.IsValid() : false;
		}
		MissileRecordLookup.Remove(Missile);
	}
}

void UScenarioMenuSubsystem::ResetMissileTestSession()
{
	MissileTestRecords.Reset();
	MissileRecordLookup.Reset();
	LastMissileSummary = FMissileTestSummary();
	TestSessionStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
}

void UScenarioMenuSubsystem::ClearAutoFire()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}
	AutoFireRemaining = 0;
}

void UScenarioMenuSubsystem::CompleteMissileTest()
{
	// 标记仍在飞行的导弹为过期，避免缺失数据
	if (MissileRecordLookup.Num() > 0)
	{
		TArray<TWeakObjectPtr<AMockMissileActor>> PendingMissiles;
		MissileRecordLookup.GetKeys(PendingMissiles);
		for (const TWeakObjectPtr<AMockMissileActor>& MissilePtr : PendingMissiles)
		{
			if (MissilePtr.IsValid())
			{
				UpdateMissileRecordOnExpired(MissilePtr.Get());
			}
		}
		MissileRecordLookup.Reset();
	}

	ClearAutoFire();

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();

	LastMissileSummary = FMissileTestSummary();
	LastMissileSummary.SessionDuration = FMath::Max(0.f, static_cast<float>(CurrentTime - TestSessionStartTime));

	if (MissileTestRecords.Num() == 0)
	{
		return;
	}

	LastMissileSummary.TotalShots = MissileTestRecords.Num();
	double TotalFlightTime = 0.0;
	int32 FlightSamples = 0;
	double TotalDistance = 0.0;
	double TotalDestroyed = 0.0;

	double LastAutoLaunchTime = -1.0;
	double AutoIntervalAccumulator = 0.0;
	int32 AutoIntervals = 0;

	int32 MissCount = 0;

	for (const FMissileTestRecord& Record : MissileTestRecords)
	{
		if (Record.bAutoFire)
		{
			++LastMissileSummary.AutoShots;
			if (LastAutoLaunchTime >= 0.0)
			{
				AutoIntervalAccumulator += Record.LaunchTimeSeconds - LastAutoLaunchTime;
				++AutoIntervals;
			}
			LastAutoLaunchTime = Record.LaunchTimeSeconds;
		}
		else
		{
			++LastMissileSummary.ManualShots;
		}

		TotalDistance += Record.InitialDistance;

		const float FlightTime = Record.GetFlightDuration();
		if (FlightTime > 0.f)
		{
			TotalFlightTime += FlightTime;
			++FlightSamples;
		}

		if (Record.DestroyedCount > 0)
		{
			++LastMissileSummary.Hits;
			if (Record.bDirectHit)
			{
				++LastMissileSummary.DirectHits;
			}
			else
			{
				++LastMissileSummary.AoEHits;
			}
			TotalDestroyed += Record.DestroyedCount;
		}
		else
		{
			++MissCount;
		}
	}

	LastMissileSummary.HitRate = LastMissileSummary.TotalShots > 0 ? (static_cast<float>(LastMissileSummary.Hits) * 100.f / LastMissileSummary.TotalShots) : 0.f;
	LastMissileSummary.DirectHitRate = LastMissileSummary.Hits > 0 ? (static_cast<float>(LastMissileSummary.DirectHits) * 100.f / LastMissileSummary.Hits) : 0.f;
	LastMissileSummary.AverageLaunchDistance = LastMissileSummary.TotalShots > 0 ? static_cast<float>(TotalDistance / LastMissileSummary.TotalShots) : 0.f;
	LastMissileSummary.AverageFlightTime = FlightSamples > 0 ? static_cast<float>(TotalFlightTime / FlightSamples) : 0.f;
	LastMissileSummary.AverageDestroyedPerHit = LastMissileSummary.Hits > 0 ? static_cast<float>(TotalDestroyed / LastMissileSummary.Hits) : 0.f;
	LastMissileSummary.AverageAutoLaunchInterval = AutoIntervals > 0 ? static_cast<float>(AutoIntervalAccumulator / AutoIntervals) : 0.f;
	LastMissileSummary.Misses = MissCount;

	UE_LOG(LogTemp, Log, TEXT("Missile test completed: Shots=%d Hits=%d HitRate=%.1f%%"),
		LastMissileSummary.TotalShots, LastMissileSummary.Hits, LastMissileSummary.HitRate);
}
