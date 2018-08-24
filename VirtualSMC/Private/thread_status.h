/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

// The contents are taken from osfmk/mach/i386/thread_status.h

#ifndef priv_thread_status_h
#define priv_thread_status_h

#define x86_SAVED_STATE32		(THREAD_STATE_NONE + 1)
#define x86_SAVED_STATE64		(THREAD_STATE_NONE + 2)

/*
 * This is the state pushed onto the 64-bit interrupt stack
 * on any exception/trap/interrupt.
 */
struct x86_64_intr_stack_frame {
	uint16_t	trapno;
	uint16_t	cpu;
	uint32_t 	_pad;
	uint64_t	trapfn;
	uint64_t	err;
	uint64_t	rip;
	uint64_t	cs;
	uint64_t	rflags;
	uint64_t	rsp;
	uint64_t	ss;
};
static_assert((sizeof(struct x86_64_intr_stack_frame) % 16) == 0,
	"interrupt stack frame size must be a multiple of 16 bytes");

/*
 * thread state format for task running in 64bit long mode
 * in long mode, the same hardware frame is always pushed regardless
 * of whether there was a change in privilege level... therefore, there
 * is no need for an x86_saved_state64_from_kernel variant
 */

struct x86_saved_state64_108 {
	/*
	 * saved state organized to reflect the
	 * system call ABI register convention
	 * so that we can just pass a pointer
	 * to the saved state when calling through
	 * to the actual system call functions
	 * the ABI limits us to 6 args passed in
	 * registers... I've add v_arg6 - v_arg8
	 * to accomodate our most 'greedy' system
	 * calls (both BSD and MACH)... the individual
	 * system call handlers will fill these in
	 * via copyin if needed...
	 */
	uint64_t	rdi;		/* arg0 for system call */
	uint64_t	rsi;
	uint64_t	rdx;
	uint64_t	r10;
	uint64_t	r8;
	uint64_t	r9;		/* arg5 for system call */
	uint64_t	v_arg6;
	uint64_t	v_arg7;
	uint64_t	v_arg8;

	uint64_t	cr2;
	uint64_t	r15;
	uint64_t	r14;
	uint64_t	r13;
	uint64_t	r12;
	uint64_t	r11;
	uint64_t	rbp;
	uint64_t	rbx;
	uint64_t	rcx;
	uint64_t	rax;

	uint32_t	gs;
	uint32_t	fs;

	uint32_t	padend[3];

	struct	x86_64_intr_stack_frame	isf;
} __attribute__((packed, aligned(4)));

struct x86_saved_state64_109 {
	/*
	 * saved state organized to reflect the
	 * system call ABI register convention
	 * so that we can just pass a pointer
	 * to the saved state when calling through
	 * to the actual system call functions
	 * the ABI limits us to 6 args passed in
	 * registers... I've add v_arg6 - v_arg8
	 * to accomodate our most 'greedy' system
	 * calls (both BSD and MACH)... the individual
	 * system call handlers will fill these in
	 * via copyin if needed...
	 */
	uint32_t	padstart[3];
	uint64_t	rdi;		/* arg0 for system call */
	uint64_t	rsi;
	uint64_t	rdx;
	uint64_t	r10;		/* R10 := RCX prior to syscall trap */
	uint64_t	r8;
	uint64_t	r9;		/* arg5 for system call */
	uint64_t	v_arg6;
	uint64_t	v_arg7;
	uint64_t	v_arg8;

	uint64_t	cr2;
	uint64_t	r15;
	uint64_t	r14;
	uint64_t	r13;
	uint64_t	r12;
	uint64_t	r11;
	uint64_t	rbp;
	uint64_t	rbx;
	uint64_t	rcx;
	uint64_t	rax;

	uint32_t	gs;
	uint32_t	fs;

	struct	x86_64_intr_stack_frame	isf;
} __attribute__((packed, aligned(4)));

// Till 10.13 inclusive
struct x86_saved_state64_1010 {
	uint32_t	padstart[3];
	uint64_t	rdi;		/* arg0 for system call */
	uint64_t	rsi;
	uint64_t	rdx;
	uint64_t	r10;		/* R10 := RCX prior to syscall trap */
	uint64_t	r8;
	uint64_t	r9;		/* arg5 for system call */

	uint64_t	cr2;
	uint64_t	r15;
	uint64_t	r14;
	uint64_t	r13;
	uint64_t	r12;
	uint64_t	r11;
	uint64_t	rbp;
	uint64_t	rbx;
	uint64_t	rcx;
	uint64_t	rax;

	uint32_t	gs;
	uint32_t	fs;

	uint64_t 	padend;

	struct	x86_64_intr_stack_frame	isf;
} __attribute__((packed, aligned(4)));

typedef struct x86_saved_state64_108 x86_saved_state64_108_t;
typedef struct x86_saved_state64_109 x86_saved_state64_109_t;
typedef struct x86_saved_state64_1010 x86_saved_state64_1010_t;

/*
 * Tagged saved state for 10.8
 */
typedef struct {
	uint32_t flavor;
	x86_saved_state64_108_t ss_64;
} x86_saved_state_108_t;

/*
 * Tagged saved state for 10.9
 */
typedef struct {
	uint32_t flavor;
	x86_saved_state64_109_t ss_64;
} x86_saved_state_109_t;

/*
 * Tagged saved state for 10.10 and newer
 */
typedef struct {
	uint32_t flavor;
	x86_saved_state64_1010_t ss_64;
} x86_saved_state_1010_t;

#if defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 5 || __GNUC__ > 3)
// offsetoff may not be a constant expression so we rely on an extension here
static_assert(__builtin_offsetof(x86_saved_state_108_t, ss_64.rdi)  == 4,  "Invalid 10.8 state alignment");
static_assert(__builtin_offsetof(x86_saved_state_109_t, ss_64.rdi)  == 16, "Invalid 10.9 state alignment");
static_assert(__builtin_offsetof(x86_saved_state_1010_t, ss_64.rdi) == 16, "Invalid 10.10 state alignment");
static_assert(__builtin_offsetof(x86_saved_state_108_t, ss_64.isf.trapno)  == 176, "Invalid 10.8 state alignment");
static_assert(__builtin_offsetof(x86_saved_state_109_t, ss_64.isf.trapno)  == 176, "Invalid 10.9 state alignment");
static_assert(__builtin_offsetof(x86_saved_state_1010_t, ss_64.isf.trapno) == 160, "Invalid 10.10 state alignment");
#endif

#endif /* priv_thread_status_h */
