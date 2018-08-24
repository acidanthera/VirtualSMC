//
//  SimpleRtc.c
//  VirtualSmc
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Library/IoLib.h>

//
// Available on all platforms, requires NMI bit handling.
//
#define R_PCH_RTC_INDEX           0x70
#define R_PCH_RTC_TARGET          0x71
#define R_PCH_RTC_EXT_INDEX       0x72
#define R_PCH_RTC_EXT_TARGET      0x73

//
// Available on Ivy Bridge and newer. Ignores NMI bit.
//
#define R_PCH_RTC_INDEX_ALT       0x74
#define R_PCH_RTC_TARGET_ALT      0x75
#define R_PCH_RTC_EXT_INDEX_ALT   0x76
#define R_PCH_RTC_EXT_TARGET_ALT  0x77

//
// RTC Memory bank size
//
#define RTC_BANK_SIZE             0x80

//
// RTC INDEX bit mask
//
#define RTC_DATA_MASK             0x7F
#define RTC_NMI_MASK              0x80

UINT8
SimpleRtcRead (
  IN  UINT8  Offset
  )
{
  UINT8  RtcIndexPort;
  UINT8  RtcDataPort;
  UINT8  RtcIndexNmi;

  if (Offset < RTC_BANK_SIZE) {
    RtcIndexPort  = R_PCH_RTC_INDEX;
    RtcDataPort   = R_PCH_RTC_TARGET;
  } else {
    RtcIndexPort  = R_PCH_RTC_EXT_INDEX;
    RtcDataPort   = R_PCH_RTC_EXT_TARGET;
  }

  RtcIndexNmi = IoRead8 (RtcIndexPort) & RTC_NMI_MASK;
  IoWrite8 (RtcIndexPort, (Offset & RTC_DATA_MASK) | RtcIndexNmi);
  return IoRead8 (RtcDataPort);
}

VOID
SimpleRtcWrite (
  IN UINT8 Offset,
  IN UINT8 Value
  )
{
  UINT8  RtcIndexPort;
  UINT8  RtcDataPort;
  UINT8  RtcIndexNmi;

  if (Offset < RTC_BANK_SIZE) {
    RtcIndexPort  = R_PCH_RTC_INDEX;
    RtcDataPort   = R_PCH_RTC_TARGET;
  } else {
    RtcIndexPort  = R_PCH_RTC_EXT_INDEX;
    RtcDataPort   = R_PCH_RTC_EXT_TARGET;
  }

  RtcIndexNmi = IoRead8 (RtcIndexPort) & RTC_NMI_MASK;
  IoWrite8 (RtcIndexPort, (Offset & RTC_DATA_MASK) | RtcIndexNmi);
  IoWrite8 (RtcDataPort, Value);
}

UINT8
SimpleRtcReadIvy (
  IN  UINT8  Offset
  )
{
  UINT8  RtcIndexPort;
  UINT8  RtcDataPort;

  //
  // CMOS access registers (using alternative access not to handle NMI bit)
  //
  if (Offset < RTC_BANK_SIZE) {
    RtcIndexPort  = R_PCH_RTC_INDEX_ALT;
    RtcDataPort   = R_PCH_RTC_TARGET_ALT;
  } else {
    RtcIndexPort  = R_PCH_RTC_EXT_INDEX_ALT;
    RtcDataPort   = R_PCH_RTC_EXT_TARGET_ALT;
  }

  IoWrite8 (RtcIndexPort, Offset & RTC_DATA_MASK);
  return IoRead8 (RtcDataPort);
}

VOID
SimpleRtcWriteIvy (
  IN UINT8 Offset,
  IN UINT8 Value
  )
{
  UINT8  RtcIndexPort;
  UINT8  RtcDataPort;

  //
  // CMOS access registers (using alternative access not to handle NMI bit)
  //
  if (Offset < RTC_BANK_SIZE) {
    RtcIndexPort  = R_PCH_RTC_INDEX_ALT;
    RtcDataPort   = R_PCH_RTC_TARGET_ALT;
  } else {
    RtcIndexPort  = R_PCH_RTC_EXT_INDEX_ALT;
    RtcDataPort   = R_PCH_RTC_EXT_TARGET_ALT;
  }

  IoWrite8 (RtcIndexPort, Offset & RTC_DATA_MASK);
  IoWrite8 (RtcDataPort, Value);
}
