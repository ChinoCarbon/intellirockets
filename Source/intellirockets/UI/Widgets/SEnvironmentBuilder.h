#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 测评环境搭建界面：左侧地图预览，右侧环境选择器
 */
class SEnvironmentBuilder : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnvironmentBuilder) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildMapPreview();      // 左侧地图预览
	TSharedRef<SWidget> BuildEnvironmentSelector(); // 右侧环境选择器

	// 选择器组件
	TSharedRef<SWidget> BuildWeatherSelector();
	TSharedRef<SWidget> BuildTimeSelector();
	TSharedRef<SWidget> BuildMapSelector();

	// 选择响应
	void OnWeatherChanged(int32 Index);
	void OnTimeChanged(int32 Index);
	void OnMapChanged(int32 Index);

	// 更新地图预览
	void UpdateMapPreview();

private:
	int32 SelectedWeatherIndex = 0;  // 0: 晴天, 1: 雨天, 2: 雾天
	int32 SelectedTimeIndex = 0;    // 0: 白天, 1: 夜晚
	int32 SelectedMapIndex = 0;     // 0: 沙漠, 1: 森林, 2: 雪地, 3: 海边

	TSharedPtr<STextBlock> MapPreviewText;  // 地图预览文本（用于实时更新）
};
