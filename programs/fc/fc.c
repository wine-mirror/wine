/*
 * Copyright 2018 Fabian Maurer
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

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <consoleapi.h>

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(fc);

static BOOL option_case_insensitive;

enum section_type
{
    SECTION_TYPE_COPY,
    SECTION_TYPE_DELETE,
    SECTION_TYPE_INSERT
};

struct section
{
    struct list entry;
    enum section_type type;
    unsigned int start;     /* index in line array */
    unsigned int len;       /* number of lines */
};
static struct list sections = LIST_INIT( sections );

struct common_subseq
{
    unsigned int pos_first;  /* position in first file */
    unsigned int pos_second;
    unsigned int len;
};

struct line
{
    const char  *start;
    unsigned int len;
};

struct lines
{
    unsigned int count;
    unsigned int capacity;
    struct line *entry;
};
static struct lines lines1, lines2;

static void append_line( struct lines *lines, const char *start, unsigned int len )
{
    if (lines->count >= lines->capacity)
    {
        lines->capacity = max( 128, lines->capacity * 2 );
        lines->entry = realloc( lines->entry, lines->capacity * sizeof(*lines->entry) );
    }
    lines->entry[lines->count].start = start;
    lines->entry[lines->count].len = len;
    lines->count++;
}

static void split_lines( struct lines *lines, const char *data, unsigned int len )
{
    const char *p = data, *q = data;

    while (len)
    {
        if (p[0] == '\n' || (p[0] == '\r' && p[1] == '\n'))
        {
            append_line( lines, q, p - q );
            if (p[0] == '\r') { p++; len--; }
            q = p + 1;
        }
        p++; len--;
    }
    if (p != q) append_line( lines, q, p - q );
}

static char *data1, *data2;
static unsigned int len_data1, len_data2;

static int equal_lines( const struct line *line1, const struct line *line2 )
{
    if (line1->len != line2->len) return 0;
    if (option_case_insensitive) return !memicmp( line1->start, line2->start, line1->len );
    else return !memcmp( line1->start, line2->start, line1->len );
}

/* return the number of equal lines at given ranges */
static unsigned int common_length( unsigned int first_pos, unsigned int first_end,
                                   unsigned int second_pos, unsigned int second_end )
{
    unsigned int len = 0;

    while (first_pos < first_end && second_pos < second_end)
    {
        if (!equal_lines( &lines1.entry[first_pos++], &lines2.entry[second_pos++] )) break;
        len++;
    }
    return len;
}

/* return the longest sequence of equal lines between given ranges */
static int longest_common_subsequence( unsigned int first_start, unsigned int first_end,
                                       unsigned int second_start, unsigned int second_end,
                                       struct common_subseq *result )
{
    unsigned int i, j;
    int ret = 0;

    memset( result, 0, sizeof(*result) );

    for (i = first_start; i < first_end; i++)
    {
        for (j = second_start; j < second_end; j++)
        {
            unsigned int len = common_length( i, first_end, j, second_end );
            if (len > result->len)
            {
                result->len = len;
                result->pos_first = i;
                result->pos_second = j;
                ret = 1;
            }
        }
    }
    return ret;
}

static struct section *alloc_section( enum section_type type, unsigned int start, unsigned int len )
{
    struct section *ret = malloc( sizeof(*ret) );

    ret->type = type;
    ret->start = start;
    ret->len = len;
    return ret;
}

static unsigned int non_matching_lines;

static void diff( unsigned int first_start, unsigned int first_end,
                  unsigned int second_start, unsigned int second_end )
{
    struct common_subseq subseq;
    struct section *section;

    if (longest_common_subsequence( first_start, first_end, second_start, second_end, &subseq ))
    {
        /* recursively find sections before this one */
        diff( first_start, subseq.pos_first, second_start, subseq.pos_second );

        section = alloc_section( SECTION_TYPE_COPY, subseq.pos_first, subseq.len );
        list_add_tail( &sections, &section->entry );

        /* recursively find sections after this one */
        diff( subseq.pos_first + subseq.len, first_end, subseq.pos_second + subseq.len, second_end );
        return;
    }

    if (first_start < first_end)
    {
        section = alloc_section( SECTION_TYPE_DELETE, first_start, first_end - first_start );
        list_add_tail( &sections, &section->entry );
        non_matching_lines++;
    }

    if (second_start < second_end)
    {
        section = alloc_section( SECTION_TYPE_INSERT, second_start, second_end - second_start );
        list_add_tail( &sections, &section->entry );
        non_matching_lines++;
    }
}

static struct section *find_context_before( struct section *section )
{
    struct section *cursor = section;

    while ((cursor = LIST_ENTRY( list_prev(&sections, &cursor->entry), struct section, entry )))
    {
        if (cursor->type == SECTION_TYPE_COPY) return cursor;
    }
    return NULL;
}

static struct section *find_context_after( struct section *section )
{
    struct section *cursor = section;

    while ((cursor = LIST_ENTRY( list_next(&sections, &cursor->entry), struct section, entry )))
    {
        if (cursor->type == SECTION_TYPE_COPY) return cursor;
    }
    return NULL;
}

static void output_stringW( const WCHAR *str, int len )
{
    DWORD count;
    int lenA;
    char *strA;

    if (len < 0) len = wcslen( str );
    if (WriteConsoleW( GetStdHandle(STD_OUTPUT_HANDLE), str, len, &count, NULL )) return;

    lenA = WideCharToMultiByte( GetOEMCP(), 0, str, len, NULL, 0, NULL, NULL );
    if ((strA = malloc( lenA )))
    {
        WideCharToMultiByte( GetOEMCP(), 0, str, len, strA, lenA, NULL, NULL );
        WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), strA, lenA, &count, FALSE );
        free( strA );
    }
}

static void output_stringA( const char *str, int len )
{
    DWORD count;

    if (len < 0) len = strlen( str );
    if (WriteConsoleA( GetStdHandle(STD_OUTPUT_HANDLE), str, len, &count, NULL )) return;
    WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), str, len, &count, FALSE );
}

static void output_header( const WCHAR *filename )
{
    output_stringW( L"***** ", 6 );
    output_stringW( filename, -1 );
    output_stringW( L"\r\n", 2 );
}

static void output_line( const struct lines *lines, unsigned int index )
{
    output_stringA( lines->entry[index].start, lines->entry[index].len );
    output_stringA( "\r\n", 2 );
}

static void output_context( unsigned int line )
{
    if (lines1.count < 2 || lines2.count < 2) return;
    output_line( &lines1, line );
}

static char no_data[] = "";

static char *map_file( HANDLE handle, unsigned int *size )
{
    HANDLE mapping;
    char *ret;

    if (!(*size = GetFileSize( handle, NULL ))) return no_data;
    if (!(mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, 0, NULL ))) return NULL;
    ret = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    return ret;
}

static int compare_files( const WCHAR *filename1, const WCHAR *filename2 )
{
    HANDLE handle1, handle2;
    struct section *section;
    unsigned int i;
    int ret = 2;

    handle1 = CreateFileW( filename1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (handle1 == INVALID_HANDLE_VALUE) return 2;

    handle2 = CreateFileW( filename2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (handle2 == INVALID_HANDLE_VALUE)
    {
        CloseHandle( handle2 );
        return 2;
    }

    if (!(data1 = map_file( handle1, &len_data1 ))) goto done;
    if (!(data2 = map_file( handle2, &len_data2 ))) goto done;

    split_lines( &lines1, data1, len_data1 );
    split_lines( &lines2, data2, len_data2 );

    diff( 0, lines1.count, 0, lines2.count );

    output_stringW( L"Comparing files ", 16 );
    output_stringW( filename1, -1 );
    output_stringW( L" and ", 5 );
    output_stringW( filename2, -1 );
    output_stringW( L"\r\n", 2 );

    if (!non_matching_lines)
    {
        output_stringW( L"FC: no differences encountered\r\n\r\n", -1 );
        ret = 0;
        goto done;
    }

    LIST_FOR_EACH_ENTRY( section, &sections, struct section, entry )
    {
        struct section *ctx_before = find_context_before( section ), *ctx_after = find_context_after( section );

        if (section->type == SECTION_TYPE_DELETE)
        {
            struct section *next_section = LIST_ENTRY( list_next(&sections, &section->entry), struct section, entry );

            output_header( filename1 );
            if (ctx_before) output_context( ctx_before->start + ctx_before->len - 1 );
            for (i = 0; i < section->len; i++) output_line( &lines1, section->start + i );
            if (ctx_after) output_context( ctx_after->start );

            if (next_section && next_section->type != SECTION_TYPE_INSERT)
            {
                output_header( filename2 );
                if (ctx_before) output_context( ctx_before->start + ctx_before->len - 1 );
                if (ctx_after) output_context( ctx_after->start );
            }
            else if (!next_section)
            {
                output_header( filename2 );
                output_stringW( L"*****\r\n\r\n", 9 );
            }
        }
        else if (section->type == SECTION_TYPE_INSERT)
        {
            struct section *prev_section = LIST_ENTRY( list_prev(&sections, &section->entry), struct section, entry );

            if (prev_section && prev_section->type != SECTION_TYPE_DELETE)
            {
                output_header( filename1 );
                if (ctx_before) output_context( ctx_before->start + ctx_before->len - 1 );
                if (ctx_after) output_context( ctx_after->start );
            }
            else if (!prev_section) output_header( filename1 );

            output_header( filename2 );
            if (ctx_before) output_context( ctx_before->start + ctx_before->len - 1 );
            for (i = 0; i < section->len; i++) output_line( &lines2, section->start + i );
            if (ctx_after) output_context( ctx_after->start );
            output_stringW( L"*****\r\n\r\n", 9 );
        }
    }

    ret = 1;

done:
    if (data1 != no_data) UnmapViewOfFile( data1 );
    if (data2 != no_data) UnmapViewOfFile( data2 );
    CloseHandle( handle1 );
    CloseHandle( handle2 );
    return ret;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    BOOL unsupported = FALSE;
    const WCHAR *filename1 = NULL, *filename2 = NULL;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            if (!wcsicmp( argv[i] + 1, L"c" )) option_case_insensitive = TRUE;
            else
            {
                FIXME( "option %s not supported\n", debugstr_w(argv[i]) );
                unsupported = TRUE;
            }
        }
        else if (!filename1) filename1 = argv[i];
        else if (!filename2) filename2 = argv[i];
        else
        {
            wprintf( L"FC: Wrong number of files %s\n", argv[i] );
            return 2;
        }
    }

    if (unsupported) return 2;

    if (!filename1 || !filename2)
    {
        wprintf( L"FC: Wrong number of files\n" );
        return 2;
    }

    if (wcschr( filename1, '?' ) || wcschr( filename1, '*' ) || wcschr( filename2, '?' ) || wcschr( filename2, '*' ))
    {
        FIXME( "wildcards not supported\n" );
        return 2;
    }

    return compare_files( filename1, filename2 );
}
