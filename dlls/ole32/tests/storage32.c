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

void test_hglobal_storage_stat(void)
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

void test_create_storage_modes(void)
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
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE |
                                   STGM_READWRITE, 0, NULL);
   ok(r==STG_E_INVALIDPOINTER, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE |
                                   STGM_READWRITE, 1, &stg);
   ok(r==STG_E_INVALIDPARAMETER, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_PRIORITY, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   ok(stg == NULL, "stg was set\n");

   /* check what happens if the file already exists */
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |
                                   STGM_TRANSACTED, 0, &stg);
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

   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |
                                   STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |
                                   STGM_TRANSACTED |STGM_FAILIFTHERE, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");

   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE |
                                   STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");

   ok(DeleteFileW(filename), "failed to delete file\n");
}

START_TEST(storage32)
{
    test_hglobal_storage_stat();
    test_create_storage_modes();
}
