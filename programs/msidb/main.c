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

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

struct msidb_state
{
    WCHAR *database_file;
    WCHAR *table_folder;
    MSIHANDLE database_handle;
    BOOL create_database;
};

static void show_usage( void )
{
    WINE_MESSAGE(
        "Usage: msidb [OPTION]...[OPTION]...\n"
        "Options:\n"
        "  -?                Show this usage message and exit.\n"
        "  -c                Create database file (instead of opening existing file).\n"
        "  -d package.msi    Path to the database file.\n"
        "  -f folder         Folder in which to open/save the tables.\n"
    );
}

static int valid_state( struct msidb_state *state )
{
    if (state->database_file == NULL)
    {
        FIXME( "GUI operation is not currently supported.\n" );
        return 0;
    }
    if (state->table_folder == NULL)
    {
        ERR( "No table folder specified (-f option).\n" );
        show_usage();
        return 0;
    }
    return 1;
}

static int process_argument( struct msidb_state *state, int i, int argc, WCHAR *argv[] )
{
    /* msidb accepts either "-" or "/" style flags */
    if (strlenW(argv[i]) != 2 || (argv[i][0] != '-' && argv[i][0] != '/'))
    {
        WINE_FIXME( "Table names are not currently supported.\n" );
        show_usage();
        exit( 1 );
    }
    switch( argv[i][1] )
    {
    case '?':
        show_usage();
        exit( 0 );
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

static void close_database( struct msidb_state *state )
{
    UINT ret;

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

int wmain( int argc, WCHAR *argv[] )
{
    struct msidb_state state;
    int i, n = 1;

    memset( &state, 0x0, sizeof(state) );
    /* process and validate all the command line flags */
    for (i = 1; n && i < argc; i += n)
        n = process_argument( &state, i, argc, argv );
    if (!valid_state( &state ))
        return 1;

    /* perform the requested operations */
    if (!open_database( &state ))
    {
        ERR( "Failed to open database '%s'.\n", wine_dbgstr_w(state.database_file) );
        return 1;
    }
    close_database( &state );
    return 0;
}
