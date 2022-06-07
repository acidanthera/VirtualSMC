//
//  kern_start.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_prov.hpp"

static const char *bootargOff[] {
	"-vsmcoff"
};

static const char *bootargDebug[] {
	"-vsmcdbg"
};

static const char *bootargBeta[] {
	"-vsmcbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::Ventura,
	[]() {
		auto prov = VirtualSMCProvider::getInstance();
		if (prov)
			DBGLOG("init", "successfully created vsmc provider instance");
	}
};
