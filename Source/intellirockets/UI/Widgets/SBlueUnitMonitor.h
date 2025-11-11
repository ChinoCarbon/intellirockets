#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSpinBox.h"

class AActor;

class STextBlock;
class SButton;
class SBox;

class SBlueUnitMonitor : public SCompoundWidget
{
public:
	enum class EMode : uint8
	{
		Units,
		Markers
	};

	DECLARE_DELEGATE_OneParam(FFocusUnit, int32 /*UnitIndex*/);
	DECLARE_DELEGATE_OneParam(FFocusMarker, int32 /*MarkerIndex*/);
	DECLARE_DELEGATE_TwoParams(FSpawnAtMarker, int32 /*MarkerIndex*/, int32 /*SlotIndex*/);
		DECLARE_DELEGATE(FFireOneMissile);
		DECLARE_DELEGATE_OneParam(FAutoFireMissiles, int32 /*MissileCount*/);

	SLATE_BEGIN_ARGS(SBlueUnitMonitor) {}
		SLATE_ARGUMENT(EMode, Mode)
		SLATE_EVENT(FFocusUnit, OnFocusUnit)
		SLATE_EVENT(FFocusMarker, OnFocusMarker)
		SLATE_EVENT(FSpawnAtMarker, OnSpawnAtMarker)
		SLATE_EVENT(FSimpleDelegate, OnReturnCamera)
			SLATE_EVENT(FFireOneMissile, OnLaunchMissile)
			SLATE_EVENT(FAutoFireMissiles, OnAutoFireMissiles)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 刷新显示的蓝方单位列表 */
	void RefreshUnits(const TArray<TWeakObjectPtr<AActor>>& Units);

	/** 显示自定义部署点列表 */
	void RefreshMarkers(const TArray<TWeakObjectPtr<AActor>>& Markers, const TArray<int32>& MarkerCounts);

	/** 切换显示模式（单位 / 部署点） */
	void SetMode(EMode InMode);

private:
	void RebuildForUnits();
	void RebuildForMarkers();
	void BuildMarkerRow(int32 MarkerIndex, const TWeakObjectPtr<AActor>& MarkerPtr, int32 Count);
	FText MakeUnitLabel(int32 Index, const TWeakObjectPtr<AActor>& ActorPtr) const;
	FReply HandleCollapseClicked();
	void UpdateCollapseState();
		FReply HandleFireOneClicked();
		FReply HandleAutoFireClicked();
		int32 GetAutoFireCount() const { return AutoFireCount; }
		void SetAutoFireCount(int32 NewCount) { AutoFireCount = FMath::Clamp(NewCount, 1, 20); }

private:
	EMode CurrentMode = EMode::Units;
	FFocusUnit FocusDelegate;
	FFocusMarker FocusMarkerDelegate;
	FSpawnAtMarker SpawnDelegate;
	FSimpleDelegate ReturnDelegate;
		FFireOneMissile LaunchDelegate;
		FAutoFireMissiles AutoFireDelegate;
	bool bCollapsed = false;
	TSharedPtr<SVerticalBox> ListBox;
	TSharedPtr<STextBlock> TitleText;
	TSharedPtr<SVerticalBox> BodyContainer;
	TSharedPtr<STextBlock> CollapseLabel;
	TSharedPtr<SBox> ScrollWrapper;
		TSharedPtr<SSpinBox<int32>> AutoFireSpinBox;
	TArray<TWeakObjectPtr<AActor>> CachedUnits;
	TArray<TWeakObjectPtr<AActor>> CachedMarkers;
	TArray<int32> CachedMarkerCounts;
		int32 AutoFireCount = 3;
};

