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

static inline SF_TYPE get_union_type(SAFEARRAY *psa)
{
    VARTYPE vt;
    HRESULT hr;

    hr = SafeArrayGetVartype(psa, &vt);
    if (FAILED(hr))
        return 0;

    if (psa->fFeatures & FADF_HAVEIID)
        return SF_HAVEIID;

    switch (vt)
    {
    case VT_I1:
    case VT_UI1:      return SF_I1;
    case VT_BOOL:
    case VT_I2:
    case VT_UI2:      return SF_I2;
    case VT_INT:
    case VT_UINT:
    case VT_I4:
    case VT_UI4:
    case VT_R4:       return SF_I4;
    case VT_DATE:
    case VT_CY:
    case VT_R8:
    case VT_I8:
    case VT_UI8:      return SF_I8;
    case VT_INT_PTR:
    case VT_UINT_PTR: return (sizeof(UINT_PTR) == 4 ? SF_I4 : SF_I8);
    case VT_BSTR:     return SF_BSTR;
    case VT_DISPATCH: return SF_DISPATCH;
    case VT_VARIANT:  return SF_VARIANT;
    case VT_UNKNOWN:  return SF_UNKNOWN;
    /* Note: Return a non-zero size to indicate vt is valid. The actual size
     * of a UDT is taken from the result of IRecordInfo_GetSize().
     */
    case VT_RECORD:   return SF_RECORD;
    default:          return SF_ERROR;
    }
}

static ULONG get_cell_count(const SAFEARRAY *psa)
{
    const SAFEARRAYBOUND* psab = psa->rgsabound;
    USHORT cCount = psa->cDims;
    ULONG ulNumCells = 1;

    while (cCount--)
    {
         if (!psab->cElements)
            return 0;
        ulNumCells *= psab->cElements;
        psab++;
    }
    return ulNumCells;
}

static void check_safearray(void *buffer, LPSAFEARRAY lpsa)
{
    unsigned char *wiresa = buffer;
    VARTYPE vt;
    SF_TYPE sftype;
    ULONG cell_count;

    if(!lpsa)
    {
        ok(*(void **)wiresa == NULL, "wiresa + 0x0 should be NULL instead of 0x%08lx\n", *(DWORD *)wiresa);
        return;
    }

    SafeArrayGetVartype(lpsa, &vt);
    sftype = get_union_type(lpsa);
    cell_count = get_cell_count(lpsa);

    ok(*(DWORD *)wiresa, "wiresa + 0x0 should be non-NULL instead of 0x%08lx\n", *(DWORD *)wiresa); /* win2k: this is lpsa. winxp: this is 0x00000001 */
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
    ok(*(WORD *)wiresa == vt, "wiresa + 0x14 should be %04x instead of 0x%04x\n", vt, *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == sftype, "wiresa + 0x18 should be %08lx instead of 0x%08lx\n", (DWORD)sftype, *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == cell_count, "wiresa + 0x1c should be %lu instead of %lu\n", cell_count, *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD_PTR *)wiresa == (DWORD_PTR)lpsa->pvData, "wirestgm + 0x20 should be lpsa->pvData instead of 0x%08lx\n", *(DWORD_PTR *)wiresa);
    wiresa += sizeof(DWORD_PTR);
    if(sftype == SF_HAVEIID)
    {
        GUID guid;
        SafeArrayGetIID(lpsa, &guid);
        ok(IsEqualGUID(&guid, (GUID*)wiresa), "guid mismatch\n");
        wiresa += sizeof(GUID);
    }
    ok(!memcmp(wiresa, lpsa->rgsabound, sizeof(lpsa->rgsabound[0]) * lpsa->cDims), "bounds mismatch\n");
    wiresa += sizeof(lpsa->rgsabound[0]) * lpsa->cDims;

    ok(*(DWORD *)wiresa == cell_count, "wiresa + 0x2c should be %lu instead of %lu\n", cell_count, *(DWORD*)wiresa);
    wiresa += sizeof(DWORD);
    /* elements are now pointed to by wiresa */
}

static void test_marshal_LPSAFEARRAY(void)
{
    unsigned char *buffer;
    unsigned long size;
    LPSAFEARRAY lpsa;
    LPSAFEARRAY lpsa2 = NULL;
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
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    ok(size == 68, "size should be 68 bytes, not %ld\n", size);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == 64, "size should be 64 bytes, not %ld\n", size);
    buffer = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);

    check_safearray(buffer, lpsa);

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
    check_safearray(buffer, lpsa);

    if (LPSAFEARRAY_UNMARSHAL_WORKS)
    {
        LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
        ok(lpsa2 == NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
        LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_R8, 1, &sab);
    *(double *)lpsa->pvData = 3.1415;

    lpsa->cLocks = 7;
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    ok(size == 128, "size should be 128 bytes, not %ld\n", size);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == 128, "size should be 128 bytes, not %ld\n", size);
    buffer = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);

    check_safearray(buffer, lpsa);

    HeapFree(GetProcessHeap(), 0, buffer);
    SafeArrayDestroy(lpsa);
}

static void check_bstr(void *buffer, BSTR b)
{
    DWORD *wireb = buffer;
    DWORD len = SysStringLen(b);

    ok(*wireb == len, "wv[0] %08lx\n", *wireb);
    wireb++;
    if(len)
        ok(*wireb == len * 2, "wv[1] %08lx\n", *wireb);
    else
        ok(*wireb == 0xffffffff, "wv[1] %08lx\n", *wireb);
    wireb++;
    ok(*wireb == len, "wv[2] %08lx\n", *wireb);
    if(len)
    {
        wireb++;
        ok(!memcmp(wireb, b, len * 2), "strings differ\n");
    }
    return;
}

static void test_marshal_BSTR(void)
{
    unsigned long size;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };
    unsigned char *buffer;
    BSTR b, b2;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t','1',0};
    DWORD len;

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
    check_bstr(buffer, b);

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

    check_bstr(buffer, b);
    HeapFree(GetProcessHeap(), 0, buffer);

}

START_TEST(usrmarshal)
{
    CoInitialize(NULL);

    test_marshal_LPSAFEARRAY();
    test_marshal_BSTR();

    CoUninitialize();
}
