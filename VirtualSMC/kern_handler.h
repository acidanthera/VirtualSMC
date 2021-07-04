//
//  kern_handler.h
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_handler_h
#define kern_handler_h

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#if defined(__x86_64__)

/**
 *  Performs a transparent jmp to process_io_result
 *  See details in kern_handler.S
 */
void ioTrapHandler();

#endif
	
#ifdef __cplusplus
}
#endif

#endif /* kern_handler_h */
