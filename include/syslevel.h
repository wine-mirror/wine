/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_SYSLEVEL_H
#define __WINE_SYSLEVEL_H

#include "wintypes.h"
#include "winbase.h"

extern WORD SYSLEVEL_Win16CurrentTeb;
extern WORD SYSLEVEL_EmergencyTeb;

void SYSLEVEL_Init(void);
VOID SYSLEVEL_EnterWin16Lock(VOID);
VOID SYSLEVEL_LeaveWin16Lock(VOID);
VOID SYSLEVEL_ReleaseWin16Lock(VOID);
VOID SYSLEVEL_RestoreWin16Lock(VOID);

VOID WINAPI GetpWin16Lock32(CRITICAL_SECTION **lock);
SEGPTR WINAPI GetpWin16Lock16(void);

VOID WINAPI _EnterSysLevel(CRITICAL_SECTION *lock);
VOID WINAPI _LeaveSysLevel(CRITICAL_SECTION *lock);

VOID WINAPI ReleaseThunkLock(DWORD *mutex_count);
VOID WINAPI RestoreThunkLock(DWORD mutex_count);

#endif  /* __WINE_SYSLEVEL_H */
