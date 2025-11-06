#include "UI/Widgets/SScenarioMainTable.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

void SScenarioMainTable::Construct(const FArguments& InArgs)
{
	OnRowEdit = InArgs._OnRowEdit;
	OnRowSave = InArgs._OnRowSave;
	OnRowDelete = InArgs._OnRowDelete;

	// 构造一些演示数据（10 行 * 6 列）
	const TArray<FText> UnitNames = {
		FText::FromString(TEXT("中国运载火箭技术研究院"))
	};
	for (int32 i = 0; i < 10; ++i)
	{
		TArray<FText> Row;
		Row.Add(FText::FromString(TEXT("XXXX"))); // 作战任务
		Row.Add(UnitNames[0]);                   // 研制单位
		Row.Add(FText::FromString(TEXT("XXXX"))); // 算法名称
		Row.Add(FText::FromString(TEXT("XXXX"))); // 训练环境
		Row.Add(FText::FromString(TEXT("XXXX"))); // 训练数据
		Row.Add(FText::FromString(TEXT("XXXX"))); // 仿真平台
		DemoRows.Add(MoveTemp(Row));
	}

	SelectedRows.Init(false, DemoRows.Num());

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		.Padding(10.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildHeader()
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				BuildRows()
			]
		]
	];
}

TSharedRef<SWidget> SScenarioMainTable::BuildHeader()
{
	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
	const TArray<FText> Headers = {
		FText::FromString(TEXT("选择")),
		FText::FromString(TEXT("作战任务")),
		FText::FromString(TEXT("研制单位")),
		FText::FromString(TEXT("算法名称")),
		FText::FromString(TEXT("训练环境")),
		FText::FromString(TEXT("训练数据")),
		FText::FromString(TEXT("仿真平台")),
		FText::FromString(TEXT("操作"))
	};

	const TArray<float> ColWidths = { 0.07f, 0.11f, 0.20f, 0.11f, 0.11f, 0.11f, 0.11f, 0.17f };

	for (int32 i = 0; i < Headers.Num(); ++i)
	{
		Header->AddSlot()
		.FillWidth(ColWidths[i])
		.Padding(6.f, 4.f)
		[
			SNew(STextBlock)
			.Text(Headers[i])
			.ColorAndOpacity(ScenarioStyle::Text)
			.Font(ScenarioStyle::Font(13))
		];
	}
	return Header;
}

TSharedRef<SWidget> SScenarioMainTable::BuildRows()
{
	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox);
	for (int32 RowIndex = 0; RowIndex < DemoRows.Num(); ++RowIndex)
	{
		Scroll->AddSlot()
		[
			MakeRow(RowIndex)
		];
	}
	return Scroll;
}

TSharedRef<SWidget> SScenarioMainTable::MakeRow(int32 RowIndex)
{
	const TArray<FText>& Row = DemoRows[RowIndex];
	TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);

	const TArray<float> ColWidths = { 0.07f, 0.11f, 0.20f, 0.11f, 0.11f, 0.11f, 0.11f, 0.17f };

	// 选择列（复选框）
	Line->AddSlot()
	.FillWidth(ColWidths[0])
	.Padding(6.f, 2.f)
	.VAlign(VAlign_Center)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this, RowIndex]()
		{
			return (SelectedRows.IsValidIndex(RowIndex) && SelectedRows[RowIndex]) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this, RowIndex](ECheckBoxState NewState)
		{
			if (SelectedRows.IsValidIndex(RowIndex))
			{
				SelectedRows[RowIndex] = (NewState == ECheckBoxState::Checked);
			}
		})
	];

	// 文本列
	for (int32 Col = 0; Col < Row.Num(); ++Col)
	{
		Line->AddSlot()
		.FillWidth(ColWidths[Col + 1])
		.Padding(6.f, 2.f)
		[
			SNew(STextBlock)
			.Text(Row[Col])
			.ColorAndOpacity(ScenarioStyle::TextDim)
			.Font(ScenarioStyle::Font(12))
		];
	}

	// 操作列
	Line->AddSlot()
	.FillWidth(ColWidths.Last())
	.Padding(6.f, 2.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			SNew(SButton)
			.OnClicked_Lambda([this, RowIndex]() { if (OnRowEdit.IsBound()) OnRowEdit.Execute(RowIndex); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("编辑")))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			SNew(SButton)
			.OnClicked_Lambda([this, RowIndex]() { if (OnRowSave.IsBound()) OnRowSave.Execute(RowIndex); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("保存")))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			SNew(SButton)
			.OnClicked_Lambda([this, RowIndex]() { if (OnRowDelete.IsBound()) OnRowDelete.Execute(RowIndex); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("删除")))
			]
		]
	];

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel * FLinearColor(1.f,1.f,1.f,0.85f))
		.Padding(4.f)
		[
			Line
		];
}


