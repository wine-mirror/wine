/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 */

#include <unistd.h>
#include <sys/types.h>
#include "syslevel.h"
#include "heap.h"
#include "selectors.h"
#include "stackframe.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32)

static SYSLEVEL Win16Mutex;
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
    SYSLEVEL **w16Mutex = SEGPTR_ALLOC(sizeof(SYSLEVEL *));

    *w16Mutex = &Win16Mutex;
    segpWin16Mutex = SEGPTR_GET(w16Mutex);

    _CreateSysLevel( &Win16Mutex, 1 );
}

/************************************************************************
 *           GetpWin16Lock    (KERNEL32.93)
 */
VOID WINAPI GetpWin16Lock(SYSLEVEL **lock)
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
 *           _CreateSysLevel    (KERNEL.438)
 */
VOID WINAPI _CreateSysLevel(SYSLEVEL *lock, INT level)
{
    InitializeCriticalSection( &lock->crst );
    lock->level = level;

    TRACE("(%p, %d): handle is %d\n", 
                  lock, level, lock->crst.LockSemaphore );
}

/************************************************************************
 *           _EnterSysLevel    (KERNEL32.97) (KERNEL.439)
 */
VOID WINAPI _EnterSysLevel(SYSLEVEL *lock)
{
    TEB *teb = NtCurrentTeb();
    int i;

    TRACE("(%p, level %d): thread %p (fs %04x, pid %ld) count before %ld\n", 
                  lock, lock->level, teb->tid, teb->teb_sel, (long) getpid(),
                  teb->sys_count[lock->level] );

    for ( i = 3; i > lock->level; i-- )
        if ( teb->sys_count[i] > 0 )
        {
            ERR("(%p, level %d): Holding %p, level %d. Expect deadlock!\n", 
                        lock, lock->level, teb->sys_mutex[i], i );
        }

    EnterCriticalSection( &lock->crst );

    teb->sys_count[lock->level]++;
    teb->sys_mutex[lock->level] = lock;

    TRACE("(%p, level %d): thread %p (fs %04x, pid %ld) count after  %ld\n",
                  lock, lock->level, teb->tid, teb->teb_sel, (long) getpid(), 
                  teb->sys_count[lock->level] );

    if (lock == &Win16Mutex)
        SYSLEVEL_Win16CurrentTeb = __get_fs();
}

/************************************************************************
 *           _LeaveSysLevel    (KERNEL32.98) (KERNEL.440)
 */
VOID WINAPI _LeaveSysLevel(SYSLEVEL *lock)
{
    TEB *teb = NtCurrentTeb();

    TRACE("(%p, level %d): thread %p (fs %04x, pid %ld) count before %ld\n", 
                  lock, lock->level, teb->tid, teb->teb_sel, (long) getpid(),
                  teb->sys_count[lock->level] );

    if ( teb->sys_count[lock->level] <= 0 || teb->sys_mutex[lock->level] != lock )
    {
        ERR("(%p, level %d): Invalid state: count %ld mutex %p.\n",
                    lock, lock->level, teb->sys_count[lock->level], 
                    teb->sys_mutex[lock->level] );
    }
    else
    {
        if ( --teb->sys_count[lock->level] == 0 )
            teb->sys_mutex[lock->level] = NULL;
    }

    LeaveCriticalSection( &lock->crst );

    TRACE("(%p, level %d): thread %p (fs %04x, pid %ld) count after  %ld\n",
                  lock, lock->level, teb->tid, teb->teb_sel, (long) getpid(), 
                  teb->sys_count[lock->level] );
}

/************************************************************************
 *		_KERNEL32_86 (KERNEL32.86)
 */
VOID WINAPI _KERNEL32_86(SYSLEVEL *lock)
{
    _LeaveSysLevel(lock);
}

/************************************************************************
 *           _ConfirmSysLevel    (KERNEL32.95) (KERNEL.436)
 */
DWORD WINAPI _ConfirmSysLevel(SYSLEVEL *lock)
{
    if ( lock && lock->crst.OwningThread == GetCurrentThreadId() )
        return lock->crst.RecursionCount;
    else
        return 0L;
}

/************************************************************************
 *           _CheckNotSysLevel    (KERNEL32.94) (KERNEL.437)
 */
VOID WINAPI _CheckNotSysLevel(SYSLEVEL *lock)
{
    FIXME("(%p)\n", lock);
}


/************************************************************************
 *           SYSLEVEL_EnterWin16Lock			[KERNEL.480]
 */
VOID WINAPI SYSLEVEL_EnterWin16Lock(VOID)
{
    _EnterSysLevel(&Win16Mutex);
}

/************************************************************************
 *           SYSLEVEL_LeaveWin16Lock		[KERNEL.481]
 */
VOID WINAPI SYSLEVEL_LeaveWin16Lock(VOID)
{
    _LeaveSysLevel(&Win16Mutex);
}
/************************************************************************
 *           _ConfirmWin16Lock    (KERNEL32.96)
 */
DWORD WINAPI _ConfirmWin16Lock(void)
{
    return _ConfirmSysLevel(&Win16Mutex);
}

/************************************************************************
 *           ReleaseThunkLock    (KERNEL32.48)
 */
VOID WINAPI ReleaseThunkLock(DWORD *mutex_count)
{
    DWORD count = _ConfirmSysLevel(&Win16Mutex);
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
    /* entry_point is never used again once the entry point has
       been called.  Thus we re-use it to hold the Win16Lock count */

    ReleaseThunkLock(&CURRENT_STACK16->entry_point);
}

/************************************************************************
 *           SYSLEVEL_RestoreWin16Lock
 */
VOID SYSLEVEL_RestoreWin16Lock(VOID)
{
    RestoreThunkLock(CURRENT_STACK16->entry_point);
}

/************************************************************************
 *           SYSLEVEL_CheckNotLevel
 */
VOID SYSLEVEL_CheckNotLevel( INT level )
{
    INT i;

    for ( i = 3; i >= level; i-- )
        if ( NtCurrentTeb()->sys_count[i] > 0 )
        {
            ERR("(%d): Holding lock of level %d!\n", 
                       level, i );
            DebugBreak();
            break;
        }
}

