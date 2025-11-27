#include "Containers/Set.h"

struct FScenarioTestConfig
{
	int32 TestMethodTypeIndex = 0; // 0: 随机采样测试方法, 1: 正交测试方法
	int32 TestMethodIndex = 0; // 0: 算法级, 1: 系统级（测试对象）
	int32 EnvironmentInterferenceIndex = 0; // 0: 无干扰场景测试方法, 1: 有干扰场景测试方法
	TArray<int32> SelectedTableRowIndices;
	TArray<TArray<FString>> SelectedTableRowTexts;
	TArray<int32> SelectedPrototypeRowIndices;
	TArray<TArray<FString>> SelectedPrototypeRowTexts;
	TArray<FString> SelectedAlgorithmNames;  // 选中的算法名称列表
	TArray<FString> SelectedPrototypeNames;  // 选中的分系统名称列表
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
	int32 EnemyForceIndex = 0; // 敌方兵力：0: 无防空系统, 1: 基础防空系统, 2: 加强型防空系统, 3: 高级防空系统, 4: 多层体系化防空系统
	int32 FriendlyForceIndex = 0; // 我方兵力：同上
	int32 EquipmentCapabilityIndex = 0; // 装备能力：0: 近程飞行器射程, 1: 中近程, 2: 中程, 3: 中远程, 4: 远程
	int32 FormationModeIndex = 0; // 编队方式：0: 单一静态目标打击, 1: 单一飞行器攻击, 2: 营级多D协同攻击, 3: 旅级, 4: 旅级以上
	int32 TargetAccuracyIndex = 0; // 概略目指准确性：0: 高精度, 1: 中等精度, 2: 低精度
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
	
	// 根据Step1的选择状态更新Step2的指标过滤
	void UpdateIndicatorFilter();

private:
	int32 StepIndex = 0;
	int32 ActiveTabIndex = 1; // 0: 智能感知算法测评, 1: 智能决策算法测评
	// Tab 切换次数限制：用于控制 Tab1/Tab2 只能来回切换一次
	int32 TabSwitchCount = 0;
	static constexpr int32 MaxTabSwitches = 2; // 例如：从 Tab2 -> Tab1 -> 再回 Tab2，共允许 2 次切换
	int32 TestMethodTypeIndex = 0; // 0: 随机采样测试方法, 1: 正交测试方法
	int32 TestMethodIndex = 0; // 0: 算法级, 1: 系统级（测试对象）
	int32 EnvironmentInterferenceIndex = 0; // 0: 无干扰场景测试方法, 1: 有干扰场景测试方法
	bool bPerceptionSubsystemOverride = false;
	bool bDecisionSubsystemOverride = false;
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


