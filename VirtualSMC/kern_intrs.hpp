//
//  kern_intrs.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_intrs_hpp
#define kern_intrs_hpp

#include <Headers/kern_util.hpp>

#include <VirtualSMCSDK/AppleSmcBridge.hpp>

struct RegisteredInterrupt {
	/**
	 *  Interrupt source (vector)
	 */
	int source {0};

	/**
	 *  Interrupt target instance for handler
	 */
	OSObject *target {nullptr};

	/**
	 *  Registered interrupt handler
	 */
	IOInterruptAction handler {nullptr};

	/**
	 *  Reference constant for the handler
	 */
	void *refCon {nullptr};

	/**
	 *  Interrupts enabled status
	 */
	bool enabled {false};
};

struct StoredInterrupt {
	/**
	 *  SMC event code
	 */
	SMC_EVENT_CODE code {};

	/**
	 *  SMC interrupt data size
	 */
	uint32_t size {};

	/**
	 *  SMC interrupt data
	 */
	uint8_t data[SMC_MAX_LOG_SIZE] {};
};

#endif /* kern_intrs_hpp */
