#include "Systems/ScenarioMenuSubsystem.h"
#include "UI/SScenarioScreen.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpectatorPawn.h"
#include "Math/RotationMatrix.h"
#include "UObject/SoftObjectPath.h"
#include "UI/Widgets/SBlueUnitMonitor.h"
#include "Engine/LevelBounds.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "Engine/LevelStreaming.h"
#include "TimerManager.h"

void UScenarioMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("UScenarioMenuSubsystem::Initialize"));
	WorldHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UScenarioMenuSubsystem::OnWorldReady);
}

void UScenarioMenuSubsystem::Deinitialize()
{
	if (UWorld* PendingWorld = PendingScenarioWorld.Get())
	{
		PendingWorld->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
	}
	PendingScenarioWorld = nullptr;

	FWorldDelegates::OnPostWorldInitialization.Remove(WorldHandle);
	if (GEngine && GEngine->GameViewport && Screen.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	}
	Screen.Reset();
	HideBlueMonitor();
	Super::Deinitialize();
}

void UScenarioMenuSubsystem::OnWorldReady(UWorld* World, const UWorld::InitializationValues IVs)
{
	UE_LOG(LogTemp, Log, TEXT("UScenarioMenuSubsystem::OnWorldReady world=%s IsGameWorld=%d"), World ? *World->GetName() : TEXT("null"), World ? World->IsGameWorld() : 0);
	if (World && World->IsGameWorld())
	{
		if (bIsRunningScenario && bHasPendingScenarioConfig)
		{
			PendingScenarioWorld = World;
			bPendingScenarioWaitingLogged = false;
			if (World->GetTimerManager().IsTimerActive(PendingScenarioTimerHandle))
			{
				World->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
			}
			World->GetTimerManager().SetTimer(PendingScenarioTimerHandle, FTimerDelegate::CreateUObject(this, &UScenarioMenuSubsystem::FinalizeScenarioAfterLoad), 0.25f, true);
			UE_LOG(LogTemp, Log, TEXT("OnWorldReady: deferring scenario finalization until streaming levels load."));
			return;
		}

		// 立即尝试显示，并在 0.1s 后再次尝试一次，避免初始化竞态
		Show(World);
		FTimerHandle Timer;
		World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateUObject(this, &UScenarioMenuSubsystem::Show, World), 0.1f, false);
	}
}

void UScenarioMenuSubsystem::Show(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogTemp, Warning, TEXT("Show(): GameViewport not ready"));
		return;
	}

	if (bIsRunningScenario)
	{
		UE_LOG(LogTemp, Log, TEXT("Show(): skipped because a scenario test is running."));
		return;
	}

	// 如果界面已存在，先移除
	if (Screen.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
		Screen.Reset();
	}
	HideBlueMonitor();

	SAssignNew(Screen, SScenarioScreen)
		.StepIndex(StepIndex)
		.OnPrevStep(FOnPrevStep::CreateUObject(this, &UScenarioMenuSubsystem::Prev))
		.OnNextStep(FOnNextStep::CreateUObject(this, &UScenarioMenuSubsystem::Next))
		.OnSaveAll(FOnSaveAll::CreateUObject(this, &UScenarioMenuSubsystem::SaveAll))
		.OnBackToMainMenu(FOnBackToMainMenu::CreateUObject(this, &UScenarioMenuSubsystem::BackToMainMenu))
		.OnStartTest(FOnStartTest::CreateUObject(this, &UScenarioMenuSubsystem::BeginScenarioTest));

	GEngine->GameViewport->AddViewportWidgetContent(Screen.ToSharedRef(), 100);
	UE_LOG(LogTemp, Log, TEXT("Show(): Screen added to GameViewport, StepIndex=%d"), StepIndex);

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		FInputModeUIOnly Mode; Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
}

void UScenarioMenuSubsystem::Hide(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport || !Screen.IsValid()) return;
	GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
	{
		FInputModeGameOnly Mode; PC->SetInputMode(Mode); PC->bShowMouseCursor = false;
	}
	Screen.Reset();
}

void UScenarioMenuSubsystem::Prev()
{
	StepIndex = FMath::Max(0, StepIndex - 1);
	UE_LOG(LogTemp, Log, TEXT("Prev clicked, StepIndex=%d"), StepIndex);
    if (Screen.IsValid()) { Screen->SetStepIndex(StepIndex); }
}

void UScenarioMenuSubsystem::Next()
{
	StepIndex = FMath::Min(4, StepIndex + 1); // 限制在0-4之间（5个步骤）
	UE_LOG(LogTemp, Log, TEXT("Next clicked, StepIndex=%d"), StepIndex);
    if (Screen.IsValid()) { Screen->SetStepIndex(StepIndex); }
}

void UScenarioMenuSubsystem::SaveAll()
{
	UE_LOG(LogTemp, Log, TEXT("SaveAll clicked"));
}

void UScenarioMenuSubsystem::BackToMainMenu()
{
	if (UWorld* World = GetWorld())
	{
		UKismetSystemLibrary::QuitGame(World, World->GetFirstPlayerController(), EQuitPreference::Quit, true);
	}
}

void UScenarioMenuSubsystem::BeginScenarioTest()
{
	if (!Screen.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginScenarioTest called but screen is invalid"));
		return;
	}

	FScenarioTestConfig Config;
	Screen->CollectScenarioConfig(Config);

	PendingScenarioConfig = Config;
	bHasPendingScenarioConfig = true;
	bIsRunningScenario = true;
	ClearSpawnedBlueUnits();

	if (OnScenarioTestRequested.IsBound())
	{
		OnScenarioTestRequested.Broadcast(Config);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No listeners bound to OnScenarioTestRequested; remember to start the scenario manually."));
	}

	UWorld* World = GetWorld();
	if (World)
	{
		Hide(World);
		if (!Config.MapLevelName.IsNone())
		{
			UE_LOG(LogTemp, Log, TEXT("Opening scenario map: %s"), *Config.MapLevelName.ToString());
			UGameplayStatics::OpenLevel(World, Config.MapLevelName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No map level specified for map index %d. The menu will stay on the current map. Create one at /Game/Maps/... if needed."), Config.MapIndex);
			ApplyEnvironmentSettings(World, Config);
			DeployBlueForScenario(World, Config);
			bHasPendingScenarioConfig = false;
		}
	}
}

void UScenarioMenuSubsystem::ApplyEnvironmentSettings(UWorld* World, const FScenarioTestConfig& Config)
{
	if (!World)
	{
		return;
	}
	

	// 时间段 -> 太阳角度和颜色
	if (ADirectionalLight* Dir = Cast<ADirectionalLight>(UGameplayStatics::GetActorOfClass(World, ADirectionalLight::StaticClass())))
	{
		if (UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(Dir->GetLightComponent()))
		{
			const bool bDay = Config.TimeIndex == 0;
			LightComp->SetIntensity(bDay ? 50.f : 5.f);
			LightComp->SetLightColor(bDay ? FLinearColor(1.f, 0.95f, 0.85f) : FLinearColor(0.1f, 0.15f, 0.15f));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Directional light found but has no directional light component."));
		}
	}

	// 雾密度
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		if (UExponentialHeightFogComponent* FogComp = It->GetComponent())
		{
			if (Config.WeatherIndex == 2) // 雾天
			{
				FogComp->SetFogDensity(0.9f);
			}
			else
			{
				FogComp->SetFogDensity(0.002f);
			}
		}
	}

	// 简单的降雨开关：查找带有 RainFX 标签或名字包含 Rain 的 Actor
/*	const bool bEnableRain = (Config.WeatherIndex == 1);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->ActorHasTag(TEXT("RainFX")) || Actor->GetName().Contains(TEXT("Rain")))
		{
			Actor->SetActorHiddenInGame(!bEnableRain);
			Actor->SetActorTickEnabled(bEnableRain);
		}
	}*/

	UE_LOG(LogTemp, Log, TEXT("Environment settings applied. Weather=%d Time=%d MapIndex=%d"), Config.WeatherIndex, Config.TimeIndex, Config.MapIndex);
}

void UScenarioMenuSubsystem::FinalizeScenarioAfterLoad()
{
	UWorld* World = PendingScenarioWorld.Get();
	if (!World)
	{
		return;
	}

	if (!bHasPendingScenarioConfig)
	{
		World->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
		PendingScenarioWorld = nullptr;
		bPendingScenarioWaitingLogged = false;
		return;
	}

	if (!AreScenarioLevelsReady(World))
	{
		if (!bPendingScenarioWaitingLogged)
		{
			UE_LOG(LogTemp, Log, TEXT("FinalizeScenarioAfterLoad: waiting for streaming levels to finish loading..."));
			bPendingScenarioWaitingLogged = true;
		}
		return;
	}

	World->GetTimerManager().ClearTimer(PendingScenarioTimerHandle);
	PendingScenarioWorld = nullptr;
	bPendingScenarioWaitingLogged = false;

	if (bHasPendingScenarioConfig)
	{
		UE_LOG(LogTemp, Log, TEXT("FinalizeScenarioAfterLoad: streaming complete, applying scenario configuration."));
		ApplyEnvironmentSettings(World, PendingScenarioConfig);
		DeployBlueForScenario(World, PendingScenarioConfig);
		bHasPendingScenarioConfig = false;
	}
}

bool UScenarioMenuSubsystem::AreScenarioLevelsReady(UWorld* World) const
{
	if (!World)
	{
		return false;
	}

	if (World->IsInSeamlessTravel())
	{
		return false;
	}

	if (!World->AreActorsInitialized())
	{
		return false;
	}

	const TArray<ULevelStreaming*>& StreamingLevels = World->GetStreamingLevels();
	for (ULevelStreaming* Streaming : StreamingLevels)
	{
		if (!Streaming)
		{
			continue;
		}

		if (Streaming->ShouldBeLoaded() && !Streaming->IsLevelLoaded())
		{
			return false;
		}

		if (Streaming->ShouldBeVisible() && !Streaming->IsLevelVisible())
		{
			return false;
		}
	}

	return true;
}

void UScenarioMenuSubsystem::DeployBlueForScenario(UWorld* World, const FScenarioTestConfig& Config)
{
	if (!World)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: start (Map=%s, Density=%d, Custom=%s)"), *Config.MapLevelName.ToString(), Config.DensityIndex, Config.bBlueCustomDeployment ? TEXT("true") : TEXT("false"));

	UClass* UnitClass = ResolveBlueUnitClass();
	if (!UnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to resolve blue unit class. Please update ResolveBlueUnitClass if you need a custom actor."));
		return;
	}

	UStaticMesh* DefaultUnitMesh = ResolveBlueUnitMeshByType(0);
	if (!DefaultUnitMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to load placeholder mesh. Please verify StarterContent is enabled."));
		return;
	}

	UMaterialInterface* DefaultUnitMaterial = ResolveBlueUnitMaterialByType(0);
	if (!DefaultUnitMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to load placeholder material. Units will use default material."));
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Using UnitClass=%s, Mesh=%s, Material=%s"), *UnitClass->GetName(), *DefaultUnitMesh->GetName(), DefaultUnitMaterial ? *DefaultUnitMaterial->GetName() : TEXT("None"));

	ClearSpawnedBlueUnits();
	ActiveBlueUnits.Reset();
	ActiveCountermeasureIndices = Config.CountermeasureIndices;
	BlueUnitMeshes.Reset();
	BlueUnitMaterials.Reset();

	constexpr int32 UnitTypeCount = 3;
	for (int32 TypeIndex = 0; TypeIndex < UnitTypeCount; ++TypeIndex)
	{
		UStaticMesh* MeshForType = ResolveBlueUnitMeshByType(TypeIndex);
		if (!MeshForType)
		{
			MeshForType = DefaultUnitMesh;
		}
		BlueUnitMeshes.Add(MeshForType);

		UMaterialInterface* MaterialForType = ResolveBlueUnitMaterialByType(TypeIndex);
		if (!MaterialForType)
		{
			MaterialForType = DefaultUnitMaterial;
		}
		BlueUnitMaterials.Add(MaterialForType);
	}

	// 暂存玩家初始视角
	APlayerStart* PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()));
	FVector Origin = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	if (PlayerStart)
	{
		Origin = PlayerStart->GetActorLocation();
		Facing = PlayerStart->GetActorRotation();
	}

	FVector Forward = Facing.Vector();
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}
	FVector Right = FRotationMatrix(Facing).GetScaledAxis(EAxis::Y);
	if (!Right.Normalize())
	{
		Right = FVector::RightVector;
	}
	BlueUnitFacing = Facing;
	BlueForward = Forward;
	BlueRight = Right;

	int32 DesiredMarkerCount = 6;
	switch (Config.DensityIndex)
	{
	case 0: // 密集
		DesiredMarkerCount = 8;
		break;
	case 2: // 稀疏
		DesiredMarkerCount = 3;
		break;
	default: // 正常
		DesiredMarkerCount = 5;
		break;
	}

	// Collect spawn markers: prefer tags, fallback to name pattern
	TArray<AActor*> SpawnMarkers;
	const FName PrimaryTag = FName(TEXT("BluePotentialDeployLocation"));
	const FName LegacyTag = FName(TEXT("PotentialBlueDeployLocation"));
	UGameplayStatics::GetAllActorsWithTag(World, PrimaryTag, SpawnMarkers);
	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: actors with tag %s = %d"), *PrimaryTag.ToString(), SpawnMarkers.Num());

	if (LegacyTag != PrimaryTag)
	{
		TArray<AActor*> LegacyActors;
		UGameplayStatics::GetAllActorsWithTag(World, LegacyTag, LegacyActors);
		if (LegacyActors.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: actors with legacy tag %s = %d"), *LegacyTag.ToString(), LegacyActors.Num());
			for (AActor* Legacy : LegacyActors)
			{
				SpawnMarkers.AddUnique(Legacy);
			}
		}
	}

	if (SpawnMarkers.Num() == 0)
	{
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
		for (AActor* Actor : AllActors)
		{
			if (!Actor)
			{
				continue;
			}

			const FString ActorName = Actor->GetName();
			if (ActorName.Contains(TEXT("BluePotentialDeployLocation")))
			{
				SpawnMarkers.Add(Actor);
				UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: fallback picked actor %s (Class=%s)"), *ActorName, *Actor->GetClass()->GetName());
			}
		}

		UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: fallback name search matched %d actors"), SpawnMarkers.Num());

		if (SpawnMarkers.Num() == 0)
		{
			const int32 SampleCount = FMath::Min(20, AllActors.Num());
			UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: fallback failed. Sample of world actors:"));
			for (int32 Index = 0; Index < SampleCount; ++Index)
			{
				AActor* Sample = AllActors[Index];
				if (Sample)
				{
					UE_LOG(LogTemp, Warning, TEXT("  [%d] %s (Class=%s, Tags=%d)"), Index, *Sample->GetName(), *Sample->GetClass()->GetName(), Sample->Tags.Num());
				}
			}
		}
	}

	if (SpawnMarkers.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("DeployBlueForScenario: 未找到任何包含标签 %s/%s 或名称包含 BluePotentialDeployLocation 的部署点，无法生成蓝方单位."), *PrimaryTag.ToString(), *LegacyTag.ToString());
		bUsingCustomDeployment = false;
		ShowBlueMonitor(World);
		RefreshBlueMonitor();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: collected %d deployment markers"), SpawnMarkers.Num());

	// 计算部署区域包围盒用于摄像机
	FBox DeployBounds(ForceInit);
	for (AActor* Marker : SpawnMarkers)
	{
		if (Marker)
		{
			DeployBounds += Marker->GetActorLocation();
		}
	}

	if (!DeployBounds.IsValid)
	{
		const FVector DefaultExtent(2000.f, 2000.f, 800.f);
		DeployBounds = FBox(Origin - DefaultExtent, Origin + DefaultExtent);
	}

	FVector DeployCenter = DeployBounds.GetCenter();
	const float OverviewHeight = FMath::Max(DeployBounds.GetExtent().Z, 500.f) + 1500.f;
	OverviewHomeLocation = DeployCenter + FVector(0.f, 0.f, OverviewHeight);
	OverviewHomeRotation = UKismetMathLibrary::FindLookAtRotation(OverviewHomeLocation, DeployCenter);
	bHasOverviewHome = true;
	EnsureOverviewPawn(World);

	BlueDeployMarkers.Reset();
	BlueMarkerSpawnCounts.Reset();
	for (AActor* Marker : SpawnMarkers)
	{
		BlueDeployMarkers.Add(Marker);
		BlueMarkerSpawnCounts.Add(0);
	}

	bUsingCustomDeployment = Config.bBlueCustomDeployment;

	ShowBlueMonitor(World);

	if (bUsingCustomDeployment)
	{
		UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Custom deployment enabled. Waiting for operator input."));
		RefreshBlueMonitor();
		return;
	}

	// Shuffle markers
	for (int32 i = SpawnMarkers.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		SpawnMarkers.Swap(i, j);
	}
	int32 MarkersToUse = SpawnMarkers.Num();
	if (DesiredMarkerCount > 0)
	{
		MarkersToUse = FMath::Clamp(DesiredMarkerCount, 1, SpawnMarkers.Num());
	}

	for (int32 MarkerIndex = 0; MarkerIndex < MarkersToUse; ++MarkerIndex)
	{
		AActor* Marker = SpawnMarkers[MarkerIndex];
		if (!Marker)
		{
			continue;
		}

		const FVector MarkerLocation = Marker->GetActorLocation();
		const FString MarkerName = Marker->GetName();
		const int32 ExistingCount = BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex) ? BlueMarkerSpawnCounts[MarkerIndex] : 0;
		const int32 UnitsForThisSpot = FMath::RandRange(1, 4);
		int32 LocalCount = 0;

		UE_LOG(LogTemp, Log, TEXT("Marker %s allocated %d units (auto deployment)"), *MarkerName, UnitsForThisSpot);

		for (int32 UnitIdx = 0; UnitIdx < UnitsForThisSpot; ++UnitIdx)
		{
			const int32 UnitType = FMath::RandRange(0, 2);
			const int32 SlotIndex = ExistingCount + LocalCount;
			constexpr float BaseRadius = 200.f;
			constexpr float RadiusStep = 220.f;
			constexpr int32 SlotsPerRing = 6;

			const int32 RingIndex = SlotIndex / SlotsPerRing;
			const int32 SlotIndexInRing = SlotIndex % SlotsPerRing;
			const float AngleDeg = SlotIndexInRing * (360.f / SlotsPerRing) + UnitType * 12.f + FMath::FRandRange(-8.f, 8.f);
			const float AngleRad = FMath::DegreesToRadians(AngleDeg);
			const float Radius = BaseRadius + RingIndex * RadiusStep;

			FVector ForwardDir = BlueForward;
			if (!ForwardDir.Normalize())
			{
				ForwardDir = FVector::ForwardVector;
			}
			FVector RightDir = BlueRight;
			if (!RightDir.Normalize())
			{
				RightDir = FVector::RightVector;
			}

			const FVector2D BaseDir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
			FVector DesiredLocation = MarkerLocation
				+ ForwardDir * (BaseDir.X * Radius)
				+ RightDir * (BaseDir.Y * Radius);

			const float Jitter = 25.f;
			DesiredLocation += ForwardDir * FMath::FRandRange(-Jitter, Jitter);
			DesiredLocation += RightDir * FMath::FRandRange(-Jitter, Jitter);

			UStaticMesh* MeshForType = BlueUnitMeshes.IsValidIndex(UnitType) ? BlueUnitMeshes[UnitType].Get() : ResolveBlueUnitMeshByType(UnitType);
			if (!MeshForType)
			{
				MeshForType = DefaultUnitMesh;
			}

			UMaterialInterface* MatForType = BlueUnitMaterials.IsValidIndex(UnitType) ? BlueUnitMaterials[UnitType].Get() : ResolveBlueUnitMaterialByType(UnitType);
			if (!MatForType)
			{
				MatForType = DefaultUnitMaterial;
			}

			if (!SpawnBlueUnitAtLocation(World, DesiredLocation, BlueUnitFacing, MarkerName, MeshForType, MatForType, Config.CountermeasureIndices))
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to place unit at marker %s"), *MarkerName);
			}
			else
			{
				++LocalCount;
			}
		}

		if (BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex))
		{
			BlueMarkerSpawnCounts[MarkerIndex] = ExistingCount + LocalCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Spawned %d blue units (DensityIndex=%d)."), ActiveBlueUnits.Num(), Config.DensityIndex);
RefreshBlueMonitor();
}

void UScenarioMenuSubsystem::ClearSpawnedBlueUnits()
{
	for (TWeakObjectPtr<AActor>& Ptr : ActiveBlueUnits)
	{
		if (AActor* Actor = Ptr.Get())
		{
			if (!Actor->IsPendingKillPending())
			{
				Actor->Destroy();
			}
		}
	}
	ActiveBlueUnits.Reset();
	if (bUsingCustomDeployment)
	{
		for (int32& Count : BlueMarkerSpawnCounts)
		{
			Count = 0;
		}
	}

	RefreshBlueMonitor();
}

UClass* UScenarioMenuSubsystem::ResolveBlueUnitClass() const
{
	return AStaticMeshActor::StaticClass();
}

UStaticMesh* UScenarioMenuSubsystem::ResolveBlueUnitMesh() const
{
	return ResolveBlueUnitMeshByType(0);
}

UMaterialInterface* UScenarioMenuSubsystem::ResolveBlueUnitMaterial() const
{
	return ResolveBlueUnitMaterialByType(0);
}

UStaticMesh* UScenarioMenuSubsystem::ResolveBlueUnitMeshByType(int32 TypeIndex) const
{
	static const TCHAR* MeshPaths[] =
	{
		TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"),
		TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"),
		TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder"),
	};

	constexpr int32 MaxTypes = UE_ARRAY_COUNT(MeshPaths);
	const int32 SafeIndex = FMath::Clamp(TypeIndex, 0, MaxTypes - 1);

	static TWeakObjectPtr<UStaticMesh> MeshCache[MaxTypes];
	if (!MeshCache[SafeIndex].IsValid())
	{
		FSoftObjectPath MeshPath(MeshPaths[SafeIndex]);
		if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(MeshPath.TryLoad()))
		{
			MeshCache[SafeIndex] = LoadedMesh;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveBlueUnitMeshByType: failed to load mesh at '%s' (Type=%d)."), MeshPaths[SafeIndex], SafeIndex);
			return nullptr;
		}
	}

	return MeshCache[SafeIndex].Get();
}

UMaterialInterface* UScenarioMenuSubsystem::ResolveBlueUnitMaterialByType(int32 TypeIndex) const
{
	static const TCHAR* MaterialPaths[] =
	{
		TEXT("/Game/StarterContent/Materials/M_Glow.M_Glow"),
		TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"),
		TEXT("/Game/StarterContent/Materials/M_Metal_Copper.M_Metal_Copper"),
	};

	constexpr int32 MaxTypes = UE_ARRAY_COUNT(MaterialPaths);
	const int32 SafeIndex = FMath::Clamp(TypeIndex, 0, MaxTypes - 1);

	static TWeakObjectPtr<UMaterialInterface> MaterialCache[MaxTypes];
	if (!MaterialCache[SafeIndex].IsValid())
	{
		FSoftObjectPath MaterialPath(MaterialPaths[SafeIndex]);
		if (UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(MaterialPath.TryLoad()))
		{
			MaterialCache[SafeIndex] = LoadedMaterial;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolveBlueUnitMaterialByType: failed to load material at '%s' (Type=%d)."), MaterialPaths[SafeIndex], SafeIndex);
			// Fallback to default engine material
			FSoftObjectPath DefaultPath(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
			if (UMaterialInterface* DefaultMat = Cast<UMaterialInterface>(DefaultPath.TryLoad()))
			{
				MaterialCache[SafeIndex] = DefaultMat;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ResolveBlueUnitMaterialByType: failed to load default engine material."));
				return nullptr;
			}
		}
	}

	return MaterialCache[SafeIndex].Get();
}

void UScenarioMenuSubsystem::ShowBlueMonitor(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (!BlueMonitorRoot.IsValid())
	{
		SAssignNew(BlueMonitorWidget, SBlueUnitMonitor)
			.Mode(bUsingCustomDeployment ? SBlueUnitMonitor::EMode::Markers : SBlueUnitMonitor::EMode::Units)
			.OnFocusUnit(SBlueUnitMonitor::FFocusUnit::CreateUObject(this, &UScenarioMenuSubsystem::FocusCameraOnUnit))
			.OnFocusMarker(SBlueUnitMonitor::FFocusMarker::CreateUObject(this, &UScenarioMenuSubsystem::FocusCameraOnMarker))
			.OnSpawnAtMarker(SBlueUnitMonitor::FSpawnAtMarker::CreateUObject(this, &UScenarioMenuSubsystem::SpawnBlueUnitAtMarker))
			.OnReturnCamera(FSimpleDelegate::CreateUObject(this, &UScenarioMenuSubsystem::ReturnCameraToInitial));

		TSharedRef<SWidget> Overlay =
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(FMargin(32.f, 120.f, 0.f, 0.f))
			[
				BlueMonitorWidget.ToSharedRef()
			];

		BlueMonitorRoot = Overlay;
		GEngine->GameViewport->AddViewportWidgetContent(Overlay, 400);
	}

	RefreshBlueMonitor();
}

void UScenarioMenuSubsystem::HideBlueMonitor()
{
	if (GEngine && GEngine->GameViewport && BlueMonitorRoot.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BlueMonitorRoot.ToSharedRef());
	}
	BlueMonitorRoot.Reset();
	BlueMonitorWidget.Reset();
	RestorePreviousPawn();
}

void UScenarioMenuSubsystem::RefreshBlueMonitor()
{
	if (!BlueMonitorWidget.IsValid())
	{
		return;
	}

	if (bUsingCustomDeployment)
	{
		BlueMonitorWidget->RefreshMarkers(BlueDeployMarkers, BlueMarkerSpawnCounts);
	}
	else
	{
		BlueMonitorWidget->RefreshUnits(ActiveBlueUnits);
	}
}

void UScenarioMenuSubsystem::FocusCameraOnMarker(int32 Index)
{
	if (!bUsingCustomDeployment)
	{
		FocusCameraOnUnit(Index);
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !BlueDeployMarkers.IsValidIndex(Index))
	{
		return;
	}

	AActor* Marker = BlueDeployMarkers[Index].Get();
	if (!Marker)
	{
		UE_LOG(LogTemp, Warning, TEXT("FocusCameraOnMarker: marker %d is no longer valid."), Index);
		return;
	}

	const FVector TargetLocation = Marker->GetActorLocation();
	FVector ForwardDir = BlueForward;
	if (!ForwardDir.Normalize())
	{
		ForwardDir = FVector::ForwardVector;
	}
	const FVector CameraLocation = TargetLocation - ForwardDir * 800.f + FVector(0.f, 0.f, 500.f);
	const FRotator CameraRotation = (TargetLocation - CameraLocation).Rotation();
	EnsureOverviewPawn(World);
	if (ASpectatorPawn* Pawn = OverviewPawn.Get())
	{
		Pawn->SetActorLocation(CameraLocation);
		Pawn->SetActorRotation(CameraRotation);
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetControlRotation(CameraRotation);
		}
	}
}

void UScenarioMenuSubsystem::FocusCameraOnUnit(int32 Index)
{
	UWorld* World = GetWorld();
	if (!World || Index < 0)
	{
		return;
	}

	ActiveBlueUnits.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });
	if (!ActiveBlueUnits.IsValidIndex(Index))
	{
		return;
	}

	AActor* Target = ActiveBlueUnits[Index].Get();
	if (!Target)
	{
		return;
	}

	const FVector TargetLocation = Target->GetActorLocation();
	FVector ForwardDir = Target->GetActorForwardVector();
	if (ForwardDir.IsNearlyZero())
	{
		ForwardDir = FVector::ForwardVector;
	}
	const FVector CameraLocation = TargetLocation - ForwardDir * 650.f + FVector(0.f, 0.f, 400.f);
	const FRotator CameraRotation = (TargetLocation - CameraLocation).Rotation();
	EnsureOverviewPawn(World);
	if (ASpectatorPawn* Pawn = OverviewPawn.Get())
	{
		Pawn->SetActorLocation(CameraLocation);
		Pawn->SetActorRotation(CameraRotation);
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetControlRotation(CameraRotation);
		}
	}
}

void UScenarioMenuSubsystem::SpawnBlueUnitAtMarker(int32 MarkerIndex, int32 UnitType)
{
	if (!bUsingCustomDeployment)
	{
		return;
	}

	if (UnitType < 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !BlueDeployMarkers.IsValidIndex(MarkerIndex))
	{
		return;
	}

	AActor* Marker = BlueDeployMarkers[MarkerIndex].Get();
	if (!Marker)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnBlueUnitAtMarker: marker %d is no longer valid."), MarkerIndex);
		return;
	}

	const FVector TargetLocation = Marker->GetActorLocation();

	FVector ForwardDir = BlueForward;
	if (!ForwardDir.Normalize())
	{
		ForwardDir = FVector::ForwardVector;
	}
	FVector RightDir = BlueRight;
	if (!RightDir.Normalize())
	{
		RightDir = FVector::RightVector;
	}

	const int32 ExistingCount = BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex) ? BlueMarkerSpawnCounts[MarkerIndex] : 0;
	constexpr float BaseRadius = 200.f;
	constexpr float RadiusStep = 220.f;
	const int32 SlotsPerRing = 6;

	const int32 RingIndex = ExistingCount / SlotsPerRing;
	const int32 SlotIndexInRing = ExistingCount % SlotsPerRing;
	const int32 SafeType = FMath::Clamp(UnitType, 0, 2);

	const float AngleDeg = SlotIndexInRing * (360.f / SlotsPerRing) + SafeType * 12.f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);
	const float Radius = BaseRadius + RingIndex * RadiusStep;

	const FVector2D BaseDir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
	FVector DesiredLocation = TargetLocation
		+ ForwardDir * (BaseDir.X * Radius)
		+ RightDir * (BaseDir.Y * Radius);

	const float Jitter = 25.f;
	DesiredLocation += ForwardDir * FMath::FRandRange(-Jitter, Jitter);
	DesiredLocation += RightDir * FMath::FRandRange(-Jitter, Jitter);

	UStaticMesh* MeshForType = BlueUnitMeshes.IsValidIndex(SafeType) ? BlueUnitMeshes[SafeType].Get() : ResolveBlueUnitMeshByType(SafeType);
	if (!MeshForType)
	{
		MeshForType = ResolveBlueUnitMeshByType(0);
	}

	UMaterialInterface* MaterialForType = BlueUnitMaterials.IsValidIndex(SafeType) ? BlueUnitMaterials[SafeType].Get() : ResolveBlueUnitMaterialByType(SafeType);
	if (!MaterialForType)
	{
		MaterialForType = ResolveBlueUnitMaterialByType(0);
	}

	if (SpawnBlueUnitAtLocation(World, DesiredLocation, BlueUnitFacing, FString::Printf(TEXT("部署点%d"), MarkerIndex + 1), MeshForType, MaterialForType, ActiveCountermeasureIndices))
	{
		if (BlueMarkerSpawnCounts.IsValidIndex(MarkerIndex))
		{
			BlueMarkerSpawnCounts[MarkerIndex] += 1;
		}

		UE_LOG(LogTemp, Log, TEXT("SpawnBlueUnitAtMarker: placed unit type %d at marker %s"), SafeType + 1, *Marker->GetName());
		RefreshBlueMonitor();
	}
}

void UScenarioMenuSubsystem::ReturnCameraToInitial()
{
	if (!bHasOverviewHome && !bHasInitialCameraTransform)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		EnsureOverviewPawn(World);

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ASpectatorPawn* Pawn = OverviewPawn.Get())
			{
				Pawn->SetActorLocation(OverviewHomeLocation);
				Pawn->SetActorRotation(OverviewHomeRotation);
				PC->SetControlRotation(OverviewHomeRotation);
			}
			else if (bHasInitialCameraTransform)
			{
				if (APawn* ControlledPawn = PC->GetPawn())
				{
					ControlledPawn->SetActorLocation(InitialCameraLocation);
					ControlledPawn->SetActorRotation(InitialCameraRotation);
					PC->SetControlRotation(InitialCameraRotation);
				}
			}
		}
	}
}

void UScenarioMenuSubsystem::EnsureOverviewPawn(UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (!PreviousPawn.IsValid())
		{
			PreviousPawn = PC->GetPawn();
		}

	ASpectatorPawn* Pawn = OverviewPawn.Get();
	if (!Pawn || Pawn->IsPendingKillPending())
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Pawn = World->SpawnActor<ASpectatorPawn>(OverviewHomeLocation, OverviewHomeRotation, Params);
			if (Pawn)
			{
				OverviewPawn = Pawn;
			}
		}

		if (Pawn)
		{
			Pawn->SetActorLocation(OverviewHomeLocation);
			Pawn->SetActorRotation(OverviewHomeRotation);
			PC->Possess(Pawn);
			PC->SetControlRotation(OverviewHomeRotation);
		}
	}
}

void UScenarioMenuSubsystem::RestorePreviousPawn()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PreviousPawn.IsValid())
			{
				APawn* Pawn = PreviousPawn.Get();
				PC->Possess(Pawn);
				PC->SetControlRotation(Pawn->GetActorRotation());
			}
		}
	}

	if (ASpectatorPawn* Pawn = OverviewPawn.Get())
	{
		if (!Pawn->IsPendingKillPending())
		{
			Pawn->Destroy();
		}
	}

	OverviewPawn.Reset();
	PreviousPawn.Reset();
	bHasOverviewHome = false;
}

bool UScenarioMenuSubsystem::SpawnBlueUnitAtLocation(UWorld* World, const FVector& DesiredLocation, const FRotator& Facing, const FString& MarkerName, UStaticMesh* UnitMesh, UMaterialInterface* UnitMaterial, const TArray<int32>& CountermeasureIndices)
{
	if (!World || !UnitMesh)
	{
		return false;
	}

	FVector SpawnLocation = DesiredLocation;

	const FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, 6000.f);
	const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 12000.f);
	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false);
	Params.bTraceComplex = true;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	if (bHit && Hit.bBlockingHit)
	{
		SpawnLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 30.f);
	}
	else if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation ProjectedNav;
		if (NavSys->ProjectPointToNavigation(SpawnLocation, ProjectedNav, FVector(400.f, 400.f, 1200.f)))
		{
			SpawnLocation = ProjectedNav.Location + FVector(0.f, 0.f, 30.f);
		}
	}

	const FString MarkerLabel = MarkerName.IsEmpty() ? TEXT("ManualLocation") : MarkerName;
	UE_LOG(LogTemp, Log, TEXT("Placing unit near %s -> %s"), *MarkerLabel, *SpawnLocation.ToString());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AStaticMeshActor* Spawned = World->SpawnActor<AStaticMeshActor>(SpawnLocation, Facing, SpawnParams);
	if (!Spawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn blue unit at %s"), *SpawnLocation.ToString());
		return false;
	}

	if (UStaticMeshComponent* MeshComp = Spawned->GetStaticMeshComponent())
	{
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetStaticMesh(UnitMesh);
		MeshComp->SetRelativeScale3D(FVector(1.0f));
		MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
		MeshComp->SetVisibility(true, true);
		MeshComp->SetHiddenInGame(false);
		if (UnitMaterial)
		{
			MeshComp->SetMaterial(0, UnitMaterial);
		}
	}

	Spawned->Tags.AddUnique(FName(TEXT("BlueUnit")));
	for (int32 Countermeasure : CountermeasureIndices)
	{
		switch (Countermeasure)
		{
		case 0: Spawned->Tags.AddUnique(FName(TEXT("Blue_ECM"))); break;
		case 1: Spawned->Tags.AddUnique(FName(TEXT("Blue_CommJamming"))); break;
		case 2: Spawned->Tags.AddUnique(FName(TEXT("Blue_TargetMobility"))); break;
		default: break;
		}
	}

	ActiveBlueUnits.Add(Spawned);
	UE_LOG(LogTemp, Log, TEXT("Blue unit spawned at %s"), *SpawnLocation.ToString());
	return true;
}

