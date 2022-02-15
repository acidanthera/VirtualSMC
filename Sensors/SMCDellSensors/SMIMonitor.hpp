/*
 *  SMIMonitor.hpp
 *  SMCDellSensors
 *
 *  Copyright (C) 2001  Massimo Dal Zotto <dz@debian.org>
 *  http://www.debian.org/~dz/i8k/
 *  more work https://www.diefer.de/i8kfan/index.html , 2007
 *  Adapted for VirtualSMC by lvs1974, 2020
 *  Original sources: https://github.com/CloverHackyColor/FakeSMC3_with_plugins
 *
 */

#ifndef SMIMonitor_hpp
#define SMIMonitor_hpp

#include <i386/proc_reg.h>
#include <stdatomic.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SMIState.hpp"


#define I8K_SMM_FN_STATUS					0x0025
#define I8K_SMM_POWER_STATUS				0x0069	/* 0x85*/ /* not confirmed*/
#define I8K_SMM_SET_FAN						0x01a3
#define I8K_SMM_GET_FAN						0x00a3
#define I8K_SMM_GET_SPEED					0x02a3
#define I8K_SMM_GET_FAN_TYPE				0x03a3
#define I8K_SMM_GET_NOM_SPEED				0x04a3
#define I8K_SMM_GET_TOLERANCE				0x05a3
#define I8K_SMM_GET_TEMP					0x10a3
#define I8K_SMM_GET_TEMP_TYPE				0x11a3
//0x12a3  arg 0x0003=NBSVC-Query
//	arg 0x0000 = NBSVC - Clear
//	arg 0x122 = NBSVC - Start Trend
//	arg 0x0100 = NBSVC - Stop Trend
//	arg 0x02 ? ? = NBSVC - Read
//0x21a3  ??? (2 args: 1 byte (oder 0x16) + 1 byte)
#define I8K_SMM_GET_POWER_TYPE				0x22a3
//0x23a3  ??? (4 args: 2x 1 byte, 1xword, 1xdword)
#define I8K_SMM_GET_POWER_STATUS			0x24a3
#define I8K_SMM_IO_DISABLE_FAN_CTL1     	0x30a3	//No automatic speed control, arg = 0? fan?
#define I8K_SMM_IO_ENABLE_FAN_CTL1      	0x31a3
#define I8K_SMM_ENABLE_FN					0x32a3
#define I8K_SMM_IO_DISABLE_FAN_CTL2     	0x34a3	//A complete strike. no command.
#define I8K_SMM_IO_ENABLE_FAN_CTL2      	0x35a3
//0x36a3  get hotkey scancode list (args see diags)
#define I8K_SMM_GET_DOCK_STATE				0x40a3
#define I8K_SMM_GET_DELL_SIG1				0xfea3
#define I8K_SMM_GET_DELL_SIG2				0xffa3
#define I8K_SMM_BIOS_VERSION				0x00a6  // not confirmed

// GET_TEMP_TYPE result codes
#define I8K_SMM_TEMP_CPU      0
#define I8K_SMM_TEMP_GPU      1
#define I8K_SMM_TEMP_MEMORY   2
#define I8K_SMM_TEMP_MISC     3
#define I8K_SMM_TEMP_AMBIENT  4
#define I8K_SMM_TEMP_OTHER    5

#define I8K_FAN_MULT            30
#define I8K_MAX_TEMP            127

#define I8K_FAN_PROCESSOR       0
#define I8K_FAN_SYSTEM          1
#define I8K_FAN_GPU             2
#define I8K_FAN_PSU             3
#define I8K_FAN_CHIPSET         4

#define I8K_FAN_OFF             0
#define I8K_FAN_LOW             1
#define I8K_FAN_HIGH            2
#define I8K_FAN_MAX             I8K_FAN_HIGH

#define I8K_POWER_AC            0x05
#define I8K_POWER_BATTERY       0x01
#define I8K_AC                  1
#define I8K_BATTERY             0

#define I8K_FN_NONE				0x00
#define I8K_FN_UP		  		0x01
#define I8K_FN_DOWN				0x02
#define I8K_FN_MUTE				0x04
#define I8K_FN_MASK				0x07
#define I8K_FN_SHIFT			8

struct PACKED SMBIOS_PKG {
	uint32_t cmd;    // = cmd
	uint32_t data;   // = data
	uint32_t stat1;  // = 0
	uint32_t stat2;  // = 0
};

struct StoredSmcUpdate {
	/**
	 *  SMC key
	 */
	SMC_KEY key {};

	/**
	 *  SMC key index (if applicable)
	 */
	size_t index {};
	
	/**
	 *  SMC data size
	 */
	uint32_t size {};

	/**
	 *  SMC data
	 */
	uint8_t data[SMC_MAX_LOG_SIZE] {};
};

class SMIMonitor : public OSObject
{
	OSDeclareDefaultStructors(SMIMonitor)

public:
	/**
	 *  Allocate battery manager instance, can only be called once
	 */
	static void createShared();

	/**
	 *  Obtain shared instance for later usage
	 *
	 *  @return battery manager shared instance
	 */
	static SMIMonitor *getShared() {
		return instance;
	}

	/**
	 *  Probe battery manager
	 *
	 *  @return true on sucess
	 */
	bool probe();

	/**
	 *  Start battery manager up
	 */
	void start();

	/**
	 *  Power-off handler
	 */
	void handlePowerOff();

	/**
	 *  Power-on handler
	 */
	void handlePowerOn();

	/**
	 *  Post request
	 */
	bool postSmcUpdate(SMC_KEY key, size_t index, const void *data, uint32_t dataSize, bool force_update = false);

	/**
	 *  Main refreshed battery state containing battery information
	 */
	SMIState state {};

	/**
	 *  Actual fan control status
	 */
	atomic_uint_fast16_t fansStatus = 0;

	/**
	 *  Actual fan count
	 */
	atomic_uint fanCount = 0;

	/**
	 *  Actual temperature sensors count
	 */
	atomic_uint tempCount = 0;

	/**
	 *  Fan multiplier
	 */
	atomic_int fanMult = 1;
	
	
	static atomic_bool busy;


private:
	/**
	 *  The only allowed battery manager instance
	 */
	static SMIMonitor *instance;

	/**
	 *  A lock to permit concurrent access
	 */
	IOLock *mainLock {nullptr};

	/**
	 *  A simple lock to permit concurrent access to queue
	 */
	IOSimpleLock *queueLock {nullptr};

	/**
	 *  A simple lock to disable preemption
	 */
	IOSimpleLock *preemptionLock {nullptr};

	/**
	 *  handle of thread used to poll SMI updates
	 */
	thread_call_t updateCall {nullptr};

	/**
	 *  variable-event, keeps thread initialization result (0 or error code)
	 */
	atomic_int initialized = -1;

	/**
	 *  Awake flag
	 */
	atomic_bool awake = true;
	
	/**
	 *  Ignore SMC updates flag
	 */
	atomic_bool ignore_new_smc_updates = false;
	
	/**
	 *  Stored events for writing to SMM (event queue)
	 */
	evector<StoredSmcUpdate&> storedSmcUpdates;

	/**
	 *  Smc updates may happen which have to be handled in thread binded to CPU 0
	 */
	static constexpr size_t MaxActiveSmcUpdates {40};
	
	/**
	 *  How many sensor updates should have access to sensors (be forced)
	 */
	atomic_int force_update_counter = 0;

private:

	/**
	 *  Bind working thread to CPU 0
	 */
	IOReturn bindCurrentThreadToCpu0();

	/**
	 *  Find available fanssensors
	 *
	 *  @return true on sucess
	 */
	bool findFanSensors();

	/**
	 *  Find available temperature sensors
	 *
	 *  @return true on sucess
	 */
	bool findTempSensors();

	/**
	 *  static Thread Entry method
	 *
	 *  @param param0 unused
	 *  @param param1 unused
	 */
	static void staticUpdateThreadEntry(thread_call_param_t param0, thread_call_param_t param1);

	/**
	 *  Update sensors values
	 */
	void updateSensorsLoop();

	/*
	 * Handle SMC updates in idle
	 */
	void handleSmcUpdatesInIdle(int idle_loop_count);

	/**
	 *  SMC update handlers
	 */
	void hanldeManualControlUpdate(size_t index, UInt8 *data);
	void hanldeManualTargetSpeedUpdate(size_t index, UInt8 *data);
	void handleManualForceFanControlUpdate(UInt8 *data);

	int  i8k_smm(SMBIOS_PKG *sc, bool force_access = false);
	bool i8k_get_dell_sig_aux(int fn);
	bool i8k_get_dell_signature();
	int  i8k_get_temp(int sensor, bool force_access = false);
	int  i8k_get_temp_type(int sensor);
	int  i8k_get_power_status();
	int  i8k_get_fan_speed(int fan, bool force_access = false);
	int  i8k_get_fan_status(int fan);
	int  i8k_get_fan_type(int fan);
	int  i8k_get_fan_nominal_speed(int fan, int speed);

	int  i8k_set_fan(int fan, int speed);
	int  i8k_set_fan_control_manual(int fan);
	int  i8k_set_fan_control_auto(int fan);
};

// Helper definitions
static constexpr SMC_KEY KeyF0Md = SMC_MAKE_IDENTIFIER('F',0,'M','d');
static constexpr SMC_KEY KeyF0Tg = SMC_MAKE_IDENTIFIER('F',0,'T','g');
static constexpr SMC_KEY KeyFS__ = SMC_MAKE_IDENTIFIER('F','S','!',' ');

#endif /* SMIMonitor_hpp */
