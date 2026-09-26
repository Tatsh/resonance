# IOP modules

Inventory of the IOP modules shipped with the game, in `freq/IOP/`. The game's own
module load table, reconstructed in `src/os/iop.cpp`, loads every IRX below at boot.

| File           | Date        | Internal name   | Origin |
| -------------- | ----------- | --------------- | ------ |
| `IOPRP23.IMG`  | Oct 8 2001  | IOPRP23         | Sony   |
| `LIBSD.IRX`    | Oct 8 2001  | `libsd`         | Sony   |
| `MCMAN.IRX`    | Oct 8 2001  | `mcman`         | Sony   |
| `MCSERV.IRX`   | Oct 8 2001  | `mcserv`        | Sony   |
| `MSIFRPC.IRX`  | Oct 8 2001  | `msifrpc`       | Sony   |
| `MTAPMAN.IRX`  | Oct 8 2001  | `mtapman`       | Sony   |
| `PADMAN.IRX`   | Oct 8 2001  | `padman`        | Sony   |
| `SDRDRV.IRX`   | Oct 8 2001  | `sdrdrv`        | Sony   |
| `SIO2MAN.IRX`  | Oct 8 2001  | `sio2man`       | Sony   |
| `USBD.IRX`     | Oct 8 2001  | `usbd`          | Sony   |
| `USBKEYBD.IRX` | Oct 12 2001 | `usbkeybd`      | Sony   |
| `EZMIDI.IRX`   | Oct 12 2001 | `ezmidi_driver` | Custom |

## Sony modules

Eleven of the twelve files are Sony's prebuilt SDK binaries. Licensed developers received
them as binaries and did not compile them. The evidence is the single SDK-drop date on ten
of them, the canonical SDK module names and behaviours, Sony-style diagnostics and version
tags (such as `PsIImsifrpc 2300` in `MSIFRPC.IRX`, which binds only Sony IOP libraries),
and, for the reboot image, the recorded
`Copyright 1999-2001 (C) Sony Computer Entertainment Inc.` string with a Sony build path
next to the standard kernel modules (`cdvdman`, `dmacman`, `intrman`, `ioman`, `sifman`,
and others). `USBKEYBD.IRX` carries a later date but is the standard Sony USB keyboard
driver. No third-party vendor marks appear in any of them.

## Custom module

`EZMIDI.IRX` is the one custom module. Its name matches no Sony convention, it retains
debug symbol records, and its diagnostics are game-specific (`EzMIDI driver error`). It
imports Sony's SIF and RPC interfaces, so it was built against the SDK, and its later date
matches the game's own mastering. All markers point to Harmonix rather than Sony or
another vendor.
