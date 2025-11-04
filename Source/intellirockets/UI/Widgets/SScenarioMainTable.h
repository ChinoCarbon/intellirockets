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

private:
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildRows();
	TSharedRef<SWidget> MakeRow(int32 RowIndex);

private:
	FOnRowEdit OnRowEdit;
	FOnRowSave OnRowSave;
	FOnRowDelete OnRowDelete;

	TArray<TArray<FText>> DemoRows; // [row][col]
};


