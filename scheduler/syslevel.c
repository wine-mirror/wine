/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 */

#include <unistd.h>
#include "syslevel.h"
#include "heap.h"
#include "stackframe.h"
#include "debug.h"

static CRITICAL_SECTION Win16Mutex;
static SEGPTR segpWin16Mutex;

/* Global variable to save current TEB while in 16-bit code */
WORD SYSLEVEL_Win16CurrentTeb = 0;

/* TEB of initial process for emergency use */
WORD SYSLEVEL_EmergencyTeb = 0;


/************************************************************************
 *           SYSLEVEL_Init
 */
void SYSLEVEL_Init(void)
{
    CRITICAL_SECTION **w16Mutex = SEGPTR_ALLOC(sizeof(CRITICAL_SECTION *));

    *w16Mutex = &Win16Mutex;
    segpWin16Mutex = SEGPTR_GET(w16Mutex);

    InitializeCriticalSection(&Win16Mutex);
}

/************************************************************************
 *           GetpWin16Lock32    (KERNEL32.93)
 */
VOID WINAPI GetpWin16Lock32(CRITICAL_SECTION **lock)
{
    *lock = &Win16Mutex;
}

/************************************************************************
 *           GetpWin16Lock16    (KERNEL.449)
 */
SEGPTR WINAPI GetpWin16Lock16(void) 
{ 
    return segpWin16Mutex;
}

/************************************************************************
 *           _EnterSysLevel    (KERNEL32.97)
 */
VOID WINAPI _EnterSysLevel(CRITICAL_SECTION *lock)
{
    EnterCriticalSection(lock);

    if (lock == &Win16Mutex)
        GET_FS( SYSLEVEL_Win16CurrentTeb );
}

/************************************************************************
 *           _LeaveSysLevel    (KERNEL32.98)
 */
VOID WINAPI _LeaveSysLevel(CRITICAL_SECTION *lock)
{
    LeaveCriticalSection(lock);
}

/************************************************************************
 *           (KERNEL32.86)
 */
VOID WINAPI _KERNEL32_86(CRITICAL_SECTION *lock)
{
    _LeaveSysLevel(lock);
}

/************************************************************************
 *           SYSLEVEL_EnterWin16Lock			[KERNEL.480]
 */
VOID WINAPI SYSLEVEL_EnterWin16Lock(VOID)
{
    TRACE(win32, "thread %04x (pid %d) about to enter\n", 
          THREAD_Current()->teb_sel, getpid());

    _EnterSysLevel(&Win16Mutex);

    TRACE(win32, "thread %04x (pid %d) entered, count is %ld\n",
          THREAD_Current()->teb_sel, getpid(), Win16Mutex.RecursionCount);
}

/************************************************************************
 *           SYSLEVEL_LeaveWin16Lock		[KERNEL.481]
 */
VOID WINAPI SYSLEVEL_LeaveWin16Lock(VOID)
{
    TRACE(win32, "thread %04x (pid %d) about to leave, count is %ld\n", 
          THREAD_Current()->teb_sel, getpid(), Win16Mutex.RecursionCount);

    _LeaveSysLevel(&Win16Mutex);
}

/************************************************************************
 *           _CheckNotSysLevel    (KERNEL32.94)
 */
VOID WINAPI _CheckNotSysLevel(CRITICAL_SECTION *lock)
{
    FIXME(win32, "()\n");
}

/************************************************************************
 *           _ConfirmSysLevel    (KERNEL32.95)
 */
VOID WINAPI _ConfirmSysLevel(CRITICAL_SECTION *lock)
{
    FIXME(win32, "()\n");
}

/************************************************************************
 *           _ConfirmWin16Lock    (KERNEL32.96)
 */
DWORD WINAPI _ConfirmWin16Lock(void)
{
    FIXME(win32, "()\n");
    return 1;
}

/************************************************************************
 *           ReleaseThunkLock    (KERNEL32.48)
 */
VOID WINAPI ReleaseThunkLock(DWORD *mutex_count)
{
    DWORD count = Win16Mutex.RecursionCount;
    *mutex_count = count;

    while (count-- > 0)
        _LeaveSysLevel(&Win16Mutex);
}

/************************************************************************
 *           RestoreThunkLock    (KERNEL32.49)
 */
VOID WINAPI RestoreThunkLock(DWORD mutex_count)
{
    while (mutex_count-- > 0)
        _EnterSysLevel(&Win16Mutex);
}

/************************************************************************
 *           SYSLEVEL_ReleaseWin16Lock
 */
VOID SYSLEVEL_ReleaseWin16Lock(VOID)
{
    DWORD count;

    TRACE(win32, "thread %04x (pid %d) about to release, count is %ld\n", 
          THREAD_Current()->teb_sel, getpid(), Win16Mutex.RecursionCount);

    ReleaseThunkLock(&count);

    if (count > 0xffff)
        ERR(win32, "Win16Mutex recursion count too large!\n");

    CURRENT_STACK16->mutex_count = (WORD)count;
}

/************************************************************************
 *           SYSLEVEL_RestoreWin16Lock
 */
VOID SYSLEVEL_RestoreWin16Lock(VOID)
{
    DWORD count = CURRENT_STACK16->mutex_count;

    if (!count)
        ERR(win32, "Win16Mutex recursion count is zero!\n");

    TRACE(win32, "thread %04x (pid %d) about to restore (count %ld)\n", 
          THREAD_Current()->teb_sel, getpid(), count);

    RestoreThunkLock(count);

    TRACE(win32, "thread %04x (pid %d) restored lock, count is %ld\n", 
          THREAD_Current()->teb_sel, getpid(), Win16Mutex.RecursionCount);
}

