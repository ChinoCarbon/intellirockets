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

void UScenarioMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("UScenarioMenuSubsystem::Initialize"));
	WorldHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UScenarioMenuSubsystem::OnWorldReady);
}

void UScenarioMenuSubsystem::Deinitialize()
{
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
			ApplyEnvironmentSettings(World, PendingScenarioConfig);
			DeployBlueForScenario(World, PendingScenarioConfig);
			bHasPendingScenarioConfig = false;
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

void UScenarioMenuSubsystem::DeployBlueForScenario(UWorld* World, const FScenarioTestConfig& Config)
{
	if (!World)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: start (Map=%s, Density=%d, Custom=%s)"), *Config.MapLevelName.ToString(), Config.DensityIndex, Config.bBlueCustomDeployment ? TEXT("true") : TEXT("false"));

	if (Config.bBlueCustomDeployment)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Custom deployment is not implemented yet. Using automatic layout instead."));
	}

	UClass* UnitClass = ResolveBlueUnitClass();
	if (!UnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to resolve blue unit class. Please update ResolveBlueUnitClass if you need a custom actor."));
		return;
	}

	UStaticMesh* UnitMesh = ResolveBlueUnitMesh();
	if (!UnitMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to load placeholder mesh. Please verify StarterContent is enabled."));
		return;
	}

	UMaterialInterface* UnitMaterial = ResolveBlueUnitMaterial();
	if (!UnitMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to load placeholder material. Units will use default material."));
	}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Using UnitClass=%s, Mesh=%s, Material=%s"), *UnitClass->GetName(), *UnitMesh->GetName(), UnitMaterial ? *UnitMaterial->GetName() : TEXT("None"));

	ClearSpawnedBlueUnits();
	ActiveBlueUnits.Reset();

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			InitialCameraLocation = Pawn->GetActorLocation();
			InitialCameraRotation = Pawn->GetActorRotation();
			bHasInitialCameraTransform = true;
		}
		else if (AActor* ViewTarget = PC->GetViewTarget())
		{
			InitialCameraLocation = ViewTarget->GetActorLocation();
			InitialCameraRotation = ViewTarget->GetActorRotation();
			bHasInitialCameraTransform = true;
		}
	}

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

	FBox SpawnBounds(ForceInit);
	if (ALevelBounds* LevelBounds = Cast<ALevelBounds>(UGameplayStatics::GetActorOfClass(World, ALevelBounds::StaticClass())))
	{
		SpawnBounds = LevelBounds->GetComponentsBoundingBox(true);
	}

	if (!SpawnBounds.IsValid || SpawnBounds.GetExtent().IsNearlyZero())
	{
		const FVector DefaultExtent(6000.f, 6000.f, 2000.f);
		SpawnBounds = FBox(Origin - DefaultExtent, Origin + DefaultExtent);
		UE_LOG(LogTemp, Warning, TEXT("SpawnBounds invalid, fallback to default around PlayerStart. Min=%s Max=%s"), *SpawnBounds.Min.ToString(), *SpawnBounds.Max.ToString());
	}
	else
	{
		SpawnBounds = SpawnBounds.ExpandBy(FVector(-500.f, -500.f, 0.f));
		UE_LOG(LogTemp, Log, TEXT("SpawnBounds from LevelBounds Min=%s Max=%s"), *SpawnBounds.Min.ToString(), *SpawnBounds.Max.ToString());
	}

	FVector DeployCenter = SpawnBounds.GetCenter();
	const float OverviewHeight = FMath::Max(SpawnBounds.GetExtent().Z, 500.f) + 1500.f;
	OverviewHomeLocation = DeployCenter + FVector(0.f, 0.f, OverviewHeight);
	OverviewHomeRotation = UKismetMathLibrary::FindLookAtRotation(OverviewHomeLocation, DeployCenter);
	bHasOverviewHome = true;
	EnsureOverviewPawn(World);

	int32 UnitCount = 8;
	float MinSpacing = 900.f;
	float MinDistanceFromPlayer = 2000.f;

	switch (Config.DensityIndex)
	{
	case 0: // 密集
		UnitCount = 12;
		MinSpacing = 650.f;
		MinDistanceFromPlayer = 1500.f;
		break;
case 2: // 稀疏
		UnitCount = 4;
		MinSpacing = 1400.f;
		MinDistanceFromPlayer = 3000.f;
		break;
default: // 正常
		UnitCount = 8;
		MinSpacing = 900.f;
		MinDistanceFromPlayer = 2200.f;
		break;
	}

	const int32 MaxPlacementAttempts = 120;
	TArray<FVector> ChosenLocations;

FActorSpawnParameters SpawnParams;
SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

for (int32 Index = 0; Index < UnitCount; ++Index)
{
	FVector SpawnLocation = DeployCenter;
	bool bPlaced = false;
	float BestScore = -FLT_MAX;

	for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
	{
		FVector Candidate(
			FMath::FRandRange(SpawnBounds.Min.X, SpawnBounds.Max.X),
			FMath::FRandRange(SpawnBounds.Min.Y, SpawnBounds.Max.Y),
			SpawnBounds.Max.Z + 800.f);

		if (FVector::Dist2D(Candidate, Origin) < MinDistanceFromPlayer)
		{
			continue;
		}

		bool bTooClose = false;
		float NearestDistSq = FLT_MAX;
		for (const FVector& Existing : ChosenLocations)
		{
			const float DistSq = FVector::DistSquared2D(Candidate, Existing);
			NearestDistSq = FMath::Min(NearestDistSq, DistSq);
			if (DistSq < MinSpacing * MinSpacing)
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		const FVector TraceStart = Candidate + FVector(0.f, 0.f, 6000.f);
		const FVector TraceEnd = Candidate - FVector(0.f, 0.f, 12000.f);
		FHitResult Hit;
		FCollisionQueryParams Params(NAME_None, false);
		Params.bTraceComplex = true;
		bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
		FVector AdjustedLocation = Candidate;
		if (bHit && Hit.bBlockingHit)
		{
			AdjustedLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 30.f);
		}
		else
		{
			AdjustedLocation.Z = SpawnBounds.Max.Z + 150.f;
		}

		FVector NavLocation;
		FNavLocation ProjectedNav;
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (NavSys && NavSys->ProjectPointToNavigation(AdjustedLocation, ProjectedNav, FVector(500.f, 500.f, 1500.f)))
		{
			AdjustedLocation = ProjectedNav.Location;
		}

		AdjustedLocation.X = FMath::Clamp(AdjustedLocation.X, SpawnBounds.Min.X, SpawnBounds.Max.X);
		AdjustedLocation.Y = FMath::Clamp(AdjustedLocation.Y, SpawnBounds.Min.Y, SpawnBounds.Max.Y);

		const float Score = NearestDistSq;
		if (Score > BestScore)
		{
			BestScore = Score;
			SpawnLocation = AdjustedLocation;
			bPlaced = true;
		}
	}

	if (!bPlaced)
	{
		SpawnLocation = FVector(
			FMath::Clamp(DeployCenter.X + FMath::FRandRange(-2000.f, 2000.f), SpawnBounds.Min.X, SpawnBounds.Max.X),
			FMath::Clamp(DeployCenter.Y + FMath::FRandRange(-2000.f, 2000.f), SpawnBounds.Min.Y, SpawnBounds.Max.Y),
			DeployCenter.Z + 120.f);

		const FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, 4000.f);
		const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 8000.f);
		FHitResult Hit;
		FCollisionQueryParams Params(NAME_None, false);
		Params.bTraceComplex = true;
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) && Hit.bBlockingHit)
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
	}

	FRotator SpawnRotation = Facing;

	const FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, 6000.f);
	const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 12000.f);
	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false);
	Params.bTraceComplex = true;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) && Hit.bBlockingHit)
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

	AStaticMeshActor* Spawned = World->SpawnActor<AStaticMeshActor>(SpawnLocation, SpawnRotation, SpawnParams);
	if (!Spawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeployBlueForScenario: Failed to spawn blue unit at index %d"), Index);
		continue;
	}

	if (UStaticMeshComponent* MeshComp = Spawned->GetStaticMeshComponent())
	{
		MeshComp->SetStaticMesh(UnitMesh);
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetRelativeScale3D(FVector(3.0f));
		MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
		if (UnitMaterial)
		{
			MeshComp->SetMaterial(0, UnitMaterial);
		}
	}

	Spawned->Tags.AddUnique(FName(TEXT("BlueUnit")));
	for (int32 Countermeasure : Config.CountermeasureIndices)
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
	ChosenLocations.Add(SpawnLocation);

	UE_LOG(LogTemp, Log, TEXT("Blue unit %d spawned at %s"), Index, *SpawnLocation.ToString());

	if (Index < 4)
	{
		UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Spawned unit %d at %s rot %s"), Index, *SpawnLocation.ToString(), *SpawnRotation.ToString());
	}
}

	UE_LOG(LogTemp, Log, TEXT("DeployBlueForScenario: Spawned %d blue units (DensityIndex=%d)."), ActiveBlueUnits.Num(), Config.DensityIndex);
	ShowBlueMonitor(World);
	if (BlueMonitorWidget.IsValid())
	{
		BlueMonitorWidget->Refresh(ActiveBlueUnits);
	}
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
	if (BlueMonitorWidget.IsValid())
	{
		BlueMonitorWidget->Refresh(ActiveBlueUnits);
	}
}

UClass* UScenarioMenuSubsystem::ResolveBlueUnitClass() const
{
	return AStaticMeshActor::StaticClass();
}

UStaticMesh* UScenarioMenuSubsystem::ResolveBlueUnitMesh() const
{
	static const TCHAR* PlaceholderMeshPath = TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube");

	static TWeakObjectPtr<UStaticMesh> CachedMesh;
	if (!CachedMesh.IsValid())
	{
		FSoftObjectPath MeshPath(PlaceholderMeshPath);
		if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(MeshPath.TryLoad()))
		{
			CachedMesh = LoadedMesh;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ResolveBlueUnitMesh: failed to load starter mesh at '%s'."), PlaceholderMeshPath);
			return nullptr;
		}
	}

	return CachedMesh.Get();
}

UMaterialInterface* UScenarioMenuSubsystem::ResolveBlueUnitMaterial() const
{
	static const TCHAR* PlaceholderMaterialPath = TEXT("/Game/StarterContent/Materials/M_Glow.M_Glow");

	static TWeakObjectPtr<UMaterialInterface> CachedMaterial;
	if (!CachedMaterial.IsValid())
	{
		FSoftObjectPath MaterialPath(PlaceholderMaterialPath);
		if (UMaterialInterface* LoadedMat = Cast<UMaterialInterface>(MaterialPath.TryLoad()))
		{
			CachedMaterial = LoadedMat;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ResolveBlueUnitMaterial: failed to load starter material at '%s'."), PlaceholderMaterialPath);
			return nullptr;
		}
	}

	return CachedMaterial.Get();
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
			.OnFocusUnit(SBlueUnitMonitor::FFocusUnit::CreateUObject(this, &UScenarioMenuSubsystem::FocusCameraOnUnit))
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

	if (BlueMonitorWidget.IsValid())
	{
		BlueMonitorWidget->Refresh(ActiveBlueUnits);
	}
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

