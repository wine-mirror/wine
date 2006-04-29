/*
 * Marshaling Tests
 *
 * Copyright 2004 Robert Shearman
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propidl.h" /* for LPSAFEARRAY_User* routines */

#include "wine/test.h"

/* doesn't work on Windows due to needing more of the
 * MIDL_STUB_MESSAGE structure to be filled out */
#define LPSAFEARRAY_UNMARSHAL_WORKS 0
#define BSTR_UNMARSHAL_WORKS 0

static void test_marshal_LPSAFEARRAY(void)
{
    unsigned char *buffer;
    unsigned long size;
    LPSAFEARRAY lpsa;
    LPSAFEARRAY lpsa2 = NULL;
    unsigned char *wiresa;
    SAFEARRAYBOUND sab;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };

    umcb.Flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    umcb.pReserve = NULL;
    umcb.pStubMsg = &stubMsg;

    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_I2, 1, &sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;

    lpsa->cLocks = 7;
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == 64, "size should be 64 bytes, not %ld\n", size);
    buffer = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    wiresa = buffer;
    ok(*(DWORD *)wiresa == TRUE, "wiresa + 0x0 should be TRUE instead of 0x%08lx\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == lpsa->cDims, "wiresa + 0x4 should be lpsa->cDims instead of 0x%08lx\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(WORD *)wiresa == lpsa->cDims, "wiresa + 0x8 should be lpsa->cDims instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(WORD *)wiresa == lpsa->fFeatures, "wiresa + 0xc should be lpsa->fFeatures instead of 0x%08x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == lpsa->cbElements, "wiresa + 0x10 should be lpsa->cbElements instead of 0x%08lx\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(WORD *)wiresa == lpsa->cLocks, "wiresa + 0x16 should be lpsa->cLocks instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(WORD *)wiresa == VT_I2, "wiresa + 0x14 should be VT_I2 instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == VT_I2, "wiresa + 0x18 should be VT_I2 instead of 0x%08lx\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == sab.cElements, "wiresa + 0x1c should be sab.cElements instead of %lu\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD_PTR *)wiresa == (DWORD_PTR)lpsa->pvData, "wirestgm + 0x20 should be lpsa->pvData instead of 0x%08lx\n", *(DWORD_PTR *)wiresa);
    wiresa += sizeof(DWORD_PTR);
    ok(*(DWORD *)wiresa == sab.cElements, "wiresa + 0x24 should be sab.cElements instead of %lu\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(LONG *)wiresa == sab.lLbound, "wiresa + 0x28 should be sab.clLbound instead of %ld\n", *(LONG *)wiresa);
    wiresa += sizeof(LONG);
    ok(*(DWORD *)wiresa == sab.cElements, "wiresa + 0x2c should be sab.cElements instead of %lu\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    /* elements are now pointed to by wiresa */

    if (LPSAFEARRAY_UNMARSHAL_WORKS)
    {
        LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
        ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal\n");
        LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SafeArrayDestroy(lpsa);

    /* test NULL safe array */
    lpsa = NULL;

    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == 4, "size should be 4 bytes, not %ld\n", size);
    buffer = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    wiresa = buffer;
    ok(*(DWORD *)wiresa == FALSE, "wiresa + 0x0 should be FALSE instead of 0x%08lx\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);

    if (LPSAFEARRAY_UNMARSHAL_WORKS)
    {
        LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
        ok(lpsa2 == NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
        LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
}

static void test_marshal_BSTR(void)
{
    unsigned long size;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };
    unsigned char *buffer;
    BSTR b, b2;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t','1',0};
    DWORD *wireb, len;

    umcb.Flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    umcb.pReserve = NULL;
    umcb.pStubMsg = &stubMsg;

    b = SysAllocString(str);
    len = SysStringLen(b);
    ok(len == 13, "get %ld\n", len);

    /* BSTRs are DWORD aligned */
    size = BSTR_UserSize(&umcb.Flags, 1, &b);
    ok(size == 42, "size %ld\n", size);

    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 38, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    wireb = (DWORD*)buffer;

    ok(*wireb == len, "wv[0] %08lx\n", *wireb);
    wireb++;
    ok(*wireb == len * 2, "wv[1] %08lx\n", *wireb);
    wireb++;
    ok(*wireb == len, "wv[2] %08lx\n", *wireb);
    wireb++;
    ok(!memcmp(wireb, str, len * 2), "strings differ\n");

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(b2 != NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
        ok(!memcmp(b, b2, (len + 1) * 2), "strings differ\n");
        BSTR_UserFree(&umcb.Flags, &b2);
    }

    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = NULL;
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    wireb = (DWORD*)buffer;
    ok(*wireb == 0, "wv[0] %08lx\n", *wireb);
    wireb++;
    ok(*wireb == 0xffffffff, "wv[1] %08lx\n", *wireb);
    wireb++;
    ok(*wireb == 0, "wv[2] %08lx\n", *wireb);

    HeapFree(GetProcessHeap(), 0, buffer);

}

START_TEST(usrmarshal)
{
    CoInitialize(NULL);

    test_marshal_LPSAFEARRAY();
    test_marshal_BSTR();

    CoUninitialize();
}
