#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;

DECLARE_DELEGATE_OneParam(FOnRowEdit, int32 /*RowIndex*/);
DECLARE_DELEGATE_OneParam(FOnRowDelete, int32 /*RowIndex*/);

/**
 * 样机列表表格组件
 */
class SScenarioPrototypeTable : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioPrototypeTable){}
		SLATE_EVENT(FOnRowEdit, OnRowEdit)
		SLATE_EVENT(FOnRowDelete, OnRowDelete)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Summary helpers
	void GetSelectedRowIndices(TArray<int32>& OutIndices) const;
	void GetRowTexts(int32 RowIndex, TArray<FText>& OutColumns) const;

private:
	struct FPrototypeEntry
	{
		FString PrototypeName;
		FString Description;

		TArray<FText> ToTextArray() const;
	};

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildRows();
	TSharedRef<SWidget> MakeRow(int32 RowIndex);
	void RefreshRows();

	void AddPrototype();
	void BeginEditRow(int32 RowIndex);
	void CommitEditRow(int32 RowIndex);
	void RemoveRow(int32 RowIndex);
	void UpdateEditingBuffer(int32 ColumnIndex, const FString& NewValue);

private:
	FOnRowEdit OnRowEdit;
	FOnRowDelete OnRowDelete;

	TArray<FPrototypeEntry> PrototypeRows;
	TArray<bool> SelectedRows;

	int32 EditingRowIndex = INDEX_NONE;
	FPrototypeEntry EditingBuffer;

	TSharedPtr<SScrollBox> RowScrollBox;
};


