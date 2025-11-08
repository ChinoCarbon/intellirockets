#include "UI/Widgets/SBlueUnitMonitor.h"

#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlueUnitMonitor::Construct(const FArguments& InArgs)
{
	FocusDelegate = InArgs._OnFocusUnit;
	ReturnDelegate = InArgs._OnReturnCamera;

	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.65f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("蓝方单位监控")))
				.Font(FCoreStyle::Get().GetFontStyle("EmbossedText"))
				.ColorAndOpacity(FLinearColor::Yellow)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(8.f, 4.f))
				.OnClicked_Lambda([this]()
				{
					if (ReturnDelegate.IsBound())
					{
						ReturnDelegate.Execute();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("返回初始视角")))
					.ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				ScrollBox
			]
		]
	];

	ScrollBox->AddSlot()
	[
		SAssignNew(UnitListBox, SVerticalBox)
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SBlueUnitMonitor::Refresh(const TArray<TWeakObjectPtr<AActor>>& Units)
{
	CachedUnits = Units;

	if (!UnitListBox.IsValid())
	{
		return;
	}

	UnitListBox->ClearChildren();

	int32 ValidCount = 0;
	for (int32 Index = 0; Index < CachedUnits.Num(); ++Index)
	{
		const TWeakObjectPtr<AActor>& ActorPtr = CachedUnits[Index];
		if (!ActorPtr.IsValid())
		{
			continue;
		}

		const int32 DisplayIndex = ValidCount;
		++ValidCount;

		UnitListBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(8.f, 4.f))
			.HAlign(HAlign_Left)
			.OnClicked_Lambda([this, DisplayIndex]()
			{
				if (FocusDelegate.IsBound())
				{
					FocusDelegate.Execute(DisplayIndex);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(MakeUnitLabel(DisplayIndex, ActorPtr))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
	}

	if (ValidCount == 0)
	{
		UnitListBox->AddSlot().AutoHeight().Padding(0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("暂无蓝方单位")))
			.ColorAndOpacity(FLinearColor::White)
		];
	}
}

FText SBlueUnitMonitor::MakeUnitLabel(int32 Index, const TWeakObjectPtr<AActor>& ActorPtr) const
{
	if (!ActorPtr.IsValid())
	{
		return FText::FromString(FString::Printf(TEXT("单位 %d（无效）"), Index + 1));
	}

	const FVector Location = ActorPtr->GetActorLocation();
	return FText::FromString(FString::Printf(TEXT("单位 %d  (%.0f, %.0f, %.0f)"), Index + 1, Location.X, Location.Y, Location.Z));
}

