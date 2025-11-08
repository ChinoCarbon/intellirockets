#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/SScenarioScreen.h"
#include "ScenarioMenuSubsystem.generated.h"

class SScenarioScreen;
class SBlueUnitMonitor;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnScenarioTestRequested, const FScenarioTestConfig&);

UCLASS()
class UScenarioMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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
	void FinalizeScenarioAfterLoad();
	bool AreScenarioLevelsReady(UWorld* World) const;
	UClass* ResolveBlueUnitClass() const;
	class UStaticMesh* ResolveBlueUnitMesh() const;
	class UMaterialInterface* ResolveBlueUnitMaterial() const;
	void ShowBlueMonitor(UWorld* World);
	void HideBlueMonitor();
	void FocusCameraOnUnit(int32 Index);
	void ReturnCameraToInitial();
	void EnsureOverviewPawn(UWorld* World);
	void RestorePreviousPawn();

public:
	FOnScenarioTestRequested OnScenarioTestRequested;

private:
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
	TSharedPtr<SBlueUnitMonitor> BlueMonitorWidget;
	TSharedPtr<SWidget> BlueMonitorRoot;
	bool bHasInitialCameraTransform = false;
	FVector InitialCameraLocation = FVector::ZeroVector;
	FRotator InitialCameraRotation = FRotator::ZeroRotator;
	bool bHasOverviewHome = false;
	FVector OverviewHomeLocation = FVector::ZeroVector;
	FRotator OverviewHomeRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<class ASpectatorPawn> OverviewPawn;
	TWeakObjectPtr<class APawn> PreviousPawn;
};


