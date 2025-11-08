#include "UI/SScenarioScreen.h"
#include "UI/Widgets/SScenarioBreadcrumb.h"
#include "UI/Widgets/SScenarioMainTable.h"
#include "UI/Widgets/SIndicatorSelector.h"
#include "UI/Widgets/SEnvironmentBuilder.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"

void SScenarioScreen::Construct(const FArguments& InArgs)
{
	StepIndex = InArgs._StepIndex;
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
					BuildTabWidget(0, FText::FromString(TEXT("智能感知算法测评")), ActiveTabIndex == 0)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
				[
					BuildTabWidget(1, FText::FromString(TEXT("智能决策算法测评")), ActiveTabIndex == 1)
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
        DecisionContentBox->AddSlot().FillHeight(1.f)[ BuildPerceptionContent() ];
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
    DecisionContentBox->AddSlot().AutoHeight()[ BuildDecisionContent() ];
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
			.OnPrevStep(OnPrevStep)
			.OnNextStep(OnNextStep)
			.OnSaveAll(OnSaveAll)
			.OnBackToMainMenu(OnBackToMainMenu));
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
			.OnPrevStep(OnPrevStep)
			.OnNextStep(OnNextStep)
			.OnSaveAll(OnSaveAll)
			.OnBackToMainMenu(OnBackToMainMenu));
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
	// 原第1步的内容：说明条 + 表格 + 测试方法选择
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
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 8.f)
		[
            (MainTable.IsValid()
                ? StaticCastSharedRef<SWidget>(MainTable.ToSharedRef())
                : StaticCastSharedRef<SWidget>(
                    SAssignNew(MainTable, SScenarioMainTable)
                    .OnRowEdit(FOnRowEdit::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Edit Row")); }))
                    .OnRowSave(FOnRowSave::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Save Row")); }))
                    .OnRowDelete(FOnRowDelete::CreateLambda([](int32){ UE_LOG(LogTemp, Log, TEXT("Delete Row")); }))
                  )
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
					.Text(FText::FromString(TEXT("测试方法选择")))
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
						SNew(STextBlock).Text(FText::FromString(TEXT("正交测试"))).Font(ScenarioStyle::Font(12))
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
						SNew(STextBlock).Text(FText::FromString(TEXT("单独测试"))).Font(ScenarioStyle::Font(12))
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
            (IndicatorSelector.IsValid() ? IndicatorSelector.ToSharedRef() : SAssignNew(IndicatorSelector, SIndicatorSelector))
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

	// Step1: 测评方法
	const FString MethodText = (Snapshot.TestMethodIndex == 0) ? TEXT("正交测试") : TEXT("单独测试");
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
				.Text(FText::FromString(TEXT("测评方法")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::BoldFont(13))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MethodText))
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

	// Step1：表格已选行
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
				.Text(FText::FromString(TEXT("未选择对象/算法")))
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
					.Text(FText::FromString(TEXT("已选对象/算法（来自Step1）")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::BoldFont(13))
				]
				+ SVerticalBox::Slot().AutoHeight()[ ListBox ]
			]
		];
	}

	// Step3 card（使用快照数据展示）
	{
		static const TCHAR* WeatherNames[] = { TEXT("晴天"), TEXT("雨天"), TEXT("雾天") };
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
		[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("预设：%s"), Snapshot.PresetIndex >= 0 ? *FString::Printf(TEXT("预设%d"), Snapshot.PresetIndex) : TEXT("未使用预设")))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];

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

	OutConfig.SelectedTableRowIndices.Reset();
	OutConfig.SelectedTableRowTexts.Reset();
	if (MainTable.IsValid())
	{
		MainTable->GetSelectedRowIndices(OutConfig.SelectedTableRowIndices);
		for (int32 idx : OutConfig.SelectedTableRowIndices)
		{
			TArray<FText> Cols;
			MainTable->GetRowTexts(idx, Cols);
			TArray<FString> Row;
			for (const FText& Txt : Cols)
			{
				Row.Add(Txt.ToString());
			}
			OutConfig.SelectedTableRowTexts.Add(Row);
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
		TEXT("/Game/Maps/Snow/SnowMap"),
		TEXT("/Game/Maps/Coast/CoastMap")
	};
	if (OutConfig.MapIndex >= 0 && OutConfig.MapIndex < UE_ARRAY_COUNT(MapLevelPaths))
	{
		OutConfig.MapLevelName = FName(MapLevelPaths[OutConfig.MapIndex]);
	}
	else
	{
		OutConfig.MapLevelName = NAME_None;
	}

	return true;
}

TSharedRef<SWidget> SScenarioScreen::BuildDecisionStep5()
{
	return SNew(SSpacer);
}

TSharedRef<SWidget> SScenarioScreen::BuildPerceptionContent()
{
	// 智能感知算法测评的内容：居中、较大、带边框包裹的空面板
	UE_LOG(LogTemp, Log, TEXT("BuildPerceptionContent called - returning centered bordered panel"));
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.Padding(1.f)
				.BorderImage(FCoreStyle::Get().GetBrush("Border"))
				[
					SNew(SBorder)
					.Padding(30.f)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					.BorderBackgroundColor(ScenarioStyle::Panel)
					[
						SNew(SSpacer)
						.Size(FVector2D(800.f, 420.f))
					]
				]
			]
		];
}


