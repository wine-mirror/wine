/*
 * structure definitions for ACCELERATORS
 *
 * Copyright  Martin Ayotte, 1994
 *
 */

#include "windows.h"

typedef struct {
	WORD		wEvent;
	WORD		wIDval;
	BYTE		type;
	} ACCELENTRY, *LPACCELENTRY;

typedef struct {
	WORD		wCount;
	ACCELENTRY 	tbl[1];
	} ACCELHEADER, *LPACCELHEADER;

#define VIRTKEY_ACCEL	0x01
#define SHIFT_ACCEL	0x04
#define CONTROL_ACCEL	0x08
#define ALT_ACCEL       0x10
#define SYSTEM_ACCEL	0x80
