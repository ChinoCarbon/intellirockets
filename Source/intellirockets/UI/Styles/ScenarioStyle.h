#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

/**
 * 简单的科幻深色主题常量，便于统一样式。
 */
namespace ScenarioStyle
{
	static const FLinearColor Background = FLinearColor(0.06f, 0.10f, 0.12f, 1.f);      // 深蓝灰
	static const FLinearColor Panel = FLinearColor(0.08f, 0.14f, 0.17f, 0.95f);
	static const FLinearColor PanelBorder = FLinearColor(0.12f, 0.20f, 0.24f, 1.f);
	static const FLinearColor Accent = FLinearColor(0.03f, 0.65f, 0.76f, 1.f);          // 青色高亮
	static const FLinearColor AccentDim = FLinearColor(0.03f, 0.45f, 0.56f, 1.f);
	static const FLinearColor Text = FLinearColor(0.80f, 0.90f, 0.95f, 1.f);
	static const FLinearColor TextDim = FLinearColor(0.60f, 0.75f, 0.80f, 1.f);

	// 统一中文字体：将 CJK 字体放在 Content/Fonts/NotoSansSC-Regular.otf
	inline FSlateFontInfo Font(int32 Size, EFontHinting Hinting = EFontHinting::Auto)
	{
		// 兼容 ttf 与 otf，优先 ttf；都不存在或不可读时回退到引擎自带的 CJK 字体
		static FString Base = FPaths::ProjectContentDir() / TEXT("Fonts/NotoSansSC-Regular");
		static FString TtfPath = Base + TEXT(".ttf");
		static FString OtfPath = Base + TEXT(".otf");
		static FString EngineFallback = FPaths::EngineContentDir() / TEXT("Slate/Fonts/DroidSansFallback.ttf");

		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		FString UsePath;
		if (PF.FileExists(*TtfPath))
		{
			UsePath = TtfPath;
		}
		else if (PF.FileExists(*OtfPath))
		{
			UsePath = OtfPath;
		}
		else
		{
			UsePath = EngineFallback;
		}

		if (!PF.FileExists(*UsePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("ScenarioStyle::Font missing font, fallback failed: %s"), *UsePath);
		}
		else if (UsePath == EngineFallback)
		{
			UE_LOG(LogTemp, Log, TEXT("ScenarioStyle::Font using engine fallback: %s"), *UsePath);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("ScenarioStyle::Font using project font: %s"), *UsePath);
		}

		return FSlateFontInfo(UsePath, Size, Hinting);
	}

	inline FSlateFontInfo BoldFont(int32 Size, EFontHinting Hinting = EFontHinting::Auto)
	{
		FSlateFontInfo Info = Font(Size, Hinting);
		Info.TypefaceFontName = FName(TEXT("Bold"));
		return Info;
	}
}


