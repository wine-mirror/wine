/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 */

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "syslevel.h"
#include "heap.h"
#include "stackframe.h"
#include "debug.h"

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
 *           GetpWin16Lock32    (KERNEL32.93)
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
    MakeCriticalSectionGlobal( &lock->crst );
    lock->level = level;

    TRACE( win32, "(%p, %d): handle is %d\n", 
                  lock, level, lock->crst.LockSemaphore );
}

/************************************************************************
 *           _EnterSysLevel    (KERNEL32.97) (KERNEL.439)
 */
VOID WINAPI _EnterSysLevel(SYSLEVEL *lock)
{
    THDB *thdb = THREAD_Current();
    int i;

    TRACE( win32, "(%p, level %d): thread %p (fs %04x, pid %d) count before %ld\n", 
                  lock, lock->level, thdb->server_tid, thdb->teb_sel, getpid(),
                  thdb->sys_count[lock->level] );

    for ( i = 3; i > lock->level; i-- )
        if ( thdb->sys_count[i] > 0 )
        {
            ERR( win32, "(%p, level %d): Holding %p, level %d. Expect deadlock!\n", 
                        lock, lock->level, thdb->sys_mutex[i], i );
        }

    EnterCriticalSection( &lock->crst );

    thdb->sys_count[lock->level]++;
    thdb->sys_mutex[lock->level] = lock;

    TRACE( win32, "(%p, level %d): thread %p (fs %04x, pid %d) count after  %ld\n",
                  lock, lock->level, thdb->server_tid, thdb->teb_sel, getpid(), 
                  thdb->sys_count[lock->level] );

    if (lock == &Win16Mutex)
        GET_FS( SYSLEVEL_Win16CurrentTeb );
}

/************************************************************************
 *           _LeaveSysLevel    (KERNEL32.98) (KERNEL.440)
 */
VOID WINAPI _LeaveSysLevel(SYSLEVEL *lock)
{
    THDB *thdb = THREAD_Current();

    TRACE( win32, "(%p, level %d): thread %p (fs %04x, pid %d) count before %ld\n", 
                  lock, lock->level, thdb->server_tid, thdb->teb_sel, getpid(),
                  thdb->sys_count[lock->level] );

    if ( thdb->sys_count[lock->level] <= 0 || thdb->sys_mutex[lock->level] != lock )
    {
        ERR( win32, "(%p, level %d): Invalid state: count %ld mutex %p.\n",
                    lock, lock->level, thdb->sys_count[lock->level], 
                    thdb->sys_mutex[lock->level] );
    }
    else
    {
        if ( --thdb->sys_count[lock->level] == 0 )
            thdb->sys_mutex[lock->level] = NULL;
    }

    LeaveCriticalSection( &lock->crst );

    TRACE( win32, "(%p, level %d): thread %p (fs %04x, pid %d) count after  %ld\n",
                  lock, lock->level, thdb->server_tid, thdb->teb_sel, getpid(), 
                  thdb->sys_count[lock->level] );
}

/************************************************************************
 *           (KERNEL32.86)
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
    FIXME(win32, "(%p)\n", lock);
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
    DWORD count;

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

    RestoreThunkLock(count);
}

/************************************************************************
 *           SYSLEVEL_CheckNotLevel
 */
VOID SYSLEVEL_CheckNotLevel( INT level )
{
    THDB *thdb = THREAD_Current();
    INT i;

    for ( i = 3; i >= level; i-- )
        if ( thdb->sys_count[i] > 0 )
        {
            ERR( win32, "(%d): Holding lock of level %d!\n", 
                       level, i );

            kill( getpid(), SIGHUP );
            break;
        }
}

