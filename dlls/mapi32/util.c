/*
 * MAPI Utility functions
 *
 * Copyright 2004 Jon Griffiths
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winternl.h"
#include "objbase.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "mapival.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

/**************************************************************************
 *  ScInitMapiUtil (MAPI32.33)
 *
 * Initialise Mapi utility functions.
 *
 * PARAMS
 *  ulReserved [I] Reserved, pass 0.
 *
 * RETURNS
 *  Success: S_OK. Mapi utility functions may be called.
 *  Failure: MAPI_E_INVALID_PARAMETER, if ulReserved is not 0.
 *
 * NOTES
 *  Your application does not need to call this function unless it does not
 *  call MAPIInitialize()/MAPIUninitialize().
 */
SCODE WINAPI ScInitMapiUtil(ULONG ulReserved)
{
    FIXME("(0x%08lx)stub!\n", ulReserved);
    if (ulReserved)
        return MAPI_E_INVALID_PARAMETER;
    return S_OK;
}

/**************************************************************************
 *  DeinitMapiUtil (MAPI32.34)
 *
 * Uninitialise Mapi utility functions.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Your application does not need to call this function unless it does not
 *  call MAPIInitialize()/MAPIUninitialize().
 */
VOID WINAPI DeinitMapiUtil(void)
{
    FIXME("()stub!\n");
}

typedef LPVOID *LPMAPIALLOCBUFFER;

/**************************************************************************
 *  MAPIAllocateBuffer   (MAPI32.12)
 *  MAPIAllocateBuffer@8 (MAPI32.13)
 *
 * Allocate a block of memory.
 *
 * PARAMS
 *  cbSize    [I] Size of the block to allocate in bytes
 *  lppBuffer [O] Destination for pointer to allocated memory
 *
 * RETURNS
 *  Success: S_OK. *lppBuffer is filled with a pointer to a memory block of
 *           length cbSize bytes.
 *  Failure: MAPI_E_INVALID_PARAMETER, if lppBuffer is NULL.
 *           MAPI_E_NOT_ENOUGH_MEMORY, if the memory allocation fails.
 *
 * NOTES
 *  Memory allocated with this function should be freed with MAPIFreeBuffer().
 *  Further allocations of memory may be linked to the pointer returned using
 *  MAPIAllocateMore(). Linked allocations are freed when the initial pointer
 *  is feed.
 */
SCODE WINAPI MAPIAllocateBuffer(ULONG cbSize, LPVOID *lppBuffer)
{
    LPMAPIALLOCBUFFER lpBuff;

    TRACE("(%ld,%p)\n", cbSize, lppBuffer);

    if (!lppBuffer)
        return E_INVALIDARG;

    lpBuff = (LPMAPIALLOCBUFFER)HeapAlloc(GetProcessHeap(), 0, cbSize + sizeof(*lpBuff));
    if (!lpBuff)
        return MAPI_E_NOT_ENOUGH_MEMORY;

    TRACE("initial allocation:%p, returning %p\n", lpBuff, lpBuff + 1);
    *lpBuff++ = NULL;
    *lppBuffer = lpBuff;
    return S_OK;
}

/**************************************************************************
 *  MAPIAllocateMore    (MAPI32.14)
 *  MAPIAllocateMore@12 (MAPI32.15)
 *
 * Allocate a block of memory linked to a previous allocation.
 *
 * PARAMS
 *  cbSize    [I] Size of the block to allocate in bytes
 *  lpOrig    [I] Initial allocation to link to, from MAPIAllocateBuffer()
 *  lppBuffer [O] Destination for pointer to allocated memory
 *
 * RETURNS
 *  Success: S_OK. *lppBuffer is filled with a pointer to a memory block of
 *           length cbSize bytes.
 *  Failure: MAPI_E_INVALID_PARAMETER, if lpOrig or lppBuffer is invalid.
 *           MAPI_E_NOT_ENOUGH_MEMORY, if memory allocation fails.
 *
 * NOTES
 *  Memory allocated with this function and stored in *lppBuffer is freed
 *  when lpOrig is passed to MAPIFreeBuffer(). It should not be freed independently.
 */
SCODE WINAPI MAPIAllocateMore(ULONG cbSize, LPVOID lpOrig, LPVOID *lppBuffer)
{
    LPMAPIALLOCBUFFER lpBuff = lpOrig;

    TRACE("(%ld,%p,%p)\n", cbSize, lpOrig, lppBuffer);

    if (!lppBuffer || !lpBuff || !--lpBuff)
        return E_INVALIDARG;

    /* Find the last allocation in the chain */
    while (*lpBuff)
    {
        TRACE("linked:%p->%p\n", lpBuff, *lpBuff);
        lpBuff = *lpBuff;
    }

    if (SUCCEEDED(MAPIAllocateBuffer(cbSize, lppBuffer)))
    {
        *lpBuff = ((LPMAPIALLOCBUFFER)*lppBuffer) - 1;
        TRACE("linking %p->%p\n", lpBuff, *lpBuff);
    }
    return *lppBuffer ? S_OK : MAPI_E_NOT_ENOUGH_MEMORY;
}

/**************************************************************************
 *  MAPIFreeBuffer   (MAPI32.16)
 *  MAPIFreeBuffer@4 (MAPI32.17)
 *
 * Free a block of memory and any linked allocations associated with it.
 *
 * PARAMS
 *  lpBuffer [I] Memory to free, returned from MAPIAllocateBuffer()
 *
 * RETURNS
 *  S_OK.
 */
ULONG WINAPI MAPIFreeBuffer(LPVOID lpBuffer)
{
    LPMAPIALLOCBUFFER lpBuff = lpBuffer;

    TRACE("(%p)\n", lpBuffer);

    if (lpBuff && --lpBuff)
    {
        while (lpBuff)
        {
            LPVOID lpFree = lpBuff;

            lpBuff = *lpBuff;

            TRACE("linked:%p->%p, freeing %p\n", lpFree, lpBuff, lpFree);
            HeapFree(GetProcessHeap(), 0, lpFree);
        }
    }
    return S_OK;
}

/*************************************************************************
 * HrThisThreadAdviseSink@8 (MAPI32.42)
 *
 * Ensure that an advise sink is only notified in its originating thread.
 *
 * PARAMS
 *  lpSink     [I] IMAPIAdviseSink interface to be protected
 *  lppNewSink [I] Destination for wrapper IMAPIAdviseSink interface
 *
 * RETURNS
 * Success: S_OK. *lppNewSink contains a new sink to use in place of lpSink.
 * Failure: E_INVALIDARG, if any parameter is invalid.
 */
HRESULT WINAPI HrThisThreadAdviseSink(LPMAPIADVISESINK lpSink, LPMAPIADVISESINK* lppNewSink)
{
    FIXME("(%p,%p)semi-stub\n", lpSink, lppNewSink);

    if (!lpSink || !lppNewSink)
        return E_INVALIDARG;

    /* Don't wrap the sink for now, just copy it */
    *lppNewSink = lpSink;
    IMAPIAdviseSink_AddRef(lpSink);
    return S_OK;
}

/*************************************************************************
 * SwapPlong@8 (MAPI32.47)
 *
 * Swap the bytes in a ULONG array.
 *
 * PARAMS
 *  lpData [O] Array to swap bytes in
 *  ulLen  [I] Number of ULONG element to swap the bytes of
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SwapPlong(PULONG lpData, ULONG ulLen)
{
    ULONG i;

    for (i = 0; i < ulLen; i++)
        lpData[i] = RtlUlongByteSwap(lpData[i]);
}

/*************************************************************************
 * SwapPword@8 (MAPI32.48)
 *
 * Swap the bytes in a USHORT array.
 *
 * PARAMS
 *  lpData [O] Array to swap bytes in
 *  ulLen  [I] Number of USHORT element to swap the bytes of
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SwapPword(PUSHORT lpData, ULONG ulLen)
{
    ULONG i;

    for (i = 0; i < ulLen; i++)
        lpData[i] = RtlUshortByteSwap(lpData[i]);
}

/**************************************************************************
 *  MNLS_lstrlenW@4 (MAPI32.62)
 *
 * Calculate the length of a Unicode string.
 *
 * PARAMS
 *  lpszStr [I] String to calculate the length of
 *
 * RETURNS
 *  The length of lpszStr in Unicode characters.
 */
ULONG WINAPI MNLS_lstrlenW(LPCWSTR lpszStr)
{
    TRACE("(%s)\n", debugstr_w(lpszStr));
    return strlenW(lpszStr);
}

/*************************************************************************
 * MNLS_lstrcmpW@8 (MAPI32.63)
 *
 * Compare two Unicode strings.
 *
 * PARAMS
 *  lpszLeft  [I] First string to compare
 *  lpszRight [I] Second string to compare
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszLeft is less than, the same, or greater than lpszRight.
 */
INT WINAPI MNLS_lstrcmpW(LPCWSTR lpszLeft, LPCWSTR lpszRight)
{
    TRACE("(%s,%s)\n", debugstr_w(lpszLeft), debugstr_w(lpszRight));
    return strcmpW(lpszLeft, lpszRight);
}

/*************************************************************************
 * MNLS_lstrcpyW@8 (MAPI32.64)
 *
 * Copy a Unicode string to another string.
 *
 * PARAMS
 *  lpszDest [O] Destination string
 *  lpszSrc  [I] Source string
 *
 * RETURNS
 *  The length lpszDest in Unicode characters.
 */
ULONG WINAPI MNLS_lstrcpyW(LPWSTR lpszDest, LPCWSTR lpszSrc)
{
    ULONG len;

    TRACE("(%p,%s)\n", lpszDest, debugstr_w(lpszSrc));
    len = (strlenW(lpszSrc) + 1) * sizeof(WCHAR);
    memcpy(lpszDest, lpszSrc, len);
    return len;
}

/*************************************************************************
 * MNLS_CompareStringW@12 (MAPI32.65)
 *
 * Compare two Unicode strings.
 *
 * PARAMS
 *  dwCp      [I] Copde page for the comparason
 *  lpszLeft  [I] First string to compare
 *  lpszRight [I] Second string to compare
 *
 * RETURNS
 *  CSTR_LESS_THAN, CSTR_EQUAL or CSTR_GREATER_THAN, indicating that
 *  lpszLeft is less than, the same, or greater than lpszRight.
 */
INT WINAPI MNLS_CompareStringW(DWORD dwCp, LPCWSTR lpszLeft, LPCWSTR lpszRight)
{
    INT ret;

    TRACE("0x%08lx,%s,%s\n", dwCp, debugstr_w(lpszLeft), debugstr_w(lpszRight));
    ret = MNLS_lstrcmpW(lpszLeft, lpszRight);
    return ret < 0 ? CSTR_LESS_THAN : ret ? CSTR_GREATER_THAN : CSTR_EQUAL;
}


/**************************************************************************
 *  FtAddFt@16 (MAPI32.121)
 *
 * Add two FILETIME's together.
 *
 * PARAMS
 *  ftLeft  [I] FILETIME to add to ftRight
 *  ftRight [I] FILETIME to add to ftLeft
 *
 * RETURNS
 *  The sum of ftLeft and ftRight
 */
LONGLONG WINAPI MAPI32_FtAddFt(FILETIME ftLeft, FILETIME ftRight)
{
    LONGLONG *pl = (LONGLONG*)&ftLeft, *pr = (LONGLONG*)&ftRight;
    
    return *pl + *pr;
}

/**************************************************************************
 *  FtSubFt@16 (MAPI32.123)
 *
 * Subtract two FILETIME's together.
 *
 * PARAMS
 *  ftLeft  [I] Initial FILETIME 
 *  ftRight [I] FILETIME to subtract from ftLeft
 *
 * RETURNS
 *  The remainder after ftRight is subtracted from ftLeft.
 */
LONGLONG WINAPI MAPI32_FtSubFt(FILETIME ftLeft, FILETIME ftRight)
{
    LONGLONG *pl = (LONGLONG*)&ftLeft, *pr = (LONGLONG*)&ftRight;
    
    return *pr - *pl;
}

/**************************************************************************
 *  FtMulDw@12 (MAPI32.124)
 *
 * Multiply a FILETIME by a DWORD.
 *
 * PARAMS
 *  dwLeft  [I] DWORD to multiply with ftRight
 *  ftRight [I] FILETIME to multiply with dwLeft
 *
 * RETURNS
 *  The product of dwLeft and ftRight
 */
LONGLONG WINAPI MAPI32_FtMulDw(DWORD dwLeft, FILETIME ftRight)
{
    LONGLONG *pr = (LONGLONG*)&ftRight;
    
    return (LONGLONG)dwLeft * (*pr);
}
 
/**************************************************************************
 *  FtMulDwDw@8 (MAPI32.125)
 *
 * Multiply two DWORD, giving the result as a FILETIME.
 *
 * PARAMS
 *  dwLeft  [I] DWORD to multiply with dwRight
 *  dwRight [I] DWORD to multiply with dwLeft
 *
 * RETURNS
 *  The product of ftMultiplier and ftMultiplicand as a FILETIME.
 */
LONGLONG WINAPI MAPI32_FtMulDwDw(DWORD dwLeft, DWORD dwRight)
{
    return (LONGLONG)dwLeft * (LONGLONG)dwRight;
}

/**************************************************************************
 *  FtNegFt@8 (MAPI32.126)
 *
 * Negate a FILETIME.
 *
 * PARAMS
 *  ft [I] FILETIME to negate
 *
 * RETURNS
 *  The negation of ft.
 */
LONGLONG WINAPI MAPI32_FtNegFt(FILETIME ft)
{
    LONGLONG *p = (LONGLONG*)&ft;
    
    return - *p;
}

/**************************************************************************
 *  UlAddRef@4 (MAPI32.128)
 *
 * Add a reference to an object.
 *
 * PARAMS
 *  lpUnk [I] Object to add a reference to.
 *
 * RETURNS
 *  The new reference count of the object, or 0 if lpUnk is NULL.
 *
 * NOTES
 * See IUnknown_AddRef.
 */
ULONG WINAPI UlAddRef(void *lpUnk)
{
    TRACE("(%p)\n", lpUnk);

    if (!lpUnk)
        return 0UL;
    return IUnknown_AddRef((LPUNKNOWN)lpUnk);
}

/**************************************************************************
 *  UlRelease@4 (MAPI32.129)
 *
 * Remove a reference from an object.
 *
 * PARAMS
 *  lpUnk [I] Object to remove reference from.
 *
 * RETURNS
 *  The new reference count of the object, or 0 if lpUnk is NULL. If lpUnk is
 *  non-NULL and this function returns 0, the object pointed to by lpUnk has
 *  been released.
 *
 * NOTES
 * See IUnknown_Release.
 */
ULONG WINAPI UlRelease(void *lpUnk)
{
    TRACE("(%p)\n", lpUnk);

    if (!lpUnk)
        return 0UL;
    return IUnknown_Release((LPUNKNOWN)lpUnk);
}

/*************************************************************************
 * OpenStreamOnFile@24 (MAPI32.147)
 *
 * Create a stream on a file.
 *
 * PARAMS
 *  lpAlloc    [I] Memory allocation function
 *  lpFree     [I] Memory free function
 *  ulFlags    [I] Flags controlling the opening process
 *  lpszPath   [I] Path of file to create stream on
 *  lpszPrefix [I] Prefix of the temporary file name (if ulFlags includes SOF_UNIQUEFILENAME)
 *  lppStream  [O] Destination for created stream
 *
 * RETURNS
 * Success: S_OK. lppStream contains the new stream object
 * Failure: E_INVALIDARG if any parameter is invalid, or an HRESULT error code
 *          describing the error.
 */
HRESULT WINAPI OpenStreamOnFile(LPALLOCATEBUFFER lpAlloc, LPFREEBUFFER lpFree,
                                ULONG ulFlags, LPWSTR lpszPath, LPWSTR lpszPrefix,
                                LPSTREAM *lppStream)
{
    WCHAR szBuff[MAX_PATH];
    DWORD dwMode = STGM_READWRITE, dwAttributes = 0;
    HRESULT hRet;

    TRACE("(%p,%p,0x%08lx,%s,%s,%p)\n", lpAlloc, lpFree, ulFlags,
          debugstr_a((LPSTR)lpszPath), debugstr_a((LPSTR)lpszPrefix), lppStream);

    if (lppStream)
        *lppStream = NULL;

    if (ulFlags & SOF_UNIQUEFILENAME)
    {
        FIXME("Should generate a temporary name\n");
        return E_INVALIDARG;
    }

    if (!lpszPath || !lppStream)
        return E_INVALIDARG;

    /* FIXME: Should probably munge mode and attributes, and should handle
     *        Unicode arguments (I assume MAPI_UNICODE is set in ulFlags if
     *        we are being passed Unicode strings; MSDN doesn't say).
     *        This implementation is just enough for Outlook97 to start.
     */
    MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpszPath, -1, szBuff, MAX_PATH);
    hRet = SHCreateStreamOnFileEx(szBuff, dwMode, dwAttributes, TRUE,
                                  NULL, lppStream);
    return hRet;
}
