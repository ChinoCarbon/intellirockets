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
	CurrentMode = InArgs._Mode;
	FocusDelegate = InArgs._OnFocusUnit;
	FocusMarkerDelegate = InArgs._OnFocusMarker;
	SpawnDelegate = InArgs._OnSpawnAtMarker;
	ReturnDelegate = InArgs._OnReturnCamera;
	bCollapsed = false;

	TSharedRef<SScrollBox> ScrollBox =
		SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Visible);

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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SAssignNew(TitleText, STextBlock)
					.Text(FText::FromString(TEXT("蓝方单位监控")))
					.Font(FCoreStyle::Get().GetFontStyle("EmbossedText"))
					.ColorAndOpacity(FLinearColor::Yellow)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.f, 2.f))
					.OnClicked(this, &SBlueUnitMonitor::HandleCollapseClicked)
					[
						SAssignNew(CollapseLabel, STextBlock)
						.Text(FText::FromString(TEXT("收起")))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(BodyContainer, SVerticalBox)
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
					SAssignNew(ScrollWrapper, SBox)
					.MaxDesiredHeight(540.f)
					[
						ScrollBox
					]
				]
			]
		]
	];

	ScrollBox->AddSlot()
	[
		SAssignNew(ListBox, SVerticalBox)
	];

	SetMode(CurrentMode);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SBlueUnitMonitor::RefreshUnits(const TArray<TWeakObjectPtr<AActor>>& Units)
{
	CachedUnits = Units;
	SetMode(EMode::Units);
}

void SBlueUnitMonitor::RefreshMarkers(const TArray<TWeakObjectPtr<AActor>>& Markers, const TArray<int32>& MarkerCounts)
{
	CachedMarkers = Markers;
	CachedMarkerCounts = MarkerCounts;
	SetMode(EMode::Markers);
}

void SBlueUnitMonitor::SetMode(EMode InMode)
{
	CurrentMode = InMode;

	if (!ListBox.IsValid())
	{
		return;
	}

	if (CurrentMode == EMode::Markers)
	{
		RebuildForMarkers();
	}
	else
	{
		RebuildForUnits();
	}

	UpdateCollapseState();
}

void SBlueUnitMonitor::RebuildForUnits()
{
	if (!ListBox.IsValid())
	{
		return;
	}

	if (TitleText.IsValid())
	{
		TitleText->SetText(FText::FromString(TEXT("蓝方单位监控")));
	}

	ListBox->ClearChildren();

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
		const TWeakObjectPtr<AActor> ActorForRow = ActorPtr;

		ListBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
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
				.Text(MakeUnitLabel(DisplayIndex, ActorForRow))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
	}

	if (ValidCount == 0)
	{
		ListBox->AddSlot().AutoHeight().Padding(0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("暂无蓝方单位")))
			.ColorAndOpacity(FLinearColor::White)
		];
	}
}

void SBlueUnitMonitor::RebuildForMarkers()
{
	if (!ListBox.IsValid())
	{
		return;
	}

	if (TitleText.IsValid())
	{
		TitleText->SetText(FText::FromString(TEXT("蓝方部署点")));
	}

	ListBox->ClearChildren();

	if (CachedMarkers.Num() == 0)
	{
		ListBox->AddSlot().AutoHeight().Padding(0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("未找到可用的部署点")))
			.ColorAndOpacity(FLinearColor::White)
		];
		return;
	}

	for (int32 Index = 0; Index < CachedMarkers.Num(); ++Index)
	{
		const TWeakObjectPtr<AActor>& MarkerPtr = CachedMarkers[Index];
		const int32 Count = CachedMarkerCounts.IsValidIndex(Index) ? CachedMarkerCounts[Index] : 0;
		BuildMarkerRow(Index, MarkerPtr, Count);
	}
}

void SBlueUnitMonitor::BuildMarkerRow(int32 MarkerIndex, const TWeakObjectPtr<AActor>& MarkerPtr, int32 Count)
{
	const FText Title = MarkerPtr.IsValid()
		? FText::FromString(FString::Printf(TEXT("部署点 %d  (已部署 %d)"), MarkerIndex + 1, Count))
		: FText::FromString(FString::Printf(TEXT("部署点 %d（无效）"), MarkerIndex + 1));

	ListBox->AddSlot().AutoHeight().Padding(0.f, 4.f)
	[
		SNew(SBorder)
		.Padding(6.f)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.6f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SButton)
				.ContentPadding(FMargin(6.f, 4.f))
				.OnClicked_Lambda([this, MarkerIndex]()
				{
					if (FocusMarkerDelegate.IsBound())
					{
						FocusMarkerDelegate.Execute(MarkerIndex);
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(Title)
					.ColorAndOpacity(FLinearColor::Yellow)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.f, 4.f))
					.OnClicked_Lambda([this, MarkerIndex]()
					{
						if (SpawnDelegate.IsBound())
						{
							SpawnDelegate.Execute(MarkerIndex, 0);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("部署单位1")))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.f, 4.f))
					.OnClicked_Lambda([this, MarkerIndex]()
					{
						if (SpawnDelegate.IsBound())
						{
							SpawnDelegate.Execute(MarkerIndex, 1);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("部署单位2")))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.f, 4.f))
					.OnClicked_Lambda([this, MarkerIndex]()
					{
						if (SpawnDelegate.IsBound())
						{
							SpawnDelegate.Execute(MarkerIndex, 2);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("部署单位3")))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		]
	];
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

FReply SBlueUnitMonitor::HandleCollapseClicked()
{
	bCollapsed = !bCollapsed;
	UpdateCollapseState();
	return FReply::Handled();
}

void SBlueUnitMonitor::UpdateCollapseState()
{
	if (BodyContainer.IsValid())
	{
		BodyContainer->SetVisibility(bCollapsed ? EVisibility::Collapsed : EVisibility::Visible);
	}

	if (CollapseLabel.IsValid())
	{
		CollapseLabel->SetText(bCollapsed ? FText::FromString(TEXT("展开")) : FText::FromString(TEXT("收起")));
	}
}

