using UnrealBuildTool;

public class FlexSpline : ModuleRules
{
	public FlexSpline(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		PublicIncludePaths.AddRange(new string[] {
		});


		PrivateIncludePaths.AddRange(new[] {
			"FlexSpline/Private"
		});
			
		
		PublicDependencyModuleNames.AddRange(new[] {
			"Core"
		});
			
		
		PrivateDependencyModuleNames.AddRange(new[] {
			"CoreUObject",
			"Engine"
		});


		PrivateIncludePathModuleNames.AddRange(new string[] {
		});

		DynamicallyLoadedModuleNames.AddRange(new string[] {
		});
	}
}
