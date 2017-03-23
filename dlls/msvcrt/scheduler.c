/*
 * msvcrt.dll C++ objects
 *
 * Copyright 2017 Piotr Caban
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

#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "msvcrt.h"
#include "cppexcept.h"
#include "cxx.h"

#if _MSVCR_VER >= 100

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct {
    const vtable_ptr *vtable;
} Context;

/* ?CurrentContext@Context@Concurrency@@SAPAV12@XZ */
/* ?CurrentContext@Context@Concurrency@@SAPEAV12@XZ */
Context* __cdecl Context_CurrentContext(void)
{
    FIXME("()\n");
    return NULL;
}

/* ?Id@Context@Concurrency@@SAIXZ */
unsigned int __cdecl Context_Id(void)
{
    FIXME("()\n");
    return -1;
}

/* ?Block@Context@Concurrency@@SAXXZ */
void __cdecl Context_Block(void)
{
    FIXME("()\n");
}

/* ?Yield@Context@Concurrency@@SAXXZ */
void __cdecl Context_Yield(void)
{
    FIXME("()\n");
}

/* ?_SpinYield@Context@Concurrency@@SAXXZ */
void __cdecl Context__SpinYield(void)
{
    FIXME("()\n");
}

/* ?IsCurrentTaskCollectionCanceling@Context@Concurrency@@SA_NXZ */
MSVCRT_bool __cdecl Context_IsCurrentTaskCollectionCanceling(void)
{
    FIXME("()\n");
    return FALSE;
}

/* ?Oversubscribe@Context@Concurrency@@SAX_N@Z */
void __cdecl Context_Oversubscribe(MSVCRT_bool begin)
{
    FIXME("(%x)\n", begin);
}

/* ?ScheduleGroupId@Context@Concurrency@@SAIXZ */
unsigned int __cdecl Context_ScheduleGroupId(void)
{
    FIXME("()\n");
    return -1;
}

/* ?VirtualProcessorId@Context@Concurrency@@SAIXZ */
unsigned int __cdecl Context_VirtualProcessorId(void)
{
    FIXME("()\n");
    return -1;
}

#endif /* _MSVCR_VER >= 100 */
