#include "Containers/Set.h"

struct FScenarioTestConfig
{
	int32 TestMethodIndex = 0;
	int32 EnvironmentInterferenceIndex = 0; // 0: 无干扰场景测试方法, 1: 有干扰场景测试方法
	TArray<int32> SelectedTableRowIndices;
	TArray<TArray<FString>> SelectedTableRowTexts;
	TArray<int32> SelectedPrototypeRowIndices;
	TArray<TArray<FString>> SelectedPrototypeRowTexts;
	TArray<FString> SelectedIndicatorIds;
	TArray<FString> SelectedIndicatorDetails;
	int32 WeatherIndex = 0;
	int32 TimeIndex = 0;
	int32 MapIndex = 0;
	FName MapLevelName = NAME_None;
	int32 DensityIndex = 1;
	TArray<int32> CountermeasureIndices;
	bool bBlueCustomDeployment = false;
	int32 PresetIndex = -1;
};
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SScenarioBreadcrumb;
class SScenarioMainTable;
class SScenarioPrototypeTable;
class UScenarioMenuSubsystem;

DECLARE_DELEGATE(FOnPrevStep);
DECLARE_DELEGATE(FOnNextStep);
DECLARE_DELEGATE(FOnSaveAll);
DECLARE_DELEGATE(FOnBackToMainMenu);
DECLARE_DELEGATE(FOnStartTest);

/**
 * 根界面：标题、说明、步骤导航、主表格、底部按钮。
 */
class SScenarioScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioScreen){}
		SLATE_ARGUMENT(int32, StepIndex)
		SLATE_ARGUMENT(int32, InitialTabIndex)
		SLATE_ARGUMENT(class UScenarioMenuSubsystem*, OwnerSubsystem)
		SLATE_EVENT(FOnPrevStep, OnPrevStep)
		SLATE_EVENT(FOnNextStep, OnNextStep)
		SLATE_EVENT(FOnSaveAll, OnSaveAll)
		SLATE_EVENT(FOnBackToMainMenu, OnBackToMainMenu)
		SLATE_EVENT(FOnStartTest, OnStartTest)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetStepIndex(int32 InStepIndex);
	bool CollectScenarioConfig(FScenarioTestConfig& OutConfig) const;
	int32 GetActiveTabIndex() const { return ActiveTabIndex; }
	void SetActiveTabIndex(int32 InTabIndex);

	// 持久化 Step1 表格数据（在 SaveAll 时调用）
	void SavePersistentTables() const;

private:
	FReply OnPrevClicked();
	FReply OnNextClicked();
	FReply OnSaveClicked();
	FReply OnBackClicked();
	FReply OnTab1Clicked();
	FReply OnTab2Clicked();
	TSharedRef<SWidget> BuildTabWidget(int32 TabIndex, const FText& TabText, bool bIsActive);
	TSharedRef<SWidget> BuildDecisionContent(); // 智能决策算法测评的内容
	TSharedRef<SWidget> BuildPerceptionContent(); // 智能感知算法测评的内容（目前为空）
	TSharedRef<SWidget> BuildDecisionStepContent(int32 InStepIndex);
	TSharedRef<SWidget> BuildDecisionStep1();
	TSharedRef<SWidget> BuildDecisionStep2();
	TSharedRef<SWidget> BuildDecisionStep3();
	TSharedRef<SWidget> BuildDecisionStep4();
	TSharedRef<SWidget> BuildDecisionStep5();

private:
	int32 StepIndex = 0;
	int32 ActiveTabIndex = 1; // 0: 智能感知算法测评, 1: 智能决策算法测评
	int32 TestMethodIndex = 0; // 0: 正交测试, 1: 单独测试
	int32 EnvironmentInterferenceIndex = 0; // 0: 无干扰场景测试方法, 1: 有干扰场景测试方法
	TSharedPtr<SScenarioBreadcrumb> Breadcrumb;
	// Tab2（决策）与 Tab1（感知）分别维护自己的表实例，避免数据相互影响
	TSharedPtr<SScenarioMainTable> MainTableDecision; // Step1 - 决策
	TSharedPtr<SScenarioPrototypeTable> PrototypeTableDecision; // Step1 - 决策
	TSharedPtr<SScenarioMainTable> MainTablePerception; // Step1 - 感知
	TSharedPtr<SScenarioPrototypeTable> PrototypeTablePerception; // Step1 - 感知
	TSharedPtr<class SIndicatorSelector> IndicatorSelector; // Step2
	TSharedPtr<class SEnvironmentBuilder> EnvironmentBuilder; // Step3
	TSharedPtr<class SVerticalBox> DecisionContentBox; // content container for step area
	TWeakObjectPtr<class UScenarioMenuSubsystem> OwnerSubsystemWeak;

	FOnPrevStep OnPrevStep;
	FOnNextStep OnNextStep;
	FOnSaveAll OnSaveAll;
	FOnBackToMainMenu OnBackToMainMenu;
	FOnStartTest OnStartTest;
};


