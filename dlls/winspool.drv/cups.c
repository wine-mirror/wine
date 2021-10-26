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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
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
#include "wine/unicode.h"

#include "wspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

/* Temporary helpers until switch to unixlib */
#include "winnls.h"
#include "wine/heap.h"
#define malloc( sz ) heap_alloc( sz )
#define free( ptr ) heap_free( ptr )
static DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen )
{
    return MultiByteToWideChar( CP_UNIXCP, 0, src, srclen, dst, dstlen );
}
static int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict )
{
    /* FIXME: strict */
    return WideCharToMultiByte( CP_UNIXCP, 0, src, srclen, dst, dstlen, NULL, NULL );
}

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

static BOOL copy_file( const char *src, const char *dst )
{
    int fds[2] = { -1, -1 }, num;
    char buf[1024];
    BOOL ret = FALSE;

    fds[0] = open( src, O_RDONLY );
    fds[1] = open( dst, O_CREAT | O_TRUNC | O_WRONLY, 0666 );
    if (fds[0] == -1 || fds[1] == -1) goto fail;

    while ((num = read( fds[0], buf, sizeof(buf) )) != 0)
    {
        if (num == -1) goto fail;
        if (write( fds[1], buf, num ) != num) goto fail;
    }
    ret = TRUE;

fail:
    if (fds[1] != -1) close( fds[1] );
    if (fds[0] != -1) close( fds[0] );
    return ret;
}

static char *get_unix_file_name( LPCWSTR path )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG size = 256;
    char *buffer;

    nt_name.Buffer = (WCHAR *)path;
    nt_name.MaximumLength = nt_name.Length = lstrlenW( path ) * sizeof(WCHAR);
    InitializeObjectAttributes( &attr, &nt_name, 0, 0, NULL );
    for (;;)
    {
        if (!(buffer = malloc( size ))) return NULL;
        status = wine_nt_to_unix_file_name( &attr, buffer, &size, FILE_OPEN_IF );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        free( buffer );
    }
    if (status && status != STATUS_NO_SUCH_FILE)
    {
        free( buffer );
        return NULL;
    }
    return buffer;
}


#ifdef SONAME_LIBCUPS
static WCHAR *cups_get_optionW( const char *opt_name, int num_options, cups_option_t *options )
{
    const char *value;
    WCHAR *ret;
    int len;

    value = pcupsGetOption( opt_name, num_options, options );
    if (!value) return NULL;

    len = strlen( value ) + 1;
    ret = malloc( len * sizeof(WCHAR) );
    if (ret) ntdll_umbstowcs( value, len, ret, len );

    return ret;
}

static cups_ptype_t cups_get_printer_type( const cups_dest_t *dest )
{
    const char *value;
    cups_ptype_t ret;
    char *end;

    value = pcupsGetOption( "printer-type", dest->num_options, dest->options );
    if (!value) return 0;
    ret = (cups_ptype_t)strtoul( value, &end, 10 );
    if (*end) ret = 0;
    return ret;
}

static BOOL cups_is_scanner( cups_dest_t *dest )
{
    return cups_get_printer_type( dest ) & 0x2000000 /* CUPS_PRINTER_SCANNER */;
}

static http_status_t cupsGetPPD3_wrapper( http_t *http, const char *name, time_t *modtime,
                                          char *buffer, size_t bufsize )
{
    const char *ppd;

    if (pcupsGetPPD3) return pcupsGetPPD3( http, name, modtime, buffer, bufsize );
    if (!pcupsGetPPD) return HTTP_NOT_FOUND;

    TRACE( "No cupsGetPPD3 implementation, so calling cupsGetPPD\n" );

    *modtime = 0;
    ppd = pcupsGetPPD( name );

    TRACE( "cupsGetPPD returns %s\n", debugstr_a(ppd) );

    if (!ppd) return HTTP_NOT_FOUND;

    if (rename( ppd, buffer ) == -1)
    {
        BOOL res = copy_file( ppd, buffer );
        unlink( ppd );
        if (!res) return HTTP_NOT_FOUND;
    }
    return HTTP_OK;
}
#endif /* SONAME_LIBCUPS */

NTSTATUS unix_enum_printers( void *args )
{
    struct enum_printers_params *params = args;
#ifdef SONAME_LIBCUPS
    unsigned int num, i, name_len, comment_len, location_len, needed;
    WCHAR *comment, *location, *ptr;
    struct printer_info *info;
    cups_dest_t *dests;

    params->num = 0;
    if (!pcupsGetDests) return STATUS_NOT_SUPPORTED;

    num = pcupsGetDests( &dests );

    for (i = 0; i < num; i++)
    {
        if (cups_is_scanner( dests + i ))
        {
            TRACE( "Printer %d: %s - skipping scanner\n", i, debugstr_a( dests[i].name ) );
            continue;
        }
        TRACE( "Printer %d: %s\n", i, debugstr_a( dests[i].name ) );
        params->num++;
    }

    needed = sizeof( *info ) * params->num;
    info = params->printers;
    ptr = (WCHAR *)(info + params->num);

    for (i = 0; i < num; i++)
    {
        if (cups_is_scanner( dests + i )) continue;

        comment = cups_get_optionW( "printer-info", dests[i].num_options, dests[i].options );
        location = cups_get_optionW( "printer-location", dests[i].num_options, dests[i].options );

        name_len = strlen( dests[i].name ) + 1;
        comment_len = comment ? strlenW( comment ) + 1 : 0;
        location_len = location ? strlenW( location ) + 1 : 0;
        needed += (name_len + comment_len + location_len) * sizeof(WCHAR);

        if (needed <= params->size)
        {
            info->name = ptr;
            ntdll_umbstowcs( dests[i].name, name_len, info->name, name_len );
            info->comment = comment ? ptr + name_len : NULL;
            memcpy( info->comment, comment, comment_len * sizeof(WCHAR) );
            info->location = location ? ptr + name_len + comment_len : NULL;
            memcpy( info->location, location, location_len * sizeof(WCHAR) );
            info->is_default = dests[i].is_default;
            info++;
            ptr += name_len + comment_len + location_len;
        }
        free( comment );
        free( location );
    }
    pcupsFreeDests( num, dests );

    if (needed > params->size)
    {
        params->size = needed;
        return STATUS_BUFFER_OVERFLOW;
    }
    return STATUS_SUCCESS;
#else
    params->num = 0;
    return STATUS_NOT_SUPPORTED;
#endif /* SONAME_LIBCUPS */
}

NTSTATUS unix_get_ppd( void *args )
{
    struct get_ppd_params *params = args;
    char *unix_ppd = get_unix_file_name( params->ppd );
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "(%s, %s)\n", debugstr_w( params->printer ), debugstr_w( params->ppd ) );

    if (!unix_ppd) return STATUS_NO_SUCH_FILE;

    if (!params->printer) /* unlink */
    {
        unlink( unix_ppd );
    }
    else
    {
#ifdef SONAME_LIBCUPS
        http_status_t http_status;
        time_t modtime = 0;
        char *printer_name;
        int len;

        len = strlenW( params->printer );
        printer_name = malloc( len * 3 + 1 );
        ntdll_wcstoumbs( params->printer, len + 1, printer_name, len * 3 + 1, FALSE );

        http_status = cupsGetPPD3_wrapper( 0, printer_name, &modtime, unix_ppd, strlen( unix_ppd ) + 1 );
        if (http_status != HTTP_OK)
        {
            unlink( unix_ppd );
            status = STATUS_DEVICE_UNREACHABLE;
        }
        free( printer_name );
#else
        status = STATUS_NOT_SUPPORTED;
#endif
    }
    free( unix_ppd );
    return status;
}
