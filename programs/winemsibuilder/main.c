/*
 * winemsibuilder - tool to build MSI packages
 *
 * Copyright 2010 Hans Leidekker for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <objbase.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winemsibuilder);

static UINT open_database( const WCHAR *msifile, MSIHANDLE *handle )
{
    UINT r;
    MSIHANDLE hdb;

    if (GetFileAttributesW( msifile ) == INVALID_FILE_ATTRIBUTES)
    {
        r = MsiOpenDatabaseW( msifile, MSIDBOPEN_CREATE, &hdb );
        if (r != ERROR_SUCCESS)
        {
            WINE_ERR( "failed to create package database %s (%u)\n", wine_dbgstr_w(msifile), r );
            return r;
        }
        r = MsiDatabaseCommit( hdb );
        if (r != ERROR_SUCCESS)
        {
            WINE_ERR( "failed to commit database (%u)\n", r );
            MsiCloseHandle( hdb );
            return r;
        }
    }
    else
    {
        r = MsiOpenDatabaseW( msifile, MSIDBOPEN_TRANSACT, &hdb );
        if (r != ERROR_SUCCESS)
        {
            WINE_ERR( "failed to open package database %s (%u)\n", wine_dbgstr_w(msifile), r );
            return r;
        }
    }

    *handle = hdb;
    return ERROR_SUCCESS;
}

static int import_tables( const WCHAR *msifile, WCHAR **tables )
{
    UINT r;
    MSIHANDLE hdb;
    WCHAR *dir;
    DWORD len;

    r = open_database( msifile, &hdb );
    if (r != ERROR_SUCCESS) return 1;

    len = GetCurrentDirectoryW( 0, NULL );
    if (!(dir = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
    {
        MsiCloseHandle( hdb );
        return 1;
    }
    GetCurrentDirectoryW( len + 1, dir );

    while (*tables)
    {
        r = MsiDatabaseImportW( hdb, dir, *tables );
        if (r != ERROR_SUCCESS)
        {
            WINE_ERR( "failed to import table %s (%u)\n", wine_dbgstr_w(*tables), r );
            break;
        }
        tables++;
    }

    if (r == ERROR_SUCCESS)
    {
        r = MsiDatabaseCommit( hdb );
        if (r != ERROR_SUCCESS)
            WINE_ERR( "failed to commit changes (%u)\n", r );
    }

    HeapFree( GetProcessHeap(), 0, dir );
    MsiCloseHandle( hdb );
    return (r != ERROR_SUCCESS);
}

/* taken from dlls/msi/table.c */
static int utf2mime( int x )
{
    if (x >= '0' && x <= '9')
        return x - '0';
    if (x >= 'A' && x <= 'Z')
        return x - 'A' + 10;
    if (x >= 'a' && x <= 'z')
        return x - 'a' + 10 + 26;
    if (x == '.')
        return 10 + 26 + 26;
    if (x == '_')
        return 10 + 26 + 26 + 1;
    return -1;
}

#define MAX_STREAM_NAME 0x1f

static WCHAR *encode_stream( const WCHAR *in )
{
    DWORD c, next, count;
    WCHAR *out, *p;

    count = lstrlenW( in );
    if (count > MAX_STREAM_NAME)
        return NULL;

    count += 2;
    if (!(out = HeapAlloc( GetProcessHeap(), 0, count * sizeof(WCHAR) ))) return NULL;
    p = out;
    while (count--)
    {
        c = *in++;
        if (!c)
        {
            *p = c;
            return out;
        }
        if (c < 0x80 && utf2mime( c ) >= 0)
        {
            c = utf2mime( c ) + 0x4800;
            next = *in;
            if (next && next < 0x80)
            {
                next = utf2mime( next );
                if (next != -1)
                {
                     next += 0x3ffffc0;
                     c += next << 6;
                     in++;
                }
            }
        }
        *p++ = c;
    }
    HeapFree( GetProcessHeap(), 0, out );
    return NULL;
}

static int add_stream( const WCHAR *msifile, const WCHAR *stream, const WCHAR *file )
{
    UINT r;
    HRESULT hr;
    MSIHANDLE hdb;
    IStorage *stg;
    IStream *stm = NULL;
    HANDLE handle;
    char buffer[4096];
    ULARGE_INTEGER size;
    DWORD low, high, read;
    WCHAR *encname;
    int ret = 1;

    /* make sure we have the right type of file  */
    r = open_database( msifile, &hdb );
    if (r != ERROR_SUCCESS) return 1;
    MsiCloseHandle( hdb );

    hr = StgOpenStorage( msifile, NULL, STGM_TRANSACTED|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &stg );
    if (hr != S_OK)
    {
        WINE_WARN( "failed to open storage %s (0x%08x)\n", wine_dbgstr_w(msifile), hr );
        return 1;
    }
    encname = encode_stream( stream );
    if (!encname)
    {
        WINE_WARN( "failed to encode stream name %s\n", wine_dbgstr_w(stream) );
        goto done;
    }
    hr = IStorage_CreateStream( stg, encname, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
    if (hr != S_OK)
    {
        WINE_WARN( "failed to create stream %s (0x%08x)\n", wine_dbgstr_w(encname), hr );
        goto done;
    }
    handle = CreateFileW( file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        WINE_WARN( "failed to open file %s (%u)\n", wine_dbgstr_w(file), GetLastError() );
        goto done;
    }
    low = GetFileSize( handle, &high );
    if (low == INVALID_FILE_SIZE || high)
    {
        WINE_WARN( "file %s too big\n", wine_dbgstr_w(file) );
        CloseHandle( handle );
        goto done;
    }
    size.QuadPart = low;
    hr = IStream_SetSize( stm, size );
    if (hr != S_OK) goto done;

    while (ReadFile( handle, buffer, sizeof(buffer), &read, NULL ) && read)
    {
        hr = IStream_Write( stm, buffer, read, NULL );
        if (hr != S_OK) break;
        size.QuadPart -= read;
    }
    CloseHandle( handle );
    if (size.QuadPart)
    {
        WINE_WARN( "failed to write stream contents\n" );
        goto done;
    }
    IStorage_Commit( stg, 0 );
    ret = 0;

done:
    HeapFree( GetProcessHeap(), 0, encname );
    if (stm) IStream_Release( stm );
    IStorage_Release( stg );
    return ret;
}

static void show_usage( void )
{
    WINE_MESSAGE(
        "Usage: winemsibuilder [OPTION] [MSIFILE] ...\n"
        "Options:\n"
        "  -i package.msi table1.idt [table2.idt ...]    Import one or more tables into the database.\n"
        "  -a package.msi stream file                    Add 'stream' to storage with contents of 'file'.\n"
        "\nExisting tables or streams will be overwritten. If package.msi does not exist a new file\n"
        "will be created with an empty database.\n"
    );
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    if (argc < 3 || argv[1][0] != '-')
    {
        show_usage();
        return 1;
    }

    switch (argv[1][1])
    {
    case 'i':
        if (argc < 4) break;
        return import_tables( argv[2], argv + 3 );
    case 'a':
        if (argc < 5) break;
        return add_stream( argv[2], argv[3], argv[4] );
    default:
        WINE_WARN( "unknown option\n" );
        break;
    }

    show_usage();
    return 1;
}
