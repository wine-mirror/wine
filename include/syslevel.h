/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_SYSLEVEL_H
#define __WINE_SYSLEVEL_H

#include "windef.h"
#include "winbase.h"

extern WORD SYSLEVEL_Win16CurrentTeb;
extern WORD SYSLEVEL_EmergencyTeb;

VOID SYSLEVEL_CheckNotLevel( INT level );

#endif  /* __WINE_SYSLEVEL_H */
