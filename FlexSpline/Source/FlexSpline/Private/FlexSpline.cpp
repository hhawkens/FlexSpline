#include "FlexSplineModule.h"

#define LOCTEXT_NAMESPACE "FFlexSplineModule"

void FFlexSplineModule::StartupModule()
{
}

void FFlexSplineModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

DEFINE_LOG_CATEGORY(FlexLog)
IMPLEMENT_MODULE(FFlexSplineModule, FlexSpline)
