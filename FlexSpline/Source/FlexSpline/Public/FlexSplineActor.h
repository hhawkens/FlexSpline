#pragma once

#include "GameFramework/Actor.h"
#include "FlexSplineEnums.h"
#include "FlexSplineStructs.h"
#include "FlexSplineActor.generated.h"

/**
* This Actor contains a spline component that can be flexibly configured on a per mesh
* or per spline-point basis. Multiple meshes can be placed along the spline either
* as a spline mesh or retain their form as a static mesh
*/
UCLASS(HideCategories = ("Actor Tick", Rendering, Input, Actor))
class FLEXSPLINE_API AFlexSplineActor : public AActor
{
	GENERATED_BODY()

public:

	AFlexSplineActor();
	void OnConstruction(const FTransform& Transform) override;
	void PreInitializeComponents() override;

	int32 GetMeshCountForType(EFlexSplineMeshType MeshType) const;


protected:

	/** Spawns and initiates spline mesh components for each spline point */
	void ConstructSplineMesh();

	/** If mesh data has just been created initialize it with template */
	void InitializeNewMeshData();

	/** Create new point data if there is a new spline point */
	void AddPointDataEntries();

	/** Remove point data associated with deleted spline point */
	void RemovePointDataEntries(const TArray<int32>& DeletedIndices);

	/** Create new mesh components if there are fewer meshes than spline points */
	void InitDataAddMeshes();

	/** Remove mesh components if there are more meshes than spline points */
	void InitDataRemoveMeshes(const TArray<int32>& DeletedIndices);

	/** Bring point data identifiers up to date */
	void UpdatePointData();

	/** Adjust text renderer position and text according to points and meshes */
	void UpdateDebugInformation();


	/** Set mesh values according to mesh and point data */
	void UpdateMeshComponents();

	/** Called by UpdateMeshComponents, specialized for spline meshes */
	void UpdateSplineMesh(const FSplineMeshInitData& MeshInitData, class USplineMeshComponent* SplineMesh,
						  int32 CurrentIndex);

	/** Called by UpdateMeshComponents, specialized for static meshes */
	void UpdateStaticMesh(const FSplineMeshInitData& MeshInitData, class UStaticMeshComponent* StaticMesh,
						  int32 CurrentIndex);


protected:

	/** Return data's layer name, if available */
	FName GetLayerName(const FSplineMeshInitData& MeshInitData) const;

	/** Find and return all indices of spline mesh that were deleted since last update */
	void GetDeletedIndices(TArray<int32>& OutIndexArray) const;

	/** Find best position for the text renderer at this index */
	FVector GetTextPosition(int32 Index) const;

	/** Is rendering allowed, given the current index? */
	bool CanRenderFromMode(const FSplineMeshInitData& MeshInitData, int32 CurrentIndex, int32 FinalIndex) const;

	/** Find appropriate collision taking Mesh Layer and Flex Spline config into account */
	ECollisionEnabled::Type GetCollisionEnabled(const FSplineMeshInitData& MeshInitData) const;

	/** See if looping is enabled globally and for given mesh data */
	bool GetCanLoop(const FSplineMeshInitData& MeshInitData) const;

	/** Find out if current spline point should be synchronized */
	bool GetCanSynchronize(const FSplinePointData& PointData) const;

	/** Compute location for mesh according to spline, point and layer information, using the configured coordinate system */
	FVector CalculateLocation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, int32 Index) const;

	/** Compute rotation for mesh according to spline, point and layer information, using the configured coordinate system */
	FRotator CalculateRotation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, int32 index) const;

	/** Compute scale for mesh according to spline, point and layer information*/
	FVector CalculateScale(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, int32 Index) const;

	/** Get up direction for spline according to chosen local space */
	FVector CalculateUpDirection(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, int32 Index) const;

	/** Calculate location for spline mesh and apply to param outSplineMesh */
	void SetSplineMeshLocation(const FSplineMeshInitData& MeshInitData, class USplineMeshComponent* OutSplineMesh, int32 Index);

	/**
	* Create a new mesh component of class meshType, add to mesh init data array.
	* If no valid index is specified, it is appended
	*/
	class UStaticMeshComponent* CreateMeshComponent(UClass* MeshType, FSplineMeshInitData& MeshInitData, int32 Index = -1);

	/** Create arrow component, add to Actor root, cache inside @param MeshInitData */
	class UArrowComponent* CreateArrowComponent(FSplineMeshInitData& MeshInitData);


protected:

	UPROPERTY(VisibleAnywhere, Category = "FlexSpline", meta = (AllowPrivateAccess = "true"))
	class USplineComponent* SplineComponent;


protected:

	/**
	* Sets all collisions Active, Inactive or Defined per Mesh Layer (see PhysicsInfo -> Collision)
	*/
	UPROPERTY(EditAnywhere, Category = "FlexSpline|Global", meta = (DisplayName = "CollisionActive"))
	EFlexGlobalConfigType CollisionActiveConfig;

	/**
	* Allow spline points to synchronize their start values with the previous point's
	* end values. Can be configured per spline point.
	*/
	UPROPERTY(EditAnywhere, Category = "FlexSpline|Global", meta = (DisplayName = "Synchronize"))
	EFlexGlobalConfigType SynchronizeConfig;

	/** Should spline bite its own tail? */
	UPROPERTY(EditAnywhere, Category = "FlexSpline|Global", meta = (DisplayName = "Loop"))
	EFlexGlobalConfigType LoopConfig;

	/** Blueprint for new "Mesh Layer" entries */
	UPROPERTY(EditAnywhere, Category = "FlexSpline|Global", meta = (DisplayName = "Mesh Layer Template"))
	FSplineMeshInitData MeshDataTemplate;


	/** Should the index for each spline point be displayed? */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline")
	bool bShowPointNumbers;

	/** Spline index text renderer size */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline", meta = (ClampMin = "0.0", UIMax = "500.0"))
	float PointNumberSize;

	/** Debug up direction arrow component size */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline", meta = (ClampMin = "0.0", UIMax = "10.0"))
	float UpDirectionArrowSize;

	/** Debug up direction arrow vertical offset for better visibility */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline", meta = (ClampMin = "0.0", UIMax = "200.0"))
	float UpDirectionArrowOffset;

	/** Color of the spline point text renderer */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline")
	FColor TextRenderColor;


	/**
	* Mesh configuration for each spline point, resizes automatically
	*/
	UPROPERTY()
	TArray<FSplinePointData> PointDataArray;

	/** Stores all meshes(and related info) that should be spawned per spline point */
	UPROPERTY(EditAnywhere, Category = "FlexSpline", meta = (DisplayName = "Mesh Layers", NoElementDuplicate))
	TMap<FName, FSplineMeshInitData> MeshDataInitMap;


private:

	/** Cache lastly generated MeshDataInitMap key to circumvent strange engine behavior */
	FName LastUsedKey;

	/** Details customizer class needs access to all members */
	friend class FFlexSplineNodeBuilder;
};
