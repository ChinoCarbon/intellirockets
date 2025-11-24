#include "UI/SScenarioScreen.h"
#include "UI/Widgets/SScenarioBreadcrumb.h"
#include "UI/Widgets/SScenarioMainTable.h"
#include "UI/Widgets/SScenarioPrototypeTable.h"
#include "UI/Widgets/SIndicatorSelector.h"
#include "UI/Widgets/SEnvironmentBuilder.h"
#include "UI/Styles/ScenarioStyle.h"
#include "Systems/ScenarioMenuSubsystem.h"
#include "Systems/ScenarioTestMetrics.h"
#include "Misc/PackageName.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SGridPanel.h"

void SScenarioScreen::Construct(const FArguments& InArgs)
{
	StepIndex = InArgs._StepIndex;
	if (InArgs._InitialTabIndex >= 0)
	{
		ActiveTabIndex = InArgs._InitialTabIndex;
	}
	OwnerSubsystemWeak = InArgs._OwnerSubsystem;
	OnPrevStep = InArgs._OnPrevStep;
	OnNextStep = InArgs._OnNextStep;
	OnSaveAll = InArgs._OnSaveAll;
	OnBackToMainMenu = InArgs._OnBackToMainMenu;
	OnStartTest = InArgs._OnStartTest;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(ScenarioStyle::Background) // 使用纯色画刷实现完全不透明背景
		[
			SNew(SVerticalBox)

			// Tab 标签页
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					BuildTabWidget(0, FText::FromString(TEXT("感知类测评")), ActiveTabIndex == 0)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
				[
					BuildTabWidget(1, FText::FromString(TEXT("决策类测评")), ActiveTabIndex == 1)
				]
			]

            // 根据选中的Tab显示不同内容（可重建的容器）
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SAssignNew(DecisionContentBox, SVerticalBox)
            ]
		]
	];

    // 初始内容
    if (ActiveTabIndex == 1)
    {
        DecisionContentBox->AddSlot().AutoHeight()[ BuildDecisionContent() ];
    }
    else
    {
        DecisionContentBox->AddSlot().AutoHeight()[ BuildPerceptionContent() ];
    }
}

void SScenarioScreen::SetActiveTabIndex(int32 InTabIndex)
{
	ActiveTabIndex = FMath::Clamp(InTabIndex, 0, 1);
	// 触发内容重建以反映 Tab 切换
	if (DecisionContentBox.IsValid())
	{
		DecisionContentBox->ClearChildren();
		if (ActiveTabIndex == 1)
		{
			DecisionContentBox->AddSlot().AutoHeight()[ BuildDecisionContent() ];
		}
		else
		{
			DecisionContentBox->AddSlot().AutoHeight()[ BuildPerceptionContent() ];
		}
	}
}

void SScenarioScreen::SavePersistentTables() const
{
	// 四个表格分别持久化（如果已创建）
	if (MainTablePerception.IsValid())
	{
		MainTablePerception->SavePersistent();
	}
	if (PrototypeTablePerception.IsValid())
	{
		PrototypeTablePerception->SavePersistent();
	}
	if (MainTableDecision.IsValid())
	{
		MainTableDecision->SavePersistent();
	}
	if (PrototypeTableDecision.IsValid())
	{
		PrototypeTableDecision->SavePersistent();
	}
}
void SScenarioScreen::SetStepIndex(int32 InStepIndex)
{
    StepIndex = FMath::Clamp(InStepIndex, 0, 4);
    if (Breadcrumb.IsValid())
    {
        Breadcrumb->SetCurrentStep(StepIndex);
    }
    if (!DecisionContentBox.IsValid()) return;
    DecisionContentBox->ClearChildren();
    if (ActiveTabIndex == 1)
    {
        DecisionContentBox->AddSlot().AutoHeight()[ BuildDecisionContent() ];
    }
    else
    {
        DecisionContentBox->AddSlot().AutoHeight()[ BuildPerceptionContent() ];
    }
}

FReply SScenarioScreen::OnPrevClicked()
{
	if (OnPrevStep.IsBound()) OnPrevStep.Execute();
	return FReply::Handled();
}

FReply SScenarioScreen::OnNextClicked()
{
	if (OnNextStep.IsBound()) OnNextStep.Execute();
	return FReply::Handled();
}

FReply SScenarioScreen::OnSaveClicked()
{
	if (OnSaveAll.IsBound()) OnSaveAll.Execute();
	return FReply::Handled();
}

FReply SScenarioScreen::OnBackClicked()
{
	if (OnBackToMainMenu.IsBound()) OnBackToMainMenu.Execute();
	return FReply::Handled();
}

FReply SScenarioScreen::OnTab1Clicked()
{
	if (ActiveTabIndex != 0)
	{
		ActiveTabIndex = 0;
		UE_LOG(LogTemp, Log, TEXT("Switching to Tab 0 (智能感知算法测评), ActiveTabIndex=%d"), ActiveTabIndex);
		// 重新构建整个界面以更新 tab 状态
		Construct(FArguments()
			.StepIndex(StepIndex)
			.OwnerSubsystem(OwnerSubsystemWeak.Get())
			.OnPrevStep(OnPrevStep)
			.OnNextStep(OnNextStep)
			.OnSaveAll(OnSaveAll)
			.OnBackToMainMenu(OnBackToMainMenu)
			.OnStartTest(OnStartTest));
	}
	return FReply::Handled();
}

FReply SScenarioScreen::OnTab2Clicked()
{
	if (ActiveTabIndex != 1)
	{
		ActiveTabIndex = 1;
		UE_LOG(LogTemp, Log, TEXT("Switching to Tab 1 (智能决策算法测评), ActiveTabIndex=%d"), ActiveTabIndex);
		// 重新构建整个界面以更新 tab 状态
		Construct(FArguments()
			.StepIndex(StepIndex)
			.OwnerSubsystem(OwnerSubsystemWeak.Get())
			.OnPrevStep(OnPrevStep)
			.OnNextStep(OnNextStep)
			.OnSaveAll(OnSaveAll)
			.OnBackToMainMenu(OnBackToMainMenu)
			.OnStartTest(OnStartTest));
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SScenarioScreen::BuildTabWidget(int32 TabIndex, const FText& TabText, bool bIsActive)
{
	return SNew(SButton)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		.OnClicked(TabIndex == 0 ? 
			FOnClicked::CreateSP(this, &SScenarioScreen::OnTab1Clicked) :
			FOnClicked::CreateSP(this, &SScenarioScreen::OnTab2Clicked))
		[
			SNew(SBorder)
			.Padding(FMargin(20.f, 10.f))
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(bIsActive ? ScenarioStyle::Accent : FLinearColor::Transparent)
			[
				SNew(STextBlock)
				.Text(TabText)
				.ColorAndOpacity(bIsActive ? ScenarioStyle::Text : ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(18))
			]
		];
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionContent()
{
	UE_LOG(LogTemp, Log, TEXT("BuildDecisionContent called - building decision content"));
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SAssignNew(Breadcrumb, SScenarioBreadcrumb)
			.CurrentStep(StepIndex)
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			BuildDecisionStepContent(StepIndex)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.Padding(10.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(SButton).OnClicked(this, &SScenarioScreen::OnPrevClicked)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("上一步"))).Font(ScenarioStyle::Font(12))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(SButton).OnClicked(this, &SScenarioScreen::OnBackClicked)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("返回主菜单"))).Font(ScenarioStyle::Font(12))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(SButton).OnClicked(this, &SScenarioScreen::OnSaveClicked)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("保存"))).Font(ScenarioStyle::Font(12))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(SButton).OnClicked(this, &SScenarioScreen::OnNextClicked)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("下一步"))).Font(ScenarioStyle::Font(12))
					]
				]
			]
		];
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStepContent(int32 InStepIndex)
{
	switch (InStepIndex)
	{
		case 0: return BuildDecisionStep1();
		case 1: return BuildDecisionStep2();
		case 2: return BuildDecisionStep3();
		case 3: return BuildDecisionStep4();
		case 4: return BuildDecisionStep5();
		default: return SNew(SSpacer);
	}
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStep1()
{
	// 原第1步的内容：说明条 + 算法列表 + 样机列表 + 测试方法选择
    return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.Padding(10.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("使用说明：此页面用于配置测评对象列表，统一进行编辑与管理。")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
				.WrapTextAt(1200.f)
			]
		]
		+ SVerticalBox::Slot().FillHeight(0.5f).Padding(0.f, 8.f, 0.f, 8.f)
		[
            (
				[&]() -> TSharedRef<SWidget>
				{
					const bool bPerception = (ActiveTabIndex == 0);
					if (bPerception)
					{
						return (MainTablePerception.IsValid()
							? MainTablePerception.ToSharedRef()
							: StaticCastSharedRef<SWidget>(
								SAssignNew(MainTablePerception, SScenarioMainTable)
								.DataPresetIndex(1) // 感知数据
								.OnRowEdit(FOnRowEdit::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Edit Row")); }))
								.OnRowDelete(FOnRowDelete::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Delete Row")); }))
							));
					}
					else
					{
						return (MainTableDecision.IsValid()
							? MainTableDecision.ToSharedRef()
							: StaticCastSharedRef<SWidget>(
								SAssignNew(MainTableDecision, SScenarioMainTable)
								.DataPresetIndex(0) // 决策数据
								.OnRowEdit(FOnRowEdit::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Edit Row")); }))
								.OnRowDelete(FOnRowDelete::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Delete Row")); }))
							));
					}
				}()
            )
		]
		+ SVerticalBox::Slot().FillHeight(0.5f).Padding(0.f, 8.f, 0.f, 8.f)
		[
            (
				[&]() -> TSharedRef<SWidget>
				{
					const bool bPerception = (ActiveTabIndex == 0);
					if (bPerception)
					{
						return (PrototypeTablePerception.IsValid()
							? PrototypeTablePerception.ToSharedRef()
							: StaticCastSharedRef<SWidget>(
								SAssignNew(PrototypeTablePerception, SScenarioPrototypeTable)
								.DataPresetIndex(1) // 感知数据
								.OnRowEdit(FOnRowEdit::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Edit Prototype Row")); }))
								.OnRowDelete(FOnRowDelete::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Delete Prototype Row")); }))
							));
					}
					else
					{
						return (PrototypeTableDecision.IsValid()
							? PrototypeTableDecision.ToSharedRef()
							: StaticCastSharedRef<SWidget>(
								SAssignNew(PrototypeTableDecision, SScenarioPrototypeTable)
								.DataPresetIndex(0) // 决策数据
								.OnRowEdit(FOnRowEdit::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Edit Prototype Row")); }))
								.OnRowDelete(FOnRowDelete::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Delete Prototype Row")); }))
							));
					}
				}()
            )
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBorder)
			.Padding(10.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("多层级通用测试方法")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::Font(12))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return TestMethodIndex == 0 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						if (NewState == ECheckBoxState::Checked) { TestMethodIndex = 0; UE_LOG(LogTemp, Log, TEXT("TestMethod: 正交测试")); }
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("算法级"))).Font(ScenarioStyle::Font(12))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.f, 0.f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return TestMethodIndex == 1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						if (NewState == ECheckBoxState::Checked) { TestMethodIndex = 1; UE_LOG(LogTemp, Log, TEXT("TestMethod: 单独测试")); }
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("系统级"))).Font(ScenarioStyle::Font(12))
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBorder)
			.Padding(10.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("典型场景测试方法")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::Font(12))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return EnvironmentInterferenceIndex == 0 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						if (NewState == ECheckBoxState::Checked)
						{
							EnvironmentInterferenceIndex = 0;
							UE_LOG(LogTemp, Log, TEXT("EnvironmentInterference: 无干扰场景测试方法"));
						}
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("无干扰场景测试方法"))).Font(ScenarioStyle::Font(12))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.f, 0.f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return EnvironmentInterferenceIndex == 1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						if (NewState == ECheckBoxState::Checked)
						{
							EnvironmentInterferenceIndex = 1;
							UE_LOG(LogTemp, Log, TEXT("EnvironmentInterference: 有干扰场景测试方法"));
						}
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("有干扰场景测试方法"))).Font(ScenarioStyle::Font(12))
					]
				]
			]
		];
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStep2()
{
	// 指标体系构建：指标选择器
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBorder)
			.Padding(10.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("使用说明：请从左侧分类列表中选择指标分类，然后在中间列表中选择需要的指标，已选择的指标会显示在右侧列表中。")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
				.WrapTextAt(1200.f)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
            (
				[&]() -> TSharedRef<SWidget>
				{
					const bool bPerception = (ActiveTabIndex == 0);
					if (IndicatorSelector.IsValid())
					{
						return IndicatorSelector.ToSharedRef();
					}
					else
					{
						if (bPerception)
						{
							return SAssignNew(IndicatorSelector, SIndicatorSelector)
								.IndicatorsJsonPath(FPaths::ProjectContentDir() / TEXT("Config/PerceptionIndicators.json"));
						}
						else
						{
							return SAssignNew(IndicatorSelector, SIndicatorSelector)
								.IndicatorsJsonPath(FPaths::ProjectContentDir() / TEXT("Config/DecisionIndicators.json"));
						}
					}
				}()
			)
		];
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStep3()
{
	// 测评环境搭建：左侧地图预览，右侧环境选择器
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBorder)
			.Padding(10.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("使用说明：请在右侧选择天气、时段和地图类型，左侧会实时显示对应的地图预览。")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
				.WrapTextAt(1200.f)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
            (EnvironmentBuilder.IsValid() ? EnvironmentBuilder.ToSharedRef() : SAssignNew(EnvironmentBuilder, SEnvironmentBuilder))
		];
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStep4()
{
	FScenarioTestConfig Snapshot;
	CollectScenarioConfig(Snapshot);

	// 容器
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	const auto MakeCard = [](const FString& Title, const TSharedRef<SWidget>& Content) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.Padding(12.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Title))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::BoldFont(13))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Content
				]
			];
	};

	// 标题
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("测评配置总览（Step1~Step3）")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::BoldFont(16))
		]
	];

	// Step2: 指标详情
	int32 IndicatorCount = 0;
	TArray<FString> IndicatorIds;
	TArray<FString> IndicatorDetails;
	if (IndicatorSelector.IsValid())
	{
		IndicatorSelector->GetSelectedIndicatorDetails(IndicatorIds, IndicatorDetails);
		IndicatorCount = IndicatorDetails.Num();
	}

	// Step1: 多层级通用测试方法 / 典型场景测试方法
	const FString MultiLevelText = (Snapshot.TestMethodIndex == 0) ? TEXT("算法级") : TEXT("系统级");
	const FString SceneMethodText = (Snapshot.EnvironmentInterferenceIndex == 0)
		? TEXT("无干扰场景测试方法")
		: TEXT("有干扰场景测试方法");

	// 多层级通用测试方法
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f,0.f,8.f,0.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("多层级通用测试方法")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::BoldFont(13))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MultiLevelText))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			]
		]
	];

	// 典型场景测试方法
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f,0.f,8.f,0.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("典型场景测试方法")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::BoldFont(13))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(SceneMethodText))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			]
		]
	];

	// Step2 card：
	{
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("已选指标（%d）"), IndicatorCount)))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::BoldFont(13))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				(IndicatorCount > 0 && IndicatorSelector.IsValid())
				? StaticCastSharedRef<SWidget>([&]()
				{
					TSharedRef<SVerticalBox> ScrollBox = SNew(SVerticalBox);
					for (int32 i=0; i<IndicatorDetails.Num(); ++i)
					{
						ScrollBox->AddSlot().AutoHeight().Padding(0.f,4.f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("• %s"), *IndicatorDetails[i])))
							.ColorAndOpacity(ScenarioStyle::TextDim)
							.Font(ScenarioStyle::Font(12))
							.WrapTextAt(1100.f)
						];
					}
					return StaticCastSharedRef<SWidget>(ScrollBox);
				}())
				: StaticCastSharedRef<SWidget>( SNew(STextBlock).Text(FText::FromString(TEXT("未选择指标"))).ColorAndOpacity(ScenarioStyle::TextDim).Font(ScenarioStyle::Font(12)) )
			]
		]
	];
	}

	// Step1：算法列表已选行
	{
		TSharedRef<SVerticalBox> ListBox = SNew(SVerticalBox);
		if (Snapshot.SelectedTableRowTexts.Num() > 0)
		{
			for (const TArray<FString>& Row : Snapshot.SelectedTableRowTexts)
			{
				FString Line = TEXT("• ");
				for (int32 c=0; c<Row.Num(); ++c)
				{
					if (c>0) Line += TEXT(" | ");
					Line += Row[c];
				}
				ListBox->AddSlot().AutoHeight().Padding(0.f,2.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Line))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::Font(12))
					.WrapTextAt(1200.f)
				];
			}
		}
		else
		{
			ListBox->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("未选择算法")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			];
		}

		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
		[
			SNew(SBorder).Padding(12.f).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("已选算法（来自Step1）")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::BoldFont(13))
				]
				+ SVerticalBox::Slot().AutoHeight()[ ListBox ]
			]
		];
	}

	// Step1：样机列表已选行
	{
		TSharedRef<SVerticalBox> ListBox = SNew(SVerticalBox);
		if (Snapshot.SelectedPrototypeRowTexts.Num() > 0)
		{
			for (const TArray<FString>& Row : Snapshot.SelectedPrototypeRowTexts)
			{
				FString Line = TEXT("• ");
				for (int32 c=0; c<Row.Num(); ++c)
				{
					if (c>0) Line += TEXT(" | ");
					Line += Row[c];
				}
				ListBox->AddSlot().AutoHeight().Padding(0.f,2.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Line))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::Font(12))
					.WrapTextAt(1200.f)
				];
			}
		}
		else
		{
			ListBox->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("未选择分系统")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			];
		}

		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
		[
			SNew(SBorder).Padding(12.f).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("已选分系统（来自Step1）")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::BoldFont(13))
				]
				+ SVerticalBox::Slot().AutoHeight()[ ListBox ]
			]
		];
	}

	// Step3 card（使用快照数据展示）
	{
		static const TCHAR* WeatherNames[] = { TEXT("晴天"), TEXT("海杂波"), TEXT("气动热效应"), TEXT("雾天") };
		static const TCHAR* TimeNames[] = { TEXT("白天"), TEXT("夜晚") };
		static const TCHAR* MapNames[] = { TEXT("沙漠"), TEXT("森林"), TEXT("雪地"), TEXT("海边") };
		static const TCHAR* DensityNames[] = { TEXT("密集"), TEXT("正常"), TEXT("稀疏") };
		static const TCHAR* CountermeasureNames[] = { TEXT("电磁干扰"), TEXT("通信干扰"), TEXT("目标移动") };

		const FString WeatherStr = WeatherNames[FMath::Clamp(Snapshot.WeatherIndex, 0, 2)];
		const FString TimeStr = TimeNames[FMath::Clamp(Snapshot.TimeIndex, 0, 1)];
		const FString MapStr = MapNames[FMath::Clamp(Snapshot.MapIndex, 0, 3)];
		const FString DensityStr = DensityNames[FMath::Clamp(Snapshot.DensityIndex, 0, 2)];

		FString CmStr;
		for (int32 i=0;i<Snapshot.CountermeasureIndices.Num();++i)
		{
			if (i>0) CmStr += TEXT("、");
			const int32 Index = FMath::Clamp(Snapshot.CountermeasureIndices[i], 0, 2);
			CmStr += CountermeasureNames[Index];
		}
		if (CmStr.IsEmpty())
		{
			CmStr = TEXT("无");
		}

		const FString LevelPath = Snapshot.MapLevelName.IsNone() ? TEXT("未指定（将保持当前关卡）") : Snapshot.MapLevelName.ToString();

		TSharedRef<SVerticalBox> EnvironmentCard = SNew(SVerticalBox);
		EnvironmentCard->AddSlot().AutoHeight().Padding(0.f,2.f)
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("地图：%s（%s）"), *MapStr, *LevelPath))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];
		EnvironmentCard->AddSlot().AutoHeight().Padding(0.f,2.f)
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("天气 / 时段：%s / %s"), *WeatherStr, *TimeStr))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];
		EnvironmentCard->AddSlot().AutoHeight().Padding(0.f,2.f)
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("密集度：%s"), *DensityStr))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];
		EnvironmentCard->AddSlot().AutoHeight().Padding(0.f,2.f)
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("反制方式：%s"), *CmStr))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];
		EnvironmentCard->AddSlot().AutoHeight().Padding(0.f,2.f)
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("启用蓝方自定义部署：%s"), Snapshot.bBlueCustomDeployment ? TEXT("是") : TEXT("否")))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];
		EnvironmentCard->AddSlot().AutoHeight().Padding(0.f,2.f)
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("复杂度等级：%s"), Snapshot.PresetIndex >= 1 && Snapshot.PresetIndex <= 5 ? *FString::Printf(TEXT("%d级"), Snapshot.PresetIndex) : TEXT("未使用预设")))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];

		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
		[ MakeCard(TEXT("环境参数（来自 Step3）"), EnvironmentCard) ];
	}

	// 开始测试按钮
	Box->AddSlot().AutoHeight().Padding(0.f,24.f,0.f,0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(24.f,12.f))
			.OnClicked_Lambda([this]()
			{
				if (OnStartTest.IsBound())
				{
					OnStartTest.Execute();
				}
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.Padding(FMargin(18.f,10.f))
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.BorderBackgroundColor(ScenarioStyle::Accent)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("开始测试")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::BoldFont(14))
					.Justification(ETextJustify::Center)
				]
			]
		]
	];

	return Box;
}

bool SScenarioScreen::CollectScenarioConfig(FScenarioTestConfig& OutConfig) const
{
	OutConfig.TestMethodIndex = TestMethodIndex;
	OutConfig.EnvironmentInterferenceIndex = EnvironmentInterferenceIndex;

	OutConfig.SelectedTableRowIndices.Reset();
	OutConfig.SelectedTableRowTexts.Reset();
	{
		const bool bPerception = (ActiveTabIndex == 0);
		TSharedPtr<SScenarioMainTable> Table = bPerception ? MainTablePerception : MainTableDecision;
		if (Table.IsValid())
	{
		Table->GetSelectedRowIndices(OutConfig.SelectedTableRowIndices);
		for (int32 idx : OutConfig.SelectedTableRowIndices)
		{
			TArray<FText> Cols;
			Table->GetRowTexts(idx, Cols);
			TArray<FString> Row;
			for (const FText& Txt : Cols)
			{
				Row.Add(Txt.ToString());
			}
			OutConfig.SelectedTableRowTexts.Add(Row);
		}
	}
	}

	OutConfig.SelectedPrototypeRowIndices.Reset();
	OutConfig.SelectedPrototypeRowTexts.Reset();
	{
		const bool bPerception = (ActiveTabIndex == 0);
		TSharedPtr<SScenarioPrototypeTable> Table = bPerception ? PrototypeTablePerception : PrototypeTableDecision;
		if (Table.IsValid())
	{
		Table->GetSelectedRowIndices(OutConfig.SelectedPrototypeRowIndices);
		for (int32 idx : OutConfig.SelectedPrototypeRowIndices)
		{
			TArray<FText> Cols;
			Table->GetRowTexts(idx, Cols);
			TArray<FString> Row;
			for (const FText& Txt : Cols)
			{
				Row.Add(Txt.ToString());
			}
			OutConfig.SelectedPrototypeRowTexts.Add(Row);
		}
	}
	}

	OutConfig.SelectedIndicatorIds.Reset();
	OutConfig.SelectedIndicatorDetails.Reset();
	if (IndicatorSelector.IsValid())
	{
		IndicatorSelector->GetSelectedIndicatorDetails(OutConfig.SelectedIndicatorIds, OutConfig.SelectedIndicatorDetails);
	}

	if (EnvironmentBuilder.IsValid())
	{
		OutConfig.WeatherIndex = EnvironmentBuilder->GetWeatherIndex();
		OutConfig.TimeIndex = EnvironmentBuilder->GetTimeIndex();
		OutConfig.MapIndex = EnvironmentBuilder->GetMapIndex();
		OutConfig.DensityIndex = EnvironmentBuilder->GetDensityIndex();
		OutConfig.CountermeasureIndices.Reset();
		EnvironmentBuilder->GetCountermeasures(OutConfig.CountermeasureIndices);
		OutConfig.bBlueCustomDeployment = EnvironmentBuilder->IsBlueCustomEnabled();
		OutConfig.PresetIndex = EnvironmentBuilder->GetPresetIndex();
	}
	else
	{
		OutConfig.CountermeasureIndices.Reset();
		OutConfig.bBlueCustomDeployment = false;
		OutConfig.PresetIndex = -1;
	}

	static const TCHAR* MapLevelPaths[] =
	{
		TEXT("/Game/Desert/Desert"),
		TEXT("/Game/EF_Grounds/Maps/ExampleMap"),
		TEXT("/Game/ProceduralNtr_vol2/maps/Demo_Scene"),
		TEXT("/Game/Maps/Coast/CoastMap")
	};
	if (OutConfig.MapIndex >= 0 && OutConfig.MapIndex < UE_ARRAY_COUNT(MapLevelPaths))
	{
		// 校验关卡包是否存在，不存在则置空，避免 OpenLevel 失败无反馈
		const FString LevelRef = MapLevelPaths[OutConfig.MapIndex];
		bool bExists = false;
		{
			// 允许传递 /Game/Path/Level 的地图名；检查对应包是否存在
			// 这里只能做轻量校验：若编辑器环境无法确认，则继续沿用默认行为
			bExists = FPackageName::DoesPackageExist(LevelRef);
		}
		OutConfig.MapLevelName = bExists ? FName(*LevelRef) : NAME_None;
	}
	else
	{
		OutConfig.MapLevelName = NAME_None;
	}

	UE_LOG(LogTemp, Log, TEXT("CollectScenarioConfig: ActiveTabIndex=%d MapIndex=%d -> MapLevelName=%s (exists=%s)"),
		ActiveTabIndex,
		OutConfig.MapIndex,
		OutConfig.MapLevelName.IsNone() ? TEXT("None") : *OutConfig.MapLevelName.ToString(),
		(OutConfig.MapLevelName.IsNone() ? TEXT("No") : TEXT("Yes")));

	return true;
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStep5()
{
	const TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	Root->AddSlot()
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 12.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("导弹测试结果汇总")))
		.ColorAndOpacity(ScenarioStyle::Text)
		.Font(ScenarioStyle::BoldFont(18))
	];

	UScenarioMenuSubsystem* Subsystem = OwnerSubsystemWeak.Get();
	const bool bHasData = Subsystem && Subsystem->HasMissileTestData();

	if (!bHasData)
	{
		Root->AddSlot()
		.FillHeight(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SBorder)
				.Padding(12.f)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.BorderBackgroundColor(ScenarioStyle::Panel)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("尚未记录导弹测试。请发射至少一枚导弹，并按 P 键结束测试。")))
					.ColorAndOpacity(ScenarioStyle::TextDim)
					.Font(ScenarioStyle::Font(13))
				]
			]
		];

		return Root;
	}

	const FMissileTestSummary& Summary = Subsystem->GetMissileTestSummary();
	const TArray<FMissileTestRecord>& Records = Subsystem->GetMissileTestRecords();

	auto FormatPercent = [](float Value) -> FText
	{
		return FText::FromString(FString::Printf(TEXT("%.1f%%"), Value));
	};

	auto FormatSeconds = [](float Value) -> FText
	{
		return FText::FromString(FString::Printf(TEXT("%.2f s"), Value));
	};

	// 创建一个内容容器，包含所有测试结果内容
	TSharedRef<SVerticalBox> ContentBox = SNew(SVerticalBox);

	TSharedRef<SGridPanel> SummaryGrid = SNew(SGridPanel)
		.FillColumn(0, 0.45f)
		.FillColumn(1, 0.55f);

	const auto AddSummaryRow = [&SummaryGrid](int32 Row, const FString& Label, const FText& Value, const FLinearColor& Color)
	{
		SummaryGrid->AddSlot(0, Row)
		.Padding(FMargin(0.f, 2.f, 8.f, 2.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(Color)
			.Font(ScenarioStyle::Font(12))
		];

		SummaryGrid->AddSlot(1, Row)
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(Value)
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::BoldFont(12))
		];
	};

	AddSummaryRow(0, TEXT("发射总数"), FText::AsNumber(Summary.TotalShots), ScenarioStyle::Text);
	AddSummaryRow(1, TEXT("手动 / 自动"), FText::FromString(FString::Printf(TEXT("%d / %d"), Summary.ManualShots, Summary.AutoShots)), ScenarioStyle::Text);
	AddSummaryRow(2, TEXT("命中次数（直接 / 范围）"), FText::FromString(FString::Printf(TEXT("%d（%d / %d）"), Summary.Hits, Summary.DirectHits, Summary.AoEHits)), ScenarioStyle::Text);
	AddSummaryRow(3, TEXT("命中率"), FormatPercent(Summary.HitRate), ScenarioStyle::Accent);
	AddSummaryRow(4, TEXT("直接命中率"), FormatPercent(Summary.DirectHitRate), ScenarioStyle::Text);
	AddSummaryRow(5, TEXT("平均飞行时间"), FormatSeconds(Summary.AverageFlightTime), ScenarioStyle::Text);
	AddSummaryRow(6, TEXT("平均发射距离"), FText::FromString(FString::Printf(TEXT("%.1f m"), Summary.AverageLaunchDistance)), ScenarioStyle::Text);
	AddSummaryRow(7, TEXT("每次命中平均摧毁数"), FText::FromString(FString::Printf(TEXT("%.2f"), Summary.AverageDestroyedPerHit)), ScenarioStyle::Text);
	AddSummaryRow(8, TEXT("自动发射间隔（平均）"), FormatSeconds(Summary.AverageAutoLaunchInterval), ScenarioStyle::TextDim);
	AddSummaryRow(9, TEXT("测试时长"), FormatSeconds(Summary.SessionDuration), ScenarioStyle::TextDim);

	ContentBox->AddSlot()
	.AutoHeight()
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SummaryGrid
		]
	];

	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 12.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("发射明细（按时间排序）")))
		.ColorAndOpacity(ScenarioStyle::Text)
		.Font(ScenarioStyle::BoldFont(14))
	];

	TSharedRef<SVerticalBox> TableHeader = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("#"))).Font(ScenarioStyle::BoldFont(12)) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("模式"))).Font(ScenarioStyle::BoldFont(12)) ]
			+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("目标"))).Font(ScenarioStyle::BoldFont(12)) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("发射距离"))).Font(ScenarioStyle::BoldFont(12)) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("飞行时间"))).Font(ScenarioStyle::BoldFont(12)) ]
			+ SHorizontalBox::Slot().FillWidth(0.3f).Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("结果"))).Font(ScenarioStyle::BoldFont(12)) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("摧毁数量"))).Font(ScenarioStyle::BoldFont(12)) ]
		];

	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox);

	constexpr int32 MaxRows = 50;
	const int32 StartIndex = FMath::Max(0, Records.Num() - MaxRows);

	for (int32 Index = StartIndex; Index < Records.Num(); ++Index)
	{
		const FMissileTestRecord& Record = Records[Index];

		const FString ModeText = Record.bAutoFire ? TEXT("自动") : TEXT("手动");
		const float LaunchDistance = Record.InitialDistance;
		const float FlightTime = Record.GetFlightDuration();

		FString Outcome;
		if (Record.bImpactRegistered)
		{
			if (Record.DestroyedCount > 0)
			{
				Outcome = Record.bDirectHit ? TEXT("直接命中") : TEXT("范围命中");
			}
			else
			{
				Outcome = TEXT("命中未摧毁");
			}
		}
		else if (Record.bExpired)
		{
			Outcome = TEXT("未命中（失去目标）");
		}
		else
		{
			Outcome = TEXT("进行中");
		}

		Scroll->AddSlot()
		[
			SNew(SBorder)
			.Padding(6.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(Index % 2 == 0 ? ScenarioStyle::Panel : (ScenarioStyle::Panel * FLinearColor(1.f, 1.f, 1.f, 0.85f)))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(STextBlock).Text(FText::AsNumber(Index + 1)).Font(ScenarioStyle::Font(11))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(STextBlock).Text(FText::FromString(ModeText)).Font(ScenarioStyle::Font(11))
				]
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(4.f)
				[
					SNew(STextBlock).Text(FText::FromString(Record.TargetName)).Font(ScenarioStyle::Font(11))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%.1f m"), LaunchDistance))).Font(ScenarioStyle::Font(11))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(STextBlock).Text(FormatSeconds(FlightTime)).Font(ScenarioStyle::Font(11))
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f).Padding(4.f)
				[
					SNew(STextBlock).Text(FText::FromString(Outcome)).Font(ScenarioStyle::Font(11))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f)
				[
					SNew(STextBlock).Text(FText::AsNumber(Record.DestroyedCount)).Font(ScenarioStyle::Font(11))
				]
			]
		];
	}

	ContentBox->AddSlot()
	.AutoHeight()
	[
		SNew(SBorder)
		.Padding(0.f)
		.BorderImage(FCoreStyle::Get().GetBrush("Border"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				TableHeader
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(200.f)
				[
					Scroll
				]
			]
		]
	];

	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 16.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("指标评估结果")))
		.ColorAndOpacity(ScenarioStyle::Text)
		.Font(ScenarioStyle::BoldFont(14))
	];

	TArray<FIndicatorEvaluationResult> IndicatorResults;
	Subsystem->GetIndicatorEvaluations(IndicatorResults);

	if (IndicatorResults.Num() == 0)
	{
		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(12.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("当前测试未选择指标，或尚未开始导弹测试。")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			]
		];
	}
	else
	{
		TSharedRef<SVerticalBox> IndicatorTable = SNew(SVerticalBox);

		const auto AddIndicatorRow = [&IndicatorTable](int32 Index, const FIndicatorEvaluationResult& Eval)
		{
			IndicatorTable->AddSlot().AutoHeight()
			[
				SNew(SBorder)
				.Padding(10.f)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.BorderBackgroundColor(Index % 2 == 0 ? ScenarioStyle::Panel : (ScenarioStyle::Panel * FLinearColor(1.f, 1.f, 1.f, 0.9f)))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d. %s"), Index + 1, *Eval.DisplayName)))
						.ColorAndOpacity(ScenarioStyle::Text)
						.Font(ScenarioStyle::BoldFont(12))
						.WrapTextAt(700.f)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("当前值：%s"), *Eval.ValueText)))
							.ColorAndOpacity(ScenarioStyle::Text)
							.Font(ScenarioStyle::Font(11))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("目标：%s"), *Eval.TargetText)))
							.ColorAndOpacity(ScenarioStyle::TextDim)
							.Font(ScenarioStyle::Font(11))
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("状态：%s"), *Eval.StatusText)))
							.ColorAndOpacity(Eval.StatusColor)
							.Font(ScenarioStyle::BoldFont(11))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Eval.RemarkText))
						.ColorAndOpacity(ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(10))
						.WrapTextAt(720.f)
					]
				]
			];
		};

		for (int32 EvalIndex = 0; EvalIndex < IndicatorResults.Num(); ++EvalIndex)
		{
			AddIndicatorRow(EvalIndex, IndicatorResults[EvalIndex]);
		}

		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(0.f)
			.BorderImage(FCoreStyle::Get().GetBrush("Border"))
			[
				SNew(SBox)
				.HeightOverride(300.f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						IndicatorTable
					]
				]
			]
		];
	}

	// 将内容容器放在滚动框中
	Root->AddSlot()
	.FillHeight(1.f)
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			ContentBox
		]
	];

	return Root;
}

TSharedRef<SWidget> SScenarioScreen::BuildPerceptionContent()
{
	// 与决策 Tab 完全一致的面板与按钮，确保样式/间距/按钮完全相同
	UE_LOG(LogTemp, Log, TEXT("BuildPerceptionContent called - reuse BuildDecisionContent"));
	return BuildDecisionContent();
}


