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
#define VARIANT_UNMARSHAL_WORKS 0

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
    ok(*(DWORD_PTR *)wiresa == (DWORD_PTR)lpsa->pvData, "wiresa + 0x20 should be lpsa->pvData instead of 0x%08lx\n", *(DWORD_PTR *)wiresa);
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
        VARTYPE vt, vt2;
        LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
        ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal\n");
        SafeArrayGetVartype(lpsa, &vt);
        SafeArrayGetVartype(lpsa2, &vt2);
        ok(vt == vt2, "vts differ %x %x\n", vt, vt2);
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
    DWORD len = SysStringByteLen(b);

    ok(*wireb == (len + 1) / 2, "wv[0] %08lx\n", *wireb);
    wireb++;
    if(b)
        ok(*wireb == len, "wv[1] %08lx\n", *wireb);
    else
        ok(*wireb == 0xffffffff, "wv[1] %08lx\n", *wireb);
    wireb++;
    ok(*wireb == (len + 1) / 2, "wv[2] %08lx\n", *wireb);
    if(len)
    {
        wireb++;
        ok(!memcmp(wireb, b, (len + 1) & ~1), "strings differ\n");
    }
    return;
}

static void test_marshal_BSTR(void)
{
    unsigned long size;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };
    unsigned char *buffer, *next;
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
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 != NULL, "BSTR didn't unmarshal\n");
        ok(!memcmp(b, b2, (len + 1) * 2), "strings differ\n");
        BSTR_UserFree(&umcb.Flags, &b2);
    }

    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = NULL;
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);

    check_bstr(buffer, b);
    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 == NULL, "NULL BSTR didn't unmarshal\n");
        BSTR_UserFree(&umcb.Flags, &b2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    b = SysAllocStringByteLen("abc", 3);
    *(((char*)b) + 3) = 'd';
    len = SysStringLen(b);
    ok(len == 1, "get %ld\n", len);
    len = SysStringByteLen(b);
    ok(len == 3, "get %ld\n", len);

    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 16, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);
    ok(buffer[15] == 'd', "buffer[15] %02x\n", buffer[15]);

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 != NULL, "BSTR didn't unmarshal\n");
        ok(!memcmp(b, b2, len), "strings differ\n");
        BSTR_UserFree(&umcb.Flags, &b2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = SysAllocStringByteLen("", 0);
    len = SysStringLen(b);
    ok(len == 0, "get %ld\n", len);
    len = SysStringByteLen(b);
    ok(len == 0, "get %ld\n", len);

    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 != NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
        len = SysStringByteLen(b2);
        ok(len == 0, "byte len %ld\n", len);
        BSTR_UserFree(&umcb.Flags, &b2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);
}

static void check_variant_header(DWORD *wirev, VARIANT *v, unsigned long size)
{
    WORD *wp;
    DWORD switch_is;

    ok(*wirev == (size + 7) >> 3, "wv[0] %08lx, expected %08lx\n", *wirev, (size + 7) >> 3);
    wirev++;
    ok(*wirev == 0, "wv[1] %08lx\n", *wirev);
    wirev++;
    wp = (WORD*)wirev;
    ok(*wp == V_VT(v), "vt %04x expected %04x\n", *wp, V_VT(v));
    wp++;
    ok(*wp == v->n1.n2.wReserved1, "res1 %04x expected %04x\n", *wp, v->n1.n2.wReserved1);
    wp++;
    ok(*wp == v->n1.n2.wReserved2, "res2 %04x expected %04x\n", *wp, v->n1.n2.wReserved2);
    wp++;
    ok(*wp == v->n1.n2.wReserved3, "res3 %04x expected %04x\n", *wp, v->n1.n2.wReserved3);
    wp++;
    wirev = (DWORD*)wp;
    switch_is = V_VT(v);
    if(switch_is & VT_ARRAY)
        switch_is &= ~VT_TYPEMASK;
    ok(*wirev == switch_is, "switch_is %08lx expected %08lx\n", *wirev, switch_is);
}

static void test_marshal_VARIANT(void)
{
    unsigned long size;
    VARIANT v, v2;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };
    unsigned char *buffer, *next;
    unsigned long ul;
    short s;
    double d;
    DWORD *wirev;
    BSTR b;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t',0};
    SAFEARRAYBOUND sab;
    LPSAFEARRAY lpsa;
    DECIMAL dec, dec2;

    umcb.Flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    umcb.pReserve = NULL;
    umcb.pStubMsg = &stubMsg;

    /*** I1 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I1;
    V_I1(&v) = 0x12;

    /* Variants have an alignment of 8 */
    size = VARIANT_UserSize(&umcb.Flags, 1, &v);
    ok(size == 29, "size %ld\n", size);

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 21, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*(char*)wirev == V_I1(&v), "wv[5] %08lx\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_I1(&v) == V_I1(&v2), "got i1 %x expect %x\n", V_I1(&v), V_I1(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** I2 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I2;
    V_I2(&v) = 0x1234;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 22, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*(short*)wirev == V_I2(&v), "wv[5] %08lx\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_I2(&v) == V_I2(&v2), "got i2 %x expect %x\n", V_I2(&v), V_I2(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** I2 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_I2 | VT_BYREF;
    s = 0x1234;
    V_I2REF(&v) = &s;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 26, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 0x4, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(*(short*)wirev == s, "wv[6] %08lx\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(*V_I2REF(&v) == *V_I2REF(&v2), "got i2 ref %x expect ui4 ref %x\n", *V_I2REF(&v), *V_I2REF(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** I4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0x1234;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 24, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == V_I4(&v), "wv[5] %08lx\n", *wirev);

    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_I4(&v) == V_I4(&v2), "got i4 %lx expect %lx\n", V_I4(&v), V_I4(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }

    HeapFree(GetProcessHeap(), 0, buffer);

    /*** UI4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4;
    V_UI4(&v) = 0x1234;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 24, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 0x1234, "wv[5] %08lx\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_UI4(&v) == V_UI4(&v2), "got ui4 %lx expect %lx\n", V_UI4(&v), V_UI4(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }

    HeapFree(GetProcessHeap(), 0, buffer);

    /*** UI4 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_BYREF;
    ul = 0x1234;
    V_UI4REF(&v) = &ul;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 28, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 0x4, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(*wirev == ul, "wv[6] %08lx\n", *wirev);

    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(*V_UI4REF(&v) == *V_UI4REF(&v2), "got ui4 ref %lx expect ui4 ref %lx\n", *V_UI4REF(&v), *V_UI4REF(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** R4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_R4;
    V_R8(&v) = 3.1415;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 24, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
     
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*(float*)wirev == V_R4(&v), "wv[5] %08lx\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_R4(&v) == V_R4(&v2), "got r4 %f expect %f\n", V_R4(&v), V_R4(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** R8 ***/
    VariantInit(&v);
    V_VT(&v) = VT_R8;
    V_R8(&v) = 3.1415;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 32, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 0xcccccccc, "wv[5] %08lx\n", *wirev); /* pad */
    wirev++;
    ok(*(double*)wirev == V_R8(&v), "wv[6] %08lx, wv[7] %08lx\n", *wirev, *(wirev+1));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_R8(&v) == V_R8(&v2), "got r8 %f expect %f\n", V_R8(&v), V_R8(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** R8 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_R8 | VT_BYREF;
    d = 3.1415;
    V_R8REF(&v) = &d;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 32, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 8, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(*(double*)wirev == d, "wv[6] %08lx wv[7] %08lx\n", *wirev, *(wirev+1));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(*V_R8REF(&v) == *V_R8REF(&v2), "got r8 ref %f expect %f\n", *V_R8REF(&v), *V_R8REF(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** VARIANT_BOOL ***/
    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = 0x1234;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 22, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*(short*)wirev == V_BOOL(&v), "wv[5] %04x\n", *(WORD*)wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_BOOL(&v) == V_BOOL(&v2), "got bool %x expect %x\n", V_BOOL(&v), V_BOOL(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** DECIMAL ***/
    VarDecFromI4(0x12345678, &dec);
    VariantInit(&v);
    V_DECIMAL(&v) = dec;
    V_VT(&v) = VT_DECIMAL;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 40, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 0xcccccccc, "wirev[5] %08lx\n", *wirev); /* pad */
    wirev++;
    dec2 = dec;
    dec2.wReserved = VT_DECIMAL;
    ok(!memcmp(wirev, &dec2, sizeof(dec2)), "wirev[6] %08lx wirev[7] %08lx wirev[8] %08lx wirev[9] %08lx\n",
       *wirev, *(wirev + 1), *(wirev + 2), *(wirev + 3));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(!memcmp(&V_DECIMAL(&v), & V_DECIMAL(&v2), sizeof(DECIMAL)), "decimals differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** DECIMAL BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_DECIMAL | VT_BYREF;
    V_DECIMALREF(&v) = &dec;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 40, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 16, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(!memcmp(wirev, &dec, sizeof(dec)), "wirev[6] %08lx wirev[7] %08lx wirev[8] %08lx wirev[9] %08lx\n", *wirev, *(wirev + 1), *(wirev + 2), *(wirev + 3));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(!memcmp(V_DECIMALREF(&v), V_DECIMALREF(&v2), sizeof(DECIMAL)), "decimals differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** EMPTY ***/
    VariantInit(&v);
    V_VT(&v) = VT_EMPTY;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 20, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, size);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** NULL ***/
    VariantInit(&v);
    V_VT(&v) = VT_NULL;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 20, "size %ld\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, size);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** BSTR ***/
    b = SysAllocString(str);
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = b;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 60, "size %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev, "wv[5] %08lx\n", *wirev); /* win2k: this is b. winxp: this is (char*)b + 1 */
    wirev++;
    check_bstr(wirev, V_BSTR(&v));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SysStringByteLen(V_BSTR(&v)) == SysStringByteLen(V_BSTR(&v2)), "bstr string lens differ\n");
        ok(!memcmp(V_BSTR(&v), V_BSTR(&v2), SysStringByteLen(V_BSTR(&v))), "bstrs differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** BSTR BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&v) = &b;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 64, "size %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 0x4, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08lx\n", *wirev); /* win2k: this is b. winxp: this is (char*)b + 1 */
    wirev++;
    check_bstr(wirev, b);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SysStringByteLen(*V_BSTRREF(&v)) == SysStringByteLen(*V_BSTRREF(&v2)), "bstr string lens differ\n");
        ok(!memcmp(*V_BSTRREF(&v), *V_BSTRREF(&v2), SysStringByteLen(*V_BSTRREF(&v))), "bstrs differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    /*** ARRAY ***/
    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_R8, 1, &sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;
    *((DWORD *)lpsa->pvData + 1) = 0xdeadbeef;
    lpsa->cLocks = 7;

    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY;
    V_ARRAY(&v) = lpsa;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 152, "size %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev, "wv[5] %08lx\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        long bound, bound2;
        VARTYPE vt, vt2;
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SafeArrayGetDim(V_ARRAY(&v)) == SafeArrayGetDim(V_ARRAY(&v)), "array dims differ\n");  
        SafeArrayGetLBound(V_ARRAY(&v), 1, &bound);
        SafeArrayGetLBound(V_ARRAY(&v2), 1, &bound2);
        ok(bound == bound2, "array lbounds differ\n");
        SafeArrayGetUBound(V_ARRAY(&v), 1, &bound);
        SafeArrayGetUBound(V_ARRAY(&v2), 1, &bound2);
        ok(bound == bound2, "array ubounds differ\n");
        SafeArrayGetVartype(V_ARRAY(&v), &vt);
        SafeArrayGetVartype(V_ARRAY(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    /*** ARRAY BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY | VT_BYREF;
    V_ARRAYREF(&v) = &lpsa;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 152, "size %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, size);
    wirev += 5;
    ok(*wirev == 4, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08lx\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        long bound, bound2;
        VARTYPE vt, vt2;
        VariantInit(&v2);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SafeArrayGetDim(*V_ARRAYREF(&v)) == SafeArrayGetDim(*V_ARRAYREF(&v)), "array dims differ\n");  
        SafeArrayGetLBound(*V_ARRAYREF(&v), 1, &bound);
        SafeArrayGetLBound(*V_ARRAYREF(&v2), 1, &bound2);
        ok(bound == bound2, "array lbounds differ\n");
        SafeArrayGetUBound(*V_ARRAYREF(&v), 1, &bound);
        SafeArrayGetUBound(*V_ARRAYREF(&v2), 1, &bound2);
        ok(bound == bound2, "array ubounds differ\n");
        SafeArrayGetVartype(*V_ARRAYREF(&v), &vt);
        SafeArrayGetVartype(*V_ARRAYREF(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SafeArrayDestroy(lpsa);

    /*** VARIANT BYREF ***/
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v2) = VT_R8;
    V_R8(&v2) = 3.1415;
    V_VT(&v) = VT_VARIANT | VT_BYREF;
    V_VARIANTREF(&v) = &v2;

    size = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(size == 64, "size %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    wirev = (DWORD*)buffer;
    check_variant_header(wirev, &v, size);
    wirev += 5;

    ok(*wirev == 16, "wv[5] %08lx\n", *wirev);
    wirev++;
    ok(*wirev == ('U' | 's' << 8 | 'e' << 16 | 'r' << 24), "wv[6] %08lx\n", *wirev); /* 'User' */
    wirev++;
    ok(*wirev == 0xcccccccc, "wv[7] %08lx\n", *wirev); /* pad */
    wirev++;
    check_variant_header(wirev, &v2, size - 32);
    wirev += 5;
    ok(*wirev == 0xcccccccc, "wv[13] %08lx\n", *wirev); /* pad for VT_R8 */
    wirev++;
    ok(*(double*)wirev == V_R8(&v2), "wv[6] %08lx wv[7] %08lx\n", *wirev, *(wirev+1));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VARIANT v3;
        VariantInit(&v3);
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
        ok(V_VT(V_VARIANTREF(&v)) == V_VT(V_VARIANTREF(&v3)), "vts differ %x %x\n",
           V_VT(V_VARIANTREF(&v)), V_VT(V_VARIANTREF(&v3))); 
        ok(V_R8(V_VARIANTREF(&v)) == V_R8(V_VARIANTREF(&v3)), "r8s differ\n"); 
        VARIANT_UserFree(&umcb.Flags, &v3);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
}


START_TEST(usrmarshal)
{
    CoInitialize(NULL);

    test_marshal_LPSAFEARRAY();
    test_marshal_BSTR();
    test_marshal_VARIANT();

    CoUninitialize();
}
