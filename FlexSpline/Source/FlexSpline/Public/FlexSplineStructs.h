#pragma once

#include "CoreMinimal.h"
#include "FlexSplineMacros.h"
#include "FlexSplineStructs.generated.h"

using FStaticMeshWeakPtr = TWeakObjectPtr<class UStaticMeshComponent>;
using FArrowWeakPtr = TWeakObjectPtr<class UArrowComponent>;

USTRUCT(BlueprintType)
struct FFlexMeshInfo
{
	GENERATED_BODY()

	/** How should the mesh be rendered? */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	EFlexSplineMeshType MeshType;

	/** Which axis of the mesh should be defined as its front? Only relevant for spline meshes */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	EFlexSplineAxis MeshForwardAxis;

	/** Visual representation and collision */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	UStaticMesh* Mesh;

	/** Material override for mesh. If set to null, mesh resets to its default material */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	UMaterialInterface* MeshMaterial;

	FFlexMeshInfo(EFlexSplineAxis InForwardAxis = EFlexSplineAxis::X, EFlexSplineMeshType InType = EFlexSplineMeshType::SplineMesh):
		MeshType(InType),
		MeshForwardAxis(InForwardAxis),
		Mesh(nullptr),
		MeshMaterial(nullptr)
	{
	}
};

USTRUCT(BlueprintType)
struct FFlexRenderInfo
{
	GENERATED_BODY()

	/** Let mesh be spawned linearly or randomly according to its spawn chance? */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	uint32 bRandomizeSpawnChance : 1;

	/** How likely is mesh to spawn on spline point? */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance;

	/** At what places of the spline should this mesh be rendered? */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (Bitmask, BitmaskEnum = "EFlexSplineRenderMode"))
	int32 RenderMode;

	/** Define indices at which to render the mesh. Only used if "Custom" Render Mode is active */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	TSet<uint32> RenderModeCustomIndices;

	FFlexRenderInfo(float InSpawnChance = 1.f, bool bInRandomizeSpawnChance = true):
		bRandomizeSpawnChance(bInRandomizeSpawnChance),
		SpawnChance(InSpawnChance),
		RenderMode(0)
	{
		SET_BIT(RenderMode, EFlexSplineRenderMode::Head);
		SET_BIT(RenderMode, EFlexSplineRenderMode::Tail);
		SET_BIT(RenderMode, EFlexSplineRenderMode::Middle);
	}
};

USTRUCT(BlueprintType)
struct FFlexPhysicsInfo
{
	GENERATED_BODY()

	/** Set collision for this layer. Collision needs to be globally activated for this to take effect */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	TEnumAsByte<ECollisionEnabled::Type> Collision;

	/** What collision preset to use for this mesh type */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	FName CollisionProfileName;

	/** Overlap when collision is active? */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	uint32 bGenerateOverlapEvent : 1;

	FFlexPhysicsInfo(ECollisionEnabled::Type InCollision = ECollisionEnabled::QueryOnly,
					 FName InCollisionProfileName = "BlockAll",
					 bool bInGenerateOverlapEvent = false):
		Collision(InCollision),
		CollisionProfileName(InCollisionProfileName),
		bGenerateOverlapEvent(bInGenerateOverlapEvent)
	{
	}
};

USTRUCT(BlueprintType)
struct FFlexRotationInfo
{
	GENERATED_BODY()

	/** Choose relative coordinate system */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	EFlexCoordinateSystem CoordinateSystem;

	/** Rotation relative to chosen coordinate system  */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	FRotator Rotation;

	/** Randomize rotation, seeded */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	FRotator RotationRandomOffset;

	FFlexRotationInfo(FRotator InRotation = FRotator::ZeroRotator, FRotator InRotationRandomOffset = FRotator::ZeroRotator):
		CoordinateSystem(EFlexCoordinateSystem::SplinePoint),
		Rotation(InRotation),
		RotationRandomOffset(InRotationRandomOffset)
	{
	}
};

USTRUCT(BlueprintType)
struct FFlexLocationInfo
{
	GENERATED_BODY()

	/** Choose relative coordinate system */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	EFlexCoordinateSystem CoordinateSystem;

	/** Location relative to chosen coordinate system */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	FVector Location;

	/** Randomize location, seeded */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	FVector LocationRandomOffset;

	FFlexLocationInfo(FVector InLocation = FVector::ZeroVector, FVector InLocationRandomOffset = FVector::ZeroVector):
		CoordinateSystem(EFlexCoordinateSystem::SplinePoint),
		Location(InLocation),
		LocationRandomOffset(InLocationRandomOffset)
	{
	}
};

USTRUCT(BlueprintType)
struct FFlexScaleInfo
{
	GENERATED_BODY()

	/** If active, "Uniform Scale" will be used to determine scale" */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	uint32 bUseUniformScale : 1;

	/** Set X, Y and Z scale value simultaneously */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (ClampMin = "0.0", EditCondition = "bUseUniformScale"))
	float UniformScale;

	/** Scale, relative to spline point */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (EditCondition = "!bUseUniformScale"))
	FVector Scale;

	/** If active, "Uniform Scale Random Offset" will be used to determine scale offset" */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	uint32 bUseUniformScaleRandomOffset : 1;

	/** Randomize scale, seeded. The X,Y and Z offsets will always have the same value */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (ClampMin = "0.0", EditCondition = "bUseUniformScaleRandomOffset"))
	float UniformScaleRandomOffset;

	/** Randomize scale, seeded */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (EditCondition = "!bUseUniformScaleRandomOffset"))
	FVector ScaleRandomOffset;


	FFlexScaleInfo(float InUniformScale = 1.f, FVector InScale = FVector(1.f), FVector InScaleRandomOffset = FVector::ZeroVector):
		bUseUniformScale(true),
		UniformScale(InUniformScale),
		Scale(InScale),
		bUseUniformScaleRandomOffset(true),
		UniformScaleRandomOffset(0.f),
		ScaleRandomOffset(InScaleRandomOffset)
	{
	}
};

USTRUCT(BlueprintType)
struct FFlexUpVectorInfo
{
	GENERATED_BODY()

	/** Editor feature: Should the up vector for each spline point for this mesh be displayed? */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	uint32 bShowUpDirection : 1;

	/** Choose local coordinate system */
	UPROPERTY(EditAnywhere, Category = FlexSpline)
	EFlexCoordinateSystem CoordinateSystem;

	/** Up direction for all spline meshes of this kind */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Up Direction"))
	FVector CustomMeshUpDirection;

	FFlexUpVectorInfo(bool bInShowUpDirection = false, FVector InCustomMeshUpDirection = FVector(0.f,0.f,1.f)):
		bShowUpDirection(bInShowUpDirection),
		CoordinateSystem(EFlexCoordinateSystem::SplineSystem),
		CustomMeshUpDirection(InCustomMeshUpDirection)
	{
	}
};


/**
* Stores info on what meshes and which default values on each spline point are initialized
*/
USTRUCT(BlueprintType)
struct FSplineMeshInitData
{
	GENERATED_BODY()


	/** General Mesh Layer settings. Only apply if correspondent global settings are set to Custom */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (Bitmask, BitmaskEnum = "EFlexGeneralFlags", DisplayName = "General"))
	int32 GeneralInfo;

	/** Mesh information */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Mesh"))
	FFlexMeshInfo MeshInfo;

	/** Rendering configuration */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Rendering"))
	FFlexRenderInfo RenderInfo;

	/** Configuration of physics and collision */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Physics"))
	FFlexPhysicsInfo PhysicsInfo;

	/** Rotation control relative to spline point */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Rotation"))
	FFlexRotationInfo RotationInfo;

	/** Location control relative to spline point */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Location"))
	FFlexLocationInfo LocationInfo;

	/** Scale control relative to spline point */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Scale"))
	FFlexScaleInfo ScaleInfo;

	/** Up vector control relative to spline point */
	UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Up Vector"))
	FFlexUpVectorInfo UpVectorInfo;


	/**
	* Stores all spline mesh components, driven by data from this instance
	* Each mesh is associated to a spline point via its index
	*/
	TArray<FStaticMeshWeakPtr> MeshComponentsArray;

	/** Shows the spline up vector at each spline point */
	TArray<FArrowWeakPtr> ArrowSplineUpIndicatorArray;


	FSplineMeshInitData()
		: bTemplatedInitialized(false)
	{
		SET_BIT(GeneralInfo, EFlexGeneralFlags::Active);
	}

	/** Delete all spline meshes and arrows on destruction */
	~FSplineMeshInitData();

	bool operator==(const FSplineMeshInitData& Other) const
	{
		return this == &Other;
	}

	bool IsInitialized() const { return bTemplatedInitialized; }
	void Initialize() { bTemplatedInitialized = true; }


private:

	/** Has this init data been initialized with given template? Use Initialize() to confirm */
	UPROPERTY()
	uint32 bTemplatedInitialized : 1;
};


/**
* Stores data for each spline point, may override initial data
*/
USTRUCT(BlueprintType)
struct FSplinePointData
{
	GENERATED_BODY()

// ============================= SPLINE MESH FEATURES

	/** Only editable if not synchronized with previous point */
	UPROPERTY()
	float StartRoll;

	UPROPERTY()
	float EndRoll;

	/** Only editable if not synchronized with previous point */
	UPROPERTY()
	FVector2D StartScale;

	UPROPERTY()
	FVector2D EndScale;

	/** Only editable if not synchronized with previous point */
	UPROPERTY()
	FVector2D StartOffset;

	UPROPERTY()
	FVector2D EndOffset;

	/** Up direction for all spline meshes of this point */
	UPROPERTY()
	FVector CustomPointUpDirection;

	/**
	* If this is active, the spline at this point will deform its start values
	* to match the last point's end values. Start values will be overridden
	*/
	UPROPERTY()
	bool bSynchroniseWithPrevious;


	// ============================= STATIC MESH FEATURES

	UPROPERTY()
	FVector SMLocationOffset;

	UPROPERTY()
	FVector SMScale;

	UPROPERTY()
	FRotator SMRotation;


	/** Displays the index for the associated spline point */
	UPROPERTY()
	class UTextRenderComponent* IndexTextRenderer;

	/** Unique identifier, hash-value */
	uint32 ID;

	/** CONSTRUCTOR */
	FSplinePointData():
		StartRoll(0.f),
		EndRoll(0.f),
		StartScale(1.f, 1.f),
		EndScale(1.f, 1.f),
		StartOffset(0.f, 0.f),
		EndOffset(0.f, 0.f),
		CustomPointUpDirection(0.f),
		bSynchroniseWithPrevious(true),
		SMLocationOffset(0.f),
		SMScale(0.f),
		SMRotation(0.f),
		IndexTextRenderer(nullptr),
		ID(0)
		{
		}
};