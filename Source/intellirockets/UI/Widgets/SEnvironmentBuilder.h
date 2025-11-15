#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SImage;
struct FSlateBrush;
class UTexture2D;

/**
 * 测评环境搭建界面：左侧地图预览，右侧环境选择器
 */
class SEnvironmentBuilder : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnvironmentBuilder) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

    // Getters for summary
    int32 GetWeatherIndex() const { return SelectedWeatherIndex; }
    int32 GetTimeIndex() const { return SelectedTimeIndex; }
    int32 GetMapIndex() const { return SelectedMapIndex; }
    int32 GetDensityIndex() const { return SelectedDensityIndex; }
    void GetCountermeasures(TArray<int32>& Out) const { Out.Reset(); for (int32 Id : SelectedCountermeasures) { Out.Add(Id); } }
    int32 GetPresetIndex() const { return SelectedPresetIndex; }
    bool IsBlueCustomEnabled() const { return bEnableBlueCustomDeployment; }

private:
	TSharedRef<SWidget> BuildMapPreview();      // 左侧地图预览
	TSharedRef<SWidget> BuildEnvironmentSelector(); // 右侧环境选择器

	// 选择器组件
	TSharedRef<SWidget> BuildWeatherSelector();
	TSharedRef<SWidget> BuildTimeSelector();
	TSharedRef<SWidget> BuildMapSelector();
	TSharedRef<SWidget> BuildBlueDeploymentSelector(); // 蓝方部署设置
	TSharedRef<SWidget> BuildDensitySelector();
	TSharedRef<SWidget> BuildCountermeasureSelector();
	TSharedRef<SWidget> BuildPresetSelector();
	TSharedRef<SWidget> BuildBlueCustomDeployment();

	// 选择响应
	void OnWeatherChanged(int32 Index);
	void OnTimeChanged(int32 Index);
	void OnMapChanged(int32 Index);
	void OnDensityChanged(int32 Index);
	void OnCountermeasureToggled(int32 Index);
	void OnApplyPreset(int32 PresetIndex);

	// 更新地图预览
	void UpdateMapPreview();
	void Rebuild();

private:
	int32 SelectedWeatherIndex = 0;  // 0: 晴天, 1: 海杂波, 2: 气动热效应, 3: 雾天
	int32 SelectedTimeIndex = 0;    // 0: 白天, 1: 夜晚
	int32 SelectedMapIndex = 0;     // 0: 沙漠, 1: 森林, 2: 雪地, 3: 海边
	int32 SelectedDensityIndex = 1; // 0: 密集, 1: 正常, 2: 稀疏
	TSet<int32> SelectedCountermeasures; // 0: 电磁干扰, 1: 通信干扰, 2: 目标移动（可多选）
	int32 SelectedPresetIndex = -1; // -1: 无预设，1..5 为预设编号
	bool bEnableBlueCustomDeployment = false; // 蓝方自定义部署是否启用

	TSharedPtr<SImage> MapPreviewImage;
	TSharedPtr<STextBlock> MapPreviewText;
	TSharedPtr<FSlateBrush> MapPreviewBrush;
	TWeakObjectPtr<UTexture2D> CachedPreviewTexture;
};
