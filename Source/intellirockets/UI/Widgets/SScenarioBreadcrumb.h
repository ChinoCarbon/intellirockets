#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 顶部步骤导航（类似面包屑/流程步骤）。
 */
class SScenarioBreadcrumb : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioBreadcrumb){}
		SLATE_ARGUMENT(int32, CurrentStep)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetCurrentStep(int32 InStep);

private:
	TSharedRef<SWidget> BuildSteps();

private:
	int32 CurrentStep = 0;
};


