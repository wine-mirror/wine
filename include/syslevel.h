/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_SYSLEVEL_H
#define __WINE_SYSLEVEL_H

#include "windef.h"
#include "winbase.h"

typedef struct tagSYSLEVEL
{
    CRITICAL_SECTION crst;
    INT level;
} SYSLEVEL;

extern WORD SYSLEVEL_Win16CurrentTeb;
extern WORD SYSLEVEL_EmergencyTeb;

void SYSLEVEL_Init(void);
VOID WINAPI SYSLEVEL_EnterWin16Lock(VOID);
VOID WINAPI SYSLEVEL_LeaveWin16Lock(VOID);
VOID SYSLEVEL_ReleaseWin16Lock(VOID);
VOID SYSLEVEL_RestoreWin16Lock(VOID);
VOID SYSLEVEL_CheckNotLevel( INT level );

VOID WINAPI GetpWin16Lock(SYSLEVEL **lock);
SEGPTR WINAPI GetpWin16Lock16(void);
DWORD WINAPI _ConfirmWin16Lock(void);

VOID WINAPI _CreateSysLevel(SYSLEVEL *lock, INT level);
VOID WINAPI _EnterSysLevel(SYSLEVEL *lock);
VOID WINAPI _LeaveSysLevel(SYSLEVEL *lock);
DWORD WINAPI _ConfirmSysLevel(SYSLEVEL *lock);

VOID WINAPI ReleaseThunkLock(DWORD *mutex_count);
VOID WINAPI RestoreThunkLock(DWORD mutex_count);

#endif  /* __WINE_SYSLEVEL_H */
