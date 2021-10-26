/*
 * CUPS functions
 *
 * Copyright 2021 Huw Davies
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
#include <string.h>
#ifdef HAVE_CUPS_CUPS_H
#include <cups/cups.h>
#endif
#ifdef HAVE_CUPS_PPD_H
#include <cups/ppd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "winspool.h"
#include "ddk/winsplp.h"
#include "wine/debug.h"

#include "wspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

#ifdef SONAME_LIBCUPS

void *libcups_handle = NULL;

#define CUPS_FUNCS \
    DO_FUNC(cupsAddOption); \
    DO_FUNC(cupsFreeDests); \
    DO_FUNC(cupsFreeOptions); \
    DO_FUNC(cupsGetDests); \
    DO_FUNC(cupsGetOption); \
    DO_FUNC(cupsParseOptions); \
    DO_FUNC(cupsPrintFile)
#define CUPS_OPT_FUNCS \
    DO_FUNC(cupsGetNamedDest); \
    DO_FUNC(cupsGetPPD); \
    DO_FUNC(cupsGetPPD3); \
    DO_FUNC(cupsLastErrorString)

#define DO_FUNC(f) typeof(f) *p##f = NULL
CUPS_FUNCS;
#undef DO_FUNC
cups_dest_t * (*pcupsGetNamedDest)(http_t *, const char *, const char *) = NULL;
const char *  (*pcupsGetPPD)(const char *) = NULL;
http_status_t (*pcupsGetPPD3)(http_t *, const char *, time_t *, char *, size_t) = NULL;
const char *  (*pcupsLastErrorString)(void) = NULL;

#endif /* SONAME_LIBCUPS */

NTSTATUS unix_process_attach( void *arg )
{
#ifdef SONAME_LIBCUPS
    libcups_handle = dlopen( SONAME_LIBCUPS, RTLD_NOW );
    TRACE( "%p: %s loaded\n", libcups_handle, SONAME_LIBCUPS );
    if (!libcups_handle) return STATUS_DLL_NOT_FOUND;

#define DO_FUNC(x) \
    p##x = dlsym( libcups_handle, #x ); \
    if (!p##x) \
    { \
        ERR( "failed to load symbol %s\n", #x ); \
        libcups_handle = NULL; \
        return STATUS_ENTRYPOINT_NOT_FOUND; \
    }
    CUPS_FUNCS;
#undef DO_FUNC
#define DO_FUNC(x) p##x = dlsym( libcups_handle, #x )
    CUPS_OPT_FUNCS;
#undef DO_FUNC
    return STATUS_SUCCESS;
#else /* SONAME_LIBCUPS */
    return STATUS_NOT_SUPPORTED;
#endif /* SONAME_LIBCUPS */
}
