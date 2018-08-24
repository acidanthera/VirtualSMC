//
//  VirtualSmc.h
//  VirtualSmc
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef VIRTUAL_SMC_H
#define VIRTUAL_SMC_H

#include <Guid/LiluVariables.h>
#include <Protocol/AppleSmcIo.h>

typedef struct {
	SMC_KEY             Key;
	SMC_KEY_TYPE        Type;
	SMC_DATA_SIZE       Size;
	SMC_KEY_ATTRIBUTES  Attributes;
	SMC_DATA            Data[SMC_MAX_DATA_SIZE];
} VIRTUALSMC_KEY_VALUE;

#define VIRTUALSMC_STATUS_KEY           L"vsmc-status"
#define VIRTUALSMC_STATUS_KEY_GUID      LILU_READ_ONLY_VARIABLE_GUID

#define VIRTUALSMC_ENCRYPTION_KEY       L"vsmc-key"
#define VIRTUALSMC_ENCRYPTION_KEY_GUID  LILU_WRITE_ONLY_VARIABLE_GUID


EFI_STATUS
EFIAPI
SmcIoVirtualSmcReadValue (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  OUT SMC_DATA               *Value
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcWriteValue (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  IN  SMC_DATA               *Value
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcMakeKey (
  IN  CHAR8    *Name,
  OUT SMC_KEY  *Key
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyCount (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  OUT UINT32                 *Count
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyFromIndex (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY_INDEX          Index,
  OUT SMC_KEY                *Key
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyInfo (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  OUT SMC_DATA_SIZE          *Size,
  OUT SMC_KEY_TYPE           *Type,
  OUT SMC_KEY_ATTRIBUTES     *Attributes
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcReset (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Mode
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashType (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_TYPE         Type
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnsupported (
  VOID
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashWrite (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Unknown,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashAuth (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown1 (
  VOID
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown2 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown3 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown4 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1
  );

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown5 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1
  );

#endif