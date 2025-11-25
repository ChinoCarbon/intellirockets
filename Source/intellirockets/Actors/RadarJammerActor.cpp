#include "Actors/RadarJammerActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Math/UnrealMathUtility.h"

ARadarJammerActor::ARadarJammerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// 创建程序化网格组件（用于生成透明球体）
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetRenderCustomDepth(false); // 禁用自定义深度，避免额外渲染层
	MeshComponent->SetRenderInMainPass(true); // 确保在主渲染通道中渲染
	MeshComponent->SetRenderInDepthPass(false); // 禁用深度通道渲染，避免双重渲染
	MeshComponent->SetTranslucentSortPriority(1); // 设置半透明排序优先级

	MeshComponent->SetRelativeLocation(FVector::ZeroVector);
}

void ARadarJammerActor::BeginPlay()
{
	Super::BeginPlay();
	
	BaseRadius = JammerRadius;
	UpdateVisualization();
}

void ARadarJammerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 如果启用了反制，半径会根据导弹距离在UpdateRadiusByMissileDistance中更新
	// 这里只处理未启用反制的情况
	if (!bCountermeasureActive)
	{
		JammerRadius = BaseRadius;
	}
	
	// 使用平滑插值更新可视化，避免闪烁
	UpdateVisualizationSmooth(DeltaTime);
}

void ARadarJammerActor::UpdateRadiusByMissileDistance(float MissileDistance)
{
	// 基于距离的反制公式：
	// 激活距离 = BaseRadius * CountermeasureActivationDistanceMultiplier
	// 这样导弹在距离干扰器中心 BaseRadius * (1 + Multiplier) 时就开始缩小
	// 例如：如果BaseRadius=5000cm，Multiplier=1.5，则激活距离=7500cm
	// 导弹在距离中心12500cm时就开始缩小，此时还在原始干扰区域（5000cm）外
	
	const float ActivationDistance = BaseRadius * CountermeasureActivationDistanceMultiplier;
	const float ActivationThreshold = BaseRadius + ActivationDistance;
	
	if (MissileDistance >= ActivationThreshold)
	{
		// 距离太远，不缩小
		JammerRadius = BaseRadius;
		bCountermeasureActive = false;
	}
	else
	{
		// 导弹接近，开始缩小
		bCountermeasureActive = true;
		
		// 计算归一化距离（0 = 在表面，1 = 在激活距离）
		// 使用更激进的曲线，让缩小更明显
		float NormalizedDistance = (ActivationThreshold - MissileDistance) / ActivationDistance;
		NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.f, 1.f);
		
		// 使用立方衰减公式，让缩小更明显、更快
		// 当导弹在表面时（NormalizedDistance = 1），半径减小到原来的5%
		// 当导弹在激活距离时（NormalizedDistance = 0），半径保持100%
		// 使用立方曲线让缩小更明显
		float NormalizedDistanceCubed = NormalizedDistance * NormalizedDistance * NormalizedDistance;
		float ReductionFactor = FMath::Lerp(1.f, 0.05f, NormalizedDistanceCubed);
		JammerRadius = BaseRadius * ReductionFactor;
		
		// 确保半径不会太小
		JammerRadius = FMath::Max(JammerRadius, BaseRadius * 0.05f);
	}
}

float ARadarJammerActor::GetActivationDistance() const
{
	return BaseRadius * CountermeasureActivationDistanceMultiplier;
}

float ARadarJammerActor::GetDetectionRadius() const
{
	return BaseRadius + GetActivationDistance();
}

void ARadarJammerActor::SetJammerRadius(float InRadius)
{
	BaseRadius = FMath::Max(100.f, InRadius);
	JammerRadius = BaseRadius;
	VisualizedRadius = JammerRadius;
	TargetVisualRadius = JammerRadius;
	LastGeneratedRadius = 0.f; // 强制重新生成网格
	UpdateVisualization();
	
	// 如果已经开始，立即更新可视化，确保半径变化可见
	if (HasActorBegunPlay())
	{
		// 强制重新生成网格，确保半径变化立即生效
		GenerateSphereMesh(VisualizedRadius);
		LastGeneratedRadius = VisualizedRadius;
	}
}

bool ARadarJammerActor::IsPointInJammerRange(const FVector& Point) const
{
	FVector Center = GetActorLocation();
	FVector ToPoint = Point - Center;
	
	// 检查是否在半球内（Z坐标必须大于等于中心点）
	if (ToPoint.Z < 0.f)
	{
		return false; // 在半球下方
	}
	
	float Distance = ToPoint.Size();
	// 注意：应该使用 BaseRadius 来判断是否在干扰区域内
	// JammerRadius 会因为反制而缩小，但干扰区域的实际范围应该基于 BaseRadius
	// 只有当导弹在原始干扰区域内时，才会被干扰（失去目标）
	// 反制只是缩小干扰区域，但不应该影响"是否在干扰区域内"的判断
	return Distance <= BaseRadius;
}

void ARadarJammerActor::SetCountermeasureActive(bool bActive)
{
	bCountermeasureActive = bActive;
	if (!bActive)
	{
		CountermeasureStrength = 0.f;
	}
}

void ARadarJammerActor::ClearCountermeasure()
{
	bCountermeasureActive = false;
	CountermeasureStrength = 0.f;
	JammerRadius = BaseRadius; // 立即恢复原始半径
	TargetVisualRadius = BaseRadius; // 更新目标可视化半径
}

void ARadarJammerActor::UpdateVisualization()
{
	// 初始化可视化半径
	VisualizedRadius = JammerRadius;
	TargetVisualRadius = JammerRadius;
	LastGeneratedRadius = 0.f; // 重置，强制重新生成
	
	// 生成球体网格
	GenerateSphereMesh(VisualizedRadius);
	LastGeneratedRadius = VisualizedRadius;

	// 创建或更新材质
	if (!DynamicMaterial)
	{
		// 尝试加载支持透明的材质（按优先级顺序）
		UMaterialInterface* BaseMaterial = nullptr;
		
		// 1. 优先尝试加载自定义透明材质
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_RadarJammerTranslucent"));
		if (BaseMaterial)
		{
			UE_LOG(LogTemp, Log, TEXT("RadarJammerActor: Loaded custom translucent material"));
		}
		
		// 2. 尝试使用StarterContent中的透明材质（但这些可能有反射）
		// 跳过这些，因为它们可能有反射效果
		
		// 3. 尝试使用引擎内置的可能支持透明的材质
		if (!BaseMaterial)
		{
			BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EditorMaterials/WidgetMaterial"));
		}
		if (!BaseMaterial)
		{
			BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EditorMaterials/UnlitColor"));
		}
		
		// 4. 最后使用基础材质（可能不支持透明）
		if (!BaseMaterial)
		{
			BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
			UE_LOG(LogTemp, Warning, TEXT("RadarJammerActor: Using BasicShapeMaterial (may not support transparency). Please create a translucent material at Content/Materials/M_RadarJammerTranslucent"));
		}
		
		if (BaseMaterial)
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (DynamicMaterial)
			{
				// 设置颜色和透明度参数
				DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.2f, 0.4f, 1.f, 1.f));
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.4f, 1.f, 0.3f));
				DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.3f);
				DynamicMaterial->SetScalarParameterValue(TEXT("OpacityMask"), 0.3f);
				DynamicMaterial->SetScalarParameterValue(TEXT("TranslucentOpacity"), 0.3f);
				DynamicMaterial->SetScalarParameterValue(TEXT("OpacityAmount"), 0.3f);
				
				// 尝试减少反射效果（如果材质支持这些参数）
				DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.f); // 无金属度
				DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), 1.f); // 完全粗糙，无反射
				DynamicMaterial->SetScalarParameterValue(TEXT("Specular"), 0.f); // 无高光
				
				MeshComponent->SetMaterial(0, DynamicMaterial);
				MeshComponent->SetTranslucentSortPriority(1);
				
				UE_LOG(LogTemp, Log, TEXT("RadarJammerActor: Material instance created. If it looks reflective, create an Unlit translucent material at Content/Materials/M_RadarJammerTranslucent"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("RadarJammerActor: Failed to load any material!"));
		}
	}
	
	// 确保网格组件可见
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
}

void ARadarJammerActor::UpdateVisualizationSmooth(float DeltaTime)
{
	// 更新目标半径
	TargetVisualRadius = JammerRadius;
	
	// 使用平滑插值，避免闪烁（插值速度：每秒变化50%的差值）
	const float InterpSpeed = 5.f; // 每秒插值速度
	const float RadiusDelta = FMath::Abs(TargetVisualRadius - VisualizedRadius);
	
	// 如果差值很小，直接设置目标值，避免微小抖动
	if (RadiusDelta < 1.f)
	{
		VisualizedRadius = TargetVisualRadius;
	}
	else
	{
		// 平滑插值到目标半径
		VisualizedRadius = FMath::FInterpTo(VisualizedRadius, TargetVisualRadius, DeltaTime, InterpSpeed);
	}
	
	// 如果半径变化，重新生成球体网格
	const float MeshRadiusDelta = FMath::Abs(VisualizedRadius - LastGeneratedRadius);
	if (MeshRadiusDelta > 1.f) // 如果半径变化超过1cm，重新生成网格
	{
		GenerateSphereMesh(VisualizedRadius);
		LastGeneratedRadius = VisualizedRadius;
	}
}

void ARadarJammerActor::GenerateSphereMesh(float Radius)
{
	// 清除现有网格
	MeshComponent->ClearAllMeshSections();
	
	// 球体生成参数
	const int32 Segments = 32; // 球体的分段数（越高越平滑，但性能开销越大）
	const int32 Rings = 16;    // 球体的环数
	
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	
	// 生成球体顶点
	for (int32 Ring = 0; Ring <= Rings; ++Ring)
	{
		float Theta = Ring * UE_PI / Rings; // 从0到PI
		float SinTheta = FMath::Sin(Theta);
		float CosTheta = FMath::Cos(Theta);
		
		for (int32 Segment = 0; Segment <= Segments; ++Segment)
		{
			float Phi = Segment * 2.f * UE_PI / Segments; // 从0到2PI
			float SinPhi = FMath::Sin(Phi);
			float CosPhi = FMath::Cos(Phi);
			
			// 计算顶点位置
			FVector Position = FVector(
				Radius * SinTheta * CosPhi,
				Radius * SinTheta * SinPhi,
				Radius * CosTheta
			);
			
			Vertices.Add(Position);
			Normals.Add(Position.GetSafeNormal());
			UVs.Add(FVector2D((float)Segment / Segments, (float)Ring / Rings));
			
			// 计算切线
			FVector Tangent = FVector(-SinPhi, CosPhi, 0.f);
			Tangents.Add(FProcMeshTangent(Tangent, false));
		}
	}
	
	// 生成三角形索引
	for (int32 Ring = 0; Ring < Rings; ++Ring)
	{
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			int32 Current = Ring * (Segments + 1) + Segment;
			int32 Next = Current + Segments + 1;
			
			// 第一个三角形
			Triangles.Add(Current);
			Triangles.Add(Next);
			Triangles.Add(Current + 1);
			
			// 第二个三角形
			Triangles.Add(Current + 1);
			Triangles.Add(Next);
			Triangles.Add(Next + 1);
		}
	}
	
	// 创建网格section
	MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);
}

void ARadarJammerActor::CreateDefaultTranslucentMaterial()
{
	// 尝试创建一个简单的透明材质
	// 注意：在C++中直接创建支持透明的材质比较复杂
	// 这里我们尝试使用一个可能支持透明的默认材质
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	
	if (BaseMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (DynamicMaterial)
		{
			// 设置颜色
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.2f, 0.4f, 1.f, 1.f));
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.4f, 1.f, 0.3f));
			
			MeshComponent->SetMaterial(0, DynamicMaterial);
			MeshComponent->SetTranslucentSortPriority(1);
			
			UE_LOG(LogTemp, Warning, TEXT("RadarJammerActor: Using default material. For true transparency, create a material at Content/Materials/M_RadarJammerTranslucent with Blend Mode = Translucent."));
		}
	}
}


