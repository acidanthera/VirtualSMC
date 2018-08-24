//
//  KeyImplementations.hpp
//  SMCProcessor
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef KeyImplementations_hpp
#define KeyImplementations_hpp

#include <libkern/libkern.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

class SMCProcessor;

class CpIdxKey : public VirtualSMCValue {
protected:
	SMCProcessor *cp;
	size_t package;
	size_t core;
public:
	CpIdxKey(SMCProcessor *cp, size_t package, size_t core=0) : cp(cp), package(package), core(core) {}
};

class TempPackage    : public CpIdxKey { using CpIdxKey::CpIdxKey; protected: SMC_RESULT readAccess() override; };
class TempCore       : public CpIdxKey { using CpIdxKey::CpIdxKey; protected: SMC_RESULT readAccess() override; };
class VoltagePackage : public CpIdxKey { using CpIdxKey::CpIdxKey; protected: SMC_RESULT readAccess() override; };

class CpEnergyKey : public VirtualSMCValue {
protected:
	SMCProcessor *cp;
	size_t index;
	SMC_RESULT readAccess() override;
public:
	CpEnergyKey(SMCProcessor *cp, size_t index) : cp(cp), index(index) {}
};

#endif /* KeyImplementations_hpp */
