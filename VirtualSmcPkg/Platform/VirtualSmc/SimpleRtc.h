//
//  SimpleRtc.h
//  VirtualSmc
//
//  Copyright © 2017 vit9696. All rights reserved.
//

UINT8
SimpleRtcRead (
  IN  UINT8  Offset
  );

VOID
SimpleRtcWrite (
  IN UINT8 Offset,
  IN UINT8 Value
  );

UINT8
SimpleRtcReadIvy (
  IN  UINT8  Offset
  );

VOID
SimpleRtcWriteIvy (
  IN UINT8 Offset,
  IN UINT8 Value
  );