//
//  sens_cpu.hpp
//  SensorsCPU
//
//  Based on code by mercurysquad, superhai © 2008
//  Based on code from Open Hardware Monitor project by Michael Möller © 2011
//  Based on code by slice © 2013
//  Portions copyright © 2010 Natan Zalkin <natan.zalkin@me.com>.
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef sens_cpu_hpp
#define sens_cpu_hpp

#include <Library/LegacyIOService.h>

#include <Headers/kern_util.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_time.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#include <i386/proc_reg.h>
#pragma clang diagnostic pop

class EXPORT SMCProcessor : public IOService {
	OSDeclareDefaultStructors(SMCProcessor)

	/**
	 *  VirtualSMC service registration notifier
	 */
	IONotifier *vsmcNotifier {nullptr};

	/**
	 *  Registered plugin instance
	 */
	VirtualSMCAPI::Plugin vsmcPlugin {
		xStringify(PRODUCT_NAME),
		parseModuleVersion(xStringify(MODULE_VERSION)),
		VirtualSMCAPI::Version,
	};

	/**
	 *  MSRs missing from i386/proc_reg.h
	 */
	static constexpr uint32_t MSR_TEMPERATURE_TARGET = 0x1A2;
	static constexpr uint32_t MSR_PERF_STATUS = 0x198;
	static constexpr uint32_t MSR_IA32_THERM_STATUS = 0x19C;
	static constexpr uint32_t MSR_RAPL_POWER_UNIT = 0x606;
	static constexpr uint32_t MSR_PKG_ENERGY_STATUS = 0x611;
	static constexpr uint32_t MSR_DRAM_ENERGY_STATUS = 0x619;
	static constexpr uint32_t MSR_PP0_ENERGY_STATUS = 0x639;
	static constexpr uint32_t MSR_PP1_ENERGY_STATUS = 0x641;

	/**
	 *  Key name index mapping
	 */
	static constexpr size_t MaxIndexCount = sizeof("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") - 1;
	static constexpr const char *KeyIndexes = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	/**
	 *  Supported SMC keys
	 */
	static constexpr SMC_KEY KeyPC0C = SMC_MAKE_IDENTIFIER('P','C','0','C');
	static constexpr SMC_KEY KeyPC0G = SMC_MAKE_IDENTIFIER('P','C','0','G');
	static constexpr SMC_KEY KeyPC0R = SMC_MAKE_IDENTIFIER('P','C','0','R');
	static constexpr SMC_KEY KeyPC3C = SMC_MAKE_IDENTIFIER('P','C','3','C');
	static constexpr SMC_KEY KeyPCAC = SMC_MAKE_IDENTIFIER('P','C','A','C');
	static constexpr SMC_KEY KeyPCAM = SMC_MAKE_IDENTIFIER('P','C','A','M');
	static constexpr SMC_KEY KeyPCEC = SMC_MAKE_IDENTIFIER('P','C','E','C');
	static constexpr SMC_KEY KeyPCGC = SMC_MAKE_IDENTIFIER('P','C','G','C');
	static constexpr SMC_KEY KeyPCGM = SMC_MAKE_IDENTIFIER('P','C','G','M');
	static constexpr SMC_KEY KeyPCPC = SMC_MAKE_IDENTIFIER('P','C','P','C');
	static constexpr SMC_KEY KeyPCPG = SMC_MAKE_IDENTIFIER('P','C','P','G');
	static constexpr SMC_KEY KeyPCPR = SMC_MAKE_IDENTIFIER('P','C','P','R');
	static constexpr SMC_KEY KeyPCPT = SMC_MAKE_IDENTIFIER('P','C','P','T');
	static constexpr SMC_KEY KeyPCTR = SMC_MAKE_IDENTIFIER('P','C','T','R');
	static constexpr SMC_KEY KeyTC0C(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'C'); }
	static constexpr SMC_KEY KeyTC0c(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'c'); }
	static constexpr SMC_KEY KeyTC0D(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'D'); }
	static constexpr SMC_KEY KeyTC0E(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'E'); }
	static constexpr SMC_KEY KeyTC0F(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'F'); }
	static constexpr SMC_KEY KeyTC0G(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'G'); }
	static constexpr SMC_KEY KeyTC0J(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'J'); }
	static constexpr SMC_KEY KeyTC0H(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'H'); }
	static constexpr SMC_KEY KeyTC0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTC0p(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'p'); }
	static constexpr SMC_KEY KeyVC0C(size_t i) { return SMC_MAKE_IDENTIFIER('V','C',KeyIndexes[i],'C'); }

	/**
	 *  CPU sensor counter info
	 */
	struct Counters {
		/**
		 *  Possible values for event flags
		 */
		enum EventFlags {
			ThermalCore              = 1U << 0U,
			ThermalPackage           = 1U << 1U,
			PowerTotal               = 1U << 2U,
			PowerCores               = 1U << 3U,
			PowerUncore              = 1U << 4U,
			PowerDram                = 1U << 5U,
			PowerAny                 = PowerTotal | PowerCores | PowerUncore | PowerDram,
			Voltage                  = 1U << 6U
		};

		/**
		 *  Energy indexes for enumeration
		 */
		enum EnergyIdx {
			EnergyTotalIdx,
			EnergyCoresIdx,
			EnergyUncoreIdx,
			EnergyDramIdx,
			EnergyTotal
		};

		uint16_t eventFlags {};

		/**
		 *  For ThermalCore
		 */
		uint8_t thermalStatus[CPUInfo::MaxCpus] {};

		/**
		 *  For ThermalPackage
		 */
		uint8_t thermalStatusPackage[CPUInfo::MaxCpus] {};

		/**
		 *  Maximum temperature per package before trottling according to DTS
		 */
		uint8_t tjmax[CPUInfo::MaxCpus] {};

		/**
		 *  Units read from RAPL per CPU package
		 */
		float energyUnits[CPUInfo::MaxCpus] {};

		/**
		 *  For PowerTotal, PowerCores, PowerUncore, PowerDram
		 */
		uint64_t energyBefore[CPUInfo::MaxCpus][EnergyTotal] {};
		uint64_t energyAfter[CPUInfo::MaxCpus][EnergyTotal] {};
		float power[CPUInfo::MaxCpus][EnergyTotal] {};

		constexpr static uint16_t energyMsrs(size_t i) {
			uint16_t msrs[EnergyTotal] {
				MSR_PKG_ENERGY_STATUS,
				MSR_PP0_ENERGY_STATUS,
				MSR_PP1_ENERGY_STATUS,
				MSR_DRAM_ENERGY_STATUS
			};
			return msrs[i];
		}

		constexpr static uint16_t energyFlags(size_t i) {
			uint16_t flags[EnergyTotal] {
				PowerTotal,
				PowerCores,
				PowerUncore,
				PowerDram
			};
			return flags[i];
		}

		/**
		 *  CPU 12V voltage
		 */
		float voltage[CPUInfo::MaxCpus] {};
	};

	/**
	 *  Workloop used to poll counters updates on timer basis
	 */
	IOWorkLoop *workloop {nullptr};

	/**
	 *  Workloop timer event sources for refresh scheduling
	 */
	IOTimerEventSource *timerEventSource {nullptr};

	/**
	 *  Default event timer timeout
	 */
	static constexpr uint32_t TimerTimeoutMs {500};

	/**
	 *  Maximum delta between timer triggers for a reschedule
	 */
	static constexpr uint64_t MaxDeltaForRescheduleNs {convertScToNs(10)};

	/**
	 *  Minimum delta between energy calculation count
	 */
	static constexpr uint64_t MinDeltaForRescheduleNs {convertMsToNs(100)};

	/**
	 *  Last timer trigger timestamp in nanoseconds
	 */
	uint64_t timerEventLastTime {0};

	/**
	 *  Last energy calculation timestamp in nanoseconds
	 */
	uint64_t timerEnergyLastTime {0};

	/**
	 *  Read CPU generation
	 */
	CPUInfo::CpuGeneration cpuGeneration {CPUInfo::CpuGeneration::Unknown};

	/**
	 *  CPU model info
	 */
	uint32_t cpuFamily {0}, cpuModel {0}, cpuStepping {0};
		
	/**
	 *  Array of thread' handles used to update cpu specific counters
	 */
	thread_call_t threadHandles[CPUInfo::MaxCpus] {};
	
	/**
	 *  Helper function to bind current thread to specified CPU
	 */
	bool bindCurrentThreadToCpu(uint32_t cpu);
	
	/**
	 *  static Thread Entry method
	 *
	 *  @param param0 unused
	 *  @param param1 unused
	 */
	static void staticThreadEntry(thread_call_param_t param0, thread_call_param_t param1);

	/**
	 *  Timer scheduling status
	 */
	bool timerEventScheduled {false};

	/**
	 *  Read MSR safely as rdmsr64 does not check for GPF
	 *
	 *  @param msr    MSR register number
	 *  @param value  value read
	 *
	 *  @return true on success
	 */
	static bool readMsr(uint32_t msr, uint64_t &value) {
		uint32_t lo = 0, hi = 0;
		int err = rdmsr_carefully(msr, &lo, &hi);
		value = (static_cast<uint64_t>(hi) << 32U) | static_cast<uint64_t>(lo);
		return err == 0;
	}

	/**
	 *  Read die temperature callback
	 */
	void readTjmax();

	/**
	 *  Read running average power limit power units callback
	 */
	void readRapl();

	/**
	 *  Refresh counter values callback
	 */
	void updateCounters(uint32_t cpu);

	/**
	 *  Refresh sensor state on timer basis
	 */
	void timerCallback();

	/**
	 *  Setup SMC keys based on model and generation
	 *
	 *  @param coreOffset  Index of SMC key for the first core
	 */
	void setupKeys(size_t coreOffset);

public:
	/**
	 *  CPU sensor counters refreshed on timer basis
	 */
	Counters counters {};

	/**
	 *  CPU topology
	 */
	CPUInfo::CpuTopology cpuTopology {};

	/**
	 *  CPU sensor counter access lock
	 */
	IOSimpleLock *counterLock {nullptr};

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
	 *  Do a quick refresh reschedule if necessary
	 */
	void quickReschedule();

	/**
	 *  Submit the keys to received VirtualSMC service.
	 *
	 *  @param sensors   SensorsCPU service
	 *  @param refCon    reference
	 *  @param vsmc      VirtualSMC service
	 *  @param notifier  created notifier
	 */
	static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
};

#endif /* sens_cpu_hpp */
