#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SScenarioBreadcrumb;
class SScenarioMainTable;

DECLARE_DELEGATE(FOnPrevStep);
DECLARE_DELEGATE(FOnNextStep);
DECLARE_DELEGATE(FOnSaveAll);
DECLARE_DELEGATE(FOnBackToMainMenu);

/**
 * 根界面：标题、说明、步骤导航、主表格、底部按钮。
 */
class SScenarioScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioScreen){}
		SLATE_ARGUMENT(int32, StepIndex)
		SLATE_EVENT(FOnPrevStep, OnPrevStep)
		SLATE_EVENT(FOnNextStep, OnNextStep)
		SLATE_EVENT(FOnSaveAll, OnSaveAll)
		SLATE_EVENT(FOnBackToMainMenu, OnBackToMainMenu)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetStepIndex(int32 InStepIndex);

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
	TSharedPtr<SScenarioBreadcrumb> Breadcrumb;
	TSharedPtr<SScenarioMainTable> MainTable; // Step1
	TSharedPtr<class SIndicatorSelector> IndicatorSelector; // Step2
	TSharedPtr<class SEnvironmentBuilder> EnvironmentBuilder; // Step3
	TSharedPtr<class SVerticalBox> DecisionContentBox; // content container for step area

	FOnPrevStep OnPrevStep;
	FOnNextStep OnNextStep;
	FOnSaveAll OnSaveAll;
	FOnBackToMainMenu OnBackToMainMenu;
};


