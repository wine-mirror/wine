/*
 * Copyright (C) 2005 Mike McCormack for CodeWeavers
 *
 * A test program for MSI database files.
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

#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

static void test_msidatabase(void)
{
    MSIHANDLE hdb = 0;
    CHAR szName[] = "C:\\mytest.msi";
    UINT res;

    DeleteFile(szName);

    /* create an empty database */
    res = MsiOpenDatabase(szName, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database" );
}

void test_msiinsert(void)
{
    const char *msifile = "winetest.msi";
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    char *query, buf[80];
    DWORD sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* insert a value into it */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Abe', '8675309')";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* check the record contains what we put in it */
    r = MsiRecordGetFieldCount(hrec);
    ok(r == 3, "record count wrong\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "field 1 contents wrong\n");
    sz = sizeof buf;
    r = MsiRecordGetString(hrec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 2 content fetch failed\n");
    ok(!strcmp(buf,"Abe"), "field 2 content incorrect\n");
    sz = sizeof buf;
    r = MsiRecordGetString(hrec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 3 content fetch failed\n");
    ok(!strcmp(buf,"8675309"), "field 3 content incorrect\n");
    
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFile(msifile);
    ok(r == TRUE, "file didn't exist after commit\n");
}

typedef UINT (WINAPI *fnMsiDecomposeDescriptorA)(LPCSTR, LPCSTR, LPSTR, LPSTR, DWORD *);
fnMsiDecomposeDescriptorA MsiDecomposeDescriptorA;

void test_msidecomposedesc(void)
{
    char prod[MAX_FEATURE_CHARS+1], comp[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1];
    char *desc;
    UINT r;
    DWORD len;
    HMODULE hmod;

    hmod = GetModuleHandle("msi.dll");
    if (!hmod)
        return;
    MsiDecomposeDescriptorA = (fnMsiDecomposeDescriptorA) 
        GetProcAddress(hmod, "MsiDecomposeDescriptorA");
    if (!MsiDecomposeDescriptorA)
        return;

    /* test a valid feature descriptor */
    desc = "']gAVn-}f(ZXfeAR6.jiFollowTheWhiteRabbit>3w2x^IGfe?CxI5heAvk.";
    len = 0;
    r = MsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_SUCCESS, "returned an error\n");
    ok(len == strlen(desc), "length was wrong\n");
    ok(strcmp(prod,"{90110409-6000-11D3-8CFE-0150048383C9}")==0, "product wrong\n");
    ok(strcmp(feature,"FollowTheWhiteRabbit")==0, "feature wrong\n");
    ok(strcmp(comp,"{A7CD68DB-EF74-49C8-FBB2-A7C463B2AC24}")==0,"component wrong\n");

    /* test an invalid feature descriptor with too many characters */
    desc = "']gAVn-}f(ZXfeAR6.ji"
           "ThisWillFailIfTheresMoreThanAGuidsChars>"
           "3w2x^IGfe?CxI5heAvk.";
    len = 0;
    r = MsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_INVALID_PARAMETER, "returned wrong error\n");

    /*
     * Test a valid feature descriptor with the
     * maximum number of characters and some trailing characters.
     */
    desc = "']gAVn-}f(ZXfeAR6.ji"
           "ThisWillWorkIfTheresLTEThanAGuidsChars>"
           "3w2x^IGfe?CxI5heAvk."
           "extra";
    len = 0;
    r = MsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_SUCCESS, "returned wrong error\n");
    ok(len == (strlen(desc) - strlen("extra")), "length wrong\n");
}

START_TEST(db)
{
    test_msidatabase();
    test_msiinsert();
    test_msidecomposedesc();
}
