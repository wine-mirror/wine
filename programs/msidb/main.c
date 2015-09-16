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
#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/debug.h"
#include "wine/unicode.h"
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
    BOOL create_database;
    BOOL import_tables;
    struct list add_stream_list;
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
        "  -f folder         Folder in which to open/save the tables.\n"
        "  -i                Import tables into database.\n"
    );
}

static int valid_state( struct msidb_state *state )
{
    if (state->database_file == NULL)
    {
        FIXME( "GUI operation is not currently supported.\n" );
        return 0;
    }
    if (state->table_folder == NULL && !state->add_streams)
    {
        ERR( "No table folder specified (-f option).\n" );
        show_usage();
        return 0;
    }
    if (!state->create_database && !state->import_tables && !state->add_streams)
    {
        ERR( "No mode flag specified (-a, -c, -i).\n" );
        show_usage();
        return 0;
    }
    if (list_empty( &state->table_list ) && !state->add_streams)
    {
        ERR( "No tables specified.\n" );
        return 0;
    }
    return 1;
}

static int process_argument( struct msidb_state *state, int i, int argc, WCHAR *argv[] )
{
    /* msidb accepts either "-" or "/" style flags */
    if (strlenW(argv[i]) != 2 || (argv[i][0] != '-' && argv[i][0] != '/'))
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
    case 'f':
        if (i + 1 >= argc) return 0;
        state->table_folder = argv[i + 1];
        return 2;
    case 'i':
        state->import_tables = TRUE;
        return 1;
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

    dir_end = strrchrW( filename, '/' );
    if (dir_end) return dir_end + 1;
    dir_end = strrchrW( filename, '\\' );
    if (dir_end) return dir_end + 1;
    return filename;
}

static int add_stream( struct msidb_state *state, const WCHAR *stream_filename )
{
    static const char insert_command[] = "INSERT INTO _Streams (Name, Data) VALUES (?, ?)";
    MSIHANDLE view = 0, record = 0;
    UINT ret;

    ret = MsiDatabaseOpenViewA( state->database_handle, insert_command, &view );
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

static int import_table( struct msidb_state *state, const WCHAR *table_name )
{
    const WCHAR format[] = { '%','.','8','s','.','i','d','t',0 }; /* truncate to 8 characters */
    WCHAR table_path[MAX_PATH];
    UINT ret;

    snprintfW( table_path, sizeof(table_path)/sizeof(WCHAR), format, table_name );
    ret = MsiDatabaseImportW( state->database_handle, state->table_folder, table_path );
    if (ret != ERROR_SUCCESS)
    {
        ERR( "Failed to import table '%s', error %d.\n", wine_dbgstr_w(table_name), ret );
        return 0;
    }
    return 1;
}

static int import_tables( struct msidb_state *state )
{
    struct msidb_listentry *data;

    LIST_FOR_EACH_ENTRY( data, &state->table_list, struct msidb_listentry, entry )
    {
        if (!import_table( state, data->name ))
            return 0; /* failed, do not commit changes */
    }
    return 1;
}

int wmain( int argc, WCHAR *argv[] )
{
    struct msidb_state state;
    int i, n = 1;
    int ret = 1;

    memset( &state, 0x0, sizeof(state) );
    list_init( &state.add_stream_list );
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
    if (state.import_tables && !import_tables( &state ))
        goto cleanup; /* failed, do not commit changes */
    ret = 0;

cleanup:
    close_database( &state, ret == 0 );
    list_free( &state.add_stream_list );
    list_free( &state.table_list );
    return ret;
}
