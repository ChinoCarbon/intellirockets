#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_OneParam(FOnRowEdit, int32 /*RowIndex*/);
DECLARE_DELEGATE_OneParam(FOnRowSave, int32 /*RowIndex*/);
DECLARE_DELEGATE_OneParam(FOnRowDelete, int32 /*RowIndex*/);

/**
 * 中心表格区域（静态示例数据 + 操作按钮）。
 */
class SScenarioMainTable : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioMainTable){}
		SLATE_EVENT(FOnRowEdit, OnRowEdit)
		SLATE_EVENT(FOnRowSave, OnRowSave)
		SLATE_EVENT(FOnRowDelete, OnRowDelete)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

    // Summary helpers
    void GetSelectedRowIndices(TArray<int32>& OutIndices) const { OutIndices.Reset(); for (int32 i=0;i<SelectedRows.Num();++i){ if(SelectedRows[i]) OutIndices.Add(i);} }
    void GetRowTexts(int32 RowIndex, TArray<FText>& OutColumns) const { OutColumns = (RowIndex>=0 && RowIndex<DemoRows.Num())? DemoRows[RowIndex] : TArray<FText>{}; }

private:
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildRows();
	TSharedRef<SWidget> MakeRow(int32 RowIndex);

private:
	FOnRowEdit OnRowEdit;
	FOnRowSave OnRowSave;
	FOnRowDelete OnRowDelete;

	TArray<TArray<FText>> DemoRows; // [row][col]
	TArray<bool> SelectedRows;      // 选择状态
};


