//   lowlevel.asm - low level hardware access for x64 systems
//
//   FanIO driver V2.3
//   Copyright(c) 2001-2007 Christian Diefer
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

.intel_syntax noprefix
.globl _dell_smm_lowlevel

// rdi:SMBIOS_PKG* cmd
_dell_smm_lowlevel:

  stat2 = -0x50
  stat1 = -0x48
  data  = -0x40
  cmd   = -0x38
  sptr  = -0x30

  push rbp
  mov  rbp, rsp
  push rcx
  push rdx
  push r8
  push r9
  push rbx
  sub  rsp, 0x60

  mov qword ptr [rbp+sptr],rdi  # sptr
  mov r8,rdi
  xor rax,rax
  mov eax,dword ptr [r8+0]
  mov dword ptr [rbp+cmd],eax  # cmd
  
  xor rbx,rbx
  mov ebx,dword ptr [r8+4]
  mov dword ptr [rbp+data],ebx  # data

  xor rcx,rcx
  mov ecx,dword ptr [r8+8]
  mov dword ptr [rbp+stat1],ecx  # stat1
  
  xor rdx,rdx
  mov edx,dword ptr [r8+12]
  mov dword ptr [rbp+stat2],edx  # stat2

  out 0xb2,al
  out 0x84,al

  cmp eax,dword ptr [rbp+cmd] # cmd
  jne cmdok
  cmp ebx,dword ptr [rbp+data] # data
  jne cmdok
  cmp ecx,dword ptr [rbp+stat1] # stat1
  jne cmdok
  cmp edx,dword ptr [rbp+stat2] # stat2
  je error

cmdok:
  mov r8,qword ptr[rbp+sptr] # sptr
  mov dword ptr [r8+0],eax   # cmd
  mov dword ptr [r8+4],ebx   # data
  mov dword ptr [r8+8],ecx   # stat1
  mov dword ptr [r8+12],edx  # stat2
  and eax,0xffff
  cmp eax,0xffff
  jne ok

error:
  xor rax,rax     // Error (rax=-1)
  dec rax
  jmp ende

ok:
  xor rax,rax     // Success (rax=0)

ende:
  add rsp, 0x60
  pop rbx
  pop r9
  pop r8
  pop rdx
  pop rcx
  pop rbp
  RET
