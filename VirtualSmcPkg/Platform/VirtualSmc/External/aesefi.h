#ifndef _AES_EFI_H_
#define _AES_EFI_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

typedef UINT8   uint8_t;
typedef UINT16  uint16_t;
typedef UINT32  uint32_t;
typedef UINTN   size_t;
typedef UINTN   uintptr_t;

inline void *
memcpy(void * __restrict s1, const void * __restrict s2, size_t n)
{
  return CopyMem(s1, s2, n);
}

#define CBC 1
#define ECB 0
#define CTR 0

#endif //_AES_EFI_H_
