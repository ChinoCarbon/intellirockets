#include "UI/Widgets/SScenarioMainTable.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
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
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"

namespace
{
	static FString PromptForConfigPath()
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
		const FString Title = TEXT("选择算法配置文件");
		const FString DefaultPath = FPaths::ProjectContentDir();
		const FString DefaultFile = TEXT("");
		const FString FileTypes = TEXT("配置文件 (*.json;*.txt;*.cfg)|*.json;*.txt;*.cfg|所有文件 (*.*)|*.*");

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

void SScenarioMainTable::Construct(const FArguments& InArgs)
{
	OnRowEdit = InArgs._OnRowEdit;
	OnRowDelete = InArgs._OnRowDelete;
	PresetIndex = InArgs._DataPresetIndex;

	// 优先从持久化文件加载；如果失败则回退到默认预设
	if (!LoadPersistent())
	{
		if (PresetIndex == 1)
		{
			// 感知：更偏向检测/识别/跟踪任务
			AlgorithmRows = {
				{ TEXT("目标检测"), TEXT("复杂地面环境下的目标检测与识别"), TEXT("多模态检测融合"), TEXT("森林-薄雾"), TEXT("红外+可见光融合"), TEXT("/Configs/Perception/ObjectDetect.json") },
				{ TEXT("多目标跟踪"), TEXT("中距离多目标持续跟踪与遮挡判别"), TEXT("多目标数据关联"), TEXT("城郊-弱光"), TEXT("雷达+光电联合"), TEXT("/Configs/Perception/MOT.json") },
				{ TEXT("抗干扰识别"), TEXT("电磁干扰下的稳定识别与恢复"), TEXT("频谱分析与自适应选择"), TEXT("沿海-电磁复杂"), TEXT("频谱+雷达"), TEXT("/Configs/Perception/JammingResist.json") }
			};
		}
		else
		{
			// 决策：偏任务规划与协同
			AlgorithmRows = {
				{ TEXT("空地联合打击"), TEXT("压制敌方防空节点，确保突击通道"), TEXT("自适应航迹规划"), TEXT("高原沙漠"), TEXT("卫星+雷达复合侦察"), TEXT("/Configs/Strike/AdaptiveRoute.json") },
				{ TEXT("防空拦截"), TEXT("中远程导弹拦截入侵目标"), TEXT("多传感器融合决策"), TEXT("沿海城市"), TEXT("雷达+光电联合数据"), TEXT("/Configs/AirDefense/FusionDecision.json") },
				{ TEXT("海上巡航"), TEXT("护航编队并侦察异常信号"), TEXT("航迹重规划"), TEXT("近海气象多变"), TEXT("声呐+北斗定位"), TEXT("/Configs/Naval/PatrolPlanner.json") }
			};
		}

		SelectedRows.Init(false, AlgorithmRows.Num());
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
					.Text(FText::FromString(TEXT("算法列表")))
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
						AddAlgorithm();
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.Padding(FMargin(8.f, 4.f))
						.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
						.BorderBackgroundColor(ScenarioStyle::Accent)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("+ 新增算法")))
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

TSharedRef<SWidget> SScenarioMainTable::BuildHeader()
{
	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
	const TArray<FText> Headers = {
		FText::FromString(TEXT("选择")),
		FText::FromString(TEXT("作战任务")),
		FText::FromString(TEXT("任务")),
		FText::FromString(TEXT("算法名称")),
		FText::FromString(TEXT("训练环境")),
		FText::FromString(TEXT("训练数据")),
		FText::FromString(TEXT("操作"))
	};

	const TArray<float> ColWidths = { 0.07f, 0.14f, 0.20f, 0.15f, 0.15f, 0.13f, 0.16f };

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
	SAssignNew(RowScrollBox, SScrollBox);
	RefreshRows();
	return RowScrollBox.ToSharedRef();
}

void SScenarioMainTable::SavePersistent() const
{
	// 根据预设选择不同的保存文件（感知 / 决策）
	const FString FileName = (PresetIndex == 1)
		? TEXT("ScenarioAlgorithms_Perception.json")
		: TEXT("ScenarioAlgorithms_Decision.json");

	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	const FString FullPath = FPaths::Combine(Dir, FileName);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> RowsJson;

	for (int32 i = 0; i < AlgorithmRows.Num(); ++i)
	{
		const FAlgorithmEntry& Entry = AlgorithmRows[i];
		TSharedRef<FJsonObject> RowObj = MakeShared<FJsonObject>();
		RowObj->SetStringField(TEXT("Mission"), Entry.Mission);
		RowObj->SetStringField(TEXT("TaskDetail"), Entry.TaskDetail);
		RowObj->SetStringField(TEXT("AlgorithmName"), Entry.AlgorithmName);
		RowObj->SetStringField(TEXT("TrainingEnvironment"), Entry.TrainingEnvironment);
		RowObj->SetStringField(TEXT("TrainingData"), Entry.TrainingData);
		RowObj->SetStringField(TEXT("ConfigPath"), Entry.ConfigPath);
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

bool SScenarioMainTable::LoadPersistent()
{
	const FString FileName = (PresetIndex == 1)
		? TEXT("ScenarioAlgorithms_Perception.json")
		: TEXT("ScenarioAlgorithms_Decision.json");

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

	AlgorithmRows.Reset();
	SelectedRows.Reset();

	for (const TSharedPtr<FJsonValue>& Val : *RowsJson)
	{
		if (!Val.IsValid()) continue;
		const TSharedPtr<FJsonObject> RowObj = Val->AsObject();
		if (!RowObj.IsValid()) continue;

		FAlgorithmEntry Entry;
		RowObj->TryGetStringField(TEXT("Mission"), Entry.Mission);
		RowObj->TryGetStringField(TEXT("TaskDetail"), Entry.TaskDetail);
		RowObj->TryGetStringField(TEXT("AlgorithmName"), Entry.AlgorithmName);
		RowObj->TryGetStringField(TEXT("TrainingEnvironment"), Entry.TrainingEnvironment);
		RowObj->TryGetStringField(TEXT("TrainingData"), Entry.TrainingData);
		RowObj->TryGetStringField(TEXT("ConfigPath"), Entry.ConfigPath);

		bool bSelected = false;
		RowObj->TryGetBoolField(TEXT("Selected"), bSelected);

		AlgorithmRows.Add(Entry);
		SelectedRows.Add(bSelected);
	}

	return AlgorithmRows.Num() > 0;
}

TSharedRef<SWidget> SScenarioMainTable::MakeRow(int32 RowIndex)
{
	const bool bIsEditing = (EditingRowIndex == RowIndex);
	const FAlgorithmEntry& DisplayData = bIsEditing ? EditingBuffer : AlgorithmRows[RowIndex];
	const TArray<FText> ColumnTexts = DisplayData.ToTextArray();
	TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);

	const TArray<float> ColWidths = { 0.07f, 0.14f, 0.20f, 0.15f, 0.15f, 0.13f, 0.16f };

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

				// 持久化选择状态
				SavePersistent();
			}
		})
	];

	// 文本列
	for (int32 Col = 0; Col < 5; ++Col)
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

	const auto BuildButtonContent = [](const FString& Label, const FLinearColor& Background) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.Padding(FMargin(12.f, 4.f))
			.BorderImage(FCoreStyle::Get().GetBrush("Button"))
			.BorderBackgroundColor(Background)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(ScenarioStyle::Text)
				.Font(ScenarioStyle::Font(12))
				.Justification(ETextJustify::Center)
			];
	};

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
				if (!AlgorithmRows.IsValidIndex(RowIndex))
				{
					return FReply::Handled();
				}

				const FString PickedFile = PromptForConfigPath();
				if (!PickedFile.IsEmpty())
				{
					AlgorithmRows[RowIndex].ConfigPath = PickedFile;
					UE_LOG(LogTemp, Log, TEXT("加载文件成功：%s"), *PickedFile);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("加载文件")))
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

void SScenarioMainTable::RefreshRows()
{
	if (!RowScrollBox.IsValid())
	{
		return;
	}

	RowScrollBox->ClearChildren();
	for (int32 RowIndex = 0; RowIndex < AlgorithmRows.Num(); ++RowIndex)
	{
		RowScrollBox->AddSlot()
		[
			MakeRow(RowIndex)
		];
	}
}

void SScenarioMainTable::AddAlgorithm()
{
	FAlgorithmEntry NewEntry;
	NewEntry.Mission = TEXT("新作战任务");
	NewEntry.TaskDetail = TEXT("请填写任务描述");
	NewEntry.AlgorithmName = TEXT("算法名称");
	NewEntry.TrainingEnvironment = TEXT("训练环境");
	NewEntry.TrainingData = TEXT("训练数据");
	NewEntry.ConfigPath = TEXT("/Configs/Custom/NewAlgorithm.json");

	const int32 NewIndex = AlgorithmRows.Add(NewEntry);
	SelectedRows.Add(false);

	BeginEditRow(NewIndex);

	SavePersistent();
}

void SScenarioMainTable::BeginEditRow(int32 RowIndex)
{
	if (!AlgorithmRows.IsValidIndex(RowIndex))
	{
		return;
	}

	EditingRowIndex = RowIndex;
	EditingBuffer = AlgorithmRows[RowIndex];
	RefreshRows();
}

void SScenarioMainTable::CommitEditRow(int32 RowIndex)
{
	if (!AlgorithmRows.IsValidIndex(RowIndex) || RowIndex != EditingRowIndex)
	{
		return;
	}

	AlgorithmRows[RowIndex] = EditingBuffer;
	EditingRowIndex = INDEX_NONE;
	RefreshRows();

	SavePersistent();
}

void SScenarioMainTable::RemoveRow(int32 RowIndex)
{
	if (!AlgorithmRows.IsValidIndex(RowIndex))
	{
		return;
	}

	AlgorithmRows.RemoveAt(RowIndex);
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

	SavePersistent();
}

void SScenarioMainTable::UpdateEditingBuffer(int32 ColumnIndex, const FString& NewValue)
{
	switch (ColumnIndex)
	{
	case 0: EditingBuffer.Mission = NewValue; break;
	case 1: EditingBuffer.TaskDetail = NewValue; break;
	case 2: EditingBuffer.AlgorithmName = NewValue; break;
	case 3: EditingBuffer.TrainingEnvironment = NewValue; break;
	case 4: EditingBuffer.TrainingData = NewValue; break;
	default: break;
	}

	if (AlgorithmRows.IsValidIndex(EditingRowIndex))
	{
		AlgorithmRows[EditingRowIndex] = EditingBuffer;
	}
}

void SScenarioMainTable::GetSelectedRowIndices(TArray<int32>& OutIndices) const
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

void SScenarioMainTable::GetRowTexts(int32 RowIndex, TArray<FText>& OutColumns) const
{
	OutColumns.Reset();
	if (!AlgorithmRows.IsValidIndex(RowIndex))
	{
		return;
	}

	const TArray<FText> Texts = AlgorithmRows[RowIndex].ToTextArray();
	OutColumns.Append(Texts);
}

void SScenarioMainTable::GetSelectedAlgorithmNames(TArray<FString>& OutNames) const
{
	OutNames.Reset();
	for (int32 i = 0; i < SelectedRows.Num(); ++i)
	{
		if (SelectedRows[i] && AlgorithmRows.IsValidIndex(i))
		{
			OutNames.Add(AlgorithmRows[i].AlgorithmName);
		}
	}
}

TArray<FText> SScenarioMainTable::FAlgorithmEntry::ToTextArray() const
{
	return {
		FText::FromString(Mission),
		FText::FromString(TaskDetail),
		FText::FromString(AlgorithmName),
		FText::FromString(TrainingEnvironment),
		FText::FromString(TrainingData)
	};
}


