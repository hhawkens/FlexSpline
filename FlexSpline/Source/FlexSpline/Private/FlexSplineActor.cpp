#include "FlexSplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Helper aliases, for terser code
static const auto StaticMeshClass = UStaticMeshComponent::StaticClass();
static const auto SplineMeshClass = USplineMeshComponent::StaticClass();
static const auto LocalSpace = ESplineCoordinateSpace::Local;
static const auto WorldSpace = ESplineCoordinateSpace::World;

//////////////////////////////////////////////////////////////////////////
// STATIC HELPERS
static FColor GetColorForArrow(int32 MeshIndex)
{
	static const TArray<FColor> Colors = {
		FColor::Orange,
		FColor::Green,
		FColor::Blue,
		FColor::Red,
		FColor::Emerald,
		FColor::Magenta,
		FColor::Cyan,
		FColor::Yellow,
		FColor::Purple,
		FColor::Turquoise,
		FColor::Silver
	};

	MeshIndex = FMath::Clamp(MeshIndex, 0, Colors.Num() - 1);
	return Colors[MeshIndex];
}

static float RandomizeFloat(float InFloat, int32 Index, FName LayerName)
{
	const int32 Seed  = GetTypeHash(LayerName) + static_cast<int32>(InFloat) + Index;
	return InFloat * UKismetMathLibrary::RandomFloatInRangeFromStream(-1.f, 1.f, FRandomStream(Seed));
}

static FVector RandomizeVector(const FVector& InVec, int32 Index, FName LayerName)
{
	const float RandX = InVec.X != 0.f ? RandomizeFloat(InVec.X, Index, LayerName) : 0.f;
	const float RandY = InVec.Y != 0.f ? RandomizeFloat(InVec.Y, Index, LayerName) : 0.f;
	const float RandZ = InVec.Z != 0.f ? RandomizeFloat(InVec.Z, Index, LayerName) : 0.f;

	return {RandX, RandY, RandZ};
}

static FRotator RandomizeRotator(const FRotator& InRot, int32 Index, FName LayerName)
{
	const FVector VecFromRot = RandomizeVector(InRot.Euler(), Index, LayerName);
	return {VecFromRot.X, VecFromRot.Y, VecFromRot.Z};
}

static uint32 GeneratePointHashValue(const USplineComponent* const SplineComp, int32 Index)
{
	return SplineComp != nullptr
		 ? GetTypeHash(SplineComp->GetLocationAtSplinePoint(Index, LocalSpace))
		 : 0;
}

static float FSeededRand(int32 Seed)
{
	return UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 1.f, FRandomStream((Seed + 1) * 13));
}

static UClass* GetMeshType(EFlexSplineMeshType MeshType)
{
	switch (MeshType)
	{
		case EFlexSplineMeshType::SplineMesh: return SplineMeshClass;
		case EFlexSplineMeshType::StaticMesh: return StaticMeshClass;
		default: return nullptr;
	}
}

static bool CanRenderFromSpawnChance(const FSplineMeshInitData& MeshInitData, int32 CurrentIndex)
{
	const float SpawnChance = MeshInitData.RenderInfo.SpawnChance;
	const int32 SpawnSeed   = GetTypeHash(MeshInitData.MeshComponentsArray[CurrentIndex]->GetName()) * SpawnChance;

	if (MeshInitData.RenderInfo.bRandomizeSpawnChance)
	{
		return SpawnChance > FSeededRand(SpawnSeed);
	}

	// Compare index-spawn-chance-ratio and see if it has changed from ratio of last index
	const float Interval = 1.f / FMath::Clamp(SpawnChance, 0.00001f, 1.f);
	const int32 CurrentRatio = static_cast<int32>(CurrentIndex / Interval);
	const int32 LastRatio = CurrentIndex <= 0
		? SpawnChance > 0.f ? 1 : 0 // edge case first index
		: static_cast<int32>(((CurrentIndex - 1) / Interval));

	return CurrentRatio != LastRatio;
}

static ESplineMeshAxis::Type ToSplineAxis(EFlexSplineAxis FlexSplineAxis)
{
	return static_cast<ESplineMeshAxis::Type>( static_cast<uint8>(FlexSplineAxis) );
}

void DestroyMeshComponent(FSplineMeshInitData& MeshInitData, int32 Index)
{
	FStaticMeshWeakPtr Mesh = MeshInitData.MeshComponentsArray[Index];
	if (Mesh.IsValid())
	{
		Mesh->DestroyComponent();
	}
	MeshInitData.MeshComponentsArray.RemoveAt(Index);
}


//////////////////////////////////////////////////////////////////////////
// STRUCT FUNCTIONS
FSplineMeshInitData::~FSplineMeshInitData()
{
	for (const FStaticMeshWeakPtr& Mesh : MeshComponentsArray)
	{
		if (Mesh.IsValid())
		{
			Mesh->ConditionalBeginDestroy();
		}
	}
	for (const FArrowWeakPtr& Arrow : ArrowSplineUpIndicatorArray)
	{
		if (Arrow.IsValid())
		{
			Arrow->ConditionalBeginDestroy();
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR + BASE INTERFACE + GETTER
AFlexSplineActor::AFlexSplineActor():
	CollisionActiveConfig(EFlexGlobalConfigType::Nowhere),
	SynchronizeConfig(EFlexGlobalConfigType::Custom),
	LoopConfig(EFlexGlobalConfigType::Custom),
	bShowPointNumbers(false),
	PointNumberSize(125.f),
	UpDirectionArrowSize(3.f),
	UpDirectionArrowOffset(25.f),
	TextRenderColor(FColor::Cyan)
{
	PrimaryActorTick.bCanEverTick = false;

	// Spline Component
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SplineComponent->SetMobility(EComponentMobility::Static);
	RootComponent = SplineComponent;
}

void AFlexSplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// FlexSpline construction for editor builds here
	ConstructSplineMesh();
}

void AFlexSplineActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// FlexSpline construction for cooked builds here because it has no OnConstruction
#if !WITH_EDITOR
	ConstructSplineMesh();
#endif
}

int32 AFlexSplineActor::GetMeshCountForType(EFlexSplineMeshType MeshType) const
{
	int32 Count = 0;
	for (const TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
	{
		const FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
		if (MeshInitData.MeshInfo.MeshType == MeshType && TEST_BIT(MeshInitData.GeneralInfo, EFlexGeneralFlags::Active))
		{
			Count++;
		}
	}
	return Count;
}


//////////////////////////////////////////////////////////////////////////
// FLEX SPLINE FUNCTIONALITY
void AFlexSplineActor::ConstructSplineMesh()
{
	// Get all indices that were deleted, if any
	TArray<int32> DeletedIndices;
	GetDeletedIndices(DeletedIndices);

	InitializeNewMeshData();

	// Check if number of spline points and point data align, add or remove data accordingly
	AddPointDataEntries();
	RemovePointDataEntries(DeletedIndices);

	// Check if number of spline points and meshes align, add or remove meshes accordingly
	InitDataAddMeshes();
	InitDataRemoveMeshes(DeletedIndices);

	// Update the spline itself with the gathered data
	UpdatePointData();
	UpdateMeshComponents();
	UpdateDebugInformation();
}

void AFlexSplineActor::InitializeNewMeshData()
{
	const int32 MeshInitMapNum = MeshDataInitMap.Num();

	for (TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
	{
		FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
		if (!MeshInitData.IsInitialized())
		{
			// Generate name for new entry
			for (int32 Index = 0; Index < MeshInitMapNum; Index++)
			{
				const FName NewLayerName = FName(*FString(TEXT("Layer " + FString::FromInt(Index))));
				if ( !MeshDataInitMap.Contains(NewLayerName) && NewLayerName != LastUsedKey )
				{
					MeshInitDataPair.Key = NewLayerName;
					LastUsedKey = NewLayerName;
					break;
				}
			}

			// Init data from template
			MeshInitData = MeshDataTemplate;
			MeshInitData.Initialize();
		}
	}
}

void AFlexSplineActor::AddPointDataEntries()
{
	const int32 PointDataArraySize = PointDataArray.Num();
	const int32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();

	for (int32 i = PointDataArraySize; i < NumberOfSplinePoints; i++)
	{
		FSplinePointData NewPointData;

		// Create text renderer to show point index in editor
		UTextRenderComponent* NewTextRender = NewObject<UTextRenderComponent>(RootComponent);
		NewTextRender->RegisterComponent();
		NewTextRender->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		NewTextRender->SetWorldSize(PointNumberSize);
		NewTextRender->SetHiddenInGame(true);
		NewTextRender->SetTextRenderColor(TextRenderColor);
		NewPointData.IndexTextRenderer = NewTextRender;

		// Save entry
		PointDataArray.Add(NewPointData);
	}
}

void AFlexSplineActor::RemovePointDataEntries(const TArray<int32>& DeletedIndices)
{
	// Use gathered indices to clean up and delete redundant data
	for (const int32 Index : DeletedIndices)
	{
		const FSplinePointData& PointData = PointDataArray[Index];

		// Remove this point's text-render
		UTextRenderComponent* IndexText = PointData.IndexTextRenderer;
		if (IndexText != nullptr)
		{
			IndexText->DestroyComponent();
		}

		// Remove arrows
		for (TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
		{
			FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
			FArrowWeakPtr Arrow = MeshInitData.ArrowSplineUpIndicatorArray[Index];
			if (Arrow.IsValid())
			{
				MeshInitData.ArrowSplineUpIndicatorArray.Remove(Arrow);
				Arrow->DestroyComponent();
			}
		}

		PointDataArray.RemoveAt(Index);
	}
}

void AFlexSplineActor::InitDataAddMeshes()
{
	// Each mesh-init data stores all spline mesh components of its type, their location scattered across all spline points
	// Here we add spline meshes until it has as many meshes as there are spline points
	for (TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
	{
		FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
		UClass* MeshType = GetMeshType(MeshInitData.MeshInfo.MeshType);
		const int32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();
		const int32 NumberOfSplineMeshes = MeshInitData.MeshComponentsArray.Num();

		if (NumberOfSplineMeshes < NumberOfSplinePoints)
		{
			for (int32 i = NumberOfSplineMeshes; i < NumberOfSplinePoints; i++)
			{
				CreateMeshComponent(MeshType, MeshInitData);
				CreateArrowComponent(MeshInitData);
			}
		}
	}
}

void AFlexSplineActor::InitDataRemoveMeshes(const TArray<int32>& DeletedIndices)
{
	for (const int32 Index : DeletedIndices)
	{
		// Remove all spline Meshes at this spline index (which was removed)
		for (TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
		{
			FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
			const int32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();
			const int32 NumberOfSplineMeshes = MeshInitData.MeshComponentsArray.Num();

			if (NumberOfSplineMeshes > NumberOfSplinePoints)
			{
				DestroyMeshComponent(MeshInitData, Index);
			}
		}
	}
}

void AFlexSplineActor::UpdatePointData()
{
	const int32 PointDataArraySize = PointDataArray.Num();
	for (int32 Index = 0; Index < PointDataArraySize; Index++)
	{
		FSplinePointData& PointData = PointDataArray[Index];

		// Update ID
		PointData.ID = GeneratePointHashValue(SplineComponent, Index);
	}
}

void AFlexSplineActor::UpdateDebugInformation()
{
	const int32 PointDataArraySize = PointDataArray.Num();
	for (int32 Index = 0; Index < PointDataArraySize; Index++)
	{
		const FSplinePointData& PointData = PointDataArray[Index];

		// Update text renderer
		UTextRenderComponent* TextRenderer = PointData.IndexTextRenderer;
		if (TextRenderer != nullptr)
		{
			const FRotator SplineRotation = SplineComponent->GetRotationAtSplinePoint(Index, LocalSpace);
			TextRenderer->SetWorldLocation(GetTextPosition(Index));
			TextRenderer->SetText(FText::AsNumber(Index));
			TextRenderer->SetTextRenderColor(TextRenderColor);
			TextRenderer->SetRelativeRotation(FRotator(0.f, -SplineRotation.Yaw, 0.f));
			TextRenderer->SetWorldSize(PointNumberSize);
			TextRenderer->SetVisibility(bShowPointNumbers);
		}

		// Update up-vector-arrow
		int32 MeshInitIndex = 0;
		for (auto& MeshInitDataPair : MeshDataInitMap)
		{
			const FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
			const USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(MeshInitData.MeshComponentsArray[Index].Get());
			UArrowComponent* Arrow = MeshInitData.ArrowSplineUpIndicatorArray[Index].Get();

			if (MeshInitData.UpVectorInfo.bShowUpDirection
				&& SplineMesh != nullptr
				&& Arrow != nullptr
				&& MeshDataInitMap.Num() > 0
				&& Index != PointDataArraySize - 1)
			{
				Arrow->SetRelativeRotation(SplineMesh->GetSplineUpDir().Rotation());
				Arrow->SetWorldLocation(TextRenderer->GetComponentLocation() + TextRenderer->GetUpVector() * UpDirectionArrowOffset);
				Arrow->SetArrowColor(GetColorForArrow(MeshInitIndex));
				Arrow->ArrowSize = UpDirectionArrowSize;
				Arrow->SetVisibility(true);
			}
			else if (Arrow != nullptr)
			{
				Arrow->SetVisibility(false);
			}
			MeshInitIndex++;
		}
	}
}

void AFlexSplineActor::UpdateMeshComponents()
{
	const int32 NumSplinePoints = SplineComponent->GetNumberOfSplinePoints();

	// Update all meshes for the current mesh initializer
	for (TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
	{
		FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
		UClass* ConfiguredMeshType = GetMeshType(MeshInitData.MeshInfo.MeshType);

		for (int32 Index = 0; Index < NumSplinePoints; Index++)
		{
			UStaticMeshComponent* MeshComp = MeshInitData.MeshComponentsArray[Index].Get();
			UClass* MeshType = MeshComp->GetClass();

			// Replace mesh if type has changed
			if (ConfiguredMeshType != MeshType)
			{
				DestroyMeshComponent(MeshInitData, Index);
				CreateMeshComponent(ConfiguredMeshType, MeshInitData, Index);
				MeshComp = MeshInitData.MeshComponentsArray[Index].Get();
				MeshType = MeshComp->GetClass();
			}

			// Update mesh settings
			const int32 FinalIndex = NumSplinePoints - 1;

			if (!TEST_BIT(MeshInitData.GeneralInfo, EFlexGeneralFlags::Active) // Inactive
				|| Index == FinalIndex && !GetCanLoop(MeshInitData) // No loop, so cut out last mesh 
				|| !CanRenderFromSpawnChance(MeshInitData, Index) // Spawn chance too low
				|| !CanRenderFromMode(MeshInitData, Index, FinalIndex)) // Render-Mode check
			{
				MeshComp->SetVisibility(false);
				MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			else
			{
				// Update type agnostic mesh settings
				MeshComp->SetCollisionProfileName(MeshInitData.PhysicsInfo.CollisionProfileName);
				MeshComp->SetVisibility(true);
				MeshComp->SetCollisionEnabled(GetCollisionEnabled(MeshInitData));
				MeshComp->SetGenerateOverlapEvents(MeshInitData.PhysicsInfo.bGenerateOverlapEvent);
				MeshComp->SetMobility(EComponentMobility::Movable); // <- Required for SetStaticMesh to work correctly
				MeshComp->SetStaticMesh(MeshInitData.MeshInfo.Mesh);
				MeshComp->SetMobility(EComponentMobility::Static);
				MeshComp->SetMaterial(0, MeshInitData.MeshInfo.MeshMaterial);

				// Update type dependent mesh settings
				if (MeshType == SplineMeshClass)
				{
					USplineMeshComponent* SplineMeshComp = Cast<USplineMeshComponent>(MeshComp);
					UpdateSplineMesh(MeshInitData, SplineMeshComp, Index);
				}
				else if (MeshType == StaticMeshClass)
				{
					UpdateStaticMesh(MeshInitData, MeshComp, Index);
				}
			}
		}
	}
}

void AFlexSplineActor::UpdateSplineMesh(const FSplineMeshInitData& MeshInitData, USplineMeshComponent* SplineMesh, int32 CurrentIndex)
{
	if (SplineMesh != nullptr)
	{
		const FName LayerName = GetLayerName(MeshInitData);
		const FSplinePointData& PointData = PointDataArray[CurrentIndex];
		const bool bSync = GetCanSynchronize(PointData) && CurrentIndex > 0;
		auto&& PreviousPointData = bSync ? PointDataArray[CurrentIndex - 1] : FSplinePointData();

		const FVector RandScale = 
			MeshInitData.ScaleInfo.bUseUniformScaleRandomOffset
			? FVector(RandomizeFloat(MeshInitData.ScaleInfo.UniformScaleRandomOffset, CurrentIndex, LayerName))
			: RandomizeVector(MeshInitData.ScaleInfo.ScaleRandomOffset, CurrentIndex, LayerName);
		const FVector2D RandScale2D = FVector2D(RandScale.Y, RandScale.Z);
		const FVector MeshInitScale = 
			MeshInitData.ScaleInfo.bUseUniformScale
			? FVector(1.f, MeshInitData.ScaleInfo.UniformScale, MeshInitData.ScaleInfo.UniformScale)
			: MeshInitData.ScaleInfo.Scale;
		const FVector2D MeshInitScale2D = FVector2D(MeshInitScale.Y, MeshInitScale.Z) + RandScale2D;
		const FRotator RandRotator = RandomizeRotator(MeshInitData.RotationInfo.RotationRandomOffset, CurrentIndex, LayerName);

		// Set spline params
		SetSplineMeshLocation(MeshInitData, SplineMesh, CurrentIndex);
		SplineMesh->SetSplineUpDir(CalculateUpDirection(MeshInitData, PointData, CurrentIndex), false);
		SplineMesh->SetForwardAxis(ToSplineAxis(MeshInitData.MeshInfo.MeshForwardAxis), false);
		SplineMesh->SetRelativeRotation(MeshInitData.RotationInfo.Rotation + RandRotator);
		SplineMesh->SetRelativeScale3D(FVector(MeshInitScale.X + RandScale.X, SplineMesh->GetRelativeScale3D().Y, SplineMesh->GetRelativeScale3D().Z));

		// Apply spline point data (or sync with previous point if demanded)
		SplineMesh->SetStartRoll(bSync ? PreviousPointData.EndRoll : PointData.StartRoll, false);
		SplineMesh->SetEndRoll(PointData.EndRoll, false);
		SplineMesh->SetStartScale((bSync ? PreviousPointData.EndScale : PointData.StartScale) * MeshInitScale2D, false);
		SplineMesh->SetEndScale(PointData.EndScale * MeshInitScale2D, false);
		SplineMesh->UpdateMesh();
	}
}

void AFlexSplineActor::UpdateStaticMesh(const FSplineMeshInitData& MeshInitData, UStaticMeshComponent* StaticMesh, int32 CurrentIndex)
{
	if (StaticMesh != nullptr)
	{
		const FSplinePointData& PointData = PointDataArray[CurrentIndex];

		// Apply mesh-init configurations
		StaticMesh->SetRelativeLocation(CalculateLocation(MeshInitData, PointData, CurrentIndex));
		StaticMesh->SetRelativeRotation(CalculateRotation(MeshInitData, PointData, CurrentIndex));
		StaticMesh->SetRelativeScale3D(CalculateScale(MeshInitData, PointData, CurrentIndex));
	}
}

FName AFlexSplineActor::GetLayerName(const FSplineMeshInitData& MeshInitData) const
{
	const FName* Result = MeshDataInitMap.FindKey(MeshInitData);
	return Result ? *Result : TEXT("");
}

//////////////////////////////////////////////////////////////////////////
// HELPERS
void AFlexSplineActor::GetDeletedIndices(TArray<int32>& OutIndexArray) const
{
	OutIndexArray.Empty();
	const int32 PointDataArraySize = PointDataArray.Num();
	const int32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();

	if (PointDataArraySize > NumberOfSplinePoints)
	{
		// Move along a spline-point-data-pair and store all indices that do not match(CHECK FRONT)
		int32 DataCounter = 0;
		int32 SplinePointCounter = 0;
		for (; SplinePointCounter < NumberOfSplinePoints; SplinePointCounter++, DataCounter++)
		{
			uint32 DataID = PointDataArray[DataCounter].ID;
			const uint32 PointID = GeneratePointHashValue(SplineComponent, SplinePointCounter);

			while (DataID != PointID)
			{
				OutIndexArray.AddUnique(DataCounter);
				DataCounter++;
				DataID = PointDataArray[DataCounter].ID;
			}
		}

		// Store all indices from deleted spline points at the end of the spline(CHECK BACK)
		const int32 LastSplineIndex = NumberOfSplinePoints + OutIndexArray.Num() - 1;
		const int32 LastDataIndex = PointDataArray.Num() - 1;
		for (DataCounter = LastDataIndex; DataCounter > LastSplineIndex; DataCounter--)
		{
			OutIndexArray.AddUnique(DataCounter);
		}
		// Put highest values on top, so highest indices are deleted first to avoid out-of-range-exception
		OutIndexArray.Sort();
		Algo::Reverse(OutIndexArray);
	}
}

FVector AFlexSplineActor::GetTextPosition(int32 Index) const
{
	// Return top of the highest bounding box from all meshes than can be found at this point
	const int32 PointArrayMax = PointDataArray.Num() - 1;
	const FVector SplinePointLocation = SplineComponent->GetLocationAtSplinePoint(Index, WorldSpace);
	float HighestPoint = SplinePointLocation.Z;

	for (const TTuple<FName, FSplineMeshInitData>& MeshInitDataPair : MeshDataInitMap)
	{
		const FSplineMeshInitData& MeshInitData = MeshInitDataPair.Value;
		const FStaticMeshWeakPtr Mesh = MeshInitData.MeshComponentsArray[
			PointArrayMax == Index && Index > 0 && !GetCanLoop(MeshInitData)
			? Index - 1
			: Index];

		if (Mesh.IsValid() && Mesh->IsVisible())
		{
			const float Max = Mesh->Bounds.GetBox().Max.Z;
			HighestPoint = FMath::Max(Max, HighestPoint);
		}
	}

	return {SplinePointLocation.X, SplinePointLocation.Y, HighestPoint};
}

bool AFlexSplineActor::CanRenderFromMode(const FSplineMeshInitData& MeshInitData, int32 CurrentIndex, int32 FinalIndex) const
{
	bool bResult = false;
	// When not looping, the final index should be one point earlier
	FinalIndex = !GetCanLoop(MeshInitData) ? FinalIndex - 1 : FinalIndex;
	FinalIndex = FMath::Max<int32>(0, FinalIndex);

	if (TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Middle))
	{
		bResult = CurrentIndex != 0 && CurrentIndex != FinalIndex;
	}
	if (!bResult && TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Head))
	{
		bResult = CurrentIndex == 0;
	}
	if (!bResult && TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Tail))
	{
		bResult = CurrentIndex == FinalIndex;
	}
	if (!bResult && TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Custom))
	{
		bResult = MeshInitData.RenderInfo.RenderModeCustomIndices.Contains(CurrentIndex);
	}

	return bResult;
}

ECollisionEnabled::Type AFlexSplineActor::GetCollisionEnabled(const FSplineMeshInitData& MeshInitData) const
{
	switch (CollisionActiveConfig)
	{
		case EFlexGlobalConfigType::Everywhere: return ECollisionEnabled::QueryAndPhysics;
		case EFlexGlobalConfigType::Nowhere: return ECollisionEnabled::NoCollision;
		case EFlexGlobalConfigType::Custom: return MeshInitData.PhysicsInfo.Collision;
		default: return ECollisionEnabled::NoCollision;
	}
}

bool AFlexSplineActor::GetCanLoop(const FSplineMeshInitData& MeshInitData) const
{
	switch (LoopConfig)
	{
		case EFlexGlobalConfigType::Everywhere: return true;
		case EFlexGlobalConfigType::Nowhere: return false;
		case EFlexGlobalConfigType::Custom: return TEST_BIT(MeshInitData.GeneralInfo, EFlexGeneralFlags::Loop);
		default: return false;
	}
}

bool AFlexSplineActor::GetCanSynchronize(const FSplinePointData& PointData) const
{
	switch (SynchronizeConfig)
	{
		case EFlexGlobalConfigType::Everywhere: return true;
		case EFlexGlobalConfigType::Nowhere: return false;
		case EFlexGlobalConfigType::Custom: return PointData.bSynchroniseWithPrevious;
		default: return false;
	}
}

FVector AFlexSplineActor::CalculateLocation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
	const FVector SplinePointLocation = SplineComponent->GetLocationAtSplinePoint(Index, LocalSpace);
	FVector MeshInitLocation = MeshInitData.LocationInfo.Location;
	FVector PointDataLocationOffset = PointData.SMLocationOffset;
	FVector RandomizedVector = RandomizeVector(MeshInitData.LocationInfo.LocationRandomOffset, Index, GetLayerName(MeshInitData));

	if (MeshInitData.LocationInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint)
	{
		const FRotator CoordSystem = SplineComponent->GetDirectionAtSplinePoint(Index, LocalSpace).Rotation();
		// Rotate all values around new local coordinate system
		MeshInitLocation = CoordSystem.RotateVector(MeshInitLocation);
		PointDataLocationOffset = CoordSystem.RotateVector(PointDataLocationOffset);
		RandomizedVector = CoordSystem.RotateVector(RandomizedVector);
	}

	return SplinePointLocation + MeshInitLocation + PointDataLocationOffset + RandomizedVector;
}

FRotator AFlexSplineActor::CalculateRotation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, int32 Index) const
{
	const FRotator MeshInitRotation = MeshInitData.RotationInfo.Rotation;
	const FRotator RandomRotation = RandomizeRotator(MeshInitData.RotationInfo.RotationRandomOffset, Index, GetLayerName(MeshInitData));
	const FRotator PointDataRotation = PointData.SMRotation;
	const FRotator SplinePointRotation = MeshInitData.RotationInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint
		? SplineComponent->GetRotationAtSplinePoint(Index, LocalSpace)
		: FRotator::ZeroRotator;

	return MeshInitRotation + RandomRotation + PointDataRotation + SplinePointRotation;
}

FVector AFlexSplineActor::CalculateScale(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
	const FName LayerName = GetLayerName(MeshInitData);
	const FVector RandomScale = MeshInitData.ScaleInfo.bUseUniformScaleRandomOffset
		? FVector(RandomizeFloat(MeshInitData.ScaleInfo.UniformScaleRandomOffset, Index, LayerName))
		: RandomizeVector(MeshInitData.ScaleInfo.ScaleRandomOffset, Index, LayerName);
	const FVector PointDataScale = PointData.SMScale;
	const FVector SplinePointScale = SplineComponent->GetScaleAtSplinePoint(Index);
	const FVector MeshInitScale = MeshInitData.ScaleInfo.bUseUniformScale
		? FVector(MeshInitData.ScaleInfo.UniformScale)
		: MeshInitData.ScaleInfo.Scale;

	return MeshInitScale * SplinePointScale + PointDataScale + RandomScale;
}

FVector AFlexSplineActor::CalculateUpDirection(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
	FVector MeshInitUpDir = MeshInitData.UpVectorInfo.CustomMeshUpDirection;
	FVector PointUpDir = PointData.CustomPointUpDirection;

	if (MeshInitData.UpVectorInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint)
	{
		// Convert vectors to be local to spline point
		const int32 NextIndex = Index + 1 < SplineComponent->GetNumberOfSplinePoints() ? Index + 1 : Index;
		const int32 PreviousIndex = Index > 0 ? Index - 1 : Index;
		const FVector NextIndexDirection = SplineComponent->GetDirectionAtSplinePoint(NextIndex, LocalSpace);
		const FVector PrevIndexDirection = SplineComponent->GetDirectionAtSplinePoint(PreviousIndex, LocalSpace);
		const FRotator CoordSystem = FMath::Lerp(PrevIndexDirection, NextIndexDirection, 0.5f).Rotation();
		MeshInitUpDir = CoordSystem.RotateVector(MeshInitUpDir);
		PointUpDir = CoordSystem.RotateVector(PointUpDir);
	}

	return MeshInitUpDir + PointUpDir;
}

void AFlexSplineActor::SetSplineMeshLocation(const FSplineMeshInitData& MeshInitData, USplineMeshComponent* OutSplineMesh, int32 Index)
{
	if (OutSplineMesh)
	{
		const FSplinePointData& PointData = PointDataArray[Index];
		const int32 NextIndex = (Index + 1) % SplineComponent->GetNumberOfSplinePoints(); // Need to account for looping here
		const bool bSync = GetCanSynchronize(PointData) && Index > 0;
		auto&& PreviousPointData = bSync ? PointDataArray[Index - 1] : FSplinePointData();
		const FName LayerName = GetLayerName(MeshInitData);

		const FVector StartTangent = SplineComponent->GetTangentAtSplinePoint(Index, LocalSpace);
		const FVector EndTangent = SplineComponent->GetTangentAtSplinePoint(NextIndex, LocalSpace);
		FVector StartLocation = SplineComponent->GetLocationAtSplinePoint(Index, LocalSpace);
		FVector EndLocation = SplineComponent->GetLocationAtSplinePoint(NextIndex, LocalSpace);
		const FVector RandomVectorCurrentIndex = RandomizeVector(MeshInitData.LocationInfo.LocationRandomOffset, Index, LayerName);
		const FVector RandomVectorNextIndex = RandomizeVector(MeshInitData.LocationInfo.LocationRandomOffset, NextIndex, LayerName);

		if (MeshInitData.LocationInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint)
		{
			OutSplineMesh->SetRelativeLocation(FVector::ZeroVector); // Needs to be unset in this config
			const FRotator CurrentIndexCoordSystem = SplineComponent->GetDirectionAtSplinePoint(Index, LocalSpace).Rotation();
			const FRotator NextIndexCoordSystem = SplineComponent->GetDirectionAtSplinePoint(NextIndex, LocalSpace).Rotation();
			const FVector RotatedMeshInitLocationCurrentIndex = CurrentIndexCoordSystem.RotateVector(MeshInitData.LocationInfo.Location);
			const FVector RotatedMeshInitLocationNextIndex = NextIndexCoordSystem.RotateVector(MeshInitData.LocationInfo.Location);
			StartLocation += RotatedMeshInitLocationCurrentIndex + RandomVectorCurrentIndex;
			EndLocation += RotatedMeshInitLocationNextIndex + RandomVectorNextIndex;
		}
		else if (MeshInitData.LocationInfo.CoordinateSystem == EFlexCoordinateSystem::SplineSystem)
		{
			OutSplineMesh->SetRelativeLocation(MeshInitData.LocationInfo.Location + RandomVectorCurrentIndex);
		}

		OutSplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);
		OutSplineMesh->SetStartOffset(bSync ? PreviousPointData.EndOffset : PointData.StartOffset);
		OutSplineMesh->SetEndOffset(PointData.EndOffset);
	}
}

UStaticMeshComponent* AFlexSplineActor::CreateMeshComponent(UClass* MeshType, FSplineMeshInitData& MeshInitData, int32 Index)
{
	UStaticMeshComponent* NewMesh = NewObject<UStaticMeshComponent>(this, MeshType);
	NewMesh->RegisterComponent();
	NewMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	if (Index < 0)
	{
		MeshInitData.MeshComponentsArray.Add(NewMesh);
	}
	else
	{
		MeshInitData.MeshComponentsArray.Insert(NewMesh, Index);
	}

	return NewMesh;
}

UArrowComponent* AFlexSplineActor::CreateArrowComponent(FSplineMeshInitData& MeshInitData)
{
	UArrowComponent* NewArrow = NewObject<UArrowComponent>(RootComponent);
	NewArrow->RegisterComponent();
	NewArrow->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	NewArrow->SetHiddenInGame(true);
	NewArrow->ArrowSize = UpDirectionArrowSize;
	MeshInitData.ArrowSplineUpIndicatorArray.Add(NewArrow);

	return NewArrow;
}
