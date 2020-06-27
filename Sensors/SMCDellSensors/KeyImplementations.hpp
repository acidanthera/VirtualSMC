//
//  KeyImplementations.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef KeyImplementations_hpp
#define KeyImplementations_hpp

#include <libkern/libkern.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SMIMonitor.hpp"

class SMIKey : public VirtualSMCValue { };

class SMIIdxKey : public VirtualSMCValue {
protected:
	size_t index;
public:
	SMIIdxKey(size_t index) : index(index) {}
};

class F0Ac : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class F0As : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class F0Mn : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class F0Mx : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };

class TC0P : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class TG0P : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class Tm0P : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class TN0P : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class TA0P : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };
class TW0P : public SMIIdxKey { using SMIIdxKey::SMIIdxKey; protected: SMC_RESULT readAccess() override; };

#endif /* KeyImplementations_hpp */
