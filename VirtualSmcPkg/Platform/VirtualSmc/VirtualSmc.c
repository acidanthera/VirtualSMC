//
//  VirtualSmc.c
//  VirtualSmc
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcRtcLib.h>

#include "VirtualSmc.h"

STATIC EFI_EVENT             mSmcIoArriveEvent;
STATIC VOID                  *mSmcIoArriveNotify;
STATIC EFI_EVENT             mAuthenticationKeyEraseEvent;

STATIC CONST UINT8           mVirtualSmcKeyCount = 6;
STATIC CONST UINT8           mAuthenticationKeyIndex = mVirtualSmcKeyCount - 1;
STATIC VIRTUALSMC_KEY_VALUE  mVirtualSmcKeyValue[mVirtualSmcKeyCount] = {
  { '#KEY', 'ui32', 4, SMC_KEY_ATTRIBUTE_READ, {0, 0, 0, mVirtualSmcKeyCount} },
  { 'RMde', 'char', 1, SMC_KEY_ATTRIBUTE_READ, {SMC_MODE_APPCODE} },
  //
  // Requested yet unused (battery inside, causes missing battery in UI).
  //
  //{ 'BBIN', 'flag', 1, SMC_KEY_ATTRIBUTE_READ, {} },
  { 'BRSC', 'ui16', 2, SMC_KEY_ATTRIBUTE_READ, {} },
  { 'MSLD', 'ui8 ', 1, SMC_KEY_ATTRIBUTE_READ, {} },
  { 'BATP', 'flag', 1, SMC_KEY_ATTRIBUTE_READ, {} },
  //
  // HBKP must always be the last key in the list (see mAuthenticationKeyIndex).
  //
  { 'HBKP', 'ch8*', 32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE, {} }
};

STATIC EFI_GUID mVirtualSmcStatusKeyGuid      = VIRTUALSMC_STATUS_KEY_GUID;
STATIC EFI_GUID mVirtualSmcEncryptionKeyGuid  = VIRTUALSMC_ENCRYPTION_KEY_GUID;

STATIC APPLE_SMC_IO_PROTOCOL mSmcIoProtocol = {
  APPLE_SMC_IO_PROTOCOL_REVISION,
  SmcIoVirtualSmcReadValue,
  SmcIoVirtualSmcWriteValue,
  SmcIoVirtualSmcGetKeyCount,
  SmcIoVirtualSmcMakeKey,
  SmcIoVirtualSmcGetKeyFromIndex,
  SmcIoVirtualSmcGetKeyInfo,
  SmcIoVirtualSmcReset,
  SmcIoVirtualSmcFlashType,
  SmcIoVirtualSmcUnsupported,
  SmcIoVirtualSmcFlashWrite,
  SmcIoVirtualSmcFlashAuth,
  0,
  SMC_PORT_BASE,
  FALSE,
  SmcIoVirtualSmcUnknown1,
  SmcIoVirtualSmcUnknown2,
  SmcIoVirtualSmcUnknown3,
  SmcIoVirtualSmcUnknown4,
  SmcIoVirtualSmcUnknown5
};

EFI_STATUS
EFIAPI
SmcIoVirtualSmcReadValue (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  OUT SMC_DATA               *Value
  )
{
  UINTN      Index;

  DEBUG ((DEBUG_INFO, "SmcReadValue Key %X Size %d\n", Key, Size));

  if (Value == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  for (Index = 0; Index < mVirtualSmcKeyCount; Index++) {
    if (mVirtualSmcKeyValue[Index].Key == Key) {
      if (mVirtualSmcKeyValue[Index].Size != Size) {
        return EFI_SMC_KEY_MISMATCH;
      }

      CopyMem (Value, mVirtualSmcKeyValue[Index].Data, Size);
      
      return EFI_SMC_SUCCESS;
    }
  }

  return EFI_SMC_NOT_FOUND;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcWriteValue (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  IN  SMC_DATA               *Value
  )
{
  UINTN       Index;

  DEBUG ((DEBUG_INFO, "SmcWriteValue Key %X Size %d\n", Key, Size));

  if (!Value || Size == 0) {
    return EFI_SMC_BAD_PARAMETER;
  }

  //
  // Handle HBKP separately to let boot.efi erase its contents as early as it wants.
  //
  if (Key == 'HBKP' && Size <= SMC_HBKP_SIZE) {
    ZeroMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, SMC_HBKP_SIZE);
    CopyMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, Value, Size);
    for (Index = 0; Index < SMC_HBKP_SIZE; Index++) {
      if (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data[Index] != 0) {
        DEBUG ((DEBUG_INFO, "Not updating key with non-zero data\n"));
        break;
      }
    }

    return EFI_SUCCESS;
  }
  return EFI_SMC_NOT_WRITABLE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcMakeKey (
  IN  CHAR8    *Name,
  OUT SMC_KEY  *Key
  )
{
  UINTN      Index;

  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcMakeKey\n"));

  if (Name != NULL && Key != NULL) {
    *Key  = 0;
    Index = 0;

    do {
      if (SMC_KEY_IS_VALID_CHAR (Name[Index])) {
        *Key <<= 8;
        *Key  |= Name[Index];
        ++Index;
      } else {
        *Key = 0;
        return EFI_SMC_BAD_PARAMETER;
      }
    } while (Index < sizeof (*Key) / sizeof (*Name));

    return EFI_SMC_SUCCESS;
  }

  return EFI_SMC_BAD_PARAMETER;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyCount (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  OUT UINT32                 *Count
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcGetKeyCount %d\n", mVirtualSmcKeyCount));

  if (Count == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  *Count = SwapBytes32 (mVirtualSmcKeyCount);

  return EFI_SMC_SUCCESS;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyFromIndex (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY_INDEX          Index,
  OUT SMC_KEY                *Key
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcGetKeyFromIndex\n"));

  if (Key == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  if (Index < mVirtualSmcKeyCount) {
    *Key = mVirtualSmcKeyValue[Index].Key;
    return EFI_SMC_SUCCESS;
  }

  return EFI_SMC_NOT_FOUND;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyInfo (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  OUT SMC_DATA_SIZE          *Size,
  OUT SMC_KEY_TYPE           *Type,
  OUT SMC_KEY_ATTRIBUTES     *Attributes
  )
{
  UINTN Index;

  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcGetKeyFromIndex %X\n", Key));

  if (Size == NULL || Type == NULL || Attributes == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  for (Index = 0; Index < mVirtualSmcKeyCount; Index++) {
    if (mVirtualSmcKeyValue[Index].Key == Key) {
      *Size = mVirtualSmcKeyValue[Index].Size;
      *Type = mVirtualSmcKeyValue[Index].Type;
      *Attributes = mVirtualSmcKeyValue[Index].Attributes;
      return EFI_SMC_SUCCESS;
    }
  }

  return EFI_SMC_NOT_FOUND;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcReset (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Mode
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcReset %X\n", Mode));

  return EFI_SMC_SUCCESS;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashType (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_TYPE         Type
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcFlashType %X\n", Type));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnsupported (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcUnsupported\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashWrite (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Unknown,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcFlashWrite %d\n", Size));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashAuth (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcFlashAuth %d\n", Size));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown1 (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcUnknown1\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown2 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcUnknown2\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown3 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcUnknown3\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown4 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcUnknown4\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown5 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT8                  *Data
  )
{
  DEBUG ((DEBUG_VERBOSE, "SmcIoVirtualSmcUnknown5\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
ReinstallSmcIoProtocolOnHandle (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS             Status;
  APPLE_SMC_IO_PROTOCOL  *OldSmcIo;

  Status = gBS->HandleProtocol (Handle, &gAppleSmcIoProtocolGuid, (VOID **)&OldSmcIo);
  if (Status == EFI_UNSUPPORTED) {
    Status = gBS->InstallProtocolInterface (&Handle, &gAppleSmcIoProtocolGuid, EFI_NATIVE_INTERFACE, &mSmcIoProtocol);
  } else if (Status == EFI_SUCCESS) {
    if (OldSmcIo->SmcReadValue != mSmcIoProtocol.SmcReadValue) {
      Status = gBS->ReinstallProtocolInterface (Handle, &gAppleSmcIoProtocolGuid, OldSmcIo, &mSmcIoProtocol);
      //FIXME: Do manual reinstallation here on error
    } else {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

STATIC
EFI_STATUS
ReinstallSmcIoProtocol (
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       Index;
  UINTN       NoHandles = 0;
  BOOLEAN     Reinstalled = FALSE;

  Status = gBS->LocateHandleBuffer (ByProtocol, &gAppleSmcIoProtocolGuid, NULL, &NoHandles, &Handles);
  
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = ReinstallSmcIoProtocolOnHandle (Handles[Index]);
    
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Cannot reinstall SmcIo at %d handle with %r\n", Index, Status));
      continue;
    }

    Reinstalled = TRUE;
  }

  gBS->FreePool (Handles);

  if (Reinstalled) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
EFIAPI
HandleNewSmcIoProtocol (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       Index;
  UINTN       NoHandles = 0;

  Status = gBS->LocateHandleBuffer (ByRegisterNotify, &gAppleSmcIoProtocolGuid, mSmcIoArriveNotify, &NoHandles, &Handles);
  
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = ReinstallSmcIoProtocolOnHandle (Handles[Index]);
    
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Cannot reinstall SmcIo from event at %d handle with %r\n", Index, Status));
    }
  }

  gBS->FreePool (Handles);
}

STATIC
VOID
EFIAPI
EraseAuthenticationKey (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  ZeroMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, SMC_HBKP_SIZE);
}

STATIC
BOOLEAN
ExtractAuthentificationKey (
  UINT8   *Buffer,
  UINTN   Size
  )
{
  UINTN           Index;
  AES_CONTEXT     Context;
  UINT8           EncryptKey[CONFIG_AES_KEY_SIZE];
  CONST UINT8     *InitVector;
  UINT8           *Payload;
  UINTN           PayloadSize;
  UINT32          RealSize;

  if (Size < sizeof (UINT32) + SMC_HBKP_SIZE) {
    DEBUG ((DEBUG_INFO, "Invalid key length - %d\n", (UINT32)Size));
    return FALSE;
  }

  if (Buffer[0] == 'V' && Buffer[1] == 'S' && Buffer[2] == 'P' && Buffer[3] == 'T') {
    //
    // Perform an as-is copy of stored contents.
    //
    CopyMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, Buffer + sizeof (UINT32), SMC_HBKP_SIZE);
  } else if (Buffer[0] == 'V' && Buffer[1] == 'S' && Buffer[2] == 'E' && Buffer[3] == 'N') {
    //
    // The magic is followed by an IV and at least one AES block containing at least SMC_HBKP_SIZE bytes.
    //
    if (Size < sizeof (UINT32) + AES_BLOCK_SIZE + SMC_HBKP_SIZE || (Size - sizeof (UINT32)) % AES_BLOCK_SIZE != 0) {
      DEBUG ((DEBUG_INFO, "Invalid encrypted key length - %d\n", (UINT32)Size));
      return FALSE;
    }

    //
    // Read and erase the temporary encryption key from CMOS memory.
    //
    for (Index = 0; Index < sizeof (EncryptKey); Index++) {
      EncryptKey[Index] = OcRtcRead (0xD0 + Index);
      OcRtcWrite (0xD0 + Index, 0);
    }

    //
    // Perform the decryption.
    //
    InitVector   = Buffer + sizeof (UINT32);
    Payload      = Buffer + sizeof (UINT32) + AES_BLOCK_SIZE;
    PayloadSize  = Size - (sizeof (UINT32) + AES_BLOCK_SIZE);
    AesInitCtxIv (&Context, EncryptKey, InitVector);
    AESCbcDecryptBuffer (&Context, Payload, PayloadSize);
    ZeroMem (&Context, sizeof (Context));
    ZeroMem (EncryptKey, sizeof (EncryptKey));
    RealSize = *(const UINT32 *)Payload;

    //
    // Ensure the size matches SMC_HBKP_SIZE.
    //
    if (RealSize != SMC_HBKP_SIZE) {
      DEBUG ((DEBUG_INFO, "Invalid decrypted key length - %d\n", RealSize));
      return FALSE;
    }

    //
    // Copy the decrypted contents.
    //
    CopyMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, Payload + sizeof (UINT32), SMC_HBKP_SIZE);
  } else {
    DEBUG ((DEBUG_INFO, "Invalid key magic - %02X %02X %02X %02X\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3]));
    return FALSE;
  }

  return TRUE;
}

STATIC
VOID
LoadAuthenticationKey (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer = NULL;
  UINT32      Attributes = 0;
  UINTN       Size = 0;

  //
  // Load encryption key contents.
  //
  Status = gRT->GetVariable (VIRTUALSMC_ENCRYPTION_KEY, &mVirtualSmcEncryptionKeyGuid, &Attributes, &Size, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocateZeroPool (Size);
    if (Buffer) {
      Status = gRT->GetVariable (VIRTUALSMC_ENCRYPTION_KEY, &mVirtualSmcEncryptionKeyGuid, &Attributes, &Size, Buffer);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "Layer key (%d, %X) obtain failure - %r\n", (UINT32)Size, Attributes, Status));
      }
    } else {
      DEBUG ((DEBUG_INFO, "Key buffer (%d) allocation failure - %r\n", (UINT32)Size, Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Initial key obtain failure - %r\n", Status));
  }

  //
  // Parse encryption contents if any.
  //
  if (!EFI_ERROR (Status)) {
    ExtractAuthentificationKey (Buffer, Size);
  }

  //
  // Nullify or (at least) remove existing vsmc-key variable.
  //
  if (Buffer) {
    ZeroMem (Buffer, Size);
    Status = gRT->SetVariable (VIRTUALSMC_ENCRYPTION_KEY, &mVirtualSmcEncryptionKeyGuid, Attributes, Size, Buffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Failed to zero key - %r\n", Status));
      Status = gRT->SetVariable (VIRTUALSMC_ENCRYPTION_KEY, &mVirtualSmcEncryptionKeyGuid, 0, 0, NULL);
    }
    gBS->FreePool (Buffer);
    Buffer = NULL;
  } else {
    Status = gRT->SetVariable (VIRTUALSMC_ENCRYPTION_KEY, &mVirtualSmcEncryptionKeyGuid, 0, 0, NULL);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to remove key - %r\n", Status));
  }

  //
  // Erase local HBKP key copy at exit boot services.
  //
  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, EraseAuthenticationKey, NULL, &mAuthenticationKeyEraseEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to create exit bs event for hbkp erase\n"));
  }
}

STATIC
VOID
ExportStatusKey (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      StatusBuffer[2];
  UINT8       *StatusBufferMagic;

  StatusBufferMagic = (UINT8 *) &StatusBuffer[0];

  //
  // Build the structure.
  //
  StatusBufferMagic[0] = 'V';
  StatusBufferMagic[1] = 'S';
  StatusBufferMagic[2] = 'M';
  StatusBufferMagic[3] = 'C';
  StatusBuffer[1] = 1;

  //
  // Set status key for kext frontend.
  //
  Status = gRT->SetVariable (VIRTUALSMC_STATUS_KEY, &mVirtualSmcStatusKeyGuid, 
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof (StatusBuffer), StatusBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to create status - %r\n", Status));
  }
}

EFI_STATUS
EFIAPI
VirtualSmcMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "VirtualSmc 1.0 loading...\n"));

  LoadAuthenticationKey ();
  ExportStatusKey ();

  //
  // Perform initial SmcIo protocol installation.
  //
  Status = ReinstallSmcIoProtocol ();
  
  if (EFI_ERROR (Status)) {
    Status = ReinstallSmcIoProtocolOnHandle (ImageHandle);
    
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Can neither install nor reinstall SmcIo with %r\n", Status));
      return EFI_LOAD_ERROR;
    }
  }

  //
  // Ensure all the other installed protocols are ours.
  // Clover's implementation is borked and insults nvram storage.
  //
  Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, HandleNewSmcIoProtocol, NULL, &mSmcIoArriveEvent);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SmcIo notify event creation failed %r\n", Status));
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (&gAppleSmcIoProtocolGuid, mSmcIoArriveEvent, &mSmcIoArriveNotify);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SmcIo notify event registration failed %r\n", Status));
    gBS->CloseEvent (mSmcIoArriveEvent);
    return Status;
  }

  //
  //FIXME: In Clover they also have 5301FE59-1770-4166-A169-00C4BDE4A162, which they call gAppleSMCStateProtocolGuid.
  // This protocol is actually someting like APPLE_EMBEDDED_OS_BOOT_PROTOCOL, and it is used to bootstrap touchbar OS.
  // This protocol is not available anywhere but MBPs with touchbars, we should remove it before it starts causing harm.
  //

  return EFI_SUCCESS;
}
