/*
 * structure definitions for ACCELERATORS
 *
 * taken straight from Win32 SDK includes
 */

#ifndef __WINE_ACCEL_H
#define __WINE_ACCEL_H

#include "windows.h"

#pragma pack(1)

#define	FVIRTKEY	TRUE          /* Assumed to be == TRUE */
#define	FNOINVERT	0x02
#define	FSHIFT		0x04
#define	FCONTROL	0x08
#define	FALT		0x10

typedef struct tagACCEL16 {
	BYTE	fVirt;		/* Also called the flags field */
	WORD	key;
	WORD	cmd;
} ACCEL16, *LPACCEL16;

typedef struct tagACCEL32 {
	BYTE	fVirt;		/* Also called the flags field */
	BYTE	pad0;
	WORD	key;
	WORD	cmd;
	WORD	pad1;
} ACCEL32, *LPACCEL32;

#pragma pack(4)

#endif  /* __WINE_ACCEL_H */
