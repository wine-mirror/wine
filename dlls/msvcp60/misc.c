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


static CRITICAL_SECTION lockit_cs;

void init_lockit(void) {
    InitializeCriticalSection(&lockit_cs);
    lockit_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": _Lockit critical section");
}

void free_lockit(void) {
    lockit_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&lockit_cs);
}

_Lockit* __thiscall _Lockit_ctor_locktype(_Lockit *this, int locktype)
{
    EnterCriticalSection(&lockit_cs);
    return this;
}

/* ??0_Lockit@std@@QAE@XZ */
/* ??0_Lockit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Lockit_ctor, 4)
_Lockit* __thiscall _Lockit_ctor(_Lockit *this)
{
    return _Lockit_ctor_locktype(this, 0);
}

/* ??1_Lockit@std@@QAE@XZ */
/* ??1_Lockit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Lockit_dtor, 4)
void __thiscall _Lockit_dtor(_Lockit *this)
{
    LeaveCriticalSection(&lockit_cs);
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
