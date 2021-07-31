//
//  kern_handler.S
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#if defined(__x86_64__)

// This routine shims the control to a C function after being jumped
// right after performing SMC I/O from kernelspace.
// It is assumed that the source index is pushed on stack.
// It is assumed process_io_result has the following prototype:
// mach_vm_address_t process_io_result(size_t index);
// Where index is the passed index, and the return address is the
// address we need to jump to once we finish restoring the registers.
//         _______               _______                 _______
// -0x50  |  idx  |             |  idx  |               |  sub  |
// -0x48  |       |             | flags |               | flags |
// -0x40  |       |             |  rax  |               |  rax  |
// -0x38  |       |             |  rdi  |               |  rdi  |
// -0x30  |       |  pre call   |  rsi  |  after call   |  rsi  |
// -0x28  |       | ----------> |  rdx  | ------------> |  rdx  |
// -0x20  |       |             |  rcx  |               |  rcx  |
// -0x18  |       |             |  r8   |               |  r8   |
// -0x10  |       |             |  r9   |               |  r9   |
// -0x08  |       |             |  r10  |               |  r10  |
// -0x0   |       |             |  r11  |               |  r11  |
//        '-------'             '-------'               '-------'
//
.globl _ioTrapHandler
_ioTrapHandler:
// 1. Save the registers, the following assumptions are made:
// - xmm/ymm registers are unused (kexts do not use it atm)
// - hardware fpu is unused (kexts/kernel do not use it atm)
pushfq
push %rax
push %rdi
push %rsi
push %rdx
push %rcx
push %r8
push %r9
push %r10
push %r11
// 2. Get the index
mov 0x50(%rsp), %rdi
// 3. Align the stack and call the procedure.
// This is required, because even though the stack will always be at least 8-byte aligned,
// we may need 16-byte alignment for SSE (e.g. CoreCrypto uses it).
mov %rsp, %rax
push %rsp
push (%rsp)
andq $ -0x10, %rsp
call __ZN18VirtualSMCProvider15ioProcessResultEh
mov 0x8(%rsp), %rsp
// 4. Save the return address
mov %rax, 0x50(%rsp)
// 5. Restore the registers and transfer the control back
pop %r11
pop %r10
pop %r9
pop %r8
pop %rcx
pop %rdx
pop %rsi
pop %rdi
pop %rax
popfq
ret

#endif
