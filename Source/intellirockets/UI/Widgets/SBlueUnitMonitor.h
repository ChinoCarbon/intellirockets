#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AActor;

class SBlueUnitMonitor : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FFocusUnit, int32 /*UnitIndex*/);

	SLATE_BEGIN_ARGS(SBlueUnitMonitor) {}
		SLATE_EVENT(FFocusUnit, OnFocusUnit)
		SLATE_EVENT(FSimpleDelegate, OnReturnCamera)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 刷新显示的蓝方单位列表 */
	void Refresh(const TArray<TWeakObjectPtr<AActor>>& Units);

private:
	FText MakeUnitLabel(int32 Index, const TWeakObjectPtr<AActor>& ActorPtr) const;

private:
	FFocusUnit FocusDelegate;
	FSimpleDelegate ReturnDelegate;
	TSharedPtr<SVerticalBox> UnitListBox;
	TArray<TWeakObjectPtr<AActor>> CachedUnits;
};

