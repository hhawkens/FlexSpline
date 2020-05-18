using UnrealBuildTool;

public class FlexSplineDetails : ModuleRules
{
	public FlexSplineDetails(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		PublicIncludePaths.AddRange(new string[] {
		});


		PrivateIncludePaths.AddRange( new[] {
			"FlexSplineDetails/Private",
			"FlexSplineDetails/Private/DetailsCustomization"
		});
			
		
		PublicDependencyModuleNames.AddRange( new[] {
			"Core"
		});
			
		
		PrivateDependencyModuleNames.AddRange(new[] {
			"CoreUObject",
			"Engine",
			"InputCore",
			"UnrealEd",

			"FlexSpline",

			"DesktopWidgets",
			"Slate",
			"SlateCore",
			"EditorWidgets",
			"PropertyEditor",
			"ComponentVisualizers",
			"EditorStyle"
		});


		PrivateIncludePathModuleNames.AddRange(new string[] {
		});

		DynamicallyLoadedModuleNames.AddRange(new string[] {
		});
	}
}
