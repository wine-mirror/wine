/*
 * msidb - command line tool for assembling MSI packages
 *
 * Copyright 2015 Erich E. Hoover
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

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <shlwapi.h>

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

struct msidb_listentry
{
    struct list entry;
    WCHAR *name;
};

struct msidb_state
{
    WCHAR *database_file;
    WCHAR *table_folder;
    MSIHANDLE database_handle;
    BOOL add_streams;
    BOOL extract_streams;
    BOOL kill_streams;
    BOOL create_database;
    BOOL import_tables;
    BOOL export_tables;
    BOOL short_filenames;
    struct list add_stream_list;
    struct list extract_stream_list;
    struct list kill_stream_list;
    struct list table_list;
};

static void list_free( struct list *list )
{
    struct msidb_listentry *data, *next;

    LIST_FOR_EACH_ENTRY_SAFE( data, next, list, struct msidb_listentry, entry )
    {
        list_remove( &data->entry );
        HeapFree( GetProcessHeap(), 0, data );
    }
}

static void list_append( struct list *list, WCHAR *name )
{
    struct msidb_listentry *data;

    data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct msidb_listentry) );
    if (!data)
    {
        ERR( "Out of memory for list.\n" );
        exit( 1 );
    }
    data->name = name;
    list_add_tail( list, &data->entry );
}

static void show_usage( void )
{
    WINE_MESSAGE(
        "Usage: msidb [OPTION]...[OPTION]... [TABLE]...[TABLE]\n"
        "Options:\n"
        "  -?                Show this usage message and exit.\n"
        "  -a file.cab       Add stream/cabinet file to _Streams table.\n"
        "  -c                Create database file (instead of opening existing file).\n"
        "  -d package.msi    Path to the database file.\n"
        "  -e                Export tables from database.\n"
        "  -f folder         Folder in which to open/save the tables.\n"
        "  -i                Import tables into database.\n"
        "  -k file.cab       Kill (remove) stream/cabinet file from _Streams table.\n"
        "  -s                Export with short filenames (eight character max).\n"
        "  -x file.cab       Extract stream/cabinet file from _Streams table.\n"
    );
}

static int valid_state( struct msidb_state *state )
{
    if (state->database_file == NULL)
    {
        FIXME( "GUI operation is not currently supported.\n" );
        return 0;
    }
    if (state->table_folder == NULL && !state->add_streams && !state->kill_streams
        && !state->extract_streams)
    {
        ERR( "No table folder specified (-f option).\n" );
        show_usage();
        return 0;
    }
    if (!state->create_database && !state->import_tables && !state->export_tables
        && !state->add_streams&& !state->kill_streams && !state->extract_streams)
    {
        ERR( "No mode flag specified (-a, -c, -e, -i, -k, -x).\n" );
        show_usage();
        return 0;
    }
    if (list_empty( &state->table_list ) && !state->add_streams && !state->kill_streams
        && !state->extract_streams)
    {
        ERR( "No tables specified.\n" );
        return 0;
    }
    return 1;
}

static int process_argument( struct msidb_state *state, int i, int argc, WCHAR *argv[] )
{
    /* msidb accepts either "-" or "/" style flags */
    if (lstrlenW(argv[i]) != 2 || (argv[i][0] != '-' && argv[i][0] != '/'))
        return 0;
    switch( argv[i][1] )
    {
    case '?':
        show_usage();
        exit( 0 );
    case 'a':
        if (i + 1 >= argc) return 0;
        state->add_streams = TRUE;
        list_append( &state->add_stream_list, argv[i + 1] );
        return 2;
    case 'c':
        state->create_database = TRUE;
        return 1;
    case 'd':
        if (i + 1 >= argc) return 0;
        state->database_file = argv[i + 1];
        return 2;
    case 'e':
        state->export_tables = TRUE;
        return 1;
    case 'f':
        if (i + 1 >= argc) return 0;
        state->table_folder = argv[i + 1];
        return 2;
    case 'i':
        state->import_tables = TRUE;
        return 1;
    case 'k':
        if (i + 1 >= argc) return 0;
        state->kill_streams = TRUE;
        list_append( &state->kill_stream_list, argv[i + 1] );
        return 2;
    case 's':
        state->short_filenames = TRUE;
        return 1;
    case 'x':
        if (i + 1 >= argc) return 0;
        state->extract_streams = TRUE;
        list_append( &state->extract_stream_list, argv[i + 1] );
        return 2;
    default:
        break;
    }
    show_usage();
    exit( 1 );
}

static int open_database( struct msidb_state *state )
{
    LPCWSTR db_mode = state->create_database ? MSIDBOPEN_CREATE : MSIDBOPEN_TRANSACT;
    UINT ret;

    ret = MsiOpenDatabaseW( state->database_file, db_mode, &state->database_handle );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to open database '%s', error %d\n", wine_dbgstr_w(state->database_file), ret );
        return 0;
    }
    return 1;
}

static void close_database( struct msidb_state *state, BOOL commit_changes )
{
    UINT ret = ERROR_SUCCESS;

    if (state->database_handle == 0)
        return;
    if (commit_changes)
        ret = MsiDatabaseCommit( state->database_handle );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to commit changes to database.\n" );
        return;
    }
    ret = MsiCloseHandle( state->database_handle );
    if (ret != ERROR_SUCCESS)
    {
        WARN( "Failed to close database handle.\n" );
        return;
    }
}

static const WCHAR *basenameW( const WCHAR *filename )
{
    const WCHAR *dir_end;

    dir_end = wcsrchr( filename, '/' );
    if (dir_end) return dir_end + 1;
    dir_end = wcsrchr( filename, '\\' );
    if (dir_end) return dir_end + 1;
    return filename;
}

static int add_stream( struct msidb_state *state, const WCHAR *stream_filename )
{
    static const WCHAR insert_command[] =
        {'I','N','S','E','R','T',' ','I','N','T','O',' ','_','S','t','r','e','a','m','s',' ',
         '(','N','a','m','e',',',' ','D','a','t','a',')',' ','V','A','L','U','E','S',' ','(','?',',',' ','?',')',0};
    MSIHANDLE view = 0, record = 0;
    UINT ret;

    ret = MsiDatabaseOpenViewW( state->database_handle, insert_command, &view );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to open _Streams table.\n" );
        goto cleanup;
    }
    record = MsiCreateRecord( 2 );
    if (record == 0)
    {
        ERR( "Failed to create MSI record.\n" );
        ret = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    ret = MsiRecordSetStringW( record, 1, basenameW( stream_filename ) );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to add stream filename to MSI record.\n" );
        goto cleanup;
    }
    ret = MsiRecordSetStreamW( record, 2, stream_filename );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to add stream to MSI record.\n" );
        goto cleanup;
    }
    ret = MsiViewExecute( view, record );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to add stream to _Streams table.\n" );
        goto cleanup;
    }

cleanup:
    if (record)
        MsiCloseHandle( record );
    if (view)
        MsiViewClose( view );

    return (ret == ERROR_SUCCESS);
}

static int add_streams( struct msidb_state *state )
{
    struct msidb_listentry *data;

    LIST_FOR_EACH_ENTRY( data, &state->add_stream_list, struct msidb_listentry, entry )
    {
        if (!add_stream( state, data->name ))
            return 0; /* failed, do not commit changes */
    }
    return 1;
}

static int kill_stream( struct msidb_state *state, const WCHAR *stream_filename )
{
    static const WCHAR delete_command[] =
        {'D','E','L','E','T','E',' ','F','R','O','M',' ','_','S','t','r','e','a','m','s',' ',
         'W','H','E','R','E',' ','N','a','m','e',' ','=',' ','?',0};
    MSIHANDLE view = 0, record = 0;
    UINT ret;

    ret = MsiDatabaseOpenViewW( state->database_handle, delete_command, &view );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to open _Streams table.\n" );
        goto cleanup;
    }
    record = MsiCreateRecord( 1 );
    if (record == 0)
    {
        ERR( "Failed to create MSI record.\n" );
        ret = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    ret = MsiRecordSetStringW( record, 1, stream_filename );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to add stream filename to MSI record.\n" );
        goto cleanup;
    }
    ret = MsiViewExecute( view, record );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to delete stream from _Streams table.\n" );
        goto cleanup;
    }

cleanup:
    if (record)
        MsiCloseHandle( record );
    if (view)
        MsiViewClose( view );

    return (ret == ERROR_SUCCESS);
}

static int kill_streams( struct msidb_state *state )
{
    struct msidb_listentry *data;

    LIST_FOR_EACH_ENTRY( data, &state->kill_stream_list, struct msidb_listentry, entry )
    {
        if (!kill_stream( state, data->name ))
            return 0; /* failed, do not commit changes */
    }
    return 1;
}

static int extract_stream( struct msidb_state *state, const WCHAR *stream_filename )
{
    static const WCHAR select_command[] =
        {'S','E','L','E','C','T',' ','D','a','t','a',' ','F','R','O','M',' ','_','S','t','r','e','a','m','s',' ',
         'W','H','E','R','E',' ','N','a','m','e',' ','=',' ','?',0};
    HANDLE file = INVALID_HANDLE_VALUE;
    MSIHANDLE view = 0, record = 0;
    DWORD read_size, write_size;
    char buffer[1024];
    UINT ret;

    file = CreateFileW( stream_filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    if (file == INVALID_HANDLE_VALUE)
    {
        ret = ERROR_FILE_NOT_FOUND;
        ERR( "Failed to open destination file %s.\n", wine_dbgstr_w(stream_filename) );
        goto cleanup;
    }
    ret = MsiDatabaseOpenViewW( state->database_handle, select_command, &view );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to open _Streams table.\n" );
        goto cleanup;
    }
    record = MsiCreateRecord( 1 );
    if (record == 0)
    {
        ERR( "Failed to create MSI record.\n" );
        ret = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    ret = MsiRecordSetStringW( record, 1, stream_filename );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to add stream filename to MSI record.\n" );
        goto cleanup;
    }
    ret = MsiViewExecute( view, record );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to query stream from _Streams table.\n" );
        goto cleanup;
    }
    MsiCloseHandle( record );
    record = 0;
    ret = MsiViewFetch( view, &record );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to query row from _Streams table.\n" );
        goto cleanup;
    }
    read_size = sizeof(buffer);
    while (read_size == sizeof(buffer))
    {
        ret = MsiRecordReadStream( record, 1, buffer, &read_size );
        if (ret != ERROR_SUCCESS)
        {
            ERR( "Failed to read stream from _Streams table.\n" );
            goto cleanup;
        }
        if (!WriteFile( file, buffer, read_size, &write_size, NULL ) || read_size != write_size)
        {
            ret = ERROR_WRITE_FAULT;
            ERR( "Failed to write stream to destination file.\n" );
            goto cleanup;
        }
    }

cleanup:
    if (record)
        MsiCloseHandle( record );
    if (view)
        MsiViewClose( view );
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle( file );
    return (ret == ERROR_SUCCESS);
}

static int extract_streams( struct msidb_state *state )
{
    struct msidb_listentry *data;

    LIST_FOR_EACH_ENTRY( data, &state->extract_stream_list, struct msidb_listentry, entry )
    {
        if (!extract_stream( state, data->name ))
            return 0; /* failed, do not commit changes */
    }
    return 1;
}

static int import_table( struct msidb_state *state, const WCHAR *table_path )
{
    UINT ret;

    ret = MsiDatabaseImportW( state->database_handle, state->table_folder, table_path );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to import table '%s', error %d.\n", wine_dbgstr_w(table_path), ret );
        return 0;
    }
    return 1;
}

static int import_tables( struct msidb_state *state )
{
    const WCHAR idt_ext[] = { '.','i','d','t',0 };
    const WCHAR wildcard[] = { '*',0 };
    struct msidb_listentry *data;

    LIST_FOR_EACH_ENTRY( data, &state->table_list, struct msidb_listentry, entry )
    {
        WCHAR *table_name = data->name;
        WCHAR table_path[MAX_PATH];
        WCHAR *ext;

        /* permit specifying tables with wildcards ('Feature*') */
        if (wcsstr( table_name, wildcard ) != NULL)
        {
            WIN32_FIND_DATAW f;
            HANDLE handle;
            WCHAR *path;
            DWORD len;

            len = lstrlenW( state->table_folder ) + 1 + lstrlenW( table_name ) + 1; /* %s/%s\0 */
            path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
            if (path == NULL)
                return 0;
            lstrcpyW( path, state->table_folder );
            PathAddBackslashW( path );
            lstrcatW( path, table_name );
            handle = FindFirstFileW( path, &f );
            HeapFree( GetProcessHeap(), 0, path );
            if (handle == INVALID_HANDLE_VALUE)
                return 0;
            do
            {
                if (f.cFileName[0] == '.' && !f.cFileName[1]) continue;
                if (f.cFileName[0] == '.' && f.cFileName[1] == '.' && !f.cFileName[2]) continue;
                if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                if ((ext = PathFindExtensionW( f.cFileName )) == NULL) continue;
                if (lstrcmpW( ext, idt_ext ) != 0) continue;
                if (!import_table( state, f.cFileName ))
                {
                    FindClose( handle );
                    return 0; /* failed, do not commit changes */
                }
            } while (FindNextFileW( handle, &f ));
            FindClose( handle );
            continue;
        }
        /* permit specifying tables by filename (*.idt) */
        if ((ext = PathFindExtensionW( table_name )) == NULL || lstrcmpW( ext, idt_ext ) != 0)
        {
            const WCHAR format[] = { '%','.','8','s','.','i','d','t',0 }; /* truncate to 8 characters */
            swprintf( table_path, ARRAY_SIZE(table_path), format, table_name );
            table_name = table_path;
        }
        if (!import_table( state, table_name ))
            return 0; /* failed, do not commit changes */
    }
    return 1;
}

static int export_table( struct msidb_state *state, const WCHAR *table_name )
{
    const WCHAR format_dos[] = { '%','.','8','s','.','i','d','t',0 }; /* truncate to 8 characters */
    const WCHAR format_full[] = { '%','s','.','i','d','t',0 };
    const WCHAR *format = (state->short_filenames ? format_dos : format_full);
    WCHAR table_path[MAX_PATH];
    UINT ret;

    swprintf( table_path, ARRAY_SIZE(table_path), format, table_name );
    ret = MsiDatabaseExportW( state->database_handle, table_name, state->table_folder, table_path );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to export table '%s', error %d.\n", wine_dbgstr_w(table_name), ret );
        return 0;
    }
    return 1;
}

static int export_all_tables( struct msidb_state *state )
{
    static const WCHAR summary_information[] =
        {'_','S','u','m','m','a','r','y','I','n','f','o','r','m','a','t','i','o','n',0};
    static const WCHAR query_command[] =
        {'S','E','L','E','C','T',' ','N','a','m','e',' ','F','R','O','M',' ','_','T','a','b','l','e','s',0};
    MSIHANDLE view = 0;
    UINT ret;

    ret = MsiDatabaseOpenViewW( state->database_handle, query_command, &view );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to open _Tables table.\n" );
        goto cleanup;
    }
    ret = MsiViewExecute( view, 0 );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to query list from _Tables table.\n" );
        goto cleanup;
    }
    while( 1 )
    {
        MSIHANDLE record = 0;
        WCHAR table[256];
        DWORD size;

        ret = MsiViewFetch( view, &record );
        if (ret == ERROR_NO_MORE_ITEMS)
            break;
        if (ret != ERROR_SUCCESS)
        {
            ERR( "Failed to query row from _Tables table.\n" );
            goto cleanup;
        }
        size = ARRAY_SIZE(table);
        ret = MsiRecordGetStringW( record, 1, table, &size );
        if (ret != ERROR_SUCCESS)
        {
            ERR( "Failed to retrieve name string.\n" );
            goto cleanup;
        }
        if (!export_table( state, table ))
        {
            ret = ERROR_FUNCTION_FAILED;
            goto cleanup;
        }
        ret = MsiCloseHandle( record );
        if (ret != ERROR_SUCCESS)
        {
            ERR( "Failed to close record handle.\n" );
            goto cleanup;
        }
    }
    ret = ERROR_SUCCESS;
    /* the _SummaryInformation table is not listed in _Tables */
    if (!export_table( state, summary_information ))
    {
        ret = ERROR_FUNCTION_FAILED;
        goto cleanup;
    }

cleanup:
    if (view && MsiViewClose( view ) != ERROR_SUCCESS)
    {
        ERR( "Failed to close _Streams table.\n" );
        return 0;
    }
    return (ret == ERROR_SUCCESS);
}

static int export_tables( struct msidb_state *state )
{
    const WCHAR wildcard[] = { '*',0 };
    struct msidb_listentry *data;

    LIST_FOR_EACH_ENTRY( data, &state->table_list, struct msidb_listentry, entry )
    {
        if (lstrcmpW( data->name, wildcard ) == 0)
        {
            if (!export_all_tables( state ))
                return 0; /* failed, do not commit changes */
            continue;
        }
        if (!export_table( state, data->name ))
            return 0; /* failed, do not commit changes */
    }
    return 1;
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    struct msidb_state state;
    int i, n = 1;
    int ret = 1;

    memset( &state, 0x0, sizeof(state) );
    list_init( &state.add_stream_list );
    list_init( &state.extract_stream_list );
    list_init( &state.kill_stream_list );
    list_init( &state.table_list );
    /* process and validate all the command line flags */
    for (i = 1; n && i < argc; i += n)
        n = process_argument( &state, i, argc, argv );
    /* process all remaining arguments as table names */
    for (; i < argc; i++)
        list_append( &state.table_list, argv[i] );
    if (!valid_state( &state ))
        goto cleanup;

    /* perform the requested operations */
    if (!open_database( &state ))
    {
        ERR( "Failed to open database '%s'.\n", wine_dbgstr_w(state.database_file) );
        goto cleanup;
    }
    if (state.add_streams && !add_streams( &state ))
        goto cleanup; /* failed, do not commit changes */
    if (state.export_tables && !export_tables( &state ))
        goto cleanup; /* failed, do not commit changes */
    if (state.extract_streams && !extract_streams( &state ))
        goto cleanup; /* failed, do not commit changes */
    if (state.import_tables && !import_tables( &state ))
        goto cleanup; /* failed, do not commit changes */
    if (state.kill_streams && !kill_streams( &state ))
        goto cleanup; /* failed, do not commit changes */
    ret = 0;

cleanup:
    close_database( &state, ret == 0 );
    list_free( &state.add_stream_list );
    list_free( &state.extract_stream_list );
    list_free( &state.kill_stream_list );
    list_free( &state.table_list );
    return ret;
}
