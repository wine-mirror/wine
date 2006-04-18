/*
 * Unit tests for OLE storage
 *
 * Copyright (c) 2004 Mike McCormack
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

#define COBJMACROS

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "ole2.h"
#include "objidl.h"
#include "initguid.h"

DEFINE_GUID( test_stg_cls, 0x88888888, 0x0425, 0x0000, 0,0,0,0,0,0,0,0);

static void test_hglobal_storage_stat(void)
{
    ILockBytes *ilb = NULL;
    IStorage *stg = NULL;
    HRESULT r;
    STATSTG stat;
    DWORD mode, refcount;

    r = CreateILockBytesOnHGlobal( NULL, TRUE, &ilb );
    ok( r == S_OK, "CreateILockBytesOnHGlobal failed\n");

    mode = STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE;/*0x1012*/
    r = StgCreateDocfileOnILockBytes( ilb, mode, 0,  &stg );
    ok( r == S_OK, "CreateILockBytesOnHGlobal failed\n");

    r = WriteClassStg( stg, &test_stg_cls );
    ok( r == S_OK, "WriteClassStg failed\n");

    memset( &stat, 0, sizeof stat );
    r = IStorage_Stat( stg, &stat, 0 );

    ok( stat.pwcsName == NULL, "storage name not null\n");
    ok( stat.type == 1, "type is wrong\n");
    todo_wine {
    ok( stat.grfMode == 0x12, "grf mode is incorrect\n");
    }
    ok( !memcmp(&stat.clsid, &test_stg_cls, sizeof test_stg_cls), "CLSID is wrong\n");

    refcount = IStorage_Release( stg );
    ok( refcount == 0, "IStorage refcount is wrong\n");
    refcount = ILockBytes_Release( ilb );
    ok( refcount == 0, "ILockBytes refcount is wrong\n");
}

static void test_create_storage_modes(void)
{
   static const WCHAR szPrefix[] = { 's','t','g',0 };
   static const WCHAR szDot[] = { '.',0 };
   WCHAR filename[MAX_PATH];
   IStorage *stg = NULL;
   HRESULT r;

   if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
      return;

   DeleteFileW(filename);

   /* test with some invalid parameters */
   r = StgCreateDocfile( NULL, 0, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, 0, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, NULL);
   ok(r==STG_E_INVALIDPOINTER, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 1, &stg);
   ok(r==STG_E_INVALIDPARAMETER, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_DENY_WRITE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_PRIORITY, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");

   /* StgCreateDocfile seems to be very particular about the flags it accepts */
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | STGM_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 8, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x80, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x800, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x8000, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x80000, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x800000, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   ok(stg == NULL, "stg was set\n");

   /* check what happens if the file already exists */
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");
   r = StgCreateDocfile( filename, STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE | STGM_TRANSACTED, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_WRITE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_WRITE | STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile wrong error\n");
   ok(DeleteFileW(filename), "failed to delete file\n");

   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED |STGM_FAILIFTHERE, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");

   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_DENY_WRITE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");

   ok(DeleteFileW(filename), "failed to delete file\n");

   /* test the way excel uses StgCreateDocFile */
   r = StgCreateDocfile( filename, STGM_TRANSACTED|STGM_CREATE|STGM_SHARE_DENY_WRITE|STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the excel way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   /* looks like we need STGM_TRANSACTED or STGM_CREATE */
   r = StgCreateDocfile( filename, STGM_TRANSACTED|STGM_SHARE_DENY_WRITE|STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the excel way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   r = StgCreateDocfile( filename, STGM_TRANSACTED|STGM_CREATE|STGM_SHARE_DENY_WRITE|STGM_WRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the excel way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the powerpoint way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   /* test the way msi uses StgCreateDocfile */
   r = StgCreateDocfile( filename, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stg);
   ok(r==S_OK, "StgCreateDocFile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   ok(DeleteFileW(filename), "failed to delete file\n");
}

static void test_storage_stream(void)
{
    static const WCHAR stmname[] = { 'C','O','N','T','E','N','T','S',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR longname[] = {
        'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
        'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',0
    };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    HRESULT r;
    IStream *stm = NULL;
    ULONG count = 0;
    LARGE_INTEGER pos;
    ULARGE_INTEGER p;
    unsigned char buffer[0x100];

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    /* try create some invalid streams */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 1, 0, &stm );
    ok(r==STG_E_INVALIDPARAMETER, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 1, &stm );
    ok(r==STG_E_INVALIDPARAMETER, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, NULL );
    ok(r==STG_E_INVALIDPOINTER, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, NULL, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDNAME, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, longname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDNAME, "IStorage->CreateStream wrong error, got %ld GetLastError()=%ld\n", r, GetLastError());
    r = IStorage_CreateStream(stg, stmname, STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_READ, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_WRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_DENY_NONE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_DENY_NONE | STGM_READ, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");

    /* now really create a stream and delete it */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");
    r = IStream_Release(stm);
    ok(r == 0, "wrong ref count\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_FILEALREADYEXISTS, "IStorage->CreateStream failed\n");
    r = IStorage_DestroyElement(stg,stmname);
    ok(r==S_OK, "IStorage->DestroyElement failed\n");

    /* create a stream and write to it */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStream_Write(stm, NULL, 0, NULL );
    ok(r==STG_E_INVALIDPOINTER, "IStream->Write wrong error\n");
    r = IStream_Write(stm, "Hello\n", 0, NULL );
    ok(r==S_OK, "failed to write stream\n");
    r = IStream_Write(stm, "Hello\n", 0, &count );
    ok(r==S_OK, "failed to write stream\n");
    r = IStream_Write(stm, "Hello\n", 6, &count );
    ok(r==S_OK, "failed to write stream\n");
    r = IStream_Commit(stm, STGC_DEFAULT );
    ok(r==S_OK, "failed to commit stream\n");
    r = IStream_Commit(stm, STGC_DEFAULT );
    ok(r==S_OK, "failed to commit stream\n");

    /* seek round a bit, reset the stream size */
    pos.QuadPart = 0;
    r = IStream_Seek(stm, pos, 3, &p );
    ok(r==STG_E_INVALIDFUNCTION, "IStream->Seek returned wrong error\n");
    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, NULL);
    ok(r==S_OK, "failed to seek stream\n");
    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, &p );
    ok(r==S_OK, "failed to seek stream\n");
    r = IStream_SetSize(stm,p);
    ok(r==S_OK, "failed to set pos\n");
    pos.QuadPart = 10;
    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, &p );
    ok(r==S_OK, "failed to seek stream\n");
    ok(p.QuadPart == 10, "at wrong place\n");
    pos.QuadPart = 0;
    r = IStream_Seek(stm, pos, STREAM_SEEK_END, &p );
    ok(r==S_OK, "failed to seek stream\n");
    ok(p.QuadPart == 0, "at wrong place\n");
    r = IStream_Read(stm, buffer, sizeof buffer, &count );
    ok(r==S_OK, "failed to set pos\n");
    ok(count == 0, "read bytes from empty stream\n");

    /* wrap up */
    r = IStream_Release(stm);
    ok(r == 0, "wrong ref count\n");
    r = IStorage_Release(stg);
    ok(r == 0, "wrong ref count\n");
    r = DeleteFileW(filename);
    ok(r, "file should exist\n");
}

static BOOL touch_file(LPCWSTR filename)
{
    HANDLE file;

    file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file==INVALID_HANDLE_VALUE)
        return FALSE;
    CloseHandle(file);
    return TRUE;
}

static BOOL is_zero_length(LPCWSTR filename)
{
    HANDLE file;
    DWORD len;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, 
                OPEN_EXISTING, 0, NULL);
    if (file==INVALID_HANDLE_VALUE)
        return FALSE;
    len = GetFileSize(file, NULL);
    CloseHandle(file);
    return len == 0;
}

static void test_open_storage(void)
{
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szNonExist[] = { 'n','o','n','e','x','i','s','t',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL, *stg2 = NULL;
    HRESULT r;
    DWORD stgm;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    /* try opening a zero length file - it should stay zero length */
    DeleteFileW(filename);
    touch_file(filename);
    stgm = STGM_NOSCRATCH | STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READWRITE;
    r = StgOpenStorage( filename, NULL, stgm, NULL, 0, &stg);
    ok(r==STG_E_FILEALREADYEXISTS, "StgOpenStorage didn't fail\n");

    stgm = STGM_SHARE_EXCLUSIVE | STGM_READWRITE;
    r = StgOpenStorage( filename, NULL, stgm, NULL, 0, &stg);
    ok(r==STG_E_FILEALREADYEXISTS, "StgOpenStorage didn't fail\n");
    ok(is_zero_length(filename), "file length changed\n");

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");
    IStorage_Release(stg);

    r = StgOpenStorage( filename, NULL, 0, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( NULL, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    ok(r==STG_E_INVALIDNAME, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, NULL);
    ok(r==STG_E_INVALIDPOINTER, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 1, &stg);
    ok(r==STG_E_INVALIDPARAMETER, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( szNonExist, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_FILENOTFOUND, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_NONE | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_READ | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_WRITE | STGM_READWRITE, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");

    /* open it for real */
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_NONE | STGM_READ | STGM_TRANSACTED, NULL, 0, &stg); /* XLViewer 97/2000 */
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_WRITE | STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    /* test the way word opens its custom dictionary */
    r = StgOpenStorage( filename, NULL, STGM_NOSCRATCH | STGM_TRANSACTED |
                        STGM_SHARE_DENY_WRITE | STGM_READWRITE, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg2);
    ok(r==STG_E_SHAREVIOLATION, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    /* now try write to a storage file we opened read-only */ 
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    { 
        static const WCHAR stmname[] =  { 'w','i','n','e','t','e','s','t',0};
        IStream *stm = NULL;
        IStorage *stg2 = NULL;

        r = IStorage_CreateStream( stg, stmname, STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                                   0, 0, &stm );
        ok(r == STG_E_ACCESSDENIED, "CreateStream should fail\n");
        r = IStorage_CreateStorage( stg, stmname, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stg2);
        ok(r == STG_E_ACCESSDENIED, "CreateStream should fail\n");

        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    /* open like visio 2003 */
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_DENY_NONE, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    /* test other sharing modes with STGM_PRIORITY */
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_DENY_READ, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    /* open like Project 2003 */
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY, NULL, 0, &stg2);
    ok(r == S_OK, "should succeed\n");
    if (stg2)
        IStorage_Release(stg2);
    if (stg)
        IStorage_Release(stg);

    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_READWRITE, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_TRANSACTED | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_SIMPLE | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_DELETEONRELEASE | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFUNCTION, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_NOSCRATCH | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_NOSNAPSHOT | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = DeleteFileW(filename);
    ok(r, "file didn't exist\n");
}

static void test_storage_suminfo(void)
{
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    IPropertySetStorage *propset = NULL;
    IPropertyStorage *ps = NULL;
    HRESULT r;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | 
                            STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    r = IStorage_QueryInterface( stg, &IID_IPropertySetStorage, (LPVOID) &propset );
    ok(r == S_OK, "query interface failed\n");

    /* delete it */
    r = IPropertySetStorage_Delete( propset, &FMTID_SummaryInformation );
    ok(r == STG_E_FILENOTFOUND, "deleted property set storage\n");

    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
                                STGM_READ | STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_FILENOTFOUND, "opened property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_READ | STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_READ, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0, 0, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_WRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    /* now try really creating a a property set */
    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == S_OK, "failed to create property set storage\n");

    if( ps )
        IPropertyStorage_Release(ps);

    /* now try creating the same thing again */
    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == S_OK, "failed to create property set storage\n");
    if( ps )
        IPropertyStorage_Release(ps);

    /* should be able to open it */
    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == S_OK, "open failed\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    /* delete it */
    r = IPropertySetStorage_Delete( propset, &FMTID_SummaryInformation );
    ok(r == S_OK, "failed to delete property set storage\n");

    /* try opening with an invalid FMTID */
    r = IPropertySetStorage_Open( propset, NULL, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == E_INVALIDARG, "open succeeded\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    /* try a bad guid */
    r = IPropertySetStorage_Open( propset, &IID_IStorage, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == STG_E_FILENOTFOUND, "open succeeded\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);
    

    /* try some invalid flags */
    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
            STGM_CREATE | STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == STG_E_INVALIDFLAG, "open succeeded\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    /* after deleting it, it should be gone */
    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == STG_E_FILENOTFOUND, "open failed\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    r = IPropertySetStorage_Release( propset );
    ok(r == 1, "ref count wrong\n");

    r = IStorage_Release(stg);
    ok(r == 0, "ref count wrong\n");

    DeleteFileW(filename);
}

static void test_storage_refcount(void)
{
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    HRESULT r;
    IStream *stm = NULL;
    static const WCHAR stmname[] = { 'C','O','N','T','E','N','T','S',0 };
    LARGE_INTEGER pos;
    ULARGE_INTEGER upos;
    STATSTG stat;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | 
                            STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    /* now create a stream */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStorage_Release( stg );
    ok (r == 0, "storage not released\n");

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, 0, &upos );
    ok (r == STG_E_REVERTED, "seek should fail\n");

    r = IStream_Stat( stm, &stat, STATFLAG_DEFAULT );
    ok (r == STG_E_REVERTED, "stat should fail\n");

    r = IStream_Release(stm);
    ok (r == 0, "stream not released\n");

    /* test for grfMode open issue */

    r = StgOpenStorage( filename, NULL, 0x00010020, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_OpenStream( stg, stmname, 0, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stm );
        ok(r == S_OK, "OpenStream should succeed\n");

        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    DeleteFileW(filename);
}

START_TEST(storage32)
{
    test_hglobal_storage_stat();
    test_create_storage_modes();
    test_storage_stream();
    test_open_storage();
    test_storage_suminfo();
    test_storage_refcount();
}
