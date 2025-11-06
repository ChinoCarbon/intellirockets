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
#include "Widgets/Layout/SWrapBox.h"

void SScenarioScreen::Construct(const FArguments& InArgs)
{
	StepIndex = InArgs._StepIndex;
	OnPrevStep = InArgs._OnPrevStep;
	OnNextStep = InArgs._OnNextStep;
	OnSaveAll = InArgs._OnSaveAll;
	OnBackToMainMenu = InArgs._OnBackToMainMenu;

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
    // 汇总 Step1~3 的选择，展示为总览
    TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

    // 标题
    Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
    [
        SNew(SBorder)
        .Padding(8.f)
        .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
        .BorderBackgroundColor(ScenarioStyle::Panel)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("测评配置总览（Step1~Step3）")))
            .ColorAndOpacity(ScenarioStyle::Text)
            .Font(ScenarioStyle::Font(16))
        ]
    ];

    // Step1: 测评方法（正交/单独）
    const FString MethodText = (TestMethodIndex == 0) ? TEXT("正交测试") : TEXT("单独测试");
    Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
    [
        SNew(SBorder)
        .Padding(8.f)
        .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
        .BorderBackgroundColor(ScenarioStyle::Panel)
        [
            SNew(STextBlock)
            .Text(FText::FromString(FString::Printf(TEXT("测评方法：%s"), *MethodText)))
            .ColorAndOpacity(ScenarioStyle::Text)
            .Font(ScenarioStyle::Font(13))
        ]
    ];

    // Step2: 指标（数量与内容列表：名称（英文）: 描述）
    int32 IndicatorCount = 0; FString IndicatorsLine;
    if (IndicatorSelector.IsValid())
    {
        TArray<FString> Ids; TArray<FString> Names;
        IndicatorSelector->GetSelectedIndicatorDetails(Ids, Names);
        IndicatorCount = Names.Num();
        if (IndicatorCount > 0)
        {
            IndicatorsLine = FString::Join(Names, TEXT("；"));
        }
    }
    {
        TSharedRef<SWrapBox> Wrap = SNew(SWrapBox).UseAllottedWidth(true).InnerSlotPadding(FVector2D(6.f, 4.f));
        if (IndicatorSelector.IsValid())
        {
            TArray<FString> Ids; TArray<FString> Names;
            IndicatorSelector->GetSelectedIndicatorDetails(Ids, Names);
            for (const FString& Name : Names)
            {
                Wrap->AddSlot()
                [
                    SNew(SBorder)
                    .Padding(FMargin(8.f,4.f))
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    .BorderBackgroundColor(ScenarioStyle::Panel)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Name))
                        .ColorAndOpacity(ScenarioStyle::Text)
                        .Font(ScenarioStyle::Font(12))
                        .WrapTextAt(300.f)
                    ]
                ];
            }
        }

        Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
        [
            SNew(SBorder)
            .Padding(8.f)
            .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
            .BorderBackgroundColor(ScenarioStyle::Panel)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("已选指标（%d）"), IndicatorCount))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(13)) ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    (IndicatorCount>0)
                    ? StaticCastSharedRef<SWidget>(Wrap)
                    : StaticCastSharedRef<SWidget>( SNew(STextBlock).Text(FText::FromString(TEXT("无"))).ColorAndOpacity(ScenarioStyle::TextDim).Font(ScenarioStyle::Font(12)) )
                ]
            ]
        ];
    }

    // Step1: 表格已选行（若有）
    if (MainTable.IsValid())
    {
        TArray<int32> SelIdx; MainTable->GetSelectedRowIndices(SelIdx);
        if (SelIdx.Num() > 0)
        {
            TSharedRef<SVerticalBox> list = SNew(SVerticalBox);
            for (int32 idx : SelIdx)
            {
                TArray<FText> cols; MainTable->GetRowTexts(idx, cols);
                const FString line = FString::Printf(TEXT("• %s | %s | %s | %s | %s | %s"),
                    cols.IsValidIndex(0)? *cols[0].ToString(): TEXT(""),
                    cols.IsValidIndex(1)? *cols[1].ToString(): TEXT(""),
                    cols.IsValidIndex(2)? *cols[2].ToString(): TEXT(""),
                    cols.IsValidIndex(3)? *cols[3].ToString(): TEXT(""),
                    cols.IsValidIndex(4)? *cols[4].ToString(): TEXT(""),
                    cols.IsValidIndex(5)? *cols[5].ToString(): TEXT("")
                );
                list->AddSlot().AutoHeight()[ SNew(STextBlock).Text(FText::FromString(line)).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)).WrapTextAt(1200.f) ];
            }
            Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
            [
                SNew(SBorder).Padding(8.f).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).BorderBackgroundColor(ScenarioStyle::Panel)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(FText::FromString(TEXT("已选对象/算法（来自Step1）"))).ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(13)) ]
                    + SVerticalBox::Slot().AutoHeight()[ list ]
                ]
            ];
        }
    }

    // Step3: 环境参数（天气/时段/地图/密集度/反制方式/自定义部署开关/预设）
    if (EnvironmentBuilder.IsValid())
    {
        static const TCHAR* WeatherNames[] = { TEXT("晴天"), TEXT("雨天"), TEXT("雾天") };
        static const TCHAR* TimeNames[] = { TEXT("白天"), TEXT("夜晚") };
        static const TCHAR* MapNames[] = { TEXT("沙漠"), TEXT("森林"), TEXT("雪地"), TEXT("海边") };
        static const TCHAR* DensityNames[] = { TEXT("密集"), TEXT("正常"), TEXT("稀疏") };
        static const TCHAR* CMNames[] = { TEXT("电磁干扰"), TEXT("通信干扰"), TEXT("目标移动") };

        const int32 Wi = EnvironmentBuilder->GetWeatherIndex();
        const int32 Ti = EnvironmentBuilder->GetTimeIndex();
        const int32 Mi = EnvironmentBuilder->GetMapIndex();
        const int32 Di = EnvironmentBuilder->GetDensityIndex();
        TArray<int32> Cms; EnvironmentBuilder->GetCountermeasures(Cms);
        const bool bCustom = EnvironmentBuilder->IsBlueCustomEnabled();
        const int32 Pi = EnvironmentBuilder->GetPresetIndex();

        FString CmText;
        for (int32 i=0;i<Cms.Num();++i)
        {
            if (i>0) CmText += TEXT(", ");
            if (Cms[i] >=0 && Cms[i] <=2) CmText += CMNames[Cms[i]];
        }
        if (CmText.IsEmpty()) CmText = TEXT("无");

        Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
        [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("天气/时段/地图：%s / %s / %s"), WeatherNames[Wi], TimeNames[Ti], MapNames[Mi])))
          .ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];

        Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
        [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("密集度：%s"), DensityNames[Di])))
          .ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];

        Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
        [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("反制方式：%s"), *CmText)))
          .ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];

        Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
        [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("启用蓝方自定义部署：%s"), bCustom? TEXT("是"):TEXT("否"))))
          .ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];

        Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
        [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("预设：%d"), Pi)))
          .ColorAndOpacity(ScenarioStyle::Text).Font(ScenarioStyle::Font(12)) ];
    }

    return Box;
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


