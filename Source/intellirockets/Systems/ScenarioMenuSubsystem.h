#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScenarioMenuSubsystem.generated.h"

class SScenarioScreen;

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

private:
	TSharedPtr<SScenarioScreen> Screen;
	FDelegateHandle WorldHandle;
	int32 StepIndex = 0;
};


