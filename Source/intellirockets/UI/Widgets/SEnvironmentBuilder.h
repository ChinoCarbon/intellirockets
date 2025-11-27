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
    int32 GetEnemyForceIndex() const { return SelectedEnemyForceIndex; }
    int32 GetFriendlyForceIndex() const { return SelectedFriendlyForceIndex; }
    int32 GetEquipmentCapabilityIndex() const { return SelectedEquipmentCapabilityIndex; }
    int32 GetFormationModeIndex() const { return SelectedFormationModeIndex; }
    int32 GetTargetAccuracyIndex() const { return SelectedTargetAccuracyIndex; }

private:
	TSharedRef<SWidget> BuildMapPreview();      // 左侧地图预览
	TSharedRef<SWidget> BuildEnvironmentSelector(); // 右侧环境选择器
	TSharedRef<SWidget> BuildPage1(); // 第一页：环境配置
	TSharedRef<SWidget> BuildPage2(); // 第二页：编队配置

	// 选择器组件
	TSharedRef<SWidget> BuildWeatherSelector();
	TSharedRef<SWidget> BuildTimeSelector();
	TSharedRef<SWidget> BuildMapSelector();
	TSharedRef<SWidget> BuildBlueDeploymentSelector(); // 蓝方部署设置
	TSharedRef<SWidget> BuildDensitySelector();
	TSharedRef<SWidget> BuildCountermeasureSelector();
	TSharedRef<SWidget> BuildPresetSelector();
	TSharedRef<SWidget> BuildBlueCustomDeployment();
	TSharedRef<SWidget> BuildEnemyForceSelector();
	TSharedRef<SWidget> BuildFriendlyForceSelector();
	TSharedRef<SWidget> BuildEquipmentCapabilitySelector();
	TSharedRef<SWidget> BuildFormationModeSelector();
	TSharedRef<SWidget> BuildTargetAccuracySelector();

	// 选择响应
	void OnWeatherChanged(int32 Index);
	void OnTimeChanged(int32 Index);
	void OnMapChanged(int32 Index);
	void OnDensityChanged(int32 Index);
	void OnCountermeasureToggled(int32 Index);
	void OnApplyPreset(int32 PresetIndex);
	void OnEnemyForceChanged(int32 Index);
	void OnFriendlyForceChanged(int32 Index);
	void OnEquipmentCapabilityChanged(int32 Index);
	void OnFormationModeChanged(int32 Index);
	void OnTargetAccuracyChanged(int32 Index);

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
	int32 SelectedEnemyForceIndex = 0; // 敌方兵力：0: 无防空系统, 1: 基础防空系统, 2: 加强型防空系统, 3: 高级防空系统, 4: 多层体系化防空系统
	int32 SelectedFriendlyForceIndex = 0; // 我方兵力：同上
	int32 SelectedEquipmentCapabilityIndex = 0; // 装备能力：0: 近程飞行器射程, 1: 中近程, 2: 中程, 3: 中远程, 4: 远程
	int32 SelectedFormationModeIndex = 0; // 编队方式：0: 单一静态目标打击, 1: 单一飞行器攻击, 2: 营级多D协同攻击, 3: 旅级, 4: 旅级以上
	int32 SelectedTargetAccuracyIndex = 0; // 概略目指准确性：0: 高精度, 1: 中等精度, 2: 低精度
	int32 CurrentPageIndex = 0; // 当前页码：0: 第一页（环境参数等）, 1: 第二页（编队配置）

	TSharedPtr<SImage> MapPreviewImage;
	TSharedPtr<STextBlock> MapPreviewText;
	TSharedPtr<FSlateBrush> MapPreviewBrush;
	TWeakObjectPtr<UTexture2D> CachedPreviewTexture;
};
