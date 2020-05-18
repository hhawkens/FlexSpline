#include "FlexSplineDetailsModule.h"
#include "FlexSplineDetails/FlexSplineDetails.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FFlexSplineDetailsModule"

void FFlexSplineDetailsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("FlexSplineActor", FOnGetDetailCustomizationInstance::CreateStatic(&FFlexSplineDetails::MakeInstance));
	
}

void FFlexSplineDetailsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout("FlexSplineActor");
}

#undef LOCTEXT_NAMESPACE
	
DEFINE_LOG_CATEGORY(FlexDetailsLog);
IMPLEMENT_MODULE(FFlexSplineDetailsModule, FlexSplineDetails)
