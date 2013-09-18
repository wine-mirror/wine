/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <stdarg.h>
#include <limits.h>

#include "msvcp.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

/* ??0_Mutex@std@@QAE@XZ */
/* ??0_Mutex@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(mutex_ctor, 4)
mutex* __thiscall mutex_ctor(mutex *this)
{
    CRITICAL_SECTION *cs = MSVCRT_operator_new(sizeof(*cs));
    if(!cs) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }

    InitializeCriticalSection(cs);
    cs->DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": _Mutex critical section");
    this->mutex = cs;
    return this;
}

/* ??1_Mutex@std@@QAE@XZ */
/* ??1_Mutex@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(mutex_dtor, 4)
void __thiscall mutex_dtor(mutex *this)
{
    ((CRITICAL_SECTION*)this->mutex)->DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(this->mutex);
    MSVCRT_operator_delete(this->mutex);
}

/* ?_Lock@_Mutex@std@@QAEXXZ */
/* ?_Lock@_Mutex@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(mutex_lock, 4)
void __thiscall mutex_lock(mutex *this)
{
    EnterCriticalSection(this->mutex);
}

/* ?_Unlock@_Mutex@std@@QAEXXZ */
/* ?_Unlock@_Mutex@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(mutex_unlock, 4)
void __thiscall mutex_unlock(mutex *this)
{
    LeaveCriticalSection(this->mutex);
}

/* ?_Mutex_Lock@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_Lock@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_lock(mutex *m)
{
    mutex_lock(m);
}

/* ?_Mutex_Unlock@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_Unlock@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_unlock(mutex *m)
{
    mutex_unlock(m);
}

/* ?_Mutex_ctor@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_ctor@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_ctor(mutex *m)
{
    mutex_ctor(m);
}

/* ?_Mutex_dtor@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_dtor@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_dtor(mutex *m)
{
    mutex_dtor(m);
}

static CRITICAL_SECTION lockit_cs[_MAX_LOCK];

/* ?_Lockit_ctor@_Lockit@std@@SAXH@Z */
void __cdecl _Lockit_init(int locktype) {
    InitializeCriticalSection(&lockit_cs[locktype]);
    lockit_cs[locktype].DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": _Lockit critical section");
}

/* ?_Lockit_dtor@_Lockit@std@@SAXH@Z */
void __cdecl _Lockit_free(int locktype)
{
    lockit_cs[locktype].DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&lockit_cs[locktype]);
}

void init_lockit(void) {
    int i;

    for(i=0; i<_MAX_LOCK; i++)
        _Lockit_init(i);
}

void free_lockit(void) {
    int i;

    for(i=0; i<_MAX_LOCK; i++)
        _Lockit_free(i);
}

/* ?_Lockit_ctor@_Lockit@std@@CAXPAV12@H@Z */
/* ?_Lockit_ctor@_Lockit@std@@CAXPEAV12@H@Z */
void __cdecl _Lockit__Lockit_ctor_locktype(_Lockit *lockit, int locktype)
{
    lockit->locktype = locktype;
    EnterCriticalSection(&lockit_cs[locktype]);
}

/* ?_Lockit_ctor@_Lockit@std@@CAXPAV12@@Z */
/* ?_Lockit_ctor@_Lockit@std@@CAXPEAV12@@Z */
void __cdecl _Lockit__Lockit_ctor(_Lockit *lockit)
{
    _Lockit__Lockit_ctor_locktype(lockit, 0);
}

/* ??0_Lockit@std@@QAE@H@Z */
/* ??0_Lockit@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(_Lockit_ctor_locktype, 8)
_Lockit* __thiscall _Lockit_ctor_locktype(_Lockit *this, int locktype)
{
    _Lockit__Lockit_ctor_locktype(this, locktype);
    return this;
}

/* ??0_Lockit@std@@QAE@XZ */
/* ??0_Lockit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Lockit_ctor, 4)
_Lockit* __thiscall _Lockit_ctor(_Lockit *this)
{
    _Lockit__Lockit_ctor_locktype(this, 0);
    return this;
}

/* ?_Lockit_dtor@_Lockit@std@@CAXPAV12@@Z */
/* ?_Lockit_dtor@_Lockit@std@@CAXPEAV12@@Z */
void __cdecl _Lockit__Lockit_dtor(_Lockit *lockit)
{
    LeaveCriticalSection(&lockit_cs[lockit->locktype]);
}

/* ??1_Lockit@std@@QAE@XZ */
/* ??1_Lockit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Lockit_dtor, 4)
void __thiscall _Lockit_dtor(_Lockit *this)
{
    _Lockit__Lockit_dtor(this);
}

/* wctype */
unsigned short __cdecl wctype(const char *property)
{
    static const struct {
        const char *name;
        unsigned short mask;
    } properties[] = {
        { "alnum", _DIGIT|_ALPHA },
        { "alpha", _ALPHA },
        { "cntrl", _CONTROL },
        { "digit", _DIGIT },
        { "graph", _DIGIT|_PUNCT|_ALPHA },
        { "lower", _LOWER },
        { "print", _DIGIT|_PUNCT|_BLANK|_ALPHA },
        { "punct", _PUNCT },
        { "space", _SPACE },
        { "upper", _UPPER },
        { "xdigit", _HEX }
    };
    unsigned int i;

    for(i=0; i<sizeof(properties)/sizeof(properties[0]); i++)
        if(!strcmp(property, properties[i].name))
            return properties[i].mask;

    return 0;
}

typedef void (__cdecl *MSVCP_new_handler_func)(void);
static MSVCP_new_handler_func MSVCP_new_handler;
static int __cdecl new_handler_wrapper(MSVCP_size_t unused)
{
    MSVCP_new_handler();
    return 1;
}

/* ?set_new_handler@std@@YAP6AXXZP6AXXZ@Z */
MSVCP_new_handler_func __cdecl set_new_handler(MSVCP_new_handler_func new_handler)
{
    MSVCP_new_handler_func old_handler = MSVCP_new_handler;

    TRACE("%p\n", new_handler);

    MSVCP_new_handler = new_handler;
    MSVCRT_set_new_handler(new_handler ? new_handler_wrapper : NULL);
    return old_handler;
}

/* ?set_new_handler@std@@YAP6AXXZH@Z */
MSVCP_new_handler_func __cdecl set_new_handler_reset(int unused)
{
    return set_new_handler(NULL);
}
