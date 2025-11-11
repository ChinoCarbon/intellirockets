#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissileOverlayWidget.generated.h"

USTRUCT(BlueprintType)
struct FMissileOverlayTarget
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D BoxHalfSize = FVector2D(40.f, 40.f);

	UPROPERTY()
	FLinearColor Color = FLinearColor(1.f, 0.1f, 0.1f, 0.9f);
};

UCLASS()
class UMissileOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTargets(const TArray<FMissileOverlayTarget>& InTargets);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY()
	TArray<FMissileOverlayTarget> Targets;
};


