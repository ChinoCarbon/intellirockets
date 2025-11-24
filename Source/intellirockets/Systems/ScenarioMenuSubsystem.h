#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Systems/ScenarioTestMetrics.h"
#include "UI/SScenarioScreen.h"
#include "ScenarioMenuSubsystem.generated.h"

class SScenarioScreen;
class SBlueUnitMonitor;
class AMockMissileActor;
class UInputComponent;
class UMissileOverlayWidget;
struct FScenarioTestConfig;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnScenarioTestRequested, const FScenarioTestConfig&);

UCLASS()
class UScenarioMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	AMockMissileActor* SpawnMissile(UWorld* World, AActor* Target, bool bFromAutoFire, const FVector* OverrideLocation = nullptr, const FRotator* OverrideRotation = nullptr);

private:
	void OnWorldReady(UWorld* World, const UWorld::InitializationValues IVs);
	void Show(UWorld* World);
	void Hide(UWorld* World);

	// 按钮响应
	void Prev();
	void Next();
	void SaveAll();
	void BackToMainMenu();
	void BeginScenarioTest();
	void ApplyEnvironmentSettings(UWorld* World, const FScenarioTestConfig& Config);
	void DeployBlueForScenario(UWorld* World, const FScenarioTestConfig& Config);
	void ClearSpawnedBlueUnits();
	void SpawnRadarJammers(UWorld* World); // 生成雷达干扰区域
	void ClearRadarJammers(); // 清除雷达干扰区域
	void FinalizeScenarioAfterLoad();
	bool AreScenarioLevelsReady(UWorld* World) const;
	UClass* ResolveBlueUnitClass() const;
	class UStaticMesh* ResolveBlueUnitMesh() const;
	class UMaterialInterface* ResolveBlueUnitMaterial() const;
	class UStaticMesh* ResolveBlueUnitMeshByType(int32 TypeIndex) const;
	class UMaterialInterface* ResolveBlueUnitMaterialByType(int32 TypeIndex) const;
	void ShowBlueMonitor(UWorld* World);
	void HideBlueMonitor();
	void FocusCameraOnUnit(int32 Index);
	void FocusCameraOnMarker(int32 Index);
	void SpawnBlueUnitAtMarker(int32 MarkerIndex, int32 UnitType);
	void ReturnCameraToInitial();
	void EnsureOverviewPawn(UWorld* World);
	void RestorePreviousPawn();
	void RefreshBlueMonitor();
	bool SpawnBlueUnitAtLocation(UWorld* World, const FVector& DesiredLocation, const FRotator& Facing, const FString& MarkerName, class UStaticMesh* UnitMesh, class UMaterialInterface* UnitMaterial, const TArray<int32>& CountermeasureIndices);
	bool FireSingleMissile(bool bFromAutoFire = false);
	void FireMultipleMissiles(int32 Count);
	AActor* SelectNextBlueTarget();
	void HandleMissileImpact(AMockMissileActor* Missile, AActor* HitActor);
	void HandleMissileExpired(AMockMissileActor* Missile);
	void CleanupMissiles();
	class UStaticMesh* ResolveMissileMesh() const;
	class UMaterialInterface* ResolveMissileMaterial() const;
	FVector GetPlayerStartLocation(FRotator& OutRotation) const;
	void BeginMissileAutoFire(int32 Count);
	void HandleAutoFireTick();
	void SetupInputBindings(UWorld* World);
	void RemoveInputBindings();
	void LaunchMissileFromUI();
	void OnInputFireMissile();
	void OnInputFinishMissileTest();
	void OnInputCycleForward();
	void OnInputCycleBackward();
	void CycleViewForward();
	void CycleViewBackward();
	void FocusInitialView();
	void FocusViewAtIndex(int32 ViewIndex);
	void FocusCameraOnMissile(AMockMissileActor* Missile);
	void StopMissileCameraFollow();
	void UpdateMissileCamera();
	void RebuildViewSequence();
	void EnsureMissileOverlay();
	void RemoveMissileOverlay();
	void UpdateMissileOverlay();
	void RecordMissileLaunch(AMockMissileActor* Missile, AActor* Target, const FVector& LaunchLocation, bool bFromAutoFire);
	void UpdateMissileRecordOnImpact(AMockMissileActor* Missile, AActor* HitActor, int32 DestroyedCount, const FString& HitActorName = FString());
	void UpdateMissileRecordOnExpired(AMockMissileActor* Missile);
	void ResetMissileTestSession();
	void CompleteMissileTest();
	void ClearAutoFire();
	void BuildIndicatorEvaluations(TArray<FIndicatorEvaluationResult>& OutResults) const;

public:
	/** 获取所有有效的蓝方单位列表（供导弹搜索目标使用） */
	void GetActiveBlueUnits(TArray<AActor*>& OutUnits) const;
	/** 获取所有雷达干扰区域列表（供导弹检测干扰使用） */
	void GetActiveRadarJammers(TArray<class ARadarJammerActor*>& OutJammers) const;
	
	const FMissileTestSummary& GetMissileTestSummary() const { return LastMissileSummary; }
	const TArray<FMissileTestRecord>& GetMissileTestRecords() const { return MissileTestRecords; }
	bool HasMissileTestData() const { return MissileTestRecords.Num() > 0; }
	const FScenarioTestConfig& GetActiveScenarioConfig() const { return ActiveScenarioConfig; }
	void GetIndicatorEvaluations(TArray<FIndicatorEvaluationResult>& OutResults) const { BuildIndicatorEvaluations(OutResults); }

	// 感知算法统计上报（供蓝图/外部系统调用）
	void Perception_ReportDetection(bool bCorrect);
	void Perception_ReportFalsePositive();
	void Perception_ReportFalseNegative();
	void Perception_ReportRecognitionSample(double Seconds);
	void Perception_ReportLightCondition(bool bCorrect);
	void Perception_ReportWeatherCondition(bool bCorrect);
	void Perception_ReportSimultaneousTracks(int32 CurrentTracks);
	void Perception_ReportTrackingError(double Meters);
	void Perception_ReportJammingLight(bool bCorrect);
	void Perception_ReportJammingMedium(bool bCorrect);
	void Perception_ReportInterferenceDetected(bool bCorrect);
	void Perception_ReportFrequencyAdjust(bool bSuccess);
	void Perception_ReportRecoveryTime(double Seconds);
	void Perception_ReportKeypartRecognition(bool bCorrect);
	void Perception_ReportHeatTrackingError(double Meters);

public:
	FOnScenarioTestRequested OnScenarioTestRequested;

private:
	// 测试完成后返回界面时，回到的 Tab（0: 感知, 1: 决策）
	int32 ReturnTabIndex = 1;

	FPerceptionRuntimeStats PerceptionStats;

	TSharedPtr<SScenarioScreen> Screen;
	FDelegateHandle WorldHandle;
	int32 StepIndex = 0;
	FScenarioTestConfig PendingScenarioConfig;
	bool bHasPendingScenarioConfig = false;
	bool bIsRunningScenario = false;
	FTimerHandle PendingScenarioTimerHandle;
	TWeakObjectPtr<UWorld> PendingScenarioWorld;
	bool bPendingScenarioWaitingLogged = false;
	TArray<TWeakObjectPtr<AActor>> ActiveBlueUnits;
	TArray<TWeakObjectPtr<class ARadarJammerActor>> ActiveRadarJammers; // 雷达干扰区域
	TSharedPtr<SBlueUnitMonitor> BlueMonitorWidget;
	TSharedPtr<SWidget> BlueMonitorRoot;
	bool bUsingCustomDeployment = false;
	bool bHasInitialCameraTransform = false;
	FVector InitialCameraLocation = FVector::ZeroVector;
	FRotator InitialCameraRotation = FRotator::ZeroRotator;
	bool bHasOverviewHome = false;
	FVector OverviewHomeLocation = FVector::ZeroVector;
	FRotator OverviewHomeRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<class ASpectatorPawn> OverviewPawn;
	TWeakObjectPtr<class APawn> PreviousPawn;
	TArray<TWeakObjectPtr<AActor>> BlueDeployMarkers;
	TArray<int32> BlueMarkerSpawnCounts;
	FRotator BlueUnitFacing = FRotator::ZeroRotator;
	FVector BlueForward = FVector::ForwardVector;
	FVector BlueRight = FVector::RightVector;
	TArray<TWeakObjectPtr<class UStaticMesh>> BlueUnitMeshes;
	TArray<TWeakObjectPtr<class UMaterialInterface>> BlueUnitMaterials;
	TArray<int32> ActiveCountermeasureIndices;
	TArray<TWeakObjectPtr<AMockMissileActor>> ActiveMissiles;
	mutable TWeakObjectPtr<class UStaticMesh> CachedMissileMesh;
	mutable TWeakObjectPtr<class UMaterialInterface> CachedMissileMaterial;
	int32 NextTargetCursor = 0;
	int32 AutoFireRemaining = 0;
	FTimerHandle AutoFireTimerHandle;
	UInputComponent* MissileInputComponent = nullptr;
	bool bInputComponentPushed = false;

	struct FViewEntry
	{
		enum class EType : uint8
		{
			Initial,
			BlueUnit,
			Missile
		};

		EType Type = EType::Initial;
		int32 Index = INDEX_NONE;
	};

	TArray<FViewEntry> ViewEntries;
	int32 CurrentViewIndex = 0;
	TWeakObjectPtr<AMockMissileActor> MissileCameraTarget;
	FTimerHandle MissileCameraTimerHandle;
	FVector MissileCameraSmoothedLocation = FVector::ZeroVector;
	FRotator MissileCameraSmoothedRotation = FRotator::ZeroRotator;
	bool bMissileCameraHasHistory = false;
	bool bMissileCameraControlInitialized = false; // 是否已初始化控制旋转，初始化后不再强制设置
	TWeakObjectPtr<UMissileOverlayWidget> MissileOverlayWidget;

private:
	TArray<FMissileTestRecord> MissileTestRecords;
	TMap<TWeakObjectPtr<AMockMissileActor>, int32> MissileRecordLookup;
	FMissileTestSummary LastMissileSummary;
	double TestSessionStartTime = 0.0;
	FScenarioTestConfig ActiveScenarioConfig;
	bool bHasActiveScenarioConfig = false;
};


