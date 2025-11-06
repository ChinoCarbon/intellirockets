#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UI/Data/IndicatorData.h"

/**
 * 指标选择器：左侧分类列表，中间指标列表，右侧已选指标列表
 */
class SIndicatorSelector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SIndicatorSelector) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildCategoryList();      // 左侧分类列表
	TSharedRef<SWidget> BuildIndicatorList();    // 中间指标列表
	TSharedRef<SWidget> BuildSelectedList();     // 右侧已选指标列表

	// 按钮响应
	void OnCategorySelected(int32 CategoryIndex, int32 SubCategoryIndex);
	FReply OnIndicatorToggled(const FString& IndicatorId);
	FReply OnRemoveSelected(const FString& IndicatorId);

	// 辅助方法
	TSharedRef<SWidget> MakeCategoryItem(const FIndicatorCategory& Category, int32 CatIndex);
	TSharedRef<SWidget> MakeSubCategoryList(int32 CatIndex);
	TSharedRef<SWidget> MakeSubCategoryItem(const FIndicatorSubCategory& SubCat, int32 CatIndex, int32 SubCategoryIndex);
	TSharedRef<SWidget> MakeIndicatorItem(const FIndicatorInfo& Indicator);
	TSharedRef<SWidget> MakeSelectedItem(const FString& IndicatorId, const FString& IndicatorName);

private:
	void RefreshIndicatorList();
	void RefreshSelectedList();

private:
	FIndicatorData IndicatorData;
	int32 SelectedCategoryIndex = -1;
	int32 SelectedSubCategoryIndex = -1;
	TSet<FString> SelectedIndicatorIds;  // 已选择的指标ID集合

	TSharedPtr<SScrollBox> IndicatorListScrollBox;
	TSharedPtr<SScrollBox> SelectedListScrollBox;
	TSharedPtr<STextBlock> SelectedCountText;  // 已选指标数量文本
};
