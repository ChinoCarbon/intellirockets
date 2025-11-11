#include "UI/Widgets/MissileOverlayWidget.h"

#include "SlateOptMacros.h"
#include "Rendering/DrawElements.h"

void UMissileOverlayWidget::SetTargets(const TArray<FMissileOverlayTarget>& InTargets)
{
	Targets = InTargets;
	Invalidate(EInvalidateWidget::Paint);
}

int32 UMissileOverlayWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	int32 CurrentLayer = BaseLayer;
	for (const FMissileOverlayTarget& Target : Targets)
	{
		const FVector2D LocalCenter = AllottedGeometry.AbsoluteToLocal(Target.ScreenPosition);
		const FVector2D HalfSize = Target.BoxHalfSize;

		const FVector2D TopLeft = LocalCenter - HalfSize;
		const FVector2D TopRight = LocalCenter + FVector2D(HalfSize.X, -HalfSize.Y);
		const FVector2D BottomRight = LocalCenter + HalfSize;
		const FVector2D BottomLeft = LocalCenter + FVector2D(-HalfSize.X, HalfSize.Y);

		TArray<FVector2D> Points;
		Points.Add(TopLeft);
		Points.Add(TopRight);
		Points.Add(BottomRight);
		Points.Add(BottomLeft);
		Points.Add(TopLeft);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			CurrentLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			Target.Color,
			true,
			2.f
		);
	}

	return CurrentLayer + 1;
}


