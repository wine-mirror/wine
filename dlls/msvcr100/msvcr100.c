/*
 * msvcr100 specific functions
 *
 * Copyright 2010 Detlef Riekenberg
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

#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#define INVALID_PMT(x,err)   (*_errno() = (err), _invalid_parameter(NULL, NULL, NULL, 0, 0))
#define CHECK_PMT_ERR(x,err) ((x) || (INVALID_PMT( 0, (err) ), FALSE))
#define CHECK_PMT(x)         CHECK_PMT_ERR((x), EINVAL)

 /*********************************************************************
 *		wmemcpy_s (MSVCR100.@)
 */
int CDECL wmemcpy_s(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, (unsigned long)numberOfElements, src, (unsigned long)count);

    if (!count)
        return 0;

    if (!CHECK_PMT(dest != NULL)) return EINVAL;

    if (!CHECK_PMT(src != NULL)) {
        memset(dest, 0, numberOfElements*sizeof(wchar_t));
        return EINVAL;
    }
    if (!CHECK_PMT_ERR(count <= numberOfElements, ERANGE)) {
        memset(dest, 0, numberOfElements*sizeof(wchar_t));
        return ERANGE;
    }

    memcpy(dest, src, sizeof(wchar_t)*count);
    return 0;
}

 /*********************************************************************
 *		wmemmove_s (MSVCR100.@)
 */
int CDECL wmemmove_s(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, (unsigned long)numberOfElements, src, (unsigned long)count);

    if (!count)
        return 0;

    /* Native does not seem to conform to 6.7.1.2.3 in
     * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1225.pdf
     * in that it does not zero the output buffer on constraint violation.
     */
    if (!CHECK_PMT(dest != NULL)) return EINVAL;
    if (!CHECK_PMT(src != NULL)) return EINVAL;
    if (!CHECK_PMT_ERR(count <= numberOfElements, ERANGE)) return ERANGE;

    memmove(dest, src, sizeof(wchar_t)*count);
    return 0;
}

/*********************************************************************
 *  DllMain (MSVCR100.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
        _set_printf_count_output(0);
    }
    return TRUE;
}
