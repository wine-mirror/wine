/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(syslevel);

static SYSLEVEL Win16Mutex;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &Win16Mutex.crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": Win16Mutex") }
};
static SYSLEVEL Win16Mutex = { { &critsect_debug, -1, 0, 0, 0, 0 }, 1 };


/************************************************************************
 *           GetpWin16Lock    (KERNEL32.93)
 */
VOID WINAPI GetpWin16Lock(SYSLEVEL **lock)
{
    *lock = &Win16Mutex;
}

/************************************************************************
 *           GetpWin16Lock    (KERNEL.449)
 */
SEGPTR WINAPI GetpWin16Lock16(void)
{
    static SYSLEVEL *w16Mutex;
    static SEGPTR segpWin16Mutex;

    if (!segpWin16Mutex)
    {
        w16Mutex = &Win16Mutex;
        segpWin16Mutex = MapLS( &w16Mutex );
    }
    return segpWin16Mutex;
}

/************************************************************************
 *           _CreateSysLevel    (KERNEL.438)
 */
VOID WINAPI _CreateSysLevel(SYSLEVEL *lock, INT level)
{
    RtlInitializeCriticalSection( &lock->crst );
    lock->level = level;

    TRACE("(%p, %d): handle is %p\n",
                  lock, level, lock->crst.LockSemaphore );
}

/************************************************************************
 *           _EnterSysLevel    (KERNEL32.97)
 *           _EnterSysLevel    (KERNEL.439)
 */
VOID WINAPI _EnterSysLevel(SYSLEVEL *lock)
{
    struct kernel_thread_data *thread_data = kernel_get_thread_data();
    int i;

    TRACE("(%p, level %d): thread %lx count before %ld\n",
          lock, lock->level, GetCurrentThreadId(), thread_data->sys_count[lock->level] );

    for ( i = 3; i > lock->level; i-- )
        if ( thread_data->sys_count[i] > 0 )
        {
            ERR("(%p, level %d): Holding %p, level %d. Expect deadlock!\n",
                        lock, lock->level, thread_data->sys_mutex[i], i );
        }

    RtlEnterCriticalSection( &lock->crst );

    thread_data->sys_count[lock->level]++;
    thread_data->sys_mutex[lock->level] = lock;

    TRACE("(%p, level %d): thread %lx count after  %ld\n",
          lock, lock->level, GetCurrentThreadId(), thread_data->sys_count[lock->level] );

    if (lock == &Win16Mutex) CallTo16_TebSelector = get_fs();
}

/************************************************************************
 *           _LeaveSysLevel    (KERNEL32.98)
 *           _LeaveSysLevel    (KERNEL.440)
 */
VOID WINAPI _LeaveSysLevel(SYSLEVEL *lock)
{
    struct kernel_thread_data *thread_data = kernel_get_thread_data();

    TRACE("(%p, level %d): thread %lx count before %ld\n",
          lock, lock->level, GetCurrentThreadId(), thread_data->sys_count[lock->level] );

    if ( thread_data->sys_count[lock->level] <= 0 || thread_data->sys_mutex[lock->level] != lock )
    {
        ERR("(%p, level %d): Invalid state: count %ld mutex %p.\n",
                    lock, lock->level, thread_data->sys_count[lock->level],
                    thread_data->sys_mutex[lock->level] );
    }
    else
    {
        if ( --thread_data->sys_count[lock->level] == 0 )
            thread_data->sys_mutex[lock->level] = NULL;
    }

    RtlLeaveCriticalSection( &lock->crst );

    TRACE("(%p, level %d): thread %lx count after  %ld\n",
          lock, lock->level, GetCurrentThreadId(), thread_data->sys_count[lock->level] );
}

/************************************************************************
 *		@ (KERNEL32.86)
 */
VOID WINAPI _KERNEL32_86(SYSLEVEL *lock)
{
    _LeaveSysLevel(lock);
}

/************************************************************************
 *           _ConfirmSysLevel    (KERNEL32.95)
 *           _ConfirmSysLevel    (KERNEL.436)
 */
DWORD WINAPI _ConfirmSysLevel(SYSLEVEL *lock)
{
    if ( lock && lock->crst.OwningThread == ULongToHandle(GetCurrentThreadId()) )
        return lock->crst.RecursionCount;
    else
        return 0L;
}

/************************************************************************
 *           _CheckNotSysLevel    (KERNEL32.94)
 *           _CheckNotSysLevel    (KERNEL.437)
 */
VOID WINAPI _CheckNotSysLevel(SYSLEVEL *lock)
{
    if (lock && lock->crst.OwningThread == ULongToHandle(GetCurrentThreadId()) &&
        lock->crst.RecursionCount)
    {
        ERR( "Holding lock %p level %d\n", lock, lock->level );
        DbgBreakPoint();
    }
}


/************************************************************************
 *           _EnterWin16Lock			[KERNEL.480]
 */
VOID WINAPI _EnterWin16Lock(void)
{
    _EnterSysLevel(&Win16Mutex);
}

/************************************************************************
 *           _LeaveWin16Lock		[KERNEL.481]
 */
VOID WINAPI _LeaveWin16Lock(void)
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
 *           SYSLEVEL_CheckNotLevel
 */
VOID SYSLEVEL_CheckNotLevel( INT level )
{
    INT i;

    for ( i = 3; i >= level; i-- )
        if ( kernel_get_thread_data()->sys_count[i] > 0 )
        {
            ERR("(%d): Holding lock of level %d!\n",
                       level, i );
            DbgBreakPoint();
            break;
        }
}
