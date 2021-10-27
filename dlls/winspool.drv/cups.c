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
#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#ifdef HAVE_CUPS_CUPS_H
#include <cups/cups.h>
#endif
#ifdef HAVE_CUPS_PPD_H
#include <cups/ppd.h>
#endif

#ifdef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H
#define GetCurrentProcess GetCurrentProcess_Mac
#define GetCurrentThread GetCurrentThread_Mac
#define LoadResource LoadResource_Mac
#define AnimatePalette AnimatePalette_Mac
#define EqualRgn EqualRgn_Mac
#define FillRgn FillRgn_Mac
#define FrameRgn FrameRgn_Mac
#define GetPixel GetPixel_Mac
#define InvertRgn InvertRgn_Mac
#define LineTo LineTo_Mac
#define OffsetRgn OffsetRgn_Mac
#define PaintRgn PaintRgn_Mac
#define Polygon Polygon_Mac
#define ResizePalette ResizePalette_Mac
#define SetRectRgn SetRectRgn_Mac
#define EqualRect EqualRect_Mac
#define FillRect FillRect_Mac
#define FrameRect FrameRect_Mac
#define GetCursor GetCursor_Mac
#define InvertRect InvertRect_Mac
#define OffsetRect OffsetRect_Mac
#define PtInRect PtInRect_Mac
#define SetCursor SetCursor_Mac
#define SetRect SetRect_Mac
#define ShowCursor ShowCursor_Mac
#define UnionRect UnionRect_Mac
#include <ApplicationServices/ApplicationServices.h>
#undef GetCurrentProcess
#undef GetCurrentThread
#undef LoadResource
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef InvertRect
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef UnionRect
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
#include "wine/unixlib.h"

#include "wspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

static const WCHAR CUPS_Port[] = { 'C','U','P','S',':',0 };
static const WCHAR LPR_Port[] = { 'L','P','R',':',0 };

#ifdef SONAME_LIBCUPS

static void *libcups_handle;

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

#define DO_FUNC(f) static typeof(f) *p##f
CUPS_FUNCS;
#undef DO_FUNC
static cups_dest_t * (*pcupsGetNamedDest)(http_t *, const char *, const char *);
static const char *  (*pcupsGetPPD)(const char *);
static http_status_t (*pcupsGetPPD3)(http_t *, const char *, time_t *, char *, size_t);
static const char *  (*pcupsLastErrorString)(void);

#endif /* SONAME_LIBCUPS */

static NTSTATUS process_attach( void *args )
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

/*****************************************************************************
 *          get_cups_jobs_ticket_options
 *
 * Explicitly set CUPS options based on any %cupsJobTicket lines.
 * The CUPS scheduler only looks for these in Print-File requests, and since
 * cupsPrintFile uses Create-Job / Send-Document, the ticket lines don't get
 * parsed.
 */
static int get_cups_job_ticket_options( const char *file, int num_options, cups_option_t **options )
{
    FILE *fp = fopen( file, "r" );
    char buf[257]; /* DSC max of 256 + '\0' */
    const char *ps_adobe = "%!PS-Adobe-";
    const char *cups_job = "%cupsJobTicket:";

    if (!fp) return num_options;
    if (!fgets( buf, sizeof(buf), fp )) goto end;
    if (strncmp( buf, ps_adobe, strlen( ps_adobe ) )) goto end;
    while (fgets( buf, sizeof(buf), fp ))
    {
        if (strncmp( buf, cups_job, strlen( cups_job ) )) break;
        num_options = pcupsParseOptions( buf + strlen( cups_job ), num_options, options );
    }

end:
    fclose( fp );
    return num_options;
}

static int get_cups_default_options( const char *printer, int num_options, cups_option_t **options )
{
    cups_dest_t *dest;
    int i;

    if (!pcupsGetNamedDest) return num_options;

    dest = pcupsGetNamedDest( NULL, printer, NULL );
    if (!dest) return num_options;

    for (i = 0; i < dest->num_options; i++)
    {
        if (!pcupsGetOption( dest->options[i].name, num_options, *options ))
            num_options = pcupsAddOption( dest->options[i].name, dest->options[i].value,
                                          num_options, options );
    }

    pcupsFreeDests( 1, dest );
    return num_options;
}
#endif /* SONAME_LIBCUPS */

static NTSTATUS enum_printers( void *args )
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

static NTSTATUS get_ppd( void *args )
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

static NTSTATUS get_default_page_size( void *args )
{
#ifdef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H
    struct get_default_page_size_params *params = args;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PMPrintSession session = NULL;
    PMPageFormat format = NULL;
    CFStringRef paper_name;
    PMPaper paper;
    CFRange range;
    int size;

    if (PMCreateSession( &session )) goto end;
    if (PMCreatePageFormat( &format )) goto end;
    if (PMSessionDefaultPageFormat( session, format )) goto end;
    if (PMGetPageFormatPaper( format, &paper )) goto end;
    if (PMPaperGetPPDPaperName( paper, &paper_name )) goto end;

    range.location = 0;
    range.length = CFStringGetLength( paper_name );
    size = (range.length + 1) * sizeof(WCHAR);

    if (params->name_size >= size)
    {
        CFStringGetCharacters( paper_name, range, (UniChar*)params->name );
        params->name[range.length] = 0;
        status = STATUS_SUCCESS;
    }
    else
        status = STATUS_BUFFER_OVERFLOW;
    params->name_size = size;

end:
    if (format) PMRelease( format );
    if (session) PMRelease( session );
    return status;
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

/*****************************************************************************
 *          schedule_pipe
 */
static BOOL schedule_pipe( const WCHAR *cmd, const WCHAR *filename )
{
    char *unixname, *cmdA;
    DWORD len;
    int fds[2] = { -1, -1 }, file_fd = -1, no_read;
    BOOL ret = FALSE;
    char buf[1024];
    pid_t pid, wret;
    int status;

    if (!(unixname = get_unix_file_name( filename ))) return FALSE;

    len = strlenW( cmd );
    cmdA = malloc( len * 3 + 1);
    ntdll_wcstoumbs( cmd, len + 1, cmdA, len * 3 + 1, FALSE );

    TRACE( "printing with: %s\n", cmdA );

    if ((file_fd = open( unixname, O_RDONLY )) == -1) goto end;

    if (pipe( fds ))
    {
        ERR( "pipe() failed!\n" );
        goto end;
    }

    if ((pid = fork()) == 0)
    {
        close( 0 );
        dup2( fds[0], 0 );
        close( fds[1] );

        /* reset signals that we previously set to SIG_IGN */
        signal( SIGPIPE, SIG_DFL );

        execl( "/bin/sh", "/bin/sh", "-c", cmdA, NULL );
        _exit( 1 );
    }
    else if (pid == -1)
    {
        ERR( "fork() failed!\n" );
        goto end;
    }

    close( fds[0] );
    fds[0] = -1;
    while ((no_read = read( file_fd, buf, sizeof(buf) )) > 0)
        write( fds[1], buf, no_read );

    close( fds[1] );
    fds[1] = -1;

    /* reap child */
    do {
        wret = waitpid( pid, &status, 0 );
    } while (wret < 0 && errno == EINTR);
    if (wret < 0)
    {
        ERR( "waitpid() failed!\n" );
        goto end;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status))
    {
        ERR( "child process failed! %d\n", status );
        goto end;
    }

    ret = TRUE;

end:
    if (file_fd != -1) close( file_fd );
    if (fds[0] != -1) close( fds[0] );
    if (fds[1] != -1) close( fds[1] );

    free( cmdA );
    free( unixname );
    return ret;
}

/*****************************************************************************
 *          schedule_unixfile
 */
static BOOL schedule_unixfile( const WCHAR *output, const WCHAR *filename )
{
    char *unixname, *outputA;
    DWORD len;
    BOOL ret;

    if (!(unixname = get_unix_file_name( filename ))) return FALSE;

    len = strlenW( output );
    outputA = malloc( len * 3 + 1);
    ntdll_wcstoumbs( output, len + 1, outputA, len * 3 + 1, FALSE );

    ret = copy_file( unixname, outputA );

    free( outputA );
    free( unixname );
    return ret;
}

/*****************************************************************************
 *          schedule_lpr
 */
static BOOL schedule_lpr( const WCHAR *printer_name, const WCHAR *filename )
{
    static const WCHAR lpr[] = { 'l','p','r',' ','-','P','\'' };
    static const WCHAR quote[] = { '\'',0 };
    int printer_len = strlenW( printer_name );
    WCHAR *cmd;
    BOOL ret;

    cmd = malloc( printer_len * sizeof(WCHAR) + sizeof(lpr) + sizeof(quote) );
    memcpy( cmd, lpr, sizeof(lpr) );
    memcpy( cmd + ARRAY_SIZE(lpr), printer_name, printer_len * sizeof(WCHAR) );
    memcpy( cmd + ARRAY_SIZE(lpr) + printer_len, quote, sizeof(quote) );

    ret = schedule_pipe( cmd, filename );

    free( cmd );
    return ret;
}

/*****************************************************************************
 *          schedule_cups
 */
static BOOL schedule_cups( const WCHAR *printer_name, const WCHAR *filename, const WCHAR *document_title )
{
#ifdef SONAME_LIBCUPS
    if (pcupsPrintFile)
    {
        char *unixname, *queue, *unix_doc_title;
        cups_option_t *options = NULL;
        int num_options = 0, i;
        DWORD len;
        BOOL ret;

        if (!(unixname = get_unix_file_name( filename ))) return FALSE;
        len = strlenW( printer_name );
        queue = malloc( len * 3 + 1);
        ntdll_wcstoumbs( printer_name, len + 1, queue, len * 3 + 1, FALSE );

        len = strlenW( document_title );
        unix_doc_title = malloc( len * 3 + 1 );
        ntdll_wcstoumbs( document_title, len + 1, unix_doc_title, len + 3 + 1, FALSE );

        num_options = get_cups_job_ticket_options( unixname, num_options, &options );
        num_options = get_cups_default_options( queue, num_options, &options );

        TRACE( "printing via cups with options:\n" );
        for (i = 0; i < num_options; i++)
            TRACE( "\t%d: %s = %s\n", i, options[i].name, options[i].value );

        ret = pcupsPrintFile( queue, unixname, unix_doc_title, num_options, options );
        if (ret == 0 && pcupsLastErrorString)
            WARN( "cupsPrintFile failed with error %s\n", debugstr_a( pcupsLastErrorString() ) );

        pcupsFreeOptions( num_options, options );

        free( unix_doc_title );
        free( queue );
        free( unixname );
        return ret;
    }
    else
#endif
    {
        return schedule_lpr( printer_name, filename );
    }
}

static NTSTATUS schedule_job( void *args )
{
    struct schedule_job_params *params = args;

    if (params->wine_port[0] == '|')
        return schedule_pipe( params->wine_port + 1, params->filename );

    if (params->wine_port[0])
        return schedule_unixfile( params->wine_port, params->filename );

    if (!strncmpW( params->port, LPR_Port, ARRAY_SIZE(LPR_Port) - 1 ))
        return schedule_lpr( params->port + ARRAY_SIZE(LPR_Port) - 1, params->filename );

    if (!strncmpW( params->port, CUPS_Port, ARRAY_SIZE(CUPS_Port) - 1 ))
        return schedule_cups( params->port + ARRAY_SIZE(CUPS_Port) - 1, params->filename, params->document_title );

    return FALSE;
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    enum_printers,
    get_default_page_size,
    get_ppd,
    schedule_job,
};
