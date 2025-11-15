#include "UI/Widgets/SEnvironmentBuilder.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateBrush.h"
#include "Slate/SlateBrushAsset.h"
#include "Engine/Texture2D.h"

namespace
{
	UTexture2D* LoadMapPreviewTexture(const TCHAR* Path)
	{
		return Path
			? Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, Path))
			: nullptr;
	}
}

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
			+ SVerticalBox::Slot().AutoHeight()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
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
						SNew(SBox)
						.WidthOverride(720.f)
						.HeightOverride(720.f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							[
								SAssignNew(MapPreviewImage, SImage)
								.Visibility(EVisibility::Collapsed)
								.Image(nullptr)
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SAssignNew(MapPreviewText, STextBlock)
								.ColorAndOpacity(ScenarioStyle::TextDim)
								.Font(ScenarioStyle::Font(16))
								.Justification(ETextJustify::Center)
								.WrapTextAt(520.f)
							]
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
                .Text(FText::FromString(TEXT("复杂度等级")))
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
                        .Text(FText::FromString(TEXT("海杂波")))
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
                        .Text(FText::FromString(TEXT("气动热效应")))
                        .ColorAndOpacity(SelectedWeatherIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedWeatherIndex == 2 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnWeatherChanged(3); return FReply::Handled(); })
				[
                    SNew(SBorder)
                    .Padding(8.f, 4.f)
                    .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("雾天")))
                        .ColorAndOpacity(SelectedWeatherIndex == 3 ? ScenarioStyle::Accent : FLinearColor::White)
                        .Font(SelectedWeatherIndex == 3 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
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
        [ MakePresetButton(1, TEXT("一级")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(2, TEXT("二级")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(3, TEXT("三级")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(4, TEXT("四级")) ]
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
        [ MakePresetButton(5, TEXT("五级")) ];
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
        case 1: // 预设1：海杂波，反制方式全选
            SelectedWeatherIndex = 1; // 海杂波
            SelectedCountermeasures = {0,1,2};
            break;
        case 2: // 预设2：晴天，通信干扰
            SelectedWeatherIndex = 0; // 晴天
            SelectedCountermeasures.Empty(); SelectedCountermeasures.Add(1);
            break;
        case 3: // 预设3：气动热效应
            SelectedWeatherIndex = 2; // 气动热效应
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
	static const TCHAR* PreviewPaths[] =
	{
		TEXT("Texture2D'/Game/Map_Cover/Desert.Desert'"),
		TEXT("Texture2D'/Game/Map_Cover/Forest.Forest'"),
		TEXT("Texture2D'/Game/Map_Cover/Snow.Snow'"),
		nullptr
	};

	UTexture2D* DesiredTexture = nullptr;
	if (SelectedMapIndex >= 0 && SelectedMapIndex < UE_ARRAY_COUNT(PreviewPaths))
	{
		DesiredTexture = LoadMapPreviewTexture(PreviewPaths[SelectedMapIndex]);
	}

	if (DesiredTexture)
	{
		CachedPreviewTexture = DesiredTexture;

		if (!MapPreviewBrush.IsValid())
		{
			MapPreviewBrush = MakeShared<FSlateBrush>();
			MapPreviewBrush->DrawAs = ESlateBrushDrawType::Image;
			MapPreviewBrush->Tiling = ESlateBrushTileType::NoTile;
			MapPreviewBrush->ImageSize = FVector2D(720.f, 720.f);
		}

		MapPreviewBrush->SetResourceObject(DesiredTexture);
		MapPreviewBrush->ImageSize = FVector2D(720.f, 720.f);

		if (MapPreviewImage.IsValid())
		{
			MapPreviewImage->SetImage(MapPreviewBrush.Get());
			MapPreviewImage->SetDesiredSizeOverride(FVector2D(720.f, 720.f));
			MapPreviewImage->SetVisibility(EVisibility::Visible);
		}

		if (MapPreviewText.IsValid())
		{
			MapPreviewText->SetVisibility(EVisibility::Collapsed);
		}
	}
	else
	{
		CachedPreviewTexture = nullptr;

		if (MapPreviewImage.IsValid())
		{
			MapPreviewImage->SetVisibility(EVisibility::Collapsed);
		}

		if (MapPreviewText.IsValid())
		{
			const FString FallbackText = (SelectedMapIndex == 3)
				? TEXT("海边暂无预览图")
				: TEXT("请选择地图以查看预览");

			MapPreviewText->SetVisibility(EVisibility::Visible);
			MapPreviewText->SetText(FText::FromString(FallbackText));
			MapPreviewText->SetWrapTextAt(520.f);
		}
	}
}
