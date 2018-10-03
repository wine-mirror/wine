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
#include "wine/heap.h"
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
        InterlockedDecrement((LONG*)&this->spin);
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

/* keep in sync with msvcp90/msvcp90.h */
typedef struct cs_queue
{
    struct cs_queue *next;
#if _MSVCR_VER >= 110
    BOOL free;
    int unknown;
#endif
} cs_queue;

typedef struct
{
    ULONG_PTR unk_thread_id;
    cs_queue unk_active;
#if _MSVCR_VER >= 110
    void *unknown[2];
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

static inline void cs_lock(critical_section *cs, cs_queue *q)
{
    cs_queue *last;

    if(cs->unk_thread_id == GetCurrentThreadId())
        throw_exception(EXCEPTION_IMPROPER_LOCK, 0, "Already locked");

    memset(q, 0, sizeof(*q));
    last = InterlockedExchangePointer(&cs->tail, q);
    if(last) {
        last->next = q;
        NtWaitForKeyedEvent(keyed_event, q, 0, NULL);
    }

    cs_set_head(cs, q);
    if(InterlockedCompareExchangePointer(&cs->tail, &cs->unk_active, q) != q) {
        spin_wait_for_next_cs(q);
        cs->unk_active.next = q->next;
    }
}

/* ?lock@critical_section@Concurrency@@QAEXXZ */
/* ?lock@critical_section@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(critical_section_lock, 4)
void __thiscall critical_section_lock(critical_section *this)
{
    cs_queue q;

    TRACE("(%p)\n", this);
    cs_lock(this, &q);
}

/* ?try_lock@critical_section@Concurrency@@QAE_NXZ */
/* ?try_lock@critical_section@Concurrency@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(critical_section_try_lock, 4)
MSVCRT_bool __thiscall critical_section_try_lock(critical_section *this)
{
    cs_queue q;

    TRACE("(%p)\n", this);

    if(this->unk_thread_id == GetCurrentThreadId())
        return FALSE;

    memset(&q, 0, sizeof(q));
    if(!InterlockedCompareExchangePointer(&this->tail, &q, NULL)) {
        cs_set_head(this, &q);
        if(InterlockedCompareExchangePointer(&this->tail, &this->unk_active, &q) != &q) {
            spin_wait_for_next_cs(&q);
            this->unk_active.next = q.next;
        }
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

#if _MSVCR_VER >= 110
    while(1) {
        cs_queue *next;

        if(!InterlockedExchange(&this->unk_active.next->free, TRUE))
            break;

        next = this->unk_active.next;
        if(InterlockedCompareExchangePointer(&this->tail, NULL, next) == next) {
            HeapFree(GetProcessHeap(), 0, next);
            return;
        }
        spin_wait_for_next_cs(next);

        this->unk_active.next = next->next;
        HeapFree(GetProcessHeap(), 0, next);
    }
#endif

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

#if _MSVCR_VER >= 110
/* ?try_lock_for@critical_section@Concurrency@@QAE_NI@Z */
/* ?try_lock_for@critical_section@Concurrency@@QEAA_NI@Z */
DEFINE_THISCALL_WRAPPER(critical_section_try_lock_for, 8)
MSVCRT_bool __thiscall critical_section_try_lock_for(
        critical_section *this, unsigned int timeout)
{
    cs_queue *q, *last;

    TRACE("(%p %d)\n", this, timeout);

    if(this->unk_thread_id == GetCurrentThreadId())
        throw_exception(EXCEPTION_IMPROPER_LOCK, 0, "Already locked");

    if(!(q = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*q))))
        return critical_section_try_lock(this);

    last = InterlockedExchangePointer(&this->tail, q);
    if(last) {
        LARGE_INTEGER to;
        NTSTATUS status;
        FILETIME ft;

        last->next = q;
        GetSystemTimeAsFileTime(&ft);
        to.QuadPart = ((LONGLONG)ft.dwHighDateTime<<32) +
            ft.dwLowDateTime + (LONGLONG)timeout*10000;
        status = NtWaitForKeyedEvent(keyed_event, q, 0, &to);
        if(status == STATUS_TIMEOUT) {
            if(!InterlockedExchange(&q->free, TRUE))
                return FALSE;
            /* A thread has signaled the event and is block waiting. */
            /* We need to catch the event to wake the thread.        */
            NtWaitForKeyedEvent(keyed_event, q, 0, NULL);
        }
    }

    cs_set_head(this, q);
    if(InterlockedCompareExchangePointer(&this->tail, &this->unk_active, q) != q) {
        spin_wait_for_next_cs(q);
        this->unk_active.next = q->next;
    }

    HeapFree(GetProcessHeap(), 0, q);
    return TRUE;
}
#endif

typedef struct
{
    critical_section *cs;
    union {
        cs_queue q;
        struct {
            void *unknown[4];
            int unknown2[2];
        } unknown;
    } lock;
} critical_section_scoped_lock;

/* ??0scoped_lock@critical_section@Concurrency@@QAE@AAV12@@Z */
/* ??0scoped_lock@critical_section@Concurrency@@QEAA@AEAV12@@Z */
DEFINE_THISCALL_WRAPPER(critical_section_scoped_lock_ctor, 8)
critical_section_scoped_lock* __thiscall critical_section_scoped_lock_ctor(
        critical_section_scoped_lock *this, critical_section *cs)
{
    TRACE("(%p %p)\n", this, cs);
    this->cs = cs;
    cs_lock(this->cs, &this->lock.q);
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

typedef struct
{
    critical_section cs;
} _NonReentrantPPLLock;

/* ??0_NonReentrantPPLLock@details@Concurrency@@QAE@XZ */
/* ??0_NonReentrantPPLLock@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_NonReentrantPPLLock_ctor, 4)
_NonReentrantPPLLock* __thiscall _NonReentrantPPLLock_ctor(_NonReentrantPPLLock *this)
{
    TRACE("(%p)\n", this);

    critical_section_ctor(&this->cs);
    return this;
}

/* ?_Acquire@_NonReentrantPPLLock@details@Concurrency@@QAEXPAX@Z */
/* ?_Acquire@_NonReentrantPPLLock@details@Concurrency@@QEAAXPEAX@Z */
DEFINE_THISCALL_WRAPPER(_NonReentrantPPLLock__Acquire, 8)
void __thiscall _NonReentrantPPLLock__Acquire(_NonReentrantPPLLock *this, cs_queue *q)
{
    TRACE("(%p %p)\n", this, q);
    cs_lock(&this->cs, q);
}

/* ?_Release@_NonReentrantPPLLock@details@Concurrency@@QAEXXZ */
/* ?_Release@_NonReentrantPPLLock@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_NonReentrantPPLLock__Release, 4)
void __thiscall _NonReentrantPPLLock__Release(_NonReentrantPPLLock *this)
{
    TRACE("(%p)\n", this);
    critical_section_unlock(&this->cs);
}

typedef struct
{
    _NonReentrantPPLLock *lock;
    union {
        cs_queue q;
        struct {
            void *unknown[4];
            int unknown2[2];
        } unknown;
    } wait;
} _NonReentrantPPLLock__Scoped_lock;

/* ??0_Scoped_lock@_NonReentrantPPLLock@details@Concurrency@@QAE@AAV123@@Z */
/* ??0_Scoped_lock@_NonReentrantPPLLock@details@Concurrency@@QEAA@AEAV123@@Z */
DEFINE_THISCALL_WRAPPER(_NonReentrantPPLLock__Scoped_lock_ctor, 8)
_NonReentrantPPLLock__Scoped_lock* __thiscall _NonReentrantPPLLock__Scoped_lock_ctor(
        _NonReentrantPPLLock__Scoped_lock *this, _NonReentrantPPLLock *lock)
{
    TRACE("(%p %p)\n", this, lock);

    this->lock = lock;
    _NonReentrantPPLLock__Acquire(this->lock, &this->wait.q);
    return this;
}

/* ??1_Scoped_lock@_NonReentrantPPLLock@details@Concurrency@@QAE@XZ */
/* ??1_Scoped_lock@_NonReentrantPPLLock@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_NonReentrantPPLLock__Scoped_lock_dtor, 4)
void __thiscall _NonReentrantPPLLock__Scoped_lock_dtor(_NonReentrantPPLLock__Scoped_lock *this)
{
    TRACE("(%p)\n", this);

    _NonReentrantPPLLock__Release(this->lock);
}

typedef struct
{
    critical_section cs;
    LONG count;
    LONG owner;
} _ReentrantPPLLock;

/* ??0_ReentrantPPLLock@details@Concurrency@@QAE@XZ */
/* ??0_ReentrantPPLLock@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_ReentrantPPLLock_ctor, 4)
_ReentrantPPLLock* __thiscall _ReentrantPPLLock_ctor(_ReentrantPPLLock *this)
{
    TRACE("(%p)\n", this);

    critical_section_ctor(&this->cs);
    this->count = 0;
    this->owner = -1;
    return this;
}

/* ?_Acquire@_ReentrantPPLLock@details@Concurrency@@QAEXPAX@Z */
/* ?_Acquire@_ReentrantPPLLock@details@Concurrency@@QEAAXPEAX@Z */
DEFINE_THISCALL_WRAPPER(_ReentrantPPLLock__Acquire, 8)
void __thiscall _ReentrantPPLLock__Acquire(_ReentrantPPLLock *this, cs_queue *q)
{
    TRACE("(%p %p)\n", this, q);

    if(this->owner == GetCurrentThreadId()) {
        this->count++;
        return;
    }

    cs_lock(&this->cs, q);
    this->count++;
    this->owner = GetCurrentThreadId();
}

/* ?_Release@_ReentrantPPLLock@details@Concurrency@@QAEXXZ */
/* ?_Release@_ReentrantPPLLock@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_ReentrantPPLLock__Release, 4)
void __thiscall _ReentrantPPLLock__Release(_ReentrantPPLLock *this)
{
    TRACE("(%p)\n", this);

    this->count--;
    if(this->count)
        return;

    this->owner = -1;
    critical_section_unlock(&this->cs);
}

typedef struct
{
    _ReentrantPPLLock *lock;
    union {
        cs_queue q;
        struct {
            void *unknown[4];
            int unknown2[2];
        } unknown;
    } wait;
} _ReentrantPPLLock__Scoped_lock;

/* ??0_Scoped_lock@_ReentrantPPLLock@details@Concurrency@@QAE@AAV123@@Z */
/* ??0_Scoped_lock@_ReentrantPPLLock@details@Concurrency@@QEAA@AEAV123@@Z */
DEFINE_THISCALL_WRAPPER(_ReentrantPPLLock__Scoped_lock_ctor, 8)
_ReentrantPPLLock__Scoped_lock* __thiscall _ReentrantPPLLock__Scoped_lock_ctor(
        _ReentrantPPLLock__Scoped_lock *this, _ReentrantPPLLock *lock)
{
    TRACE("(%p %p)\n", this, lock);

    this->lock = lock;
    _ReentrantPPLLock__Acquire(this->lock, &this->wait.q);
    return this;
}

/* ??1_Scoped_lock@_ReentrantPPLLock@details@Concurrency@@QAE@XZ */
/* ??1_Scoped_lock@_ReentrantPPLLock@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_ReentrantPPLLock__Scoped_lock_dtor, 4)
void __thiscall _ReentrantPPLLock__Scoped_lock_dtor(_ReentrantPPLLock__Scoped_lock *this)
{
    TRACE("(%p)\n", this);

    _ReentrantPPLLock__Release(this->lock);
}

/* ?_GetConcurrency@details@Concurrency@@YAIXZ */
unsigned int __cdecl _GetConcurrency(void)
{
    static unsigned int val = -1;

    TRACE("()\n");

    if(val == -1) {
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        val = si.dwNumberOfProcessors;
    }

    return val;
}

#define EVT_RUNNING     (void*)1
#define EVT_WAITING     NULL

struct thread_wait;
typedef struct thread_wait_entry
{
    struct thread_wait *wait;
    struct thread_wait_entry *next;
    struct thread_wait_entry *prev;
} thread_wait_entry;

typedef struct thread_wait
{
    void *signaled;
    int pending_waits;
    thread_wait_entry entries[1];
} thread_wait;

typedef struct
{
    thread_wait_entry *waiters;
    INT_PTR signaled;
    critical_section cs;
} event;

static inline PLARGE_INTEGER evt_timeout(PLARGE_INTEGER pTime, unsigned int timeout)
{
    if(timeout == COOPERATIVE_TIMEOUT_INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

static void evt_add_queue(thread_wait_entry **head, thread_wait_entry *entry)
{
    entry->next = *head;
    entry->prev = NULL;
    if(*head) (*head)->prev = entry;
    *head = entry;
}

static void evt_remove_queue(thread_wait_entry **head, thread_wait_entry *entry)
{
    if(entry == *head)
        *head = entry->next;
    else if(entry->prev)
        entry->prev->next = entry->next;
    if(entry->next) entry->next->prev = entry->prev;
}

static MSVCRT_size_t evt_end_wait(thread_wait *wait, event **events, int count)
{
    MSVCRT_size_t i, ret = COOPERATIVE_WAIT_TIMEOUT;

    for(i = 0; i < count; i++) {
        critical_section_lock(&events[i]->cs);
        if(events[i] == wait->signaled) ret = i;
        evt_remove_queue(&events[i]->waiters, &wait->entries[i]);
        critical_section_unlock(&events[i]->cs);
    }

    return ret;
}

static inline int evt_transition(void **state, void *from, void *to)
{
    return InterlockedCompareExchangePointer(state, to, from) == from;
}

static MSVCRT_size_t evt_wait(thread_wait *wait, event **events, int count, MSVCRT_bool wait_all, unsigned int timeout)
{
    int i;
    NTSTATUS status;
    LARGE_INTEGER ntto;

    wait->signaled = EVT_RUNNING;
    wait->pending_waits = wait_all ? count : 1;
    for(i = 0; i < count; i++) {
        wait->entries[i].wait = wait;

        critical_section_lock(&events[i]->cs);
        evt_add_queue(&events[i]->waiters, &wait->entries[i]);
        if(events[i]->signaled) {
            if(!InterlockedDecrement(&wait->pending_waits)) {
                wait->signaled = events[i];
                critical_section_unlock(&events[i]->cs);

                return evt_end_wait(wait, events, i+1);
            }
        }
        critical_section_unlock(&events[i]->cs);
    }

    if(!timeout)
        return evt_end_wait(wait, events, count);

    if(!evt_transition(&wait->signaled, EVT_RUNNING, EVT_WAITING))
        return evt_end_wait(wait, events, count);

    status = NtWaitForKeyedEvent(keyed_event, wait, 0, evt_timeout(&ntto, timeout));

    if(status && !evt_transition(&wait->signaled, EVT_WAITING, EVT_RUNNING))
        NtWaitForKeyedEvent(keyed_event, wait, 0, NULL);

    return evt_end_wait(wait, events, count);
}

/* ??0event@Concurrency@@QAE@XZ */
/* ??0event@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(event_ctor, 4)
event* __thiscall event_ctor(event *this)
{
    TRACE("(%p)\n", this);

    this->waiters = NULL;
    this->signaled = FALSE;
    critical_section_ctor(&this->cs);

    return this;
}

/* ??1event@Concurrency@@QAE@XZ */
/* ??1event@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(event_dtor, 4)
void __thiscall event_dtor(event *this)
{
    TRACE("(%p)\n", this);
    critical_section_dtor(&this->cs);
    if(this->waiters)
        ERR("there's a wait on destroyed event\n");
}

/* ?reset@event@Concurrency@@QAEXXZ */
/* ?reset@event@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(event_reset, 4)
void __thiscall event_reset(event *this)
{
    thread_wait_entry *entry;

    TRACE("(%p)\n", this);

    critical_section_lock(&this->cs);
    if(this->signaled) {
        this->signaled = FALSE;
        for(entry=this->waiters; entry; entry = entry->next)
            InterlockedIncrement(&entry->wait->pending_waits);
    }
    critical_section_unlock(&this->cs);
}

/* ?set@event@Concurrency@@QAEXXZ */
/* ?set@event@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(event_set, 4)
void __thiscall event_set(event *this)
{
    thread_wait_entry *wakeup = NULL;
    thread_wait_entry *entry, *next;

    TRACE("(%p)\n", this);

    critical_section_lock(&this->cs);
    if(!this->signaled) {
        this->signaled = TRUE;
        for(entry=this->waiters; entry; entry=next) {
            next = entry->next;
            if(!InterlockedDecrement(&entry->wait->pending_waits)) {
                if(InterlockedExchangePointer(&entry->wait->signaled, this) == EVT_WAITING) {
                    evt_remove_queue(&this->waiters, entry);
                    evt_add_queue(&wakeup, entry);
                }
            }
        }
    }
    critical_section_unlock(&this->cs);

    for(entry=wakeup; entry; entry=next) {
        next = entry->next;
        entry->next = entry->prev = NULL;
        NtReleaseKeyedEvent(keyed_event, entry->wait, 0, NULL);
    }
}

/* ?wait@event@Concurrency@@QAEII@Z */
/* ?wait@event@Concurrency@@QEAA_KI@Z */
DEFINE_THISCALL_WRAPPER(event_wait, 8)
MSVCRT_size_t __thiscall event_wait(event *this, unsigned int timeout)
{
    thread_wait wait;
    MSVCRT_size_t signaled;

    TRACE("(%p %u)\n", this, timeout);

    critical_section_lock(&this->cs);
    signaled = this->signaled;
    critical_section_unlock(&this->cs);

    if(!timeout) return signaled ? 0 : COOPERATIVE_WAIT_TIMEOUT;
    return signaled ? 0 : evt_wait(&wait, &this, 1, FALSE, timeout);
}

/* ?wait_for_multiple@event@Concurrency@@SAIPAPAV12@I_NI@Z */
/* ?wait_for_multiple@event@Concurrency@@SA_KPEAPEAV12@_K_NI@Z */
int __cdecl event_wait_for_multiple(event **events, MSVCRT_size_t count, MSVCRT_bool wait_all, unsigned int timeout)
{
    thread_wait *wait;
    MSVCRT_size_t ret;

    TRACE("(%p %ld %d %u)\n", events, count, wait_all, timeout);

    if(count == 0)
        return 0;

    wait = heap_alloc(FIELD_OFFSET(thread_wait, entries[count]));
    if(!wait)
        throw_exception(EXCEPTION_BAD_ALLOC, 0, "bad allocation");
    ret = evt_wait(wait, events, count, wait_all, timeout);
    heap_free(wait);

    return ret;
}
#endif

#if _MSVCR_VER >= 110
typedef struct cv_queue {
    struct cv_queue *next;
    BOOL expired;
} cv_queue;

typedef struct {
    /* cv_queue structure is not binary compatible */
    cv_queue *queue;
    critical_section lock;
} _Condition_variable;

/* ??0_Condition_variable@details@Concurrency@@QAE@XZ */
/* ??0_Condition_variable@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Condition_variable_ctor, 4)
_Condition_variable* __thiscall _Condition_variable_ctor(_Condition_variable *this)
{
    TRACE("(%p)\n", this);

    this->queue = NULL;
    critical_section_ctor(&this->lock);
    return this;
}

/* ??1_Condition_variable@details@Concurrency@@QAE@XZ */
/* ??1_Condition_variable@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Condition_variable_dtor, 4)
void __thiscall _Condition_variable_dtor(_Condition_variable *this)
{
    TRACE("(%p)\n", this);

    while(this->queue) {
        cv_queue *next = this->queue->next;
        if(!this->queue->expired)
            ERR("there's an active wait\n");
        HeapFree(GetProcessHeap(), 0, this->queue);
        this->queue = next;
    }
    critical_section_dtor(&this->lock);
}

/* ?wait@_Condition_variable@details@Concurrency@@QAEXAAVcritical_section@3@@Z */
/* ?wait@_Condition_variable@details@Concurrency@@QEAAXAEAVcritical_section@3@@Z */
DEFINE_THISCALL_WRAPPER(_Condition_variable_wait, 8)
void __thiscall _Condition_variable_wait(_Condition_variable *this, critical_section *cs)
{
    cv_queue q;

    TRACE("(%p, %p)\n", this, cs);

    critical_section_lock(&this->lock);
    q.next = this->queue;
    q.expired = FALSE;
    this->queue = &q;
    critical_section_unlock(&this->lock);

    critical_section_unlock(cs);
    NtWaitForKeyedEvent(keyed_event, &q, 0, NULL);
    critical_section_lock(cs);
}

/* ?wait_for@_Condition_variable@details@Concurrency@@QAE_NAAVcritical_section@3@I@Z */
/* ?wait_for@_Condition_variable@details@Concurrency@@QEAA_NAEAVcritical_section@3@I@Z */
DEFINE_THISCALL_WRAPPER(_Condition_variable_wait_for, 12)
MSVCRT_bool __thiscall _Condition_variable_wait_for(_Condition_variable *this,
        critical_section *cs, unsigned int timeout)
{
    LARGE_INTEGER to;
    NTSTATUS status;
    FILETIME ft;
    cv_queue *q;

    TRACE("(%p %p %d)\n", this, cs, timeout);

    if(!(q = HeapAlloc(GetProcessHeap(), 0, sizeof(cv_queue)))) {
        throw_exception(EXCEPTION_BAD_ALLOC, 0, "bad allocation");
    }

    critical_section_lock(&this->lock);
    q->next = this->queue;
    q->expired = FALSE;
    this->queue = q;
    critical_section_unlock(&this->lock);

    critical_section_unlock(cs);

    GetSystemTimeAsFileTime(&ft);
    to.QuadPart = ((LONGLONG)ft.dwHighDateTime << 32) +
        ft.dwLowDateTime + (LONGLONG)timeout * 10000;
    status = NtWaitForKeyedEvent(keyed_event, q, 0, &to);
    if(status == STATUS_TIMEOUT) {
        if(!InterlockedExchange(&q->expired, TRUE)) {
            critical_section_lock(cs);
            return FALSE;
        }
        else
            NtWaitForKeyedEvent(keyed_event, q, 0, 0);
    }

    HeapFree(GetProcessHeap(), 0, q);
    critical_section_lock(cs);
    return TRUE;
}

/* ?notify_one@_Condition_variable@details@Concurrency@@QAEXXZ */
/* ?notify_one@_Condition_variable@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Condition_variable_notify_one, 4)
void __thiscall _Condition_variable_notify_one(_Condition_variable *this)
{
    cv_queue *node;

    TRACE("(%p)\n", this);

    if(!this->queue)
        return;

    while(1) {
        critical_section_lock(&this->lock);
        node = this->queue;
        if(!node) {
            critical_section_unlock(&this->lock);
            return;
        }
        this->queue = node->next;
        critical_section_unlock(&this->lock);

        if(!InterlockedExchange(&node->expired, TRUE)) {
            NtReleaseKeyedEvent(keyed_event, node, 0, NULL);
            return;
        } else {
            HeapFree(GetProcessHeap(), 0, node);
        }
    }
}

/* ?notify_all@_Condition_variable@details@Concurrency@@QAEXXZ */
/* ?notify_all@_Condition_variable@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Condition_variable_notify_all, 4)
void __thiscall _Condition_variable_notify_all(_Condition_variable *this)
{
    cv_queue *ptr;

    TRACE("(%p)\n", this);

    if(!this->queue)
        return;

    critical_section_lock(&this->lock);
    ptr = this->queue;
    this->queue = NULL;
    critical_section_unlock(&this->lock);

    while(ptr) {
        cv_queue *next = ptr->next;

        if(!InterlockedExchange(&ptr->expired, TRUE))
            NtReleaseKeyedEvent(keyed_event, ptr, 0, NULL);
        else
            HeapFree(GetProcessHeap(), 0, ptr);
        ptr = next;
    }
}
#endif

#if _MSVCR_VER >= 100
typedef struct rwl_queue
{
    struct rwl_queue *next;
} rwl_queue;

#define WRITER_WAITING 0x80000000
/* FIXME: reader_writer_lock structure is not binary compatible
 * it can't exceed 28/56 bytes */
typedef struct
{
    LONG count;
    LONG thread_id;
    rwl_queue active;
    rwl_queue *writer_head;
    rwl_queue *writer_tail;
    rwl_queue *reader_head;
} reader_writer_lock;

/* ??0reader_writer_lock@Concurrency@@QAE@XZ */
/* ??0reader_writer_lock@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_ctor, 4)
reader_writer_lock* __thiscall reader_writer_lock_ctor(reader_writer_lock *this)
{
    TRACE("(%p)\n", this);

    if (!keyed_event) {
        HANDLE event;

        NtCreateKeyedEvent(&event, GENERIC_READ|GENERIC_WRITE, NULL, 0);
        if (InterlockedCompareExchangePointer(&keyed_event, event, NULL) != NULL)
            NtClose(event);
    }

    memset(this, 0, sizeof(*this));
    return this;
}

/* ??1reader_writer_lock@Concurrency@@QAE@XZ */
/* ??1reader_writer_lock@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_dtor, 4)
void __thiscall reader_writer_lock_dtor(reader_writer_lock *this)
{
    TRACE("(%p)\n", this);

    if (this->thread_id != 0 || this->count)
        WARN("destroying locked reader_writer_lock\n");
}

static inline void spin_wait_for_next_rwl(rwl_queue *q)
{
    SpinWait sw;

    if(q->next) return;

    SpinWait_ctor(&sw, &spin_wait_yield);
    SpinWait__Reset(&sw);
    while(!q->next)
        SpinWait__SpinOnce(&sw);
    SpinWait_dtor(&sw);
}

/* Remove when proper InterlockedOr implementation is added to wine */
static LONG InterlockedOr(LONG *d, LONG v)
{
    LONG l;
    while (~(l = *d) & v)
        if (InterlockedCompareExchange(d, l|v, l) == l) break;
    return l;
}

static LONG InterlockedAnd(LONG *d, LONG v)
{
    LONG l = *d, old;
    while ((l & v) != l) {
        if((old = InterlockedCompareExchange(d, l&v, l)) == l) break;
        l = old;
    }
    return l;
}

/* ?lock@reader_writer_lock@Concurrency@@QAEXXZ */
/* ?lock@reader_writer_lock@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_lock, 4)
void __thiscall reader_writer_lock_lock(reader_writer_lock *this)
{
    rwl_queue q = { NULL }, *last;

    TRACE("(%p)\n", this);

    if (this->thread_id == GetCurrentThreadId())
        throw_exception(EXCEPTION_IMPROPER_LOCK, 0, "Already locked");

    last = InterlockedExchangePointer((void**)&this->writer_tail, &q);
    if (last) {
        last->next = &q;
        NtWaitForKeyedEvent(keyed_event, &q, 0, NULL);
    } else {
        this->writer_head = &q;
        if (InterlockedOr(&this->count, WRITER_WAITING))
            NtWaitForKeyedEvent(keyed_event, &q, 0, NULL);
    }

    this->thread_id = GetCurrentThreadId();
    this->writer_head = &this->active;
    this->active.next = NULL;
    if (InterlockedCompareExchangePointer((void**)&this->writer_tail, &this->active, &q) != &q) {
        spin_wait_for_next_rwl(&q);
        this->active.next = q.next;
    }
}

/* ?lock_read@reader_writer_lock@Concurrency@@QAEXXZ */
/* ?lock_read@reader_writer_lock@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_lock_read, 4)
void __thiscall reader_writer_lock_lock_read(reader_writer_lock *this)
{
    rwl_queue q;

    TRACE("(%p)\n", this);

    if (this->thread_id == GetCurrentThreadId())
        throw_exception(EXCEPTION_IMPROPER_LOCK, 0, "Already locked as writer");

    do {
        q.next = this->reader_head;
    } while(InterlockedCompareExchangePointer((void**)&this->reader_head, &q, q.next) != q.next);

    if (!q.next) {
        rwl_queue *head;
        LONG count;

        while (!((count = this->count) & WRITER_WAITING))
            if (InterlockedCompareExchange(&this->count, count+1, count) == count) break;

        if (count & WRITER_WAITING)
            NtWaitForKeyedEvent(keyed_event, &q, 0, NULL);

        head = InterlockedExchangePointer((void**)&this->reader_head, NULL);
        while(head && head != &q) {
            rwl_queue *next = head->next;
            InterlockedIncrement(&this->count);
            NtReleaseKeyedEvent(keyed_event, head, 0, NULL);
            head = next;
        }
    } else {
        NtWaitForKeyedEvent(keyed_event, &q, 0, NULL);
    }
}

/* ?try_lock@reader_writer_lock@Concurrency@@QAE_NXZ */
/* ?try_lock@reader_writer_lock@Concurrency@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_try_lock, 4)
MSVCRT_bool __thiscall reader_writer_lock_try_lock(reader_writer_lock *this)
{
    rwl_queue q = { NULL };

    TRACE("(%p)\n", this);

    if (this->thread_id == GetCurrentThreadId())
        return FALSE;

    if (InterlockedCompareExchangePointer((void**)&this->writer_tail, &q, NULL))
        return FALSE;
    this->writer_head = &q;
    if (!InterlockedCompareExchange(&this->count, WRITER_WAITING, 0)) {
        this->thread_id = GetCurrentThreadId();
        this->writer_head = &this->active;
        this->active.next = NULL;
        if (InterlockedCompareExchangePointer((void**)&this->writer_tail, &this->active, &q) != &q) {
            spin_wait_for_next_rwl(&q);
            this->active.next = q.next;
        }
        return TRUE;
    }

    if (InterlockedCompareExchangePointer((void**)&this->writer_tail, NULL, &q) == &q)
        return FALSE;
    spin_wait_for_next_rwl(&q);
    this->writer_head = q.next;
    if (!InterlockedOr(&this->count, WRITER_WAITING)) {
        this->thread_id = GetCurrentThreadId();
        this->writer_head = &this->active;
        this->active.next = q.next;
        return TRUE;
    }
    return FALSE;
}

/* ?try_lock_read@reader_writer_lock@Concurrency@@QAE_NXZ */
/* ?try_lock_read@reader_writer_lock@Concurrency@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_try_lock_read, 4)
MSVCRT_bool __thiscall reader_writer_lock_try_lock_read(reader_writer_lock *this)
{
    LONG count;

    TRACE("(%p)\n", this);

    while (!((count = this->count) & WRITER_WAITING))
        if (InterlockedCompareExchange(&this->count, count+1, count) == count) return TRUE;
    return FALSE;
}

/* ?unlock@reader_writer_lock@Concurrency@@QAEXXZ */
/* ?unlock@reader_writer_lock@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_unlock, 4)
void __thiscall reader_writer_lock_unlock(reader_writer_lock *this)
{
    LONG count;
    rwl_queue *head, *next;

    TRACE("(%p)\n", this);

    if ((count = this->count) & ~WRITER_WAITING) {
        count = InterlockedDecrement(&this->count);
        if (count != WRITER_WAITING)
            return;
        NtReleaseKeyedEvent(keyed_event, this->writer_head, 0, NULL);
        return;
    }

    this->thread_id = 0;
    next = this->writer_head->next;
    if (next) {
        NtReleaseKeyedEvent(keyed_event, next, 0, NULL);
        return;
    }
    InterlockedAnd(&this->count, ~WRITER_WAITING);
    head = InterlockedExchangePointer((void**)&this->reader_head, NULL);
    while (head) {
        next = head->next;
        InterlockedIncrement(&this->count);
        NtReleaseKeyedEvent(keyed_event, head, 0, NULL);
        head = next;
    }

    if (InterlockedCompareExchangePointer((void**)&this->writer_tail, NULL, this->writer_head) == this->writer_head)
        return;
    InterlockedOr(&this->count, WRITER_WAITING);
}

typedef struct {
    reader_writer_lock *lock;
} reader_writer_lock_scoped_lock;

/* ??0scoped_lock@reader_writer_lock@Concurrency@@QAE@AAV12@@Z */
/* ??0scoped_lock@reader_writer_lock@Concurrency@@QEAA@AEAV12@@Z */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_scoped_lock_ctor, 8)
reader_writer_lock_scoped_lock* __thiscall reader_writer_lock_scoped_lock_ctor(
        reader_writer_lock_scoped_lock *this, reader_writer_lock *lock)
{
    TRACE("(%p %p)\n", this, lock);

    this->lock = lock;
    reader_writer_lock_lock(lock);
    return this;
}

/* ??1scoped_lock@reader_writer_lock@Concurrency@@QAE@XZ */
/* ??1scoped_lock@reader_writer_lock@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_scoped_lock_dtor, 4)
void __thiscall reader_writer_lock_scoped_lock_dtor(reader_writer_lock_scoped_lock *this)
{
    TRACE("(%p)\n", this);
    reader_writer_lock_unlock(this->lock);
}

/* ??0scoped_lock_read@reader_writer_lock@Concurrency@@QAE@AAV12@@Z */
/* ??0scoped_lock_read@reader_writer_lock@Concurrency@@QEAA@AEAV12@@Z */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_scoped_lock_read_ctor, 8)
reader_writer_lock_scoped_lock* __thiscall reader_writer_lock_scoped_lock_read_ctor(
        reader_writer_lock_scoped_lock *this, reader_writer_lock *lock)
{
    TRACE("(%p %p)\n", this, lock);

    this->lock = lock;
    reader_writer_lock_lock_read(lock);
    return this;
}

/* ??1scoped_lock_read@reader_writer_lock@Concurrency@@QAE@XZ */
/* ??1scoped_lock_read@reader_writer_lock@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(reader_writer_lock_scoped_lock_read_dtor, 4)
void __thiscall reader_writer_lock_scoped_lock_read_dtor(reader_writer_lock_scoped_lock *this)
{
    TRACE("(%p)\n", this);
    reader_writer_lock_unlock(this->lock);
}

typedef struct {
    CRITICAL_SECTION cs;
} _ReentrantBlockingLock;

/* ??0_ReentrantBlockingLock@details@Concurrency@@QAE@XZ */
/* ??0_ReentrantBlockingLock@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_ReentrantBlockingLock_ctor, 4)
_ReentrantBlockingLock* __thiscall _ReentrantBlockingLock_ctor(_ReentrantBlockingLock *this)
{
    TRACE("(%p)\n", this);

    InitializeCriticalSection(&this->cs);
    this->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": _ReentrantBlockingLock");
    return this;
}

/* ??1_ReentrantBlockingLock@details@Concurrency@@QAE@XZ */
/* ??1_ReentrantBlockingLock@details@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_ReentrantBlockingLock_dtor, 4)
void __thiscall _ReentrantBlockingLock_dtor(_ReentrantBlockingLock *this)
{
    TRACE("(%p)\n", this);

    this->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&this->cs);
}

/* ?_Acquire@_ReentrantBlockingLock@details@Concurrency@@QAEXXZ */
/* ?_Acquire@_ReentrantBlockingLock@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_ReentrantBlockingLock__Acquire, 4)
void __thiscall _ReentrantBlockingLock__Acquire(_ReentrantBlockingLock *this)
{
    TRACE("(%p)\n", this);
    EnterCriticalSection(&this->cs);
}

/* ?_Release@_ReentrantBlockingLock@details@Concurrency@@QAEXXZ */
/* ?_Release@_ReentrantBlockingLock@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_ReentrantBlockingLock__Release, 4)
void __thiscall _ReentrantBlockingLock__Release(_ReentrantBlockingLock *this)
{
    TRACE("(%p)\n", this);
    LeaveCriticalSection(&this->cs);
}

/* ?_TryAcquire@_ReentrantBlockingLock@details@Concurrency@@QAE_NXZ */
/* ?_TryAcquire@_ReentrantBlockingLock@details@Concurrency@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(_ReentrantBlockingLock__TryAcquire, 4)
MSVCRT_bool __thiscall _ReentrantBlockingLock__TryAcquire(_ReentrantBlockingLock *this)
{
    TRACE("(%p)\n", this);
    return TryEnterCriticalSection(&this->cs);
}

/* ?wait@Concurrency@@YAXI@Z */
void __cdecl Concurrency_wait(unsigned int time)
{
    static int once;

    if (!once++) FIXME("(%d) stub!\n", time);

    Sleep(time);
}
#endif

#if _MSVCR_VER == 110
static LONG shared_ptr_lock;

void __cdecl _Lock_shared_ptr_spin_lock(void)
{
    LONG l = 0;

    while(InterlockedCompareExchange(&shared_ptr_lock, 1, 0) != 0) {
        if(l++ == 1000) {
            Sleep(0);
            l = 0;
        }
    }
}

void __cdecl _Unlock_shared_ptr_spin_lock(void)
{
    shared_ptr_lock = 0;
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
