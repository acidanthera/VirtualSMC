//
//  SMCLightSensor.hpp
//  SMCLightSensor
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef SMCLightSensor_hpp
#define SMCLightSensor_hpp

#include <IOKit/IOService.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <Headers/kern_util.hpp>

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOTimerEventSource.h>

#include "AmbientLightValue.hpp"

class EXPORT SMCLightSensor : public IOService {
	OSDeclareDefaultStructors(SMCLightSensor)

	/**
	 *  VirtualSMC service registration notifier
	 */
	IONotifier *vsmcNotifier {nullptr};

	/**
	 *  A workloop in charge of handling timer events with requests.
	 */
	IOWorkLoop *workloop {nullptr};

	/**
	 *  Workloop timer event source for status updates
	 */
	IOTimerEventSource *poller {nullptr};

	/**
	 *  Refresh sensor values to inform macOS with light changes
	 *
	 *  @param post  post an SMC notification
	 */
	bool refreshSensor(bool post);

	/**
	 *  Interrupt submission timeout
	 */
	static constexpr uint32_t SensorUpdateTimeoutMS {1000};

	/**
	 *  Key name definitions
	 */
	static constexpr SMC_KEY KeyAL   = SMC_MAKE_IDENTIFIER('A','L','!',' ');
	static constexpr SMC_KEY KeyALI0 = SMC_MAKE_IDENTIFIER('A','L','I','0');
	static constexpr SMC_KEY KeyALI1 = SMC_MAKE_IDENTIFIER('A','L','I','1');
	static constexpr SMC_KEY KeyALRV = SMC_MAKE_IDENTIFIER('A','L','R','V');
	static constexpr SMC_KEY KeyALV0 = SMC_MAKE_IDENTIFIER('A','L','V','0');
	static constexpr SMC_KEY KeyALV1 = SMC_MAKE_IDENTIFIER('A','L','V','1');
	static constexpr SMC_KEY KeyLKSB = SMC_MAKE_IDENTIFIER('L','K','S','B');
	static constexpr SMC_KEY KeyLKSS = SMC_MAKE_IDENTIFIER('L','K','S','S');
	static constexpr SMC_KEY KeyMSLD = SMC_MAKE_IDENTIFIER('M','S','L','D');

	/**
	 *  Registered plugin instance
	 */
	VirtualSMCAPI::Plugin vsmcPlugin {
		xStringify(PRODUCT_NAME),
		parseModuleVersion(xStringify(MODULE_VERSION)),
		VirtualSMCAPI::Version,
	};

	/**
	 *  ALSSensor structure contains sensor-specific information for this system
	 */
	struct ALSSensor {
		/**
		 *  Supported sensor types
		 */
		enum Type : uint8_t {
			NoSensor  = 0,
			BS520     = 1,
			TSL2561CS = 2,
			LX1973A   = 3,
			ISL29003  = 4,
			Unknown7  = 7
		};

		/**
		 *  Sensor type
		 */
		Type sensorType {Type::NoSensor};

		/**
		 * TRUE if no lid or if sensor works with closed lid.
		 * FALSE otherwise.
		 */
		bool validWhenLidClosed {false};

		/**
		 *  Possibly ID
		 */
		uint8_t unknown {0};

		/**
		 * TRUE if the SIL brightness depends on this sensor's value.
		 * FALSE otherwise.
		 */
		bool controlSIL {false};
	};

	/**
	 *  Keyboard backlight brightness
	 */
	struct lkb {
		uint8_t unknown0 {0};
		uint8_t unknown1 {1};
	};

	struct lks {
		uint8_t unknown0 {0};
		uint8_t unknown1 {1};
	};

	/**
	 *  Ambient light device
	 */
	IOACPIPlatformDevice *alsdDevice {nullptr};

	/**
	 *  Current lux value obtained from ACPI
	 */
	_Atomic(uint32_t) currentLux;

	/**
	 *  Supported ALS bits
	 */
	ALSForceBits forceBits;

public:

	/**
	 *  Class contstruction method
	 *
	 *  @param dict registry entry table
	 *
	 *  @return true on success
	 */
	bool init(OSDictionary *dict) override;

	/**
	 *  Decide on whether to load or not by checking the processor compatibility.
	 *
	 *  @param provider  parent IOService object
	 *  @param score     probing score
	 *
	 *  @return self if we could load anyhow
	 */
	IOService *probe(IOService *provider, SInt32 *score) override;
	
	/**
	 *  Add VirtualSMC listening notification.
	 *
	 *  @param provider  parent IOService object
	 *
	 *  @return true on success
	 */
	bool start(IOService *provider) override;
	
	/**
	 *  Detect unloads, though they are prohibited.
	 *
	 *  @param provider  parent IOService object
	 */
	void stop(IOService *provider) override;
	
	/**
	 *  Submit the keys to received VirtualSMC service.
	 *
	 *  @param sensors   SMCLightSensor service
	 *  @param refCon    reference
	 *  @param vsmc      VirtualSMC service
	 *  @param notifier  created notifier
	 */
	static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
};

#endif /* SMCLightSensor_hpp */
