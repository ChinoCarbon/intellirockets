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
		SLATE_ARGUMENT(FString, IndicatorsJsonPath) // 可选：自定义指标文件路径
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

    void GetSelectedIndicators(TArray<FString>& OutIds) const { OutIds.Reset(); for (const FString& Id : SelectedIndicatorIds) { OutIds.Add(Id); } }
    void GetSelectedIndicatorDetails(TArray<FString>& OutIds, TArray<FString>& OutNames) const;

	// 设置过滤条件：根据Step1的选择来过滤指标
	void SetFilter(bool bIsAlgorithm, const TArray<FString>& SelectedNames, bool bIsAlgorithmLevel, bool bIsPerceptionTab);

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
	void RefreshCategoryList();

private:
	FIndicatorData IndicatorData;
	FIndicatorData FilteredIndicatorData;  // 过滤后的指标数据
	int32 SelectedCategoryIndex = -1;
	int32 SelectedSubCategoryIndex = -1;
	TSet<FString> SelectedIndicatorIds;  // 已选择的指标ID集合
	
	// 过滤条件
	bool bFilterActive = false;
	bool bFilterIsAlgorithm = false;
	TArray<FString> FilterSelectedNames;
	bool bFilterIsAlgorithmLevel = false;
	bool bFilterIsPerceptionTab = false;
	
	// 检查指标是否匹配过滤条件
	bool MatchesFilter(const FIndicatorInfo& Indicator) const;

	TSharedPtr<SScrollBox> CategoryListScrollBox;
	TSharedPtr<SScrollBox> IndicatorListScrollBox;
	TSharedPtr<SScrollBox> SelectedListScrollBox;
	TSharedPtr<STextBlock> SelectedCountText;  // 已选指标数量文本
	TSharedPtr<STextBlock> IndicatorTitleText; // 中间列表标题
};
