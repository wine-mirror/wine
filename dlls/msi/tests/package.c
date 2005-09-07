/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

static UINT run_query( MSIHANDLE hdb, const char *query )
{
    MSIHANDLE hview = 0;
    UINT r;

    r = MsiDatabaseOpenView(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, 0);
    if( r == ERROR_SUCCESS )
        r = MsiViewClose(hview);
    MsiCloseHandle(hview);
    return r;
}

static UINT set_summary_info(MSIHANDLE hdb)
{
    UINT res;
    MSIHANDLE suminfo;

    /* build summmary info */
    res = MsiGetSummaryInformation(hdb, NULL, 7, &suminfo);
    ok( res == ERROR_SUCCESS , "Failed to open summaryinfo\n" );

    res = MsiSummaryInfoSetProperty(suminfo,2, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,3, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,4, VT_LPSTR, 0,NULL,
                        "Wine Hackers");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,7, VT_LPSTR, 0,NULL,
                    ";1033");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,9, VT_LPSTR, 0,NULL,
                    "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 15, VT_I4, 0, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoPersist(suminfo);
    ok( res == ERROR_SUCCESS , "Failed to make summary info persist\n" );

    res = MsiCloseHandle( suminfo);
    ok( res == ERROR_SUCCESS , "Failed to close suminfo\n" );

    return res;
}


MSIHANDLE create_package_db(void)
{
    MSIHANDLE hdb = 0;
    CHAR szName[] = "winetest.msi";
    UINT res;

    DeleteFile(szName);

    /* create an empty database */
    res = MsiOpenDatabase(szName, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = set_summary_info(hdb);

    res = run_query( hdb,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
    ok( res == ERROR_SUCCESS , "Failed to create directory table\n" );

    return hdb;
}

MSIHANDLE package_from_db(MSIHANDLE hdb)
{
    UINT res;
    CHAR szPackage[10];
    MSIHANDLE hPackage;

    sprintf(szPackage,"#%li",hdb);
    res = MsiOpenPackage(szPackage,&hPackage);
    ok( res == ERROR_SUCCESS , "Failed to open package\n" );

    res = MsiCloseHandle(hdb);
    ok( res == ERROR_SUCCESS , "Failed to close db handle\n" );

    return hPackage;
}

static void test_createpackage(void)
{
    MSIHANDLE hPackage = 0;
    UINT res;

    hPackage = package_from_db(create_package_db());
    ok( hPackage != 0, " Failed to create package\n");

    res = MsiCloseHandle( hPackage);
    ok( res == ERROR_SUCCESS , "Failed to close package\n" );
}

static void test_getsourcepath_bad( void )
{
    static const char str[] = { 0 };
    char buffer[0x80];
    DWORD sz;
    UINT r;

    r = MsiGetSourcePath( -1, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, NULL, buffer, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, str, NULL, &sz );
    ok( r == ERROR_INVALID_HANDLE, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, str, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, str, buffer, &sz );
    ok( r == ERROR_INVALID_HANDLE, "return value wrong\n");
}

static UINT add_directory_entry( MSIHANDLE hdb, char *values )
{
    char insert[] = "INSERT INTO `Directory` (`Directory`,`Directory_Parent`,`DefaultDir`) VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static void test_getsourcepath( void )
{
    static const char str[] = { 0 };
    char buffer[0x80];
    DWORD sz;
    UINT r;
    MSIHANDLE hpkg, hdb;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    sz = 0;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, str, buffer, &sz );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");
    ok( buffer[0] == 'x', "buffer modified\n");

    sz = 1;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, str, buffer, &sz );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");
    ok( buffer[0] == 'x', "buffer modified\n");

    MsiCloseHandle( hpkg );


    /* another test but try create a directory this time */
    hdb = create_package_db();
    ok( hdb, "failed to create database\n");
    
    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == S_OK, "failed\n");

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    sz = sizeof buffer -1;
    strcpy(buffer,"x bad");
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    todo_wine {
    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");
    }
    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    todo_wine {
    sz = sizeof buffer -1;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    ok( sz == strlen(buffer), "returned length wrong\n");

    sz = 0;
    strcpy(buffer,"x bad");
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "return value wrong\n");
    }
    ok( buffer[0] == 'x', "buffer modified\n");

    todo_wine {
    r = MsiGetSourcePath( hpkg, "TARGETDIR", NULL, NULL );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    }

    r = MsiGetSourcePath( hpkg, "TARGETDIR ", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "targetdir", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    todo_wine {
    r = MsiGetSourcePath( hpkg, "TARGETDIR", NULL, &sz );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    }

    MsiCloseHandle( hpkg );
}

START_TEST(package)
{
    test_createpackage();
    test_getsourcepath_bad();
    test_getsourcepath();
}
