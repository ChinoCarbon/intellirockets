#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;

DECLARE_DELEGATE_OneParam(FOnRowEdit, int32 /*RowIndex*/);
DECLARE_DELEGATE_OneParam(FOnRowDelete, int32 /*RowIndex*/);

/**
 * 中心表格区域（静态示例数据 + 操作按钮）。
 */
class SScenarioMainTable : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioMainTable){}
		SLATE_ARGUMENT(int32, DataPresetIndex) // 0: 决策默认数据, 1: 感知默认数据
		SLATE_EVENT(FOnRowEdit, OnRowEdit)
		SLATE_EVENT(FOnRowDelete, OnRowDelete)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Summary helpers
	void GetSelectedRowIndices(TArray<int32>& OutIndices) const;
	void GetRowTexts(int32 RowIndex, TArray<FText>& OutColumns) const;
	void GetSelectedAlgorithmNames(TArray<FString>& OutNames) const;
	
	// 全选/取消所有算法行（用于分系统表格联动）
	void SelectAllRows(bool bSelect);

	// 持久化：保存/加载当前表格内容（根据 DataPresetIndex 区分感知/决策）
	void SavePersistent() const;
	bool LoadPersistent();

private:
	struct FAlgorithmEntry
	{
		FString Mission;
		FString TaskDetail;
		FString AlgorithmName;
		FString TrainingEnvironment;
		FString TrainingData;
		FString ConfigPath;

		TArray<FText> ToTextArray() const;
	};

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildRows();
	TSharedRef<SWidget> MakeRow(int32 RowIndex);
	void RefreshRows();

	void AddAlgorithm();
	void BeginEditRow(int32 RowIndex);
	void CommitEditRow(int32 RowIndex);
	void RemoveRow(int32 RowIndex);
	void UpdateEditingBuffer(int32 ColumnIndex, const FString& NewValue);

private:
	FOnRowEdit OnRowEdit;
	FOnRowDelete OnRowDelete;

	TArray<FAlgorithmEntry> AlgorithmRows;
	TArray<bool> SelectedRows;

	int32 EditingRowIndex = INDEX_NONE;
	FAlgorithmEntry EditingBuffer;

	// 0: 决策默认数据, 1: 感知默认数据（同时用于区分保存文件名）
	int32 PresetIndex = 0;

	TSharedPtr<SScrollBox> RowScrollBox;
};


