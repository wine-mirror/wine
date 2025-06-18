/*
 * Copyright 2024 Hans Leidekker for CodeWeavers
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
#include <unistd.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <consoleapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sort);

static BOOL option_unique;
static BOOL option_c_locale;
static BOOL option_reverse;
static int  option_column;

struct line
{
    unsigned int start; /* offset into input buffer */
    unsigned int len;
};

struct sorted_lines
{
    unsigned int count;     /* number of lines */
    unsigned int capacity;  /* size of line array */
    struct line *entry;     /* sorted line array */
    char *buf;              /* input buffer */
    unsigned int bufsize;
};

static inline int compare_string( const char *str, unsigned int len, const char *str2, unsigned int len2 )
{
    unsigned int i;

    for (i = 0; i < min( len, len2 ); i++)
    {
        char a = tolower( str[i] ), b = tolower( str2[i] );
        if (a > b) return 1;
        if (a < b) return -1;
    }
    if (len > len2) return 1;
    if (len < len2) return -1;
    return 0;
}

static BOOL find_line( const struct sorted_lines *sorted, const char *line, unsigned int len, int *idx )
{
    int i, c, min = 0, max = sorted->count - 1;
    unsigned int offset;
    const char *ptr;

    while (min <= max)
    {
        i = (min + max) / 2;
        ptr = sorted->buf + sorted->entry[i].start;

        if (option_column && option_column < len && option_column < sorted->entry[i].len) offset = option_column - 1;
        else offset = 0;

        if (option_c_locale)
            c = compare_string( ptr + offset, sorted->entry[i].len - offset, line + offset, len - offset );
        else
            c = CompareStringA( LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                ptr + offset, sorted->entry[i].len - offset, line + offset, len - offset ) - 2;
        if (option_reverse) c = -c;

        if (c > 0) max = i - 1;
        else if (c < 0) min = i + 1;
        else
        {
            *idx = i;
            return TRUE;
        }
    }
    *idx = max + 1;
    return FALSE;
}

static int insert_line( struct sorted_lines *sorted, const char *start, unsigned int len )
{
    int idx;

    if (find_line( sorted, start, len, &idx ))
    {
        if (option_unique) return 0;
        idx++;
    }
    if (sorted->count >= sorted->capacity)
    {
        sorted->capacity = max( 1024, sorted->capacity * 2 );
        if (!(sorted->entry = realloc( sorted->entry, sorted->capacity * sizeof(*sorted->entry) )))
        {
            wprintf( L"Out of memory.\n" );
            return 1;
        }
    }
    memmove( &sorted->entry[idx] + 1, &sorted->entry[idx], (sorted->count - idx) * sizeof(*sorted->entry) );
    sorted->entry[idx].start = start - sorted->buf;
    sorted->entry[idx].len = len;
    sorted->count++;
    return 0;
}

static int sort_lines( struct sorted_lines *sorted )
{
    const char *p = sorted->buf, *q = sorted->buf;
    unsigned int size = sorted->bufsize;

    while (size)
    {
        if (p[0] == '\n' || (p[0] == '\r' && p[1] == '\n'))
        {
            if (p[0] == '\r') { p++; size--; }
            if (insert_line( sorted, q, p - q  + 1 )) return 1;
            q = p + 1;
        }
        p++; size--;
    }
    if (p != q && insert_line( sorted, q, p - q )) return 1;
    return 0;
}

static int write_lines( const struct sorted_lines *sorted, HANDLE handle )
{
    unsigned int i;

    for (i = 0; i < sorted->count; i++)
    {
        const char *ptr = sorted->buf + sorted->entry[i].start;
        DWORD count;

        if (handle == GetStdHandle( STD_OUTPUT_HANDLE ) &&
            WriteConsoleA( handle, ptr, sorted->entry[i].len, &count, NULL )) continue;

        if (!WriteFile( handle, ptr, sorted->entry[i].len, &count, NULL )) return 1;
    }
    return 0;
}

#define MAX_LINE 4096
static char *read_input( HANDLE handle, unsigned int *ret_size )
{
    DWORD buflen = 1024, offset = 0, count;
    char *ret = malloc( buflen ), *line = malloc( MAX_LINE );
    const char *ctrl_z = NULL;

    *ret_size = 0;
    if (!ret || !line)
    {
        wprintf( L"Out of memory.\n" );
        return NULL;
    }
    for (;;)
    {
        count = 0;
        if (ReadConsoleA( handle, line, MAX_LINE, &count, NULL ))
        {
            if ((ctrl_z = memchr( line, 0x1a, count ))) count = ctrl_z - line;
        }
        else ReadFile( handle, line, MAX_LINE, &count, NULL );
        if (!count) break;

        if (buflen - offset < count + 2)
        {
            buflen = max( buflen * 2, offset + count + 2 );
            if (!(ret = realloc( ret, buflen )))
            {
                wprintf( L"Out of memory.\n" );
                return NULL;
            }
        }
        memcpy( ret + offset, line, count );
        offset += count;
        if (ctrl_z)
        {
            memcpy( ret + offset, "\r\n", 2 );
            offset += 2;
            break;
        }
    }
    *ret_size = offset;
    return ret;
}

static char no_data[] = "";

static char *map_file( HANDLE handle, unsigned int *ret_size )
{
    HANDLE mapping;
    char *ret;

    if (!(*ret_size = GetFileSize( handle, NULL ))) return no_data;
    if (!(mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, 0, NULL ))) return NULL;
    ret = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    return ret;
}

static HANDLE create_tmpfile( WCHAR *tmpfile )
{
    WCHAR tmpdir[MAX_PATH];

    GetTempPathW( ARRAY_SIZE(tmpdir), tmpdir );
    GetTempFileNameW( tmpdir, L"srt", 0, tmpfile );
    return CreateFileW( tmpfile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
}

static int sort_file( const WCHAR *in_file, const WCHAR *out_file )
{
    HANDLE in_handle, out_handle;
    struct sorted_lines sorted = {0};
    WCHAR tmpfile[MAX_PATH] = {0};
    int ret = 1;

    if (!in_file)
    {
        in_handle = GetStdHandle( STD_INPUT_HANDLE );
        if (!(sorted.buf = read_input( in_handle, &sorted.bufsize )))
        {
            wprintf( L"Can't read input.\n" );
            return 1;
        }
    }
    else
    {
        in_handle = CreateFileW( in_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        if (in_handle == INVALID_HANDLE_VALUE)
        {
            wprintf( L"Can't open input file.\n" );
            return 1;
        }
        if (!(sorted.buf = map_file( in_handle, &sorted.bufsize )))
        {
            wprintf( L"Can't read input.\n" );
            CloseHandle( in_handle );
            return 1;
        }
    }

    if (!out_file) out_handle = GetStdHandle( STD_OUTPUT_HANDLE );
    else
    {
        out_handle = CreateFileW( out_file, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL );
        if (out_handle == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() != ERROR_SHARING_VIOLATION)
            {
                wprintf( L"Can't open output file.\n" );
                CloseHandle( in_handle );
                return 1;
            }
            out_handle = create_tmpfile( tmpfile );
            if (out_handle == INVALID_HANDLE_VALUE)
            {
                wprintf( L"Can't open temporary output file.\n" );
                CloseHandle( in_handle );
                return 1;
            }
        }
    }

    if (!(ret = sort_lines( &sorted ))) ret = write_lines( &sorted, out_handle );

    if (!in_file) free( sorted.buf );
    else if (sorted.buf != no_data) UnmapViewOfFile( sorted.buf );
    CloseHandle( in_handle );
    CloseHandle( out_handle );

    if (tmpfile[0])
    {
        if (ret) DeleteFileW( tmpfile );
        else if (!MoveFileExW( tmpfile, out_file, MOVEFILE_REPLACE_EXISTING ))
        {
            wprintf( L"Can't move temporary output file.\n" );
            return 1;
        }
    }
    return ret;
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    const WCHAR *in_file = NULL, *out_file = NULL, *locale = NULL;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            if (!wcsicmp( argv[i] + 1, L"unique" )) option_unique = TRUE;
            else if (!wcsicmp( argv[i] + 1, L"r" ) || !wcsicmp( argv[i] + 1, L"reverse" )) option_reverse = TRUE;
            else if (!wcsicmp( argv[i] + 1, L"l" ) || !wcsicmp( argv[i] + 1, L"locale" ))
            {
                if (argc < i + 2)
                {
                    wprintf( L"Expected locale specifier.\n" );
                    return 1;
                }
                locale = argv[i + 1];
            }
            else if (!wcsicmp( argv[i] + 1, L"o" ))
            {
                if (argc < i + 2)
                {
                    wprintf( L"Expected output filename.\n" );
                    return 1;
                }
                out_file = argv[i + 1];
            }
            else if (argv[i][1] == '+')
            {
                WCHAR *end;
                if ((option_column = wcstol( argv[i] + 2, &end, 10 )) <= 0)
                {
                    wprintf( L"Expected column greater than 0.\n" );
                    return 1;
                }
            }
            else
            {
                FIXME( "Unsupported option %s.\n", debugstr_w(argv[i]) );
                return 1;
            }
        }
        else if (argv[i] != out_file && argv[i] != locale) in_file = argv[i];
    }

    if (locale)
    {
        if (wcscmp( locale, L"C" ))
        {
            wprintf( L"Invalid locale specifier.\n" );
            return 1;
        }
        option_c_locale = TRUE;
    }

    return sort_file( in_file, out_file );
}
