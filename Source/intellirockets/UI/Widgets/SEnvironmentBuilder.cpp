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
#include "UObject/SoftObjectPath.h"

namespace
{
	UTexture2D* LoadMapPreviewTexture(const TCHAR* Path)
	{
		if (!Path)
		{
			return nullptr;
		}
		
		// 首先尝试使用完整路径
		UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, Path));
		if (Texture)
		{
			return Texture;
		}
		
		// 如果失败，尝试使用 FSoftObjectPath 和 LoadObject
		FSoftObjectPath SoftPath(Path);
		if (SoftPath.IsValid())
		{
			Texture = Cast<UTexture2D>(SoftPath.TryLoad());
			if (Texture)
			{
				return Texture;
			}
		}
		
		return nullptr;
	}
	
	// 尝试使用资源名称查找（更灵活，适用于资源名称可能不同的情况）
	UTexture2D* LoadMapPreviewTextureByName(const FString& BaseName)
	{
		// 尝试多种可能的资源名称格式
		FString PackagePath = TEXT("/Game/Map_Cover/");
		FString ResourceNames[] = { BaseName, BaseName + TEXT("_1"), BaseName + TEXT("_2"), BaseName + TEXT("_0") };
		
		for (const FString& ResourceName : ResourceNames)
		{
			FString FullPath = FString::Printf(TEXT("Texture2D'%s%s.%s'"), *PackagePath, *ResourceName, *ResourceName);
			UTexture2D* Texture = LoadMapPreviewTexture(*FullPath);
			if (Texture)
			{
				return Texture;
			}
		}
		
		return nullptr;
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
			// 分页按钮
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { CurrentPageIndex = 0; Construct(FArguments()); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(8.f, 4.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						.BorderBackgroundColor(CurrentPageIndex == 0 ? ScenarioStyle::Accent : ScenarioStyle::Panel)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("环境配置")))
							.ColorAndOpacity(CurrentPageIndex == 0 ? FLinearColor::White : ScenarioStyle::Text)
							.Font(CurrentPageIndex == 0 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { CurrentPageIndex = 1; Construct(FArguments()); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(8.f, 4.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						.BorderBackgroundColor(CurrentPageIndex == 1 ? ScenarioStyle::Accent : ScenarioStyle::Panel)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("编队配置")))
							.ColorAndOpacity(CurrentPageIndex == 1 ? FLinearColor::White : ScenarioStyle::Text)
							.Font(CurrentPageIndex == 1 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
			// 分页内容
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					CurrentPageIndex == 0 ? BuildPage1() : BuildPage2()
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildPage1()
{
	return SNew(SVerticalBox)
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
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildPage2()
{
	return SNew(SVerticalBox)
		// 编队配置
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("编队配置")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(14))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 8.f)
		[
			BuildEnemyForceSelector()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			BuildFriendlyForceSelector()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			BuildEquipmentCapabilitySelector()
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildFormationModeSelector()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
		[
			BuildTargetAccuracySelector()
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
	// 手动改变天气时，清除预设选择
	SelectedPresetIndex = -1;
	UE_LOG(LogTemp, Log, TEXT("Weather changed to index: %d, preset cleared"), Index);
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
    // 手动改变反制方式时，清除预设选择
    SelectedPresetIndex = -1;
    UE_LOG(LogTemp, Log, TEXT("Countermeasure toggled: %d -> %s, preset cleared"), Index, SelectedCountermeasures.Contains(Index) ? TEXT("ON") : TEXT("OFF"));
    Construct(FArguments());
}

void SEnvironmentBuilder::OnApplyPreset(int32 PresetIndex)
{
    SelectedPresetIndex = PresetIndex;
    // 反转后的预设配置：1级最简单，5级最复杂
    switch (PresetIndex)
    {
        case 1: // 复杂度1级：晴天，无反制（最简单）
            SelectedWeatherIndex = 0; // 晴天
            SelectedCountermeasures.Empty();
            break;
        case 2: // 复杂度2级：晴天，通信干扰（简单）
            SelectedWeatherIndex = 0; // 晴天
            SelectedCountermeasures.Empty(); SelectedCountermeasures.Add(1);
            break;
        case 3: // 复杂度3级：气动热效应，无反制（中等）
            SelectedWeatherIndex = 2; // 气动热效应
            SelectedCountermeasures.Empty();
            break;
        case 4: // 复杂度4级：气动热效应，通信干扰（较复杂）
            SelectedWeatherIndex = 2; // 气动热效应
            SelectedCountermeasures.Empty(); SelectedCountermeasures.Add(1);
            break;
        case 5: // 复杂度5级：海杂波，全反制（最复杂）
            SelectedWeatherIndex = 1; // 海杂波
            SelectedCountermeasures = {0,1,2};
            break;
        default:
            break;
    }
    UE_LOG(LogTemp, Log, TEXT("Preset %d (Complexity Level %d) applied. Weather=%d, Countermeasures=%d"), PresetIndex, PresetIndex, SelectedWeatherIndex, SelectedCountermeasures.Num());
    Construct(FArguments());
}

void SEnvironmentBuilder::UpdateMapPreview()
{
	static const TCHAR* PreviewPaths[] =
	{
		TEXT("Texture2D'/Game/Map_Cover/Desert.Desert'"),
		TEXT("Texture2D'/Game/Map_Cover/Forest.Forest'"),
		TEXT("Texture2D'/Game/Map_Cover/Snow.Snow'"),
		TEXT("Texture2D'/Game/Map_Cover/Sea.Sea'")
	};

	UTexture2D* DesiredTexture = nullptr;
	if (SelectedMapIndex >= 0 && SelectedMapIndex < UE_ARRAY_COUNT(PreviewPaths))
	{
		DesiredTexture = LoadMapPreviewTexture(PreviewPaths[SelectedMapIndex]);
		if (!DesiredTexture && SelectedMapIndex == 3)
		{
			// 如果Sea.Sea加载失败，尝试使用资源名称查找（更灵活）
			DesiredTexture = LoadMapPreviewTextureByName(TEXT("Sea"));
			if (DesiredTexture)
			{
				UE_LOG(LogTemp, Log, TEXT("Successfully loaded sea map preview using flexible name lookup"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to load sea map preview texture. Please check if the resource exists in /Game/Map_Cover/ and the name matches 'Sea' (or Sea_1, Sea_2, etc.) in the Content Browser."));
			}
		}
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
			MapPreviewText->SetVisibility(EVisibility::Visible);
			MapPreviewText->SetText(FText::FromString(TEXT("请选择地图以查看预览")));
			MapPreviewText->SetWrapTextAt(520.f);
		}
	}
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildEnemyForceSelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("敌方兵力")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEnemyForceChanged(0); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("无防空系统")))
							.ColorAndOpacity(SelectedEnemyForceIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEnemyForceIndex == 0 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEnemyForceChanged(1); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("基础防空系统")))
							.ColorAndOpacity(SelectedEnemyForceIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEnemyForceIndex == 1 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEnemyForceChanged(2); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("加强型防空系统")))
							.ColorAndOpacity(SelectedEnemyForceIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEnemyForceIndex == 2 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEnemyForceChanged(3); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("高级防空系统")))
							.ColorAndOpacity(SelectedEnemyForceIndex == 3 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEnemyForceIndex == 3 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEnemyForceChanged(4); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("多层体系化防空系统")))
							.ColorAndOpacity(SelectedEnemyForceIndex == 4 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEnemyForceIndex == 4 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildFriendlyForceSelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("我方兵力")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFriendlyForceChanged(0); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("无防空系统")))
							.ColorAndOpacity(SelectedFriendlyForceIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFriendlyForceIndex == 0 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFriendlyForceChanged(1); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("基础防空系统")))
							.ColorAndOpacity(SelectedFriendlyForceIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFriendlyForceIndex == 1 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFriendlyForceChanged(2); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("加强型防空系统")))
							.ColorAndOpacity(SelectedFriendlyForceIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFriendlyForceIndex == 2 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFriendlyForceChanged(3); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("高级防空系统")))
							.ColorAndOpacity(SelectedFriendlyForceIndex == 3 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFriendlyForceIndex == 3 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFriendlyForceChanged(4); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("多层体系化防空系统")))
							.ColorAndOpacity(SelectedFriendlyForceIndex == 4 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFriendlyForceIndex == 4 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildEquipmentCapabilitySelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("装备能力")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEquipmentCapabilityChanged(0); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("近程飞行器射程")))
							.ColorAndOpacity(SelectedEquipmentCapabilityIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEquipmentCapabilityIndex == 0 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEquipmentCapabilityChanged(1); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("中近程飞行器射程")))
							.ColorAndOpacity(SelectedEquipmentCapabilityIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEquipmentCapabilityIndex == 1 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEquipmentCapabilityChanged(2); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("中程飞行器射程")))
							.ColorAndOpacity(SelectedEquipmentCapabilityIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEquipmentCapabilityIndex == 2 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEquipmentCapabilityChanged(3); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("中远程飞行器射程")))
							.ColorAndOpacity(SelectedEquipmentCapabilityIndex == 3 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEquipmentCapabilityIndex == 3 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnEquipmentCapabilityChanged(4); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("远程飞行器射程")))
							.ColorAndOpacity(SelectedEquipmentCapabilityIndex == 4 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedEquipmentCapabilityIndex == 4 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildFormationModeSelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("编队方式")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFormationModeChanged(0); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("单一静态目标打击")))
							.ColorAndOpacity(SelectedFormationModeIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFormationModeIndex == 0 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFormationModeChanged(1); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("单一飞行器攻击")))
							.ColorAndOpacity(SelectedFormationModeIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFormationModeIndex == 1 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFormationModeChanged(2); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("营级多D协同攻击")))
							.ColorAndOpacity(SelectedFormationModeIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFormationModeIndex == 2 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFormationModeChanged(3); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("旅级多D协同攻击")))
							.ColorAndOpacity(SelectedFormationModeIndex == 3 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFormationModeIndex == 3 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]() { OnFormationModeChanged(4); return FReply::Handled(); })
					[
						SNew(SBorder)
						.Padding(6.f, 3.f)
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("旅级以上多D协同攻击")))
							.ColorAndOpacity(SelectedFormationModeIndex == 4 ? ScenarioStyle::Accent : FLinearColor::White)
							.Font(SelectedFormationModeIndex == 4 ? ScenarioStyle::BoldFont(11) : ScenarioStyle::Font(11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

void SEnvironmentBuilder::OnEnemyForceChanged(int32 Index)
{
	SelectedEnemyForceIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Enemy force changed to index: %d"), Index);
	Construct(FArguments());
}

void SEnvironmentBuilder::OnFriendlyForceChanged(int32 Index)
{
	SelectedFriendlyForceIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Friendly force changed to index: %d"), Index);
	Construct(FArguments());
}

void SEnvironmentBuilder::OnEquipmentCapabilityChanged(int32 Index)
{
	SelectedEquipmentCapabilityIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Equipment capability changed to index: %d"), Index);
	Construct(FArguments());
}

void SEnvironmentBuilder::OnFormationModeChanged(int32 Index)
{
	SelectedFormationModeIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Formation mode changed to index: %d"), Index);
	Construct(FArguments());
}

TSharedRef<SWidget> SEnvironmentBuilder::BuildTargetAccuracySelector()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("概略目指准确性")))
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnTargetAccuracyChanged(0); return FReply::Handled(); })
				[
					SNew(SBorder)
					.Padding(8.f, 4.f)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("高精度")))
						.ColorAndOpacity(SelectedTargetAccuracyIndex == 0 ? ScenarioStyle::Accent : FLinearColor::White)
						.Font(SelectedTargetAccuracyIndex == 0 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnTargetAccuracyChanged(1); return FReply::Handled(); })
				[
					SNew(SBorder)
					.Padding(8.f, 4.f)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("中等精度")))
						.ColorAndOpacity(SelectedTargetAccuracyIndex == 1 ? ScenarioStyle::Accent : FLinearColor::White)
						.Font(SelectedTargetAccuracyIndex == 1 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { OnTargetAccuracyChanged(2); return FReply::Handled(); })
				[
					SNew(SBorder)
					.Padding(8.f, 4.f)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("低精度")))
						.ColorAndOpacity(SelectedTargetAccuracyIndex == 2 ? ScenarioStyle::Accent : FLinearColor::White)
						.Font(SelectedTargetAccuracyIndex == 2 ? ScenarioStyle::BoldFont(12) : ScenarioStyle::Font(12))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}

void SEnvironmentBuilder::OnTargetAccuracyChanged(int32 Index)
{
	SelectedTargetAccuracyIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Target accuracy changed to index: %d"), Index);
	Construct(FArguments());
}
