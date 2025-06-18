/*
 *	OLE2DISP library
 *
 *	Copyright 1995	Martin von Loewis
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "oleauto.h"
#include "winerror.h"
#include "wownt32.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"

#include "ole2disp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define E_OUTOFMEMORY16 MAKE_SCODE(SEVERITY_ERROR, FACILITY_NULL, 2)
#define E_INVALIDARG16  MAKE_SCODE(SEVERITY_ERROR, FACILITY_NULL, 3)

/* 16-bit SAFEARRAY implementation */
typedef struct tagSAFEARRAYBOUND16
{
    ULONG cElements;
    LONG  lLbound;
} SAFEARRAYBOUND16;

typedef struct tagSAFEARRAY16
{
    USHORT cDims;
    USHORT fFeatures;
    USHORT cbElements;
    USHORT cLocks;
    ULONG  handle;
    SEGPTR pvData;
    SAFEARRAYBOUND16 rgsabound[1];
} SAFEARRAY16;

static SEGPTR safearray_alloc(ULONG size)
{
    HANDLE16 h;
    return WOWGlobalAllocLock16(GPTR, size, &h);
}

static void safearray_free(SEGPTR ptr)
{
    WOWGlobalUnlockFree16(ptr);
}

static ULONG safearray_getcellcount(const SAFEARRAY16 *sa)
{
    const SAFEARRAYBOUND16 *sab = sa->rgsabound;
    USHORT count = sa->cDims;
    ULONG cells = 1;

    while (count--)
    {
        if (!sab->cElements)
            return 0;
        cells *= sab->cElements;
        sab++;
    }

    return cells;
}

static HRESULT safearray_lock(SAFEARRAY16 *sa)
{
    if (sa->cLocks == 0xffff)
        return E_UNEXPECTED;

    sa->cLocks++;
    return S_OK;
}

/******************************************************************************
 *    SafeArrayGetDim [OLE2DISP.17]
 */
USHORT WINAPI SafeArrayGetDim16(SAFEARRAY16 *sa)
{
    TRACE("(%p)\n", sa);
    return sa->cDims;
}

/******************************************************************************
 *    SafeArrayGetElemsize [OLE2DISP.18]
 */
USHORT WINAPI SafeArrayGetElemsize16(SAFEARRAY16 *sa)
{
    TRACE("(%p)\n", sa);
    return sa->cbElements;
}

/******************************************************************************
 *    SafeArrayGetUBound [OLE2DISP.19]
 */
HRESULT WINAPI SafeArrayGetUBound16(SAFEARRAY16 *sa, UINT16 dim, LONG *ubound)
{
    TRACE("(%p, %u, %p)\n", sa, dim, ubound);

    if (!sa)
        return E_INVALIDARG16;

    if (!dim || dim > sa->cDims)
        return DISP_E_BADINDEX;

    *ubound = sa->rgsabound[sa->cDims - dim].lLbound + sa->rgsabound[sa->cDims - dim].cElements - 1;

    return S_OK;
}

/******************************************************************************
 *    SafeArrayGetLBound [OLE2DISP.20]
 */
HRESULT WINAPI SafeArrayGetLBound16(SAFEARRAY16 *sa, UINT16 dim, LONG *lbound)
{
    TRACE("(%p, %u, %p)\n", sa, dim, lbound);

    if (!sa)
        return E_INVALIDARG16;

    if (!dim || dim > sa->cDims)
        return DISP_E_BADINDEX;

    *lbound = sa->rgsabound[sa->cDims - dim].lLbound;

    return S_OK;
}

/******************************************************************************
 *    SafeArrayLock [OLE2DISP.21]
 */
HRESULT WINAPI SafeArrayLock16(SAFEARRAY16 *sa)
{
    TRACE("(%p)\n", sa);

    if (!sa)
        return E_INVALIDARG16;

    return safearray_lock(sa);
}

/******************************************************************************
 *    SafeArrayUnlock [OLE2DISP.22]
 */
HRESULT WINAPI SafeArrayUnlock16(SAFEARRAY16 *sa)
{
    TRACE("(%p)\n", sa);

    if (!sa)
        return E_INVALIDARG16;

    if (sa->cLocks == 0)
        return E_UNEXPECTED;

    sa->cLocks--;
    return S_OK;
}

/******************************************************************************
 *    SafeArrayAccessData [OLE2DISP.23]
 */
HRESULT WINAPI SafeArrayAccessData16(SAFEARRAY16 *sa, SEGPTR *data)
{
    HRESULT hr;

    TRACE("(%p, %p)\n", sa, data);

    /* arguments are not tested, it crashes if any of them is NULL */

    hr = safearray_lock(sa);
    if (FAILED(hr))
        return hr;

    *data = sa->pvData;
    return S_OK;
}

/******************************************************************************
 *    SafeArrayUnaccessData [OLE2DISP.24]
 */
HRESULT WINAPI SafeArrayUnaccessData16(SAFEARRAY16 *sa)
{
    TRACE("(%p)\n", sa);
    return SafeArrayUnlock16(sa);
}

/******************************************************************************
 *    SafeArrayAllocDescriptor [OLE2DISP.38]
 */
HRESULT WINAPI SafeArrayAllocDescriptor16(UINT16 dims, SEGPTR *ret)
{
    SAFEARRAY16 *sa;
    ULONG size;

    TRACE("%u, %p\n", dims, ret);

    if (!dims)
        return E_INVALIDARG16;

    size = sizeof(SAFEARRAY16) + sizeof(SAFEARRAYBOUND16) * (dims - 1);
    *ret = safearray_alloc(size);
    if (!*ret)
        return E_OUTOFMEMORY16;

    sa = MapSL(*ret);
    sa->cDims = dims;
    return S_OK;
}

/******************************************************************************
 *    SafeArrayAllocData [OLE2DISP.39]
 */
HRESULT WINAPI SafeArrayAllocData16(SAFEARRAY16 *sa)
{
    ULONG size;

    TRACE("%p\n", sa);

    if (!sa)
        return E_INVALIDARG16;

    size = safearray_getcellcount(sa);
    sa->pvData = safearray_alloc(size * sa->cbElements);
    return sa->pvData ? S_OK : E_OUTOFMEMORY16;
}

/******************************************************************************
 *    SafeArrayDestroyDescriptor [OLE2DISP.40]
 */
HRESULT WINAPI SafeArrayDestroyDescriptor16(SEGPTR s)
{
    TRACE("0x%08lx\n", s);

    if (s)
    {
        SAFEARRAY16 *sa = MapSL(s);

        if (sa->cLocks)
            return DISP_E_ARRAYISLOCKED;

        safearray_free(s);
    }

    return S_OK;
}

/******************************************************************************
 *    SafeArrayDestroyData [OLE2DISP.41]
 */
HRESULT WINAPI SafeArrayDestroyData16(SAFEARRAY16 *sa)
{
    TRACE("%p\n", sa);

    if (!sa)
        return S_OK;

    if (sa->cLocks)
        return DISP_E_ARRAYISLOCKED;

    if (!(sa->fFeatures & FADF_STATIC))
        safearray_free(sa->pvData);

    return S_OK;
}

/* This implementation of the BSTR API is 16-bit only. It
   represents BSTR as a 16:16 far pointer, and the strings
   as ISO-8859 */

/******************************************************************************
 *		BSTR_AllocBytes	[Internal]
 */
static BSTR16 BSTR_AllocBytes(int n)
{
    void *ptr = HeapAlloc( GetProcessHeap(), 0, n );
    return (BSTR16)MapLS(ptr);
}

/******************************************************************************
 * BSTR_Free [INTERNAL]
 */
static void BSTR_Free(BSTR16 in)
{
    void *ptr = MapSL( (SEGPTR)in );
    UnMapLS( (SEGPTR)in );
    HeapFree( GetProcessHeap(), 0, ptr );
}

/******************************************************************************
 * BSTR_GetAddr [INTERNAL]
 */
static void* BSTR_GetAddr(BSTR16 in)
{
    return in ? MapSL((SEGPTR)in) : 0;
}

/******************************************************************************
 *		SysAllocString	[OLE2DISP.2]
 *
 * Create a BSTR16 from an OLESTR16 (16 Bit).
 *
 * PARAMS
 *  oleStr [I] Source to create BSTR16 from
 *
 * RETURNS
 *  Success: A BSTR16 allocated with SysAllocStringLen16().
 *  Failure: NULL, if oleStr is NULL.
 */
BSTR16 WINAPI SysAllocString16(LPCOLESTR16 oleStr)
{
	BSTR16 out;

	if (!oleStr) return 0;

	out = BSTR_AllocBytes(strlen(oleStr)+1);
	if (!out) return 0;
	strcpy(BSTR_GetAddr(out),oleStr);
	return out;
}

/******************************************************************************
 *		SysReallocString	[OLE2DISP.3]
 *
 * Change the length of a previously created BSTR16 (16 Bit).
 *
 * PARAMS
 *  pbstr  [I] BSTR16 to change the length of
 *  oleStr [I] New source for pbstr
 *
 * RETURNS
 *  Success: 1
 *  Failure: 0.
 *
 * NOTES
 *  SysAllocStringStringLen16().
 */
INT16 WINAPI SysReAllocString16(LPBSTR16 pbstr,LPCOLESTR16 oleStr)
{
	BSTR16 new=SysAllocString16(oleStr);
	BSTR_Free(*pbstr);
	*pbstr=new;
	return 1;
}

/******************************************************************************
 *		SysAllocStringLen	[OLE2DISP.4]
 *
 * Create a BSTR16 from an OLESTR16 of a given character length (16 Bit).
 *
 * PARAMS
 *  oleStr [I] Source to create BSTR16 from
 *  len    [I] Length of oleStr in wide characters
 *
 * RETURNS
 *  Success: A newly allocated BSTR16 from SysAllocStringByteLen16()
 *  Failure: NULL, if len is >= 0x80000000, or memory allocation fails.
 *
 * NOTES
 *  See SysAllocStringByteLen16().
 */
BSTR16 WINAPI SysAllocStringLen16(const char *oleStr, int len)
{
    BSTR16 out=BSTR_AllocBytes(len+1);

    if (!out)
        return 0;

    /*
    * Copy the information in the buffer.
    * Since it is valid to pass a NULL pointer here, we'll initialize the
    * buffer to nul if it is the case.
    */
    if (oleStr != 0)
        strcpy(BSTR_GetAddr(out),oleStr);
    else
        memset(BSTR_GetAddr(out), 0, len+1);

    return out;
}

/******************************************************************************
 *		SysReAllocStringLen	[OLE2DISP.5]
 *
 * Change the length of a previously created BSTR16 (16 Bit).
 *
 * PARAMS
 *  pbstr  [I] BSTR16 to change the length of
 *  oleStr [I] New source for pbstr
 *  len    [I] Length of oleStr in characters
 *
 * RETURNS
 *  Success: 1. The size of pbstr is updated.
 *  Failure: 0, if len >= 0x8000 or memory allocation fails.
 *
 * NOTES
 *  See SysAllocStringByteLen16().
 *  *pbstr may be changed by this function.
 */
int WINAPI SysReAllocStringLen16(BSTR16 *old,const char *in,int len)
{
	/* FIXME: Check input length */
	BSTR16 new=SysAllocStringLen16(in,len);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/******************************************************************************
 *		SysFreeString	[OLE2DISP.6]
 *
 * Free a BSTR16 (16 Bit).
 *
 * PARAMS
 *  str [I] String to free.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI SysFreeString16(BSTR16 str)
{
	BSTR_Free(str);
}

/******************************************************************************
 *		SysStringLen	[OLE2DISP.7]
 *
 * Get the allocated length of a BSTR16 in characters (16 Bit).
 *
 * PARAMS
 *  str [I] BSTR16 to find the length of
 *
 * RETURNS
 *  The allocated length of str, or 0 if str is NULL.
 */
int WINAPI SysStringLen16(BSTR16 str)
{
	return strlen(BSTR_GetAddr(str));
}

/******************************************************************************
 * VariantChangeType [OLE2DISP.12]
 */
HRESULT WINAPI VariantChangeType16(VARIANTARG *vargDest, VARIANTARG *varSrc, unsigned short flags, VARTYPE vt)
{
    FIXME("stub: (%p, %p, %d, %d)\n", vargDest, varSrc, flags, vt);
    return E_NOTIMPL;
}


/******************************************************************************
 * CreateDispTypeInfo [OLE2DISP.31]
 */
HRESULT WINAPI CreateDispTypeInfo16(
	INTERFACEDATA *pidata,
	LCID lcid,
	ITypeInfo **pptinfo)
{
	FIXME("(%p,%ld,%p),stub\n",pidata,lcid,pptinfo);
	return E_NOTIMPL;
}

/******************************************************************************
 * CreateStdDispatch [OLE2DISP.32]
 */
HRESULT WINAPI CreateStdDispatch16(
        IUnknown* punkOuter,
        void* pvThis,
	ITypeInfo* ptinfo,
	IUnknown** ppunkStdDisp)
{
	FIXME("(%p,%p,%p,%p),stub\n",punkOuter, pvThis, ptinfo,
               ppunkStdDisp);
	return 0;
}

/******************************************************************************
 * RegisterActiveObject [OLE2DISP.35]
 */
HRESULT WINAPI RegisterActiveObject16(
	IUnknown *punk, REFCLSID rclsid, DWORD dwFlags, unsigned long *pdwRegister
) {
	FIXME("(%p,%s,0x%08lx,%p):stub\n",punk,debugstr_guid(rclsid),dwFlags,pdwRegister);
	return E_NOTIMPL;
}

/******************************************************************************
 * VariantChangeTypeEx [OLE2DISP.108]
 */
HRESULT WINAPI VariantChangeTypeEx16(VARIANTARG *dest, const VARIANTARG *src, LCID lcid, USHORT flags, VARTYPE vt)
{
        FIXME("stub: %p %p %ld %d %d\n", dest, src, lcid, flags, vt);
        return E_INVALIDARG;
}

/******************************************************************************
 * SetErrorInfo [OLE2DISP.110]
 */
HRESULT WINAPI SetErrorInfo16(ULONG dwReserved, IErrorInfo *perrinfo)
{
        FIXME("stub: (%ld, %p)\n", dwReserved, perrinfo);
        return E_INVALIDARG;
}

/******************************************************************************
 * VariantInit [OLE2DISP.8]
 */
void WINAPI VariantInit16(VARIANTARG16 *v)
{
    TRACE("(%p)\n", v);
    v->vt = VT_EMPTY;
}
