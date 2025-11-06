#include "UI/Widgets/SIndicatorSelector.h"
#include "UI/Styles/ScenarioStyle.h"
#include "UI/Data/IndicatorData.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

void SIndicatorSelector::Construct(const FArguments& InArgs)
{
	// 加载指标数据
	FString ConfigPath = FPaths::ProjectContentDir() / TEXT("Config/DecisionIndicators.json");
	if (!FIndicatorDataLoader::LoadFromFile(ConfigPath, IndicatorData))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load indicator data from: %s"), *ConfigPath);
	}

	// 默认选择第一个分类和子分类
	if (IndicatorData.Categories.Num() > 0 && IndicatorData.Categories[0].SubCategories.Num() > 0)
	{
		SelectedCategoryIndex = 0;
		SelectedSubCategoryIndex = 0;
	}

	ChildSlot
	[
		SNew(SHorizontalBox)

		// 左侧：分类列表
		+ SHorizontalBox::Slot()
		.FillWidth(0.25f)
		.Padding(0.f, 0.f, 8.f, 0.f)
		[
			BuildCategoryList()
		]

		// 中间：指标列表
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		.Padding(4.f, 0.f)
		[
			BuildIndicatorList()
		]

		// 右侧：已选指标列表
		+ SHorizontalBox::Slot()
		.FillWidth(0.25f)
		.Padding(8.f, 0.f, 0.f, 0.f)
		[
			BuildSelectedList()
		]
	];
}

TSharedRef<SWidget> SIndicatorSelector::BuildCategoryList()
{
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox);

	for (int32 CatIndex = 0; CatIndex < IndicatorData.Categories.Num(); ++CatIndex)
	{
		ScrollBox->AddSlot()
		[
			MakeCategoryItem(IndicatorData.Categories[CatIndex], CatIndex)
		];
	}

	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("指标分类")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(14))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				ScrollBox
			]
		];
}

TSharedRef<SWidget> SIndicatorSelector::BuildIndicatorList()
{
	SAssignNew(IndicatorListScrollBox, SScrollBox);

	RefreshIndicatorList();

	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(SelectedSubCategoryIndex >= 0 && SelectedCategoryIndex >= 0 && SelectedCategoryIndex < IndicatorData.Categories.Num() ?
					FText::FromString(IndicatorData.Categories[SelectedCategoryIndex].SubCategories[SelectedSubCategoryIndex].Name) :
					FText::FromString(TEXT("请选择分类")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(14))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				IndicatorListScrollBox.ToSharedRef()
			]
		];
}

TSharedRef<SWidget> SIndicatorSelector::BuildSelectedList()
{
	SAssignNew(SelectedListScrollBox, SScrollBox);

	RefreshSelectedList();

	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SAssignNew(SelectedCountText, STextBlock)
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(14))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SelectedListScrollBox.ToSharedRef()
			]
		];
}

void SIndicatorSelector::OnCategorySelected(int32 CategoryIndex, int32 SubCategoryIndex)
{
	SelectedCategoryIndex = CategoryIndex;
	SelectedSubCategoryIndex = SubCategoryIndex;
	RefreshIndicatorList();
}

FReply SIndicatorSelector::OnIndicatorToggled(const FString& IndicatorId)
{
	if (SelectedIndicatorIds.Contains(IndicatorId))
	{
		SelectedIndicatorIds.Remove(IndicatorId);
	}
	else
	{
		SelectedIndicatorIds.Add(IndicatorId);
	}
	RefreshIndicatorList();
	RefreshSelectedList();
	return FReply::Handled();
}

FReply SIndicatorSelector::OnRemoveSelected(const FString& IndicatorId)
{
	SelectedIndicatorIds.Remove(IndicatorId);
	RefreshIndicatorList();
	RefreshSelectedList();
	return FReply::Handled();
}

void SIndicatorSelector::RefreshIndicatorList()
{
	if (!IndicatorListScrollBox.IsValid()) return;

	IndicatorListScrollBox->ClearChildren();

	if (SelectedCategoryIndex >= 0 && SelectedCategoryIndex < IndicatorData.Categories.Num() &&
		SelectedSubCategoryIndex >= 0 && SelectedSubCategoryIndex < IndicatorData.Categories[SelectedCategoryIndex].SubCategories.Num())
	{
		const FIndicatorSubCategory& SubCat = IndicatorData.Categories[SelectedCategoryIndex].SubCategories[SelectedSubCategoryIndex];
		for (const FIndicatorInfo& Indicator : SubCat.Indicators)
		{
			IndicatorListScrollBox->AddSlot()
			[
				MakeIndicatorItem(Indicator)
			];
		}
	}
}

void SIndicatorSelector::RefreshSelectedList()
{
	if (!SelectedListScrollBox.IsValid()) return;

	SelectedListScrollBox->ClearChildren();

	// 更新标题文本
	if (SelectedCountText.IsValid())
	{
		SelectedCountText->SetText(FText::FromString(FString::Printf(TEXT("已选指标 (%d)"), SelectedIndicatorIds.Num())));
	}

	for (const FString& IndicatorId : SelectedIndicatorIds)
	{
		// 查找指标名称
		FString IndicatorName;
		for (const FIndicatorCategory& Cat : IndicatorData.Categories)
		{
			for (const FIndicatorSubCategory& SubCat : Cat.SubCategories)
			{
				for (const FIndicatorInfo& Indicator : SubCat.Indicators)
				{
					if (Indicator.Id == IndicatorId)
					{
						IndicatorName = Indicator.Name;
						break;
					}
				}
				if (!IndicatorName.IsEmpty()) break;
			}
			if (!IndicatorName.IsEmpty()) break;
		}

		if (!IndicatorName.IsEmpty())
		{
			SelectedListScrollBox->AddSlot()
			[
				MakeSelectedItem(IndicatorId, IndicatorName)
			];
		}
	}
}

TSharedRef<SWidget> SIndicatorSelector::MakeCategoryItem(const FIndicatorCategory& Category, int32 CatIndex)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.Padding(8.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(ScenarioStyle::Panel * FLinearColor(1.f, 1.f, 1.f, 0.7f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Category.Name))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(13))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeSubCategoryList(CatIndex)
		];
}

TSharedRef<SWidget> SIndicatorSelector::MakeSubCategoryList(int32 CatIndex)
{
	TSharedRef<SVerticalBox> SubCatBox = SNew(SVerticalBox);
	
	if (CatIndex >= 0 && CatIndex < IndicatorData.Categories.Num())
	{
		for (int32 SubCatIndex = 0; SubCatIndex < IndicatorData.Categories[CatIndex].SubCategories.Num(); ++SubCatIndex)
		{
			SubCatBox->AddSlot().AutoHeight().Padding(4.f, 0.f, 0.f, 0.f)
			[
				MakeSubCategoryItem(IndicatorData.Categories[CatIndex].SubCategories[SubCatIndex], CatIndex, SubCatIndex)
			];
		}
	}
	
	return SubCatBox;
}

TSharedRef<SWidget> SIndicatorSelector::MakeSubCategoryItem(const FIndicatorSubCategory& SubCat, int32 CatIndex, int32 SubCatIndex)
{
	const bool bIsSelected = (SelectedCategoryIndex == CatIndex && SelectedSubCategoryIndex == SubCatIndex);
	return SNew(SButton)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		.OnClicked_Lambda([this, CatIndex, SubCatIndex]() { OnCategorySelected(CatIndex, SubCatIndex); return FReply::Handled(); })
		[
			SNew(SBorder)
			.Padding(6.f, 4.f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.BorderBackgroundColor(bIsSelected ? ScenarioStyle::Accent : FLinearColor::Transparent)
			[
				SNew(STextBlock)
				.Text(FText::FromString(SubCat.Name))
				.ColorAndOpacity(bIsSelected ? ScenarioStyle::Text : ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			]
		];
}

TSharedRef<SWidget> SIndicatorSelector::MakeIndicatorItem(const FIndicatorInfo& Indicator)
{
	const bool bIsSelected = SelectedIndicatorIds.Contains(Indicator.Id);
	return SNew(SBorder)
		.Padding(8.f, 4.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel * FLinearColor(1.f, 1.f, 1.f, 0.85f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(bIsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this, Indicator](ECheckBoxState NewState)
				{
					OnIndicatorToggled(Indicator.Id);
				})
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Indicator.Name))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::Font(12))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Indicator.Description))
					.ColorAndOpacity(ScenarioStyle::TextDim)
					.Font(ScenarioStyle::Font(11))
					.WrapTextAt(600.f)
				]
			]
		];
}

TSharedRef<SWidget> SIndicatorSelector::MakeSelectedItem(const FString& IndicatorId, const FString& IndicatorName)
{
	return SNew(SBorder)
		.Padding(6.f, 4.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel * FLinearColor(1.f, 1.f, 1.f, 0.85f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(IndicatorName))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(12))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([this, IndicatorId]() { return OnRemoveSelected(IndicatorId); })
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("×")))
					.Font(ScenarioStyle::Font(14))
				]
			]
		];
}
