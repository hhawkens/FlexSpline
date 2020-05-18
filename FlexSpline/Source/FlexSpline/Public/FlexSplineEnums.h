#pragma once

#include "CoreMinimal.h"
#include "FlexSplineEnums.generated.h"

/** Generic (XYZ - )Axis Type */
UENUM(BlueprintType)
enum class EFlexSplineAxis : uint8
{
	X,
	Y,
	Z
};

/** Defines relative coordinate system for meshes (e.g. for location, rotation...) */
UENUM(BlueprintType)
enum class EFlexCoordinateSystem : uint8
{
	/** Use coordinates local to the related spline point */
	SplinePoint,
	/** Use coordinates local to the entire blueprint instance */
	SplineSystem
};

/** Generically defines where given configurations apply */
UENUM(BlueprintType)
enum class EFlexGlobalConfigType : uint8
{
	/** Force configuration for all instances to be true */
	Everywhere,
	/** Force configuration for all instances to be false */
	Nowhere,
	/** Instances decide for themselves */
	Custom
};

/** What mesh type to use for flex spline init data */
UENUM(BlueprintType)
enum class EFlexSplineMeshType : uint8
{
	/** Deforms along with spline */
	SplineMesh,
	/** Retains its form, gets placed along spline */
	StaticMesh
};

/** At what place of the spline should a mesh be rendered */
UENUM(BlueprintType, meta = (Bitflags))
enum class EFlexSplineRenderMode : uint8
{
	/** The first spline point */
	Head,
	/** The last spline point */
	Tail,
	/** Everything between the first and the last spline point */
	Middle,
	/** Every spline point specified by "Render Mode Custom Indices" */
	Custom
};

/** Controls miscellaneous settings of mesh layers */
UENUM(BlueprintType, meta = (Bitflags))
enum class EFlexGeneralFlags : uint8
{
	/** Set Mesh Layer Visibility  */
	Active,
	/** Enable Looping for this Mesh Layer */
	Loop
};
