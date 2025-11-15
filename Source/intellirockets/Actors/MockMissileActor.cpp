#include "Actors/MockMissileActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"

AMockMissileActor::AMockMissileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("MissileCollision"));
	CollisionComponent->InitSphereRadius(120.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetMobility(EComponentMobility::Movable);

	SetRootComponent(CollisionComponent);

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MissileRoot"));
	RootSceneComponent->SetupAttachment(CollisionComponent);
	RootSceneComponent->SetMobility(EComponentMobility::Movable);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	MeshComponent->SetupAttachment(RootSceneComponent);

	if (MeshComponent)
	{
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComponent->SetGenerateOverlapEvents(true);
		MeshComponent->SetEnableGravity(false);
		MeshComponent->SetSimulatePhysics(false);
	}

	LightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("MissileGlow"));
	if (LightComponent)
	{
		LightComponent->SetupAttachment(MeshComponent);
		LightComponent->SetIntensity(8000.f);
		LightComponent->SetAttenuationRadius(400.f);
		LightComponent->SetLightColor(FLinearColor(1.f, 0.2f, 0.05f));
		LightComponent->bUseInverseSquaredFalloff = false;
		LightComponent->SetMobility(EComponentMobility::Movable);
	}
}

void AMockMissileActor::BeginPlay()
{
	Super::BeginPlay();

	LastTrailLocation = GetActorLocation();
	bTrailActive = true;
}

void AMockMissileActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!bExpiredNotified)
	{
		bExpiredNotified = true;
		OnExpired.Broadcast(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AMockMissileActor::InitializeMissile(AActor* InTarget, float InLaunchSpeed, float InMaxLifetime)
{
	TargetActor = InTarget;
	const float ClampedSpeed = FMath::Clamp(InLaunchSpeed, MinLaunchSpeed, MaxLaunchSpeed);
	Speed = ClampedSpeed;
	AscentSpeed = ClampedSpeed * 0.5f;
	AscentHeight = FMath::FRandRange(MinAscentHeight, MaxAscentHeight);
	MaxLifetime = InMaxLifetime;
	ElapsedLifetime = 0.f;
	bHasImpacted = false;
	bExpiredNotified = false;
	StartLocation = GetActorLocation();
	bAscending = true;

	CachedTargetLocation = TargetActor.IsValid() ? TargetActor->GetActorLocation() : StartLocation + GetActorForwardVector() * 4000.f;
	Velocity = FVector::UpVector * AscentSpeed;
	SetActorRotation(Velocity.ToOrientationRotator());
	LastTrailLocation = StartLocation;
	bTrailActive = true;
}

void AMockMissileActor::SetupAppearance(UStaticMesh* InMesh, UMaterialInterface* InBaseMaterial, const FLinearColor& TintColor)
{
	if (MeshComponent)
	{
		if (InMesh)
		{
			MeshComponent->SetStaticMesh(InMesh);
			MeshComponent->SetRelativeScale3D(FVector(0.55f));
			MeshComponent->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		}

		if (InBaseMaterial)
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(InBaseMaterial, this);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TintColor);
				MeshComponent->SetMaterial(0, DynamicMaterial);
			}
			else
			{
				MeshComponent->SetMaterial(0, InBaseMaterial);
			}
		}
	}
}

void AMockMissileActor::TriggerImpact(AActor* OtherActor)
{
	HandleImpact(OtherActor);
}

void AMockMissileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HandleLifetime(DeltaSeconds);

	if (!IsPendingKillPending() && !bHasImpacted)
	{
		if (bAscending)
		{
			UpdateAscent(DeltaSeconds);
		}
		else
		{
			UpdateHoming(DeltaSeconds);
		}
	}

	UpdateTrail();
}

void AMockMissileActor::HandleLifetime(float DeltaSeconds)
{
	ElapsedLifetime += DeltaSeconds;
	if (ElapsedLifetime >= MaxLifetime)
	{
		if (!bExpiredNotified)
		{
			bExpiredNotified = true;
			OnExpired.Broadcast(this);
		}
		Destroy();
	}
}

void AMockMissileActor::UpdateAscent(float DeltaSeconds)
{
	const FVector CurrentLocation = GetActorLocation();
	const float DeltaHeight = CurrentLocation.Z - StartLocation.Z;
	if (DeltaHeight >= AscentHeight)
	{
		BeginHoming();
		UpdateHoming(DeltaSeconds);
		return;
	}

	const FVector Step = FVector::UpVector * AscentSpeed * DeltaSeconds;
	FHitResult Hit;
	AddActorWorldOffset(Step, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit.GetActor());
		return;
	}
}

void AMockMissileActor::BeginHoming()
{
	bAscending = false;
	if (TargetActor.IsValid())
	{
		CachedTargetLocation = TargetActor->GetActorLocation();
	}
	FVector Direction = (CachedTargetLocation - GetActorLocation()).GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		Velocity = Direction * Speed;
		SetActorRotation(Direction.Rotation());
	}
	else
	{
		Velocity = GetActorForwardVector() * Speed;
	}
}

void AMockMissileActor::UpdateHoming(float DeltaSeconds)
{
	if (TargetActor.IsValid())
	{
		CachedTargetLocation = TargetActor->GetActorLocation();
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = CachedTargetLocation - CurrentLocation;
	const float DistanceToTarget = ToTarget.Size();
	if (DistanceToTarget <= 200.f)
	{
		HandleImpact(TargetActor.Get());
		return;
	}

	FVector DesiredDirection = ToTarget.GetSafeNormal();
	if (DesiredDirection.IsNearlyZero())
	{
		DesiredDirection = GetActorForwardVector();
	}

	Velocity = DesiredDirection * Speed;
	const FVector Step = Velocity * DeltaSeconds;
	FHitResult Hit;
	AddActorWorldOffset(Step, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit.GetActor());
		return;
	}

	SetActorRotation(DesiredDirection.Rotation());

	if (TargetActor.IsValid())
	{
		const float DistanceSq = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation());
		if (DistanceSq <= FMath::Square(200.f))
		{
			HandleImpact(TargetActor.Get());
		}
	}
}

void AMockMissileActor::HandleImpact(AActor* HitActor)
{
	if (bHasImpacted)
	{
		return;
	}

	bHasImpacted = true;

	if (bTrailActive)
	{
		if (UWorld* World = GetWorld())
		{
			DrawDebugLine(World, LastTrailLocation, GetActorLocation(), FColor::Red, true, TrailLifetime, 0, TrailThickness);
		}
		bTrailActive = false;
	}

	OnImpact.Broadcast(this, HitActor);
	if (!bExpiredNotified)
	{
		bExpiredNotified = true;
		OnExpired.Broadcast(this);
	}

	if (!IsPendingKillPending())
	{
		Destroy();
	}
}

void AMockMissileActor::UpdateTrail()
{
	if (!bTrailActive)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (FVector::DistSquared(CurrentLocation, LastTrailLocation) < FMath::Square(TrailPointSpacing))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugLine(World, LastTrailLocation, CurrentLocation, FColor::Red, true, TrailLifetime, 0, TrailThickness);
	}

	LastTrailLocation = CurrentLocation;
}


