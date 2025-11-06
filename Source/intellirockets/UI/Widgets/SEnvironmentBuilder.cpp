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
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildMapSelector()
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
					.BorderBackgroundColor(SelectedWeatherIndex == 0 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("晴天")))
						.ColorAndOpacity(SelectedWeatherIndex == 0 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedWeatherIndex == 1 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("雨天")))
						.ColorAndOpacity(SelectedWeatherIndex == 1 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedWeatherIndex == 2 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("雾天")))
						.ColorAndOpacity(SelectedWeatherIndex == 2 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedTimeIndex == 0 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("白天")))
						.ColorAndOpacity(SelectedTimeIndex == 0 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedTimeIndex == 1 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("夜晚")))
						.ColorAndOpacity(SelectedTimeIndex == 1 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedMapIndex == 0 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("沙漠")))
						.ColorAndOpacity(SelectedMapIndex == 0 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedMapIndex == 1 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("森林")))
						.ColorAndOpacity(SelectedMapIndex == 1 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedMapIndex == 2 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("雪地")))
						.ColorAndOpacity(SelectedMapIndex == 2 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
					.BorderBackgroundColor(SelectedMapIndex == 3 ? ScenarioStyle::Accent : FLinearColor::Transparent)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("海边")))
						.ColorAndOpacity(SelectedMapIndex == 3 ? ScenarioStyle::Text : ScenarioStyle::TextDim)
						.Font(ScenarioStyle::Font(12))
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
	UpdateMapPreview();
}

void SEnvironmentBuilder::OnTimeChanged(int32 Index)
{
	SelectedTimeIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Time changed to index: %d"), Index);
	UpdateMapPreview();
}

void SEnvironmentBuilder::OnMapChanged(int32 Index)
{
	SelectedMapIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("Map changed to index: %d"), Index);
	UpdateMapPreview();
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
