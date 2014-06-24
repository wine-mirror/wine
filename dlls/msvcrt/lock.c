/*
 * Copyright (c) 2002, TransGaming Technologies Inc.
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "cppexcept.h"
#include "mtdll.h"
#include "cxx.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct
{
  BOOL             bInit;
  CRITICAL_SECTION crit;
} LOCKTABLEENTRY;

static LOCKTABLEENTRY lock_table[ _TOTAL_LOCKS ];

static inline void msvcrt_mlock_set_entry_initialized( int locknum, BOOL initialized )
{
  lock_table[ locknum ].bInit = initialized;
}

static inline void msvcrt_initialize_mlock( int locknum )
{
  InitializeCriticalSection( &(lock_table[ locknum ].crit) );
  lock_table[ locknum ].crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": LOCKTABLEENTRY.crit");
  msvcrt_mlock_set_entry_initialized( locknum, TRUE );
}

static inline void msvcrt_uninitialize_mlock( int locknum )
{
  lock_table[ locknum ].crit.DebugInfo->Spare[0] = 0;
  DeleteCriticalSection( &(lock_table[ locknum ].crit) );
  msvcrt_mlock_set_entry_initialized( locknum, FALSE );
}

/**********************************************************************
 *     msvcrt_init_mt_locks (internal)
 *
 * Initialize the table lock. All other locks will be initialized
 * upon first use.
 *
 */
void msvcrt_init_mt_locks(void)
{
  int i;

  TRACE( "initializing mtlocks\n" );

  /* Initialize the table */
  for( i=0; i < _TOTAL_LOCKS; i++ )
  {
    msvcrt_mlock_set_entry_initialized( i, FALSE );
  }

  /* Initialize our lock table lock */
  msvcrt_initialize_mlock( _LOCKTAB_LOCK );
}

/**********************************************************************
 *              _lock (MSVCRT.@)
 */
void CDECL _lock( int locknum )
{
  TRACE( "(%d)\n", locknum );

  /* If the lock doesn't exist yet, create it */
  if( lock_table[ locknum ].bInit == FALSE )
  {
    /* Lock while we're changing the lock table */
    _lock( _LOCKTAB_LOCK );

    /* Check again if we've got a bit of a race on lock creation */
    if( lock_table[ locknum ].bInit == FALSE )
    {
      TRACE( ": creating lock #%d\n", locknum );
      msvcrt_initialize_mlock( locknum );
    }

    /* Unlock ourselves */
    _unlock( _LOCKTAB_LOCK );
  }

  EnterCriticalSection( &(lock_table[ locknum ].crit) );
}

/**********************************************************************
 *              _unlock (MSVCRT.@)
 *
 * NOTE: There is no error detection to make sure the lock exists and is acquired.
 */
void CDECL _unlock( int locknum )
{
  TRACE( "(%d)\n", locknum );

  LeaveCriticalSection( &(lock_table[ locknum ].crit) );
}

#if _MSVCR_VER >= 100
typedef enum
{
    SPINWAIT_INIT,
    SPINWAIT_SPIN,
    SPINWAIT_YIELD,
    SPINWAIT_DONE
} SpinWait_state;

typedef void (__cdecl *yield_func)(void);

typedef struct
{
    ULONG spin;
    ULONG unknown;
    SpinWait_state state;
    yield_func yield_func;
} SpinWait;

/* ?_Value@_SpinCount@details@Concurrency@@SAIXZ */
unsigned int __cdecl SpinCount__Value(void)
{
    static unsigned int val = -1;

    TRACE("()\n");

    if(val == -1) {
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        val = si.dwNumberOfProcessors>1 ? 4000 : 0;
    }

    return val;
}

/* ??0?$_SpinWait@$00@details@Concurrency@@QAE@P6AXXZ@Z */
/* ??0?$_SpinWait@$00@details@Concurrency@@QEAA@P6AXXZ@Z */
DEFINE_THISCALL_WRAPPER(SpinWait_ctor_yield, 8)
SpinWait*  __thiscall SpinWait_ctor_yield(SpinWait *this, yield_func yf)
{
    TRACE("(%p %p)\n", this, yf);

    this->state = SPINWAIT_INIT;
    this->unknown = 1;
    this->yield_func = yf;
    return this;
}

/* ??0?$_SpinWait@$0A@@details@Concurrency@@QAE@P6AXXZ@Z */
/* ??0?$_SpinWait@$0A@@details@Concurrency@@QEAA@P6AXXZ@Z */
DEFINE_THISCALL_WRAPPER(SpinWait_ctor, 8)
SpinWait* __thiscall SpinWait_ctor(SpinWait *this, yield_func yf)
{
    TRACE("(%p %p)\n", this, yf);

    this->state = SPINWAIT_INIT;
    this->unknown = 0;
    this->yield_func = yf;
    return this;
}

/* ??_F?$_SpinWait@$00@details@Concurrency@@QAEXXZ */
/* ??_F?$_SpinWait@$00@details@Concurrency@@QEAAXXZ */
/* ??_F?$_SpinWait@$0A@@details@Concurrency@@QAEXXZ */
/* ??_F?$_SpinWait@$0A@@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(SpinWait_dtor, 4)
void __thiscall SpinWait_dtor(SpinWait *this)
{
    TRACE("(%p)\n", this);
}

/* ?_DoYield@?$_SpinWait@$00@details@Concurrency@@IAEXXZ */
/* ?_DoYield@?$_SpinWait@$00@details@Concurrency@@IEAAXXZ */
/* ?_DoYield@?$_SpinWait@$0A@@details@Concurrency@@IAEXXZ */
/* ?_DoYield@?$_SpinWait@$0A@@details@Concurrency@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(SpinWait__DoYield, 4)
void __thiscall SpinWait__DoYield(SpinWait *this)
{
    TRACE("(%p)\n", this);

    if(this->unknown)
        this->yield_func();
}

/* ?_NumberOfSpins@?$_SpinWait@$00@details@Concurrency@@IAEKXZ */
/* ?_NumberOfSpins@?$_SpinWait@$00@details@Concurrency@@IEAAKXZ */
/* ?_NumberOfSpins@?$_SpinWait@$0A@@details@Concurrency@@IAEKXZ */
/* ?_NumberOfSpins@?$_SpinWait@$0A@@details@Concurrency@@IEAAKXZ */
DEFINE_THISCALL_WRAPPER(SpinWait__NumberOfSpins, 4)
ULONG __thiscall SpinWait__NumberOfSpins(SpinWait *this)
{
    TRACE("(%p)\n", this);
    return 1;
}

/* ?_SetSpinCount@?$_SpinWait@$00@details@Concurrency@@QAEXI@Z */
/* ?_SetSpinCount@?$_SpinWait@$00@details@Concurrency@@QEAAXI@Z */
/* ?_SetSpinCount@?$_SpinWait@$0A@@details@Concurrency@@QAEXI@Z */
/* ?_SetSpinCount@?$_SpinWait@$0A@@details@Concurrency@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(SpinWait__SetSpinCount, 8)
void __thiscall SpinWait__SetSpinCount(SpinWait *this, unsigned int spin)
{
    TRACE("(%p %d)\n", this, spin);

    this->spin = spin;
    this->state = spin ? SPINWAIT_SPIN : SPINWAIT_YIELD;
}

/* ?_Reset@?$_SpinWait@$00@details@Concurrency@@IAEXXZ */
/* ?_Reset@?$_SpinWait@$00@details@Concurrency@@IEAAXXZ */
/* ?_Reset@?$_SpinWait@$0A@@details@Concurrency@@IAEXXZ */
/* ?_Reset@?$_SpinWait@$0A@@details@Concurrency@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(SpinWait__Reset, 4)
void __thiscall SpinWait__Reset(SpinWait *this)
{
    SpinWait__SetSpinCount(this, SpinCount__Value());
}

/* ?_ShouldSpinAgain@?$_SpinWait@$00@details@Concurrency@@IAE_NXZ */
/* ?_ShouldSpinAgain@?$_SpinWait@$00@details@Concurrency@@IEAA_NXZ */
/* ?_ShouldSpinAgain@?$_SpinWait@$0A@@details@Concurrency@@IAE_NXZ */
/* ?_ShouldSpinAgain@?$_SpinWait@$0A@@details@Concurrency@@IEAA_NXZ */
DEFINE_THISCALL_WRAPPER(SpinWait__ShouldSpinAgain, 4)
MSVCRT_bool __thiscall SpinWait__ShouldSpinAgain(SpinWait *this)
{
    TRACE("(%p)\n", this);

    this->spin--;
    return this->spin > 0;
}

/* ?_SpinOnce@?$_SpinWait@$00@details@Concurrency@@QAE_NXZ */
/* ?_SpinOnce@?$_SpinWait@$00@details@Concurrency@@QEAA_NXZ */
/* ?_SpinOnce@?$_SpinWait@$0A@@details@Concurrency@@QAE_NXZ */
/* ?_SpinOnce@?$_SpinWait@$0A@@details@Concurrency@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(SpinWait__SpinOnce, 4)
MSVCRT_bool __thiscall SpinWait__SpinOnce(SpinWait *this)
{
    switch(this->state) {
    case SPINWAIT_INIT:
        SpinWait__Reset(this);
        /* fall through */
    case SPINWAIT_SPIN:
#ifdef __i386__
        __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
        __asm__ __volatile__( "" : : : "memory" );
#endif

        this->spin--;
        if(!this->spin)
            this->state = this->unknown ? SPINWAIT_YIELD : SPINWAIT_DONE;
        return TRUE;
    case SPINWAIT_YIELD:
        this->state = SPINWAIT_DONE;
        this->yield_func();
        return TRUE;
    default:
        SpinWait__Reset(this);
        return FALSE;
    }
}

static HANDLE keyed_event;

typedef struct cs_queue
{
    struct cs_queue *next;
} cs_queue;

typedef struct
{
    ULONG_PTR unk_thread_id;
    cs_queue unk_active;
#if _MSVCR_VER >= 110
    void *unknown[2];
    int unknown2[2];
#else
    void *unknown[1];
#endif
    cs_queue *head;
    void *tail;
} critical_section;

/* ??0critical_section@Concurrency@@QAE@XZ */
/* ??0critical_section@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(critical_section_ctor, 4)
critical_section* __thiscall critical_section_ctor(critical_section *this)
{
    TRACE("(%p)\n", this);

    if(!keyed_event) {
        HANDLE event;

        NtCreateKeyedEvent(&event, GENERIC_READ|GENERIC_WRITE, NULL, 0);
        if(InterlockedCompareExchangePointer(&keyed_event, event, NULL) != NULL)
            NtClose(event);
    }

    this->unk_thread_id = 0;
    this->head = this->tail = NULL;
    return this;
}

/* ??1critical_section@Concurrency@@QAE@XZ */
/* ??1critical_section@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(critical_section_dtor, 4)
void __thiscall critical_section_dtor(critical_section *this)
{
    TRACE("(%p)\n", this);
}

static void __cdecl spin_wait_yield(void)
{
    Sleep(0);
}

static inline void spin_wait_for_next_cs(cs_queue *q)
{
    SpinWait sw;

    if(q->next) return;

    SpinWait_ctor(&sw, &spin_wait_yield);
    SpinWait__Reset(&sw);
    while(!q->next)
        SpinWait__SpinOnce(&sw);
    SpinWait_dtor(&sw);
}

static inline void cs_set_head(critical_section *cs, cs_queue *q)
{
    cs->unk_thread_id = GetCurrentThreadId();
    cs->unk_active.next = q->next;
    cs->head = &cs->unk_active;
}

/* ?lock@critical_section@Concurrency@@QAEXXZ */
/* ?lock@critical_section@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(critical_section_lock, 4)
void __thiscall critical_section_lock(critical_section *this)
{
    cs_queue q, *last;

    TRACE("(%p)\n", this);

    if(this->unk_thread_id == GetCurrentThreadId()) {
        FIXME("throw exception\n");
        return;
    }

    q.next = NULL;
    last = InterlockedExchangePointer(&this->tail, &q);
    if(last) {
        last->next = &q;
        NtWaitForKeyedEvent(keyed_event, &q, 0, NULL);
    }

    this->unk_active.next = NULL;
    if(InterlockedCompareExchangePointer(&this->tail, &this->unk_active, &q) != &q)
        spin_wait_for_next_cs(&q);
    cs_set_head(this, &q);
}

/* ?try_lock@critical_section@Concurrency@@QAE_NXZ */
/* ?try_lock@critical_section@Concurrency@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(critical_section_try_lock, 4)
MSVCRT_bool __thiscall critical_section_try_lock(critical_section *this)
{
    cs_queue q;

    TRACE("(%p)\n", this);

    if(this->unk_thread_id == GetCurrentThreadId()) {
        FIXME("throw exception\n");
        return FALSE;
    }

    q.next = NULL;
    if(!InterlockedCompareExchangePointer(&this->tail, &q, NULL)) {
        this->unk_active.next = NULL;
        if(InterlockedCompareExchangePointer(&this->tail, &this->unk_active, &q) != &q)
            spin_wait_for_next_cs(&q);

        cs_set_head(this, &q);
        return TRUE;
    }
    return FALSE;
}

/* ?unlock@critical_section@Concurrency@@QAEXXZ */
/* ?unlock@critical_section@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(critical_section_unlock, 4)
void __thiscall critical_section_unlock(critical_section *this)
{
    TRACE("(%p)\n", this);

    this->unk_thread_id = 0;
    this->head = NULL;
    if(InterlockedCompareExchangePointer(&this->tail, NULL, &this->unk_active)
            == &this->unk_active) return;
    spin_wait_for_next_cs(&this->unk_active);

    NtReleaseKeyedEvent(keyed_event, this->unk_active.next, 0, NULL);
}

/* ?native_handle@critical_section@Concurrency@@QAEAAV12@XZ */
/* ?native_handle@critical_section@Concurrency@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(critical_section_native_handle, 4)
critical_section* __thiscall critical_section_native_handle(critical_section *this)
{
    TRACE("(%p)\n", this);
    return this;
}

typedef struct
{
    critical_section *cs;
    void *unknown[3];
} critical_section_scoped_lock;

/* ??0scoped_lock@critical_section@Concurrency@@QAE@AAV12@@Z */
/* ??0scoped_lock@critical_section@Concurrency@@QEAA@AEAV12@@Z */
DEFINE_THISCALL_WRAPPER(critical_section_scoped_lock_ctor, 8)
critical_section_scoped_lock* __thiscall critical_section_scoped_lock_ctor(
        critical_section_scoped_lock *this, critical_section *cs)
{
    TRACE("(%p %p)\n", this, cs);
    this->cs = cs;
    critical_section_lock(this->cs);
    return this;
}

/* ??1scoped_lock@critical_section@Concurrency@@QAE@XZ */
/* ??1scoped_lock@critical_section@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(critical_section_scoped_lock_dtor, 4)
void __thiscall critical_section_scoped_lock_dtor(critical_section_scoped_lock *this)
{
    TRACE("(%p)\n", this);
    critical_section_unlock(this->cs);
}
#endif

/**********************************************************************
 *     msvcrt_free_locks (internal)
 *
 * Uninitialize all mt locks. Assume that neither _lock or _unlock will
 * be called once we're calling this routine (ie _LOCKTAB_LOCK can be deleted)
 *
 */
void msvcrt_free_locks(void)
{
  int i;

  TRACE( ": uninitializing all mtlocks\n" );

  /* Uninitialize the table */
  for( i=0; i < _TOTAL_LOCKS; i++ )
  {
    if( lock_table[ i ].bInit )
    {
      msvcrt_uninitialize_mlock( i );
    }
  }

#if _MSVCR_VER >= 100
  if(keyed_event)
      NtClose(keyed_event);
#endif
}
