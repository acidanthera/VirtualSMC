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

#include "BatteryManager.hpp"

class BatKey : public VirtualSMCValue { };

class BatIdxKey : public VirtualSMCValue {
protected:
	size_t index;
public:
	BatIdxKey(size_t index) : index(index) {}
};

class AC_N : public BatKey { protected: SMC_RESULT readAccess() override; };
class ACID : public BatKey { protected: SMC_RESULT readAccess() override; };
class ACIN : public BatKey { protected: SMC_RESULT readAccess() override; };

class B0AC : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0AV : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0BI : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0CT : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0FC : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0PS : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0RM : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0St : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class B0TF : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };
class TB0T : public BatIdxKey { using BatIdxKey::BatIdxKey; protected: SMC_RESULT readAccess() override; };

class BATP : public BatKey { protected: SMC_RESULT readAccess() override; };
class BBAD : public BatKey { protected: SMC_RESULT readAccess() override; };
class BBIN : public BatKey { protected: SMC_RESULT readAccess() override; };
class BFCL : public BatKey { protected: SMC_RESULT readAccess() override; };
class BNum : public BatKey { protected: SMC_RESULT readAccess() override; };
class BSIn : public BatKey { protected: SMC_RESULT readAccess() override; };
class BRSC : public BatKey { protected: SMC_RESULT readAccess() override; };
class CHBI : public BatKey { protected: SMC_RESULT readAccess() override; };
class CHBV : public BatKey { protected: SMC_RESULT readAccess() override; };
class CHLC : public BatKey { protected: SMC_RESULT readAccess() override; };

//TODO: implement these
// class D0IR : public BatKey { using BatKey::BatKey; protected: SMC_RESULT readAccess() override; };
// class D0VM : public BatKey { using BatKey::BatKey; protected: SMC_RESULT readAccess() override; };
// class D0VR : public BatKey { using BatKey::BatKey; protected: SMC_RESULT readAccess() override; };
// class D0VX : public BatKey { using BatKey::BatKey; protected: SMC_RESULT readAccess() override; };

#endif /* KeyImplementations_hpp */
