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
void SIndicatorSelector::GetSelectedIndicatorDetails(TArray<FString>& OutIds, TArray<FString>& OutNames) const
{
    OutIds.Reset();
    OutNames.Reset();
    const FIndicatorData& DataToUse = bFilterActive ? FilteredIndicatorData : IndicatorData;
    for (const FIndicatorCategory& Cat : DataToUse.Categories)
    {
        for (const FIndicatorSubCategory& SubCat : Cat.SubCategories)
        {
            for (const FIndicatorInfo& Indicator : SubCat.Indicators)
            {
                if (SelectedIndicatorIds.Contains(Indicator.Id))
                {
                    OutIds.Add(Indicator.Id);
                    // 组合展示内容：名称（英文名）: 描述
                    FString Display = Indicator.Name;
                    if (!Indicator.NameEn.IsEmpty())
                    {
                        Display += FString::Printf(TEXT("（%s）"), *Indicator.NameEn);
                    }
                    if (!Indicator.Description.IsEmpty())
                    {
                        Display += TEXT("：");
                        Display += Indicator.Description;
                    }
                    OutNames.Add(Display);
                }
            }
        }
    }
}

void SIndicatorSelector::Construct(const FArguments& InArgs)
{
	// 加载指标数据（允许外部传入路径）
	FString ConfigPath = InArgs._IndicatorsJsonPath;
	if (ConfigPath.IsEmpty())
	{
		ConfigPath = FPaths::ProjectConfigDir() / TEXT("DecisionIndicators.json");
	}
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
	SAssignNew(CategoryListScrollBox, SScrollBox);

	RefreshCategoryList();

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
				CategoryListScrollBox.ToSharedRef()
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
				SAssignNew(IndicatorTitleText, STextBlock)
				.Text(FText::FromString(TEXT("请选择分类")))
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

	// 更新左侧高亮 & 中间标题
	RefreshCategoryList();
	if (IndicatorTitleText.IsValid())
	{
		const FIndicatorData& DataToUse = bFilterActive ? FilteredIndicatorData : IndicatorData;
		if (SelectedCategoryIndex >= 0 && SelectedCategoryIndex < DataToUse.Categories.Num() &&
			SelectedSubCategoryIndex >= 0 && SelectedSubCategoryIndex < DataToUse.Categories[SelectedCategoryIndex].SubCategories.Num())
		{
			FString Title = DataToUse.Categories[SelectedCategoryIndex].SubCategories[SelectedSubCategoryIndex].Name;
			if (bFilterActive)
			{
				FString LevelText = bFilterIsAlgorithmLevel ? TEXT("算法级") : TEXT("系统级");
				FString TypeText = bFilterIsAlgorithm ? TEXT("算法") : TEXT("分系统");
				if (FilterSelectedNames.Num() > 0)
				{
					Title = FString::Printf(TEXT("%s - %s - %s (%s)"), *Title, *LevelText, *TypeText, *FilterSelectedNames[0]);
				}
			}
			IndicatorTitleText->SetText(FText::FromString(Title));
		}
		else
		{
			IndicatorTitleText->SetText(FText::FromString(TEXT("请选择分类")));
		}
	}
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

void SIndicatorSelector::SetFilter(bool bIsAlgorithm, const TArray<FString>& SelectedNames, bool bIsAlgorithmLevel, bool bIsPerceptionTab)
{
	bFilterActive = (SelectedNames.Num() > 0);
	bFilterIsAlgorithm = bIsAlgorithm;
	FilterSelectedNames = SelectedNames;
	bFilterIsAlgorithmLevel = bIsAlgorithmLevel;
	bFilterIsPerceptionTab = bIsPerceptionTab;
	
	// 应用过滤，生成过滤后的数据
	FilteredIndicatorData.Categories.Empty();
	for (const FIndicatorCategory& Category : IndicatorData.Categories)
	{
		FIndicatorCategory FilteredCategory;
		FilteredCategory.Id = Category.Id;
		FilteredCategory.Name = Category.Name;
		
		for (const FIndicatorSubCategory& SubCat : Category.SubCategories)
		{
			FIndicatorSubCategory FilteredSubCat;
			FilteredSubCat.Id = SubCat.Id;
			FilteredSubCat.Name = SubCat.Name;
			
			for (const FIndicatorInfo& Indicator : SubCat.Indicators)
			{
				if (!bFilterActive || MatchesFilter(Indicator))
				{
					FilteredSubCat.Indicators.Add(Indicator);
				}
			}
			
			if (FilteredSubCat.Indicators.Num() > 0)
			{
				FilteredCategory.SubCategories.Add(FilteredSubCat);
			}
		}
		
		if (FilteredCategory.SubCategories.Num() > 0)
		{
			FilteredIndicatorData.Categories.Add(FilteredCategory);
		}
	}
	
	// 重置选择状态
	if (FilteredIndicatorData.Categories.Num() > 0 && FilteredIndicatorData.Categories[0].SubCategories.Num() > 0)
	{
		SelectedCategoryIndex = 0;
		SelectedSubCategoryIndex = 0;
	}
	else
	{
		SelectedCategoryIndex = -1;
		SelectedSubCategoryIndex = -1;
	}
	
	RefreshCategoryList();
	RefreshIndicatorList();
}

bool SIndicatorSelector::MatchesFilter(const FIndicatorInfo& Indicator) const
{
	if (!bFilterActive) return true;
	
	// 检查级别匹配
	FString ExpectedLevel = bFilterIsAlgorithmLevel ? TEXT("algorithm") : TEXT("system");
	if (!Indicator.Level.IsEmpty() && Indicator.Level != ExpectedLevel)
	{
		return false;
	}
	
	// 检查名称匹配
	if (bFilterIsAlgorithm)
	{
		// 选择的是算法，检查algorithmNames
		for (const FString& SelectedName : FilterSelectedNames)
		{
			if (Indicator.AlgorithmNames.Contains(SelectedName))
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		// 选择的是分系统，检查prototypeNames
		for (const FString& SelectedName : FilterSelectedNames)
		{
			if (Indicator.PrototypeNames.Contains(SelectedName))
			{
				return true;
			}
		}
		return false;
	}
}

void SIndicatorSelector::RefreshIndicatorList()
{
	if (!IndicatorListScrollBox.IsValid()) return;

	IndicatorListScrollBox->ClearChildren();

	// 使用过滤后的数据
	const FIndicatorData& DataToUse = bFilterActive ? FilteredIndicatorData : IndicatorData;
	
	if (SelectedCategoryIndex >= 0 && SelectedCategoryIndex < DataToUse.Categories.Num() &&
		SelectedSubCategoryIndex >= 0 && SelectedSubCategoryIndex < DataToUse.Categories[SelectedCategoryIndex].SubCategories.Num())
	{
		const FIndicatorSubCategory& SubCat = DataToUse.Categories[SelectedCategoryIndex].SubCategories[SelectedSubCategoryIndex];
		for (const FIndicatorInfo& Indicator : SubCat.Indicators)
		{
			IndicatorListScrollBox->AddSlot()
			[
				MakeIndicatorItem(Indicator)
			];
		}
	}
	
	// 更新标题
	if (IndicatorTitleText.IsValid())
	{
		FString Title = TEXT("指标列表");
		if (bFilterActive)
		{
			FString LevelText = bFilterIsAlgorithmLevel ? TEXT("算法级") : TEXT("系统级");
			FString TypeText = bFilterIsAlgorithm ? TEXT("算法") : TEXT("分系统");
			if (FilterSelectedNames.Num() > 0)
			{
				Title = FString::Printf(TEXT("指标列表 - %s - %s (%s)"), *LevelText, *TypeText, *FilterSelectedNames[0]);
			}
		}
		IndicatorTitleText->SetText(FText::FromString(Title));
	}
}

void SIndicatorSelector::RefreshCategoryList()
{
	if (!CategoryListScrollBox.IsValid()) return;
	CategoryListScrollBox->ClearChildren();
	
	// 使用过滤后的数据
	const FIndicatorData& DataToUse = bFilterActive ? FilteredIndicatorData : IndicatorData;
	
	for (int32 CatIndex = 0; CatIndex < DataToUse.Categories.Num(); ++CatIndex)
	{
		CategoryListScrollBox->AddSlot()
		[
			MakeCategoryItem(DataToUse.Categories[CatIndex], CatIndex)
		];
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

	const FIndicatorData& DataToUse = bFilterActive ? FilteredIndicatorData : IndicatorData;
	for (const FString& IndicatorId : SelectedIndicatorIds)
	{
		// 查找指标名称
		FString IndicatorName;
		for (const FIndicatorCategory& Cat : DataToUse.Categories)
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
	const FIndicatorData& DataToUse = bFilterActive ? FilteredIndicatorData : IndicatorData;
	
	if (CatIndex >= 0 && CatIndex < DataToUse.Categories.Num())
	{
		for (int32 SubCatIndex = 0; SubCatIndex < DataToUse.Categories[CatIndex].SubCategories.Num(); ++SubCatIndex)
		{
			SubCatBox->AddSlot().AutoHeight().Padding(4.f, 0.f, 0.f, 0.f)
			[
				MakeSubCategoryItem(DataToUse.Categories[CatIndex].SubCategories[SubCatIndex], CatIndex, SubCatIndex)
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
