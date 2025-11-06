#include "UI/Widgets/SEnvironmentBuilder.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
#include "Slate/SlateBrushAsset.h"
#include "Engine/Texture2D.h"

void SEnvironmentBuilder::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SHorizontalBox)

		// 左侧：地图预览
		+ SHorizontalBox::Slot()
		.FillWidth(0.6f)
		.Padding(0.f, 0.f, 8.f, 0.f)
		[
			BuildMapPreview()
		]

		// 右侧：环境选择器
		+ SHorizontalBox::Slot()
		.FillWidth(0.4f)
		[
			BuildEnvironmentSelector()
		]
	];
	
	// 初始化地图预览文本
	UpdateMapPreview();
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildMapPreview()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("地图预览")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(14))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SBorder)
				.Padding(1.f)
				.BorderImage(FCoreStyle::Get().GetBrush("Border"))
				[
					SNew(SBorder)
					.Padding(0.f)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					.BorderBackgroundColor(ScenarioStyle::Background)
					[
						// 地图预览内容（暂时使用文本标识，后续替换为实际地图图片）
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SAssignNew(MapPreviewText, STextBlock)
							.ColorAndOpacity(ScenarioStyle::TextDim)
							.Font(ScenarioStyle::Font(16))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildEnvironmentSelector()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SVerticalBox)
            // 预设（同级）
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("预设")))
                .ColorAndOpacity(ScenarioStyle::Text)
                .Font(ScenarioStyle::Font(14))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [
                BuildPresetSelector()
            ]
            // 分割线
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
            [
                SNew(SBorder)
                .Padding(0.f)
                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                .BorderBackgroundColor(ScenarioStyle::PanelBorder)
                [ SNew(SSpacer).Size(FVector2D(1.f,1.f)) ]
            ]
            // 环境参数设置
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("环境参数设置")))
                .ColorAndOpacity(ScenarioStyle::Text)
                .Font(ScenarioStyle::Font(14))
            ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
			[
				BuildWeatherSelector()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
			[
				BuildTimeSelector()
			]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildMapSelector()
			]
            // 分割线
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
            [
                SNew(SBorder)
                .Padding(0.f)
                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                .BorderBackgroundColor(ScenarioStyle::PanelBorder)
                [
                    SNew(SSpacer).Size(FVector2D(1.f, 1.f))
                ]
            ]
            // 蓝方部署设置
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("蓝方部署设置")))
                .ColorAndOpacity(ScenarioStyle::Text)
                .Font(ScenarioStyle::Font(14))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 12.f)
            [
                BuildDensitySelector()
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                BuildCountermeasureSelector()
            ]
            // 蓝方自定义部署
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("蓝方自定义部署")))
                .ColorAndOpacity(ScenarioStyle::Text)
                .Font(ScenarioStyle::Font(14))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                BuildBlueCustomDeployment()
            ]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildWeatherSelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("天气")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnWeatherChanged(0); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("晴天")))
                        .ColorAndOpacity(SelectedWeatherIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedWeatherIndex == 0 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnWeatherChanged(1); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("雨天")))
                        .ColorAndOpacity(SelectedWeatherIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedWeatherIndex == 1 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnWeatherChanged(2); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("雾天")))
                        .ColorAndOpacity(SelectedWeatherIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedWeatherIndex == 2 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildTimeSelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("时段")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnTimeChanged(0); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("白天")))
                        .ColorAndOpacity(SelectedTimeIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedTimeIndex == 0 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnTimeChanged(1); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("夜晚")))
                        .ColorAndOpacity(SelectedTimeIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedTimeIndex == 1 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildMapSelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("地图")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnMapChanged(0); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("沙漠")))
                        .ColorAndOpacity(SelectedMapIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedMapIndex == 0 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnMapChanged(1); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("森林")))
                        .ColorAndOpacity(SelectedMapIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedMapIndex == 1 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnMapChanged(2); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("雪地")))
                        .ColorAndOpacity(SelectedMapIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedMapIndex == 2 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnMapChanged(3); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("海边")))
                        .ColorAndOpacity(SelectedMapIndex == 3 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedMapIndex == 3 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}

void SEnvironmentBuilder::OnWeatherChanged(int32 Index)
{
	SelectedWeatherIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Weather changed to index: %d"), Index);
    Construct(FArguments());
}

void SEnvironmentBuilder::OnTimeChanged(int32 Index)
{
	SelectedTimeIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Time changed to index: %d"), Index);
    Construct(FArguments());
}

void SEnvironmentBuilder::OnMapChanged(int32 Index)
{
	SelectedMapIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Map changed to index: %d"), Index);
    Construct(FArguments());
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildDensitySelector()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("密集度")))
            .ColorAndOpacity(ScenarioStyle::Text)
            .Font(ScenarioStyle::Font(13))
        ]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() { OnDensityChanged(0); return FReply::Handled(); })
                [
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("密集")))
                        .ColorAndOpacity(SelectedDensityIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedDensityIndex == 0 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() { OnDensityChanged(1); return FReply::Handled(); })
                [
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("正常")))
                        .ColorAndOpacity(SelectedDensityIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedDensityIndex == 1 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() { OnDensityChanged(2); return FReply::Handled(); })
                [
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("稀疏")))
                        .ColorAndOpacity(SelectedDensityIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedDensityIndex == 2 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildCountermeasureSelector()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 6.f)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("反制方式（可多选）")))
            .ColorAndOpacity(ScenarioStyle::Text)
            .Font(ScenarioStyle::Font(13))
        ]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() { OnCountermeasureToggled(0); return FReply::Handled(); })
                [
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("电磁干扰")))
                        .ColorAndOpacity(SelectedCountermeasures.Contains(0) ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedCountermeasures.Contains(0) ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() { OnCountermeasureToggled(1); return FReply::Handled(); })
                [
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("通信干扰")))
                        .ColorAndOpacity(SelectedCountermeasures.Contains(1) ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedCountermeasures.Contains(1) ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() { OnCountermeasureToggled(2); return FReply::Handled(); })
                [
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("目标移动")))
                        .ColorAndOpacity(SelectedCountermeasures.Contains(2) ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedCountermeasures.Contains(2) ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildPresetSelector()
{
    auto MakePresetButton = [this](int32 Index, const FString& Label) -> TSharedRef<SWidget>
    {
        const bool bActive = (SelectedPresetIndex == Index);
        return SNew(SButton)
            .OnClicked_Lambda([this, Index]() { OnApplyPreset(Index); return FReply::Handled(); })
            [
                SNew(SBorder)
                .Padding(8.f, 4.f)
                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Label))
                    .ColorAndOpacity(bActive ? ScenarioStyle::Accent : FLinearColor::White)
                    .Font(bActive ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
                    .Justification(ETextJustify::Center)
                ]
            ];
    };

    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(1, TEXT("预设1")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(2, TEXT("预设2")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(3, TEXT("预设3")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(4, TEXT("预设4")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(5, TEXT("预设5")) ];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildBlueCustomDeployment()
{
    return SNew(SBorder)
        .Padding(6.f)
        .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
        [
            SNew(SCheckBox)
            .IsChecked(bEnableBlueCustomDeployment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
            {
                bEnableBlueCustomDeployment = (NewState == ECheckBoxState::Checked);
            })
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("启用蓝方自定义部署")))
                .ColorAndOpacity(ScenarioStyle::Text)
                .Font(ScenarioStyle::Font(12))
            ]
        ];
}

void SEnvironmentBuilder::OnDensityChanged(int32 Index)
{
    SelectedDensityIndex = Index;
    UE_LOG(LogTemp, Log, TEXT("Density changed to index: %d"), Index);
    Construct(FArguments());
}

void SEnvironmentBuilder::OnCountermeasureToggled(int32 Index)
{
    if (SelectedCountermeasures.Contains(Index))
    {
        SelectedCountermeasures.Remove(Index);
    }
    else
    {
        SelectedCountermeasures.Add(Index);
    }
    UE_LOG(LogTemp, Log, TEXT("Countermeasure toggled: %d -> %s"), Index, SelectedCountermeasures.Contains(Index) ? TEXT("ON") : TEXT("OFF"));
    Construct(FArguments());
}

void SEnvironmentBuilder::OnApplyPreset(int32 PresetIndex)
{
    SelectedPresetIndex = PresetIndex;
    // 清空/设置对应选项
    switch (PresetIndex)
    {
        case 1: // 预设1：雨天，反制方式全选
            SelectedWeatherIndex = 1; // 雨天
            SelectedCountermeasures = {0,1,2};
            break;
        case 2: // 预设2：晴天，通信干扰
            SelectedWeatherIndex = 0; // 晴天
            SelectedCountermeasures.Empty(); SelectedCountermeasures.Add(1);
            break;
        case 3: // 预设3：雾天
            SelectedWeatherIndex = 2; // 雾天
            // 不改变反制方式
            break;
        case 4: // 预设4：通信干扰 晴天
            SelectedWeatherIndex = 0; // 晴天
            SelectedCountermeasures.Empty(); SelectedCountermeasures.Add(1);
            break;
        case 5: // 预设5：晴天
            SelectedWeatherIndex = 0; // 晴天
            // 不改变反制方式
            break;
        default:
            break;
    }
    UE_LOG(LogTemp, Log, TEXT("Preset %d applied. Weather=%d, Countermeasures=%d"), PresetIndex, SelectedWeatherIndex, SelectedCountermeasures.Num());
    Construct(FArguments());
}

void SEnvironmentBuilder::UpdateMapPreview()
{
	// 更新地图预览文本
	// 实际项目中，这里应该加载对应的地图图片资源
	// 例如：LoadObject<UTexture2D>(nullptr, TEXT("/Game/Maps/Desert_Day_Sunny_Texture"))
	
	static const TArray<FString> WeatherNames = { TEXT("晴天"), TEXT("雨天"), TEXT("雾天") };
	static const TArray<FString> TimeNames = { TEXT("白天"), TEXT("夜晚") };
	static const TArray<FString> MapNames = { TEXT("沙漠"), TEXT("森林"), TEXT("雪地"), TEXT("海边") };
	
	if (MapPreviewText.IsValid())
	{
		FString PreviewText = FString::Printf(
			TEXT("%s - %s - %s\n\n地图预览区域\n(需要加载实际地图图片)\n\n路径示例：\n/Game/Maps/%s_%s_%s"),
			*WeatherNames[SelectedWeatherIndex],
			*TimeNames[SelectedTimeIndex],
			*MapNames[SelectedMapIndex],
			*MapNames[SelectedMapIndex],
			*TimeNames[SelectedTimeIndex],
			*WeatherNames[SelectedWeatherIndex]
		);
		MapPreviewText->SetText(FText::FromString(PreviewText));
	}
	
	UE_LOG(LogTemp, Log, TEXT("Map preview updated: Weather=%d (%s), Time=%d (%s), Map=%d (%s)"),
		SelectedWeatherIndex, *WeatherNames[SelectedWeatherIndex],
		SelectedTimeIndex, *TimeNames[SelectedTimeIndex],
		SelectedMapIndex, *MapNames[SelectedMapIndex]);
}
