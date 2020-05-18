#pragma once

#include "Modules/ModuleManager.h"

class FFlexSplineModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;
};


DECLARE_LOG_CATEGORY_EXTERN(FlexLog, Log, All);
