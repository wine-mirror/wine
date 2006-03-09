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

static void test_doaction( void )
{
    MSIHANDLE hpkg;
    UINT r;

    r = MsiDoAction( -1, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiDoAction(hpkg, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiDoAction(0, "boo");
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiDoAction(hpkg, "boo");
    ok( r == ERROR_FUNCTION_NOT_CALLED, "wrong return val\n");

    MsiCloseHandle( hpkg );
}

static void test_gettargetpath_bad(void)
{
    char buffer[0x80];
    MSIHANDLE hpkg;
    DWORD sz;
    UINT r;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiGetTargetPath( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPath( 0, NULL, NULL, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPath( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPath( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPath( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    r = MsiGetTargetPath( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    MsiCloseHandle( hpkg );
}

static void test_settargetpath_bad(void)
{
    MSIHANDLE hpkg;
    UINT r;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiSetTargetPath( 0, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPath( 0, "boo", "C:\\bogusx" );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetTargetPath( hpkg, "boo", NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPath( hpkg, "boo", "c:\\bogusx" );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    MsiCloseHandle( hpkg );
}

static void test_condition(void)
{
    MSICONDITION r;
    MSIHANDLE hpkg;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiEvaluateCondition(0, NULL);
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, NULL);
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 <> 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 >= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 >=");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " ");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> LicView");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not LicView");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not \"A\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "~not \"A\"");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 and 2");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 and 3");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 and 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 or 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(0)");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(((((1))))))");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(((((1)))))");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" < \"B\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" > \"B\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"1\" > \"12\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"100\" < \"21\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 < > 0");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(1<<1) == 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" = \"a\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~ = \"a\" ");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~= \"a\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~= 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" = 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 ~= 1 ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 ~= \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 0 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 0 < \"100\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 100 > \"0\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 XOR 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 IMP 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 EQV 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 EQV 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMPL 1");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" >< \"S\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"s\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"sss\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "mm", "5" );

    r = MsiEvaluateCondition(hpkg, "mm = 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm < 6");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm <= 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm > 4");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm < 12");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm = \"5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 AND \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "3 >< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "3 >< 4");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 1 OR 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND 1 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND 0 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "_1 = _1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "( 1 AND 1 ) = 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT ( 1 AND 1 )");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT A AND (BBBBBBBBBB=2 OR CCC=1) AND Ddddddddd");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "Installed<>\"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 1 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael>0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael>=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael~<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "asdf" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0asdf" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0 " );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "-0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0000000000000" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "--0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0x00" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "-" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "+0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0.0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");
    r = MsiEvaluateCondition(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiCloseHandle( hpkg );
}

static BOOL check_prop_empty( MSIHANDLE hpkg, char * prop)
{
    UINT r;
    DWORD sz;
    char buffer[2];

    sz = sizeof buffer;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, prop, buffer, &sz );
    return r == ERROR_SUCCESS && buffer[0] == 0 && sz == 0;
}

static void test_props(void)
{
    MSIHANDLE hpkg;
    UINT r;
    DWORD sz;
    char buffer[0x100];

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    /* test invalid values */
    r = MsiGetProperty( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetProperty( hpkg, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetProperty( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiGetProperty( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    /* test retrieving an empty/nonexistent property */
    sz = sizeof buffer;
    r = MsiGetProperty( hpkg, "boo", NULL, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( sz == 0, "wrong size returned\n");

    check_prop_empty( hpkg, "boo");
    sz = 0;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"x"), "buffer was changed\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 0, "wrong size returned\n");

    /* set the property to something */
    r = MsiSetProperty( 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetProperty( hpkg, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetProperty( hpkg, "", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    /* try set and get some illegal property identifiers */
    r = MsiSetProperty( hpkg, "", "asdf" );
    ok( r == ERROR_FUNCTION_FAILED, "wrong return val\n");

    r = MsiSetProperty( hpkg, "=", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetProperty( hpkg, " ", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetProperty( hpkg, "'", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = MsiGetProperty( hpkg, "'", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"asdf"), "buffer was not changed\n");

    /* set empty values */
    r = MsiSetProperty( hpkg, "boo", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    r = MsiSetProperty( hpkg, "boo", "" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    /* set a non-empty value */
    r = MsiSetProperty( hpkg, "boo", "xyz" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"xyz"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"xy"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    MsiCloseHandle( hpkg );
}

START_TEST(package)
{
    test_createpackage();
    test_getsourcepath_bad();
    test_getsourcepath();
    test_doaction();
    test_gettargetpath_bad();
    test_settargetpath_bad();
    test_props();
    test_condition();
}
