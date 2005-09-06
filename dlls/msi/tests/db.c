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
#include <stdio.h>

#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

static const char *msifile = "winetest.msi";

static void test_msidatabase(void)
{
    MSIHANDLE hdb = 0;
    UINT res;

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = DeleteFile( msifile );
    ok( res == TRUE, "Falled to delete database\n" );
}

static void test_msiinsert(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    const char *query;
    char buf[80];
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
    r = MsiCloseHandle(hrec);
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

static void test_msidecomposedesc(void)
{
    char prod[MAX_FEATURE_CHARS+1], comp[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1];
    const char *desc;
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

static UINT try_query_param( MSIHANDLE hdb, LPCSTR szQuery, MSIHANDLE hrec )
{
    MSIHANDLE htab = 0;
    UINT res;

    res = MsiDatabaseOpenView( hdb, szQuery, &htab );
    if(res == ERROR_SUCCESS )
    {
        UINT r;

        r = MsiViewExecute( htab, hrec );
        if(r != ERROR_SUCCESS )
            res = r;

        r = MsiViewClose( htab );
        if(r != ERROR_SUCCESS )
            res = r;

        r = MsiCloseHandle( htab );
        if(r != ERROR_SUCCESS )
            res = r;
    }
    return res;
}

static UINT try_query( MSIHANDLE hdb, LPCSTR szQuery )
{
    return try_query_param( hdb, szQuery, 0 );
}

static UINT try_insert_query( MSIHANDLE hdb, LPCSTR szQuery )
{
    MSIHANDLE hrec = 0;
    UINT r;

    hrec = MsiCreateRecord( 1 );
    MsiRecordSetString( hrec, 1, "Hello");

    r = try_query_param( hdb, szQuery, hrec );

    MsiCloseHandle( hrec );
    return r;
}

static void test_msibadqueries(void)
{
    MSIHANDLE hdb = 0;
    UINT r;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database\n");

    /* open it readonly */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_READONLY, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/o\n");

    /* add a table to it */
    r = try_query( hdb, "select * from _Tables");
    ok(r == ERROR_SUCCESS , "query 1 failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database r/o\n");

    /* open it read/write */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_TRANSACT, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/w\n");

    /* a bunch of test queries that fail with the native MSI */

    r = try_query( hdb, "CREATE TABLE");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2a return code\n");

    r = try_query( hdb, "CREATE TABLE `a`");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2b return code\n");

    r = try_query( hdb, "CREATE TABLE `a` ()");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2c return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2d return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) )");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2e return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2f return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2g return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2h return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2i return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY 'b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2j return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2k return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2l return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHA(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2m return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(-1) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2n return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(720) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2o return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2p return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2p return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "valid query 2z failed\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "created same table again\n");

    r = try_query( hdb, "CREATE TABLE `aa` (`b` CHAR(72) NOT NULL, `c` "
          "CHAR(72), `d` CHAR(255) NOT NULL LOCALIZABLE PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "query 4 failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database after write\n");

    r = try_query( hdb, "CREATE TABLE `blah` (`foo` CHAR(72) NOT NULL "
                          "PRIMARY KEY `foo`)");
    ok(r == ERROR_SUCCESS , "query 4 failed\n");

    r = try_insert_query( hdb, "insert into a  ( `b` ) VALUES ( ? )");
    ok(r == ERROR_SUCCESS , "failed to insert record in db\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database after write\n");

    r = try_query( hdb, "CREATE TABLE `boo` (`foo` CHAR(72) NOT NULL "
                          "PRIMARY KEY `ba`)");
    ok(r != ERROR_SUCCESS , "query 5 succeeded\n");

    r = try_query( hdb,"CREATE TABLE `bee` (`foo` CHAR(72) NOT NULL )");
    ok(r != ERROR_SUCCESS , "query 6 succeeded\n");

    r = try_query( hdb, "CREATE TABLE `temp` (`t` CHAR(72) NOT NULL "
                          "PRIMARY KEY `t`)");
    ok(r == ERROR_SUCCESS , "query 7 failed\n");

    r = try_query( hdb, "CREATE TABLE `c` (`b` CHAR NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "query 8 failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database transact\n");

    r = DeleteFile( msifile );
    ok(r == TRUE, "file didn't exist after commit\n");
}

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

static void test_viewmodify(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    const char *query;
    char buffer[0x100];
    DWORD sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query( hdb, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* check what the error function reports without doing anything */
    sz = 0;
    /* passing NULL as the 3rd param make function to crash on older platforms */
    r = MsiViewGetError( 0, NULL, &sz );
    ok(r == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");

    /* open a view */
    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    /* see what happens with a good hview and bad args */
    r = MsiViewGetError( hview, NULL, NULL );
    ok(r == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");
    r = MsiViewGetError( hview, buffer, NULL );
    ok(r == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");

    /* see what happens with a zero length buffer */
    sz = 0;
    buffer[0] = 'x';
    r = MsiViewGetError( hview, buffer, &sz );
    ok(r == MSIDBERROR_MOREDATA, "MsiViewGetError return\n");
    ok(buffer[0] == 'x', "buffer cleared\n");
    ok(sz == 0, "size not zero\n");

    /* ok this one is strange */
    sz = 0;
    r = MsiViewGetError( hview, NULL, &sz );
    ok(r == MSIDBERROR_NOERROR, "MsiViewGetError return\n");
    ok(sz == 0, "size not zero\n");

    /* see if it really has an error */
    sz = sizeof buffer;
    buffer[0] = 'x';
    r = MsiViewGetError( hview, buffer, &sz );
    ok(r == MSIDBERROR_NOERROR, "MsiViewGetError return\n");
    ok(buffer[0] == 0, "buffer not cleared\n");
    ok(sz == 0, "size not zero\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    /* try some invalid records */
    r = MsiViewModify(hview, MSIMODIFY_INSERT, 0 );
    ok(r == ERROR_INVALID_HANDLE, "MsiViewModify failed\n");
    r = MsiViewModify(hview, -1, 0 );
    ok(r == ERROR_INVALID_HANDLE, "MsiViewModify failed\n");

    /* try an small record */
    hrec = MsiCreateRecord(1);
    r = MsiViewModify(hview, -1, hrec );
    ok(r == ERROR_INVALID_DATA, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* insert a valid record */
    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 2, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "bob");
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    /* insert the same thing again */
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    /* should fail ... */
    todo_wine {
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");
    }

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

START_TEST(db)
{
    test_msidatabase();
    test_msiinsert();
    test_msidecomposedesc();
    test_msibadqueries();
    test_viewmodify();
}
