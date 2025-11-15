#include "UI/Widgets/SScenarioBreadcrumb.h"
#include "UI/Styles/ScenarioStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"

void SScenarioBreadcrumb::Construct(const FArguments& InArgs)
{
	CurrentStep = InArgs._CurrentStep;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(ScenarioStyle::Panel)
		[
			BuildSteps()
		]
	];
}

void SScenarioBreadcrumb::SetCurrentStep(int32 InStep)
{
	CurrentStep = InStep;
	ChildSlot.AttachWidget(BuildSteps());
}

TSharedRef<SWidget> SScenarioBreadcrumb::BuildSteps()
{
	static const TArray<FText> Steps = {
		FText::FromString(TEXT("被测对象和测试方法选择")),
		FText::FromString(TEXT("指标体系构建")),
		FText::FromString(TEXT("测评环境搭建")),
		FText::FromString(TEXT("测评流程控制")),
		FText::FromString(TEXT("测评结果分析可视化"))
	};

	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		const bool bActive = Index <= CurrentStep;
		Box->AddSlot()
		.AutoWidth()
		.Padding(6.f, 2.f)
		[
			SNew(STextBlock)
			.Text(Steps[Index])
			.ColorAndOpacity(bActive ? ScenarioStyle::Accent : ScenarioStyle::TextDim)
			.Font(ScenarioStyle::Font(14))
		];

		if (Index < Steps.Num() - 1)
		{
			Box->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT(">>")))
				.ColorAndOpacity(ScenarioStyle::TextDim)
				.Font(ScenarioStyle::Font(12))
			];
		}
	}

	return Box;
}


