/*
 * Copyright 2025 Yuxuan Shui for CodeWeavers
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

#include <corecrt_startup.h>

extern _PIFV __xi_a[];
extern _PIFV __xi_z[];
extern _PVFV __xc_a[];
extern _PVFV __xc_z[];
extern _PVFV __xt_a[];
extern _PVFV __xt_z[];

static inline int fallback_initterm_e(_PIFV *table, _PIFV *end)
{
#if _MSVCR_VER < 80
    int res;
    for (res = 0; !res && table < end; table++)
        if (*table) res = (*table)();
    return res;
#else
    return _initterm_e(table, end);
#endif
}

static __cdecl void do_global_dtors(void)
{
    _initterm(__xt_a, __xt_z);
}

static void do_global_ctors(void)
{
    if (fallback_initterm_e(__xi_a, __xi_z) != 0) return;
    _initterm(__xc_a, __xc_z);

#ifdef _UCRT
    _crt_atexit(do_global_dtors);
#else
    _onexit((_onexit_t)do_global_dtors);
#endif
}
