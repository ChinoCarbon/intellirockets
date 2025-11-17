#include "UI/Widgets/SScenarioPrototypeTable.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"

namespace
{
	static FString PromptForPrototypePath()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			return FString();
		}

		void* ParentWindowHandle = nullptr;
		TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (ActiveWindow.IsValid())
		{
			const TSharedPtr<class FGenericWindow> NativeWindow = ActiveWindow->GetNativeWindow();
			ParentWindowHandle = NativeWindow.IsValid() ? NativeWindow->GetOSWindowHandle() : nullptr;
		}

		TArray<FString> OutFiles;
		const FString Title = TEXT("选择样机文件");
		const FString DefaultPath = FPaths::ProjectContentDir();
		const FString DefaultFile = TEXT("");
		const FString FileTypes = TEXT("样机文件 (*.uasset;*.json;*.txt)|*.uasset;*.json;*.txt|所有文件 (*.*)|*.*");

		if (DesktopPlatform->OpenFileDialog(
			ParentWindowHandle,
			Title,
			DefaultPath,
			DefaultFile,
			FileTypes,
			EFileDialogFlags::None,
			OutFiles))
		{
			return OutFiles.Num() > 0 ? OutFiles[0] : FString();
		}

		return FString();
	}
}

void SScenarioPrototypeTable::Construct(const FArguments& InArgs)
{
	OnRowEdit = InArgs._OnRowEdit;
	OnRowDelete = InArgs._OnRowDelete;
	PresetIndex = InArgs._DataPresetIndex;

	// 优先从持久化文件加载；如果失败则回退到默认预设
	if (!LoadPersistent())
	{
		// 根据预设选择不同的默认数据（0: 决策，1: 感知）
		if (PresetIndex == 1)
		{
			// 感知：样机更偏向传感与识别链路
			PrototypeRows = {
				{ TEXT("近程飞行器样机A"), TEXT("近距目标快速检测与跟踪，低空抗遮挡优化") },
				{ TEXT("中程飞行器样机B"), TEXT("多目标并发识别与跟踪，抗干扰融合") },
				{ TEXT("远程飞行器样机C"), TEXT("远距离热源探测与红外伪装识别") }
			};
		}
		else
		{
			// 决策：样机更偏向任务执行与协同能力
			PrototypeRows = {
				{ TEXT("作战样机Alpha"), TEXT("编队协同与任务分配，航迹重规划") },
				{ TEXT("作战样机Bravo"), TEXT("对抗策略学习与资源分配优化") },
				{ TEXT("作战样机Charlie"), TEXT("多智能体协同决策与冲突消解") }
			};
		}

		SelectedRows.Init(false, PrototypeRows.Num());
	}

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
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("分系统列表")))
					.ColorAndOpacity(ScenarioStyle::Text)
					.Font(ScenarioStyle::BoldFont(14))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.OnClicked_Lambda([this]()
					{
						AddPrototype();
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.Padding(FMargin(8.f, 4.f))
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						.BorderBackgroundColor(ScenarioStyle::Accent)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("+ 新增分系统")))
							.ColorAndOpacity(ScenarioStyle::Text)
							.Font(ScenarioStyle::Font(12))
						]
					]
				]
			]

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

TSharedRef<SWidget> SScenarioPrototypeTable::BuildHeader()
{
	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
	const TArray<FText> Headers = {
		FText::FromString(TEXT("选择")),
		FText::FromString(TEXT("样机名")),
		FText::FromString(TEXT("简介")),
		FText::FromString(TEXT("操作"))
	};

	const TArray<float> ColWidths = { 0.1f, 0.25f, 0.45f, 0.2f };

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

TSharedRef<SWidget> SScenarioPrototypeTable::BuildRows()
{
	SAssignNew(RowScrollBox, SScrollBox);
	RefreshRows();
	return RowScrollBox.ToSharedRef();
}

void SScenarioPrototypeTable::SavePersistent() const
{
	const FString FileName = (PresetIndex == 1)
		? TEXT("ScenarioPrototypes_Perception.json")
		: TEXT("ScenarioPrototypes_Decision.json");

	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	const FString FullPath = FPaths::Combine(Dir, FileName);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> RowsJson;

	for (int32 i = 0; i < PrototypeRows.Num(); ++i)
	{
		const FPrototypeEntry& Entry = PrototypeRows[i];
		TSharedRef<FJsonObject> RowObj = MakeShared<FJsonObject>();
		RowObj->SetStringField(TEXT("PrototypeName"), Entry.PrototypeName);
		RowObj->SetStringField(TEXT("Description"), Entry.Description);
		const bool bSelected = SelectedRows.IsValidIndex(i) && SelectedRows[i];
		RowObj->SetBoolField(TEXT("Selected"), bSelected);

		RowsJson.Add(MakeShared<FJsonValueObject>(RowObj));
	}

	Root->SetArrayField(TEXT("Rows"), RowsJson);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	if (FJsonSerializer::Serialize(Root, Writer))
	{
		FFileHelper::SaveStringToFile(Output, *FullPath);
	}
}

bool SScenarioPrototypeTable::LoadPersistent()
{
	const FString FileName = (PresetIndex == 1)
		? TEXT("ScenarioPrototypes_Perception.json")
		: TEXT("ScenarioPrototypes_Decision.json");

	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"));
	const FString FullPath = FPaths::Combine(Dir, FileName);

	if (!FPaths::FileExists(FullPath))
	{
		return false;
	}

	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *FullPath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* RowsJson = nullptr;
	if (!Root->TryGetArrayField(TEXT("Rows"), RowsJson) || !RowsJson)
	{
		return false;
	}

	PrototypeRows.Reset();
	SelectedRows.Reset();

	for (const TSharedPtr<FJsonValue>& Val : *RowsJson)
	{
		if (!Val.IsValid()) continue;
		const TSharedPtr<FJsonObject> RowObj = Val->AsObject();
		if (!RowObj.IsValid()) continue;

		FPrototypeEntry Entry;
		RowObj->TryGetStringField(TEXT("PrototypeName"), Entry.PrototypeName);
		RowObj->TryGetStringField(TEXT("Description"), Entry.Description);

		bool bSelected = false;
		RowObj->TryGetBoolField(TEXT("Selected"), bSelected);

		PrototypeRows.Add(Entry);
		SelectedRows.Add(bSelected);
	}

	return PrototypeRows.Num() > 0;
}

TSharedRef<SWidget> SScenarioPrototypeTable::MakeRow(int32 RowIndex)
{
	const bool bIsEditing = (EditingRowIndex == RowIndex);
	const FPrototypeEntry& DisplayData = bIsEditing ? EditingBuffer : PrototypeRows[RowIndex];
	const TArray<FText> ColumnTexts = DisplayData.ToTextArray();
	TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);

	const TArray<float> ColWidths = { 0.1f, 0.25f, 0.45f, 0.2f };

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

	// 文本列（样机名、简介）
	for (int32 Col = 0; Col < 2; ++Col)
	{
		Line->AddSlot()
		.FillWidth(ColWidths[Col + 1])
		.Padding(6.f, 2.f)
		[
			bIsEditing
			? StaticCastSharedRef<SWidget>(SNew(SEditableTextBox)
				.Text(ColumnTexts.IsValidIndex(Col) ? ColumnTexts[Col] : FText::GetEmpty())
				.Font(ScenarioStyle::Font(12))
				.OnTextChanged_Lambda([this, Col](const FText& NewText)
				{
					UpdateEditingBuffer(Col, NewText.ToString());
				}))
			: StaticCastSharedRef<SWidget>(SNew(STextBlock)
				.Text(ColumnTexts.IsValidIndex(Col) ? ColumnTexts[Col] : FText::GetEmpty())
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12)))
		];
	}

	const TSharedRef<SWidget> EditButton = bIsEditing
		? StaticCastSharedRef<SWidget>(
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Button")
			.ContentPadding(FMargin(8.f, 4.f))
			.OnClicked_Lambda([this, RowIndex]()
			{
				CommitEditRow(RowIndex);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("完成")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(11))
			])
		: StaticCastSharedRef<SWidget>(
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Button")
			.ContentPadding(FMargin(8.f, 4.f))
			.OnClicked_Lambda([this, RowIndex]()
			{
				BeginEditRow(RowIndex);
				if (OnRowEdit.IsBound())
				{
					OnRowEdit.Execute(RowIndex);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("编辑")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(11))
			]);

	// 操作列
	Line->AddSlot()
	.FillWidth(ColWidths.Last())
	.Padding(6.f, 2.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[ EditButton ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Button")
			.ContentPadding(FMargin(8.f, 4.f))
			.OnClicked_Lambda([this, RowIndex]()
			{
				if (!PrototypeRows.IsValidIndex(RowIndex))
				{
					return FReply::Handled();
				}

				const FString PickedFile = PromptForPrototypePath();
				if (!PickedFile.IsEmpty())
				{
					UE_LOG(LogTemp, Log, TEXT("加载分系统成功：%s"), *PickedFile);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("加载分系统")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(11))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "Button")
			.ContentPadding(FMargin(8.f, 4.f))
			.OnClicked_Lambda([this, RowIndex]()
			{
				RemoveRow(RowIndex);
				if (OnRowDelete.IsBound())
				{
					OnRowDelete.Execute(RowIndex);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("删除")))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(11))
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

void SScenarioPrototypeTable::RefreshRows()
{
	if (!RowScrollBox.IsValid())
	{
		return;
	}

	RowScrollBox->ClearChildren();
	for (int32 RowIndex = 0; RowIndex < PrototypeRows.Num(); ++RowIndex)
	{
		RowScrollBox->AddSlot()
		[
			MakeRow(RowIndex)
		];
	}
}

void SScenarioPrototypeTable::AddPrototype()
{
	FPrototypeEntry NewEntry;
	NewEntry.PrototypeName = TEXT("新样机");
	NewEntry.Description = TEXT("请填写样机简介");

	const int32 NewIndex = PrototypeRows.Add(NewEntry);
	SelectedRows.Add(false);

	BeginEditRow(NewIndex);
}

void SScenarioPrototypeTable::BeginEditRow(int32 RowIndex)
{
	if (!PrototypeRows.IsValidIndex(RowIndex))
	{
		return;
	}

	EditingRowIndex = RowIndex;
	EditingBuffer = PrototypeRows[RowIndex];
	RefreshRows();
}

void SScenarioPrototypeTable::CommitEditRow(int32 RowIndex)
{
	if (!PrototypeRows.IsValidIndex(RowIndex) || RowIndex != EditingRowIndex)
	{
		return;
	}

	PrototypeRows[RowIndex] = EditingBuffer;
	EditingRowIndex = INDEX_NONE;
	RefreshRows();
}

void SScenarioPrototypeTable::RemoveRow(int32 RowIndex)
{
	if (!PrototypeRows.IsValidIndex(RowIndex))
	{
		return;
	}

	PrototypeRows.RemoveAt(RowIndex);
	SelectedRows.RemoveAt(RowIndex);

	if (EditingRowIndex == RowIndex)
	{
		EditingRowIndex = INDEX_NONE;
	}
	else if (EditingRowIndex > RowIndex)
	{
		EditingRowIndex--;
	}

	RefreshRows();
}

void SScenarioPrototypeTable::UpdateEditingBuffer(int32 ColumnIndex, const FString& NewValue)
{
	switch (ColumnIndex)
	{
	case 0: EditingBuffer.PrototypeName = NewValue; break;
	case 1: EditingBuffer.Description = NewValue; break;
	default: break;
	}
}

TArray<FText> SScenarioPrototypeTable::FPrototypeEntry::ToTextArray() const
{
	return {
		FText::FromString(PrototypeName),
		FText::FromString(Description)
	};
}

void SScenarioPrototypeTable::GetSelectedRowIndices(TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	for (int32 i = 0; i < SelectedRows.Num(); ++i)
	{
		if (SelectedRows[i])
		{
			OutIndices.Add(i);
		}
	}
}

void SScenarioPrototypeTable::GetRowTexts(int32 RowIndex, TArray<FText>& OutColumns) const
{
	OutColumns.Reset();
	if (!PrototypeRows.IsValidIndex(RowIndex))
	{
		return;
	}

	const TArray<FText> Texts = PrototypeRows[RowIndex].ToTextArray();
	OutColumns.Append(Texts);
}


