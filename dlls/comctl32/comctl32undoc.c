/*
 * Undocumented functions from COMCTL32.DLL
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *           1998 Juergen Schmied <j.schmied@metronet.de>
 * NOTES
 *     All of these functions are UNDOCUMENTED!! And I mean UNDOCUMENTED!!!!
 *     Do NOT rely on names or contents of undocumented structures and types!!!
 *     These functions are used by EXPLORER.EXE, IEXPLORE.EXE and
 *     COMCTL32.DLL (internally).
 *
 * TODO
 *     - Add more functions.
 *     - Write some documentation.
 */

#include <string.h>
#include <stdlib.h> /* atoi */
#include <ctype.h>

#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "commctrl.h"
#include "crtdll.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(commctrl)


extern HANDLE COMCTL32_hHeap; /* handle to the private heap */

/*
 * We put some function prototypes here that don't seem to belong in
 * any header file. When they find their place, we can remove them.
 */
extern LPSTR WINAPI lstrrchr(LPCSTR, LPCSTR, WORD);
extern LPWSTR WINAPI lstrrchrw(LPCWSTR, LPCWSTR, WORD);


typedef struct _STREAMDATA
{
    DWORD dwSize;
    DWORD dwData2;
    DWORD dwItems;
} STREAMDATA, *PSTREAMDATA;

typedef struct _LOADDATA
{
    INT   nCount;
    PVOID ptr;
} LOADDATA, *LPLOADDATA;

typedef HRESULT(CALLBACK *DPALOADPROC)(LPLOADDATA,IStream*,LPARAM);


/**************************************************************************
 * DPA_LoadStream [COMCTL32.9]
 *
 * Loads a dynamic pointer array from a stream
 *
 * PARAMS
 *     phDpa    [O] pointer to a handle to a dynamic pointer array
 *     loadProc [I] pointer to a callback function
 *     pStream  [I] pointer to a stream
 *     lParam   [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

HRESULT WINAPI
DPA_LoadStream (HDPA *phDpa, DPALOADPROC loadProc, IStream *pStream, LPARAM lParam)
{
    HRESULT errCode;
    LARGE_INTEGER position;
    ULARGE_INTEGER newPosition;
    STREAMDATA  streamData;
    LOADDATA loadData;
    ULONG ulRead;
    HDPA hDpa;
    PVOID *ptr;

    FIXME ("phDpa=%p loadProc=%p pStream=%p lParam=%lx\n",
	   phDpa, loadProc, pStream, lParam);

    if (!phDpa || !loadProc || !pStream)
	return E_INVALIDARG;

    *phDpa = (HDPA)NULL;

    position.LowPart = 0;
    position.HighPart = 0;

    errCode = IStream_Seek (pStream, position, STREAM_SEEK_CUR, &newPosition);
    if (errCode != S_OK)
	return errCode;

    errCode = IStream_Read (pStream, &streamData, sizeof(STREAMDATA), &ulRead);
    if (errCode != S_OK)
	return errCode;

    FIXME ("dwSize=%lu dwData2=%lu dwItems=%lu\n",
	   streamData.dwSize, streamData.dwData2, streamData.dwItems);

    if (lParam < sizeof(STREAMDATA) ||
	streamData.dwSize < sizeof(STREAMDATA) ||
	streamData.dwData2 < 1) {
	errCode = E_FAIL;
    }

    /* create the dpa */
    hDpa = DPA_Create (streamData.dwItems);
    if (!hDpa)
	return E_OUTOFMEMORY;

    if (!DPA_Grow (hDpa, streamData.dwItems))
	return E_OUTOFMEMORY;

    /* load data from the stream into the dpa */
    ptr = hDpa->ptrs;
    for (loadData.nCount = 0; loadData.nCount < streamData.dwItems; loadData.nCount++) {
        errCode = (loadProc)(&loadData, pStream, lParam);
	if (errCode != S_OK) {
	    errCode = S_FALSE;
	    break;
	}

	*ptr = loadData.ptr;
	ptr++;
    }

    /* set the number of items */
    hDpa->nItemCount = loadData.nCount;

    /* store the handle to the dpa */
    *phDpa = hDpa;
    FIXME ("new hDpa=%p\n", hDpa);

    return errCode;
}


/**************************************************************************
 * DPA_SaveStream [COMCTL32.10]
 *
 * Saves a dynamic pointer array to a stream
 *
 * PARAMS
 *     hDpa     [I] handle to a dynamic pointer array
 *     loadProc [I] pointer to a callback function
 *     pStream  [I] pointer to a stream
 *     lParam   [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

HRESULT WINAPI
DPA_SaveStream (const HDPA hDpa, DPALOADPROC loadProc, IStream *pStream, LPARAM lParam)
{

    FIXME ("hDpa=%p loadProc=%p pStream=%p lParam=%lx\n",
	   hDpa, loadProc, pStream, lParam);

    return E_FAIL;
}


/**************************************************************************
 * DPA_Merge [COMCTL32.11]
 *
 * PARAMS
 *     hdpa1    [I] handle to a dynamic pointer array
 *     hdpa2    [I] handle to a dynamic pointer array
 *     dwFlags  [I] flags
 *     pfnSort  [I] pointer to sort function
 *     pfnMerge [I] pointer to merge function
 *     lParam   [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

BOOL WINAPI
DPA_Merge (const HDPA hdpa1, const HDPA hdpa2, DWORD dwFlags,
	   PFNDPACOMPARE pfnCompare, PFNDPAMERGE pfnMerge, LPARAM lParam)
{
    INT nCount;

#if 0    /* these go with the "incomplete implementation" below */
    LPVOID pWork1, pWork2;
    INT nResult;
    INT nIndex;
    INT nNewItems;
#endif

    TRACE("(%p %p %08lx %p %p %08lx): semi stub!\n",
	   hdpa1, hdpa2, dwFlags, pfnCompare, pfnMerge, lParam);

    if (IsBadWritePtr (hdpa1, sizeof(DPA)))
	return FALSE;

    if (IsBadWritePtr (hdpa2, sizeof(DPA)))
	return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnCompare))
	return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnMerge))
	return FALSE;

    if (dwFlags & DPAM_SORT) {
	TRACE("sorting dpa's!\n");
	if (hdpa1->nItemCount > 0)
	DPA_Sort (hdpa1, pfnCompare, lParam);
	TRACE ("dpa 1 sorted!\n");
	if (hdpa2->nItemCount > 0)
	DPA_Sort (hdpa2, pfnCompare, lParam);
	TRACE ("dpa 2 sorted!\n");
    }

    if (hdpa2->nItemCount < 1)
	return TRUE;

    TRACE("hdpa1->nItemCount=%d hdpa2->nItemCount=%d\n",
	   hdpa1->nItemCount, hdpa2->nItemCount);


    /* preliminary hack - simply append the pointer list hdpa2 to hdpa1*/
    for (nCount = 0; nCount < hdpa2->nItemCount; nCount++)
	DPA_InsertPtr (hdpa1, hdpa1->nItemCount + 1, hdpa2->ptrs[nCount]);

#if 0
    /* incomplete implementation */

    pWork1 = &(hdpa1->ptrs[hdpa1->nItemCount - 1]);
    pWork2 = &(hdpa2->ptrs[hdpa2->nItemCount - 1]);

    nIndex = hdpa1->nItemCount - 1;
    nCount = hdpa2->nItemCount - 1;

    do
    {
	nResult = (pfnCompare)(pWork1, pWork2, lParam);

	if (nResult == 0)
	{
	    PVOID ptr;

	    ptr = (pfnMerge)(1, pWork1, pWork2, lParam);
	    if (!ptr)
		return FALSE;

	    nCount--;
	    pWork2--;
	    pWork1 = ptr;
	}
	else if (nResult < 0)
	{
	    if (!dwFlags & 8)
	    {
		PVOID ptr;

		ptr = DPA_DeletePtr (hdpa1, hdpa1->nItemCount - 1);

		(pfnMerge)(2, ptr, NULL, lParam);
	    }
	}
	else
	{
	    if (!dwFlags & 4)
	    {
		PVOID ptr;

		ptr = (pfnMerge)(3, pWork2, NULL, lParam);
		if (!ptr)
		    return FALSE;
		DPA_InsertPtr (hdpa1, nIndex, ptr);
    }
	    nCount--;
	    pWork2--;
	}

	nIndex--;
	pWork1--;

    }
    while (nCount >= 0);
#endif

    return TRUE;
}


/**************************************************************************
 * Alloc [COMCTL32.71]
 *
 * Allocates memory block from the dll's private heap
 *
 * PARAMS
 *     dwSize [I] size of the allocated memory block
 *
 * RETURNS
 *     Success: pointer to allocated memory block
 *     Failure: NULL
 */

LPVOID WINAPI
COMCTL32_Alloc (DWORD dwSize)
{
    LPVOID lpPtr;

    TRACE("(0x%lx)\n", dwSize);

    lpPtr = HeapAlloc (COMCTL32_hHeap, HEAP_ZERO_MEMORY, dwSize);

    TRACE("-- ret=%p\n", lpPtr);

    return lpPtr;
}


/**************************************************************************
 * ReAlloc [COMCTL32.72]
 *
 * Changes the size of an allocated memory block or allocates a memory
 * block using the dll's private heap.
 *
 * PARAMS
 *     lpSrc  [I] pointer to memory block which will be resized
 *     dwSize [I] new size of the memory block.
 *
 * RETURNS
 *     Success: pointer to the resized memory block
 *     Failure: NULL
 *
 * NOTES
 *     If lpSrc is a NULL-pointer, then COMCTL32_ReAlloc allocates a memory
 *     block like COMCTL32_Alloc.
 */

LPVOID WINAPI
COMCTL32_ReAlloc (LPVOID lpSrc, DWORD dwSize)
{
    LPVOID lpDest;

    TRACE("(%p 0x%08lx)\n", lpSrc, dwSize);

    if (lpSrc)
	lpDest = HeapReAlloc (COMCTL32_hHeap, HEAP_ZERO_MEMORY, lpSrc, dwSize);
    else
	lpDest = HeapAlloc (COMCTL32_hHeap, HEAP_ZERO_MEMORY, dwSize);

    TRACE("-- ret=%p\n", lpDest);

    return lpDest;
}


/**************************************************************************
 * Free [COMCTL32.73]
 *
 * Frees an allocated memory block from the dll's private heap.
 *
 * PARAMS
 *     lpMem [I] pointer to memory block which will be freed
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
COMCTL32_Free (LPVOID lpMem)
{
    TRACE("(%p)\n", lpMem);

    return HeapFree (COMCTL32_hHeap, 0, lpMem);
}


/**************************************************************************
 * GetSize [COMCTL32.74]
 *
 * Retrieves the size of the specified memory block from the dll's
 * private heap.
 *
 * PARAMS
 *     lpMem [I] pointer to an allocated memory block
 *
 * RETURNS
 *     Success: size of the specified memory block
 *     Failure: 0
 */

DWORD WINAPI
COMCTL32_GetSize (LPVOID lpMem)
{
    TRACE("(%p)\n", lpMem);

    return HeapSize (COMCTL32_hHeap, 0, lpMem);
}


/**************************************************************************
 * The MRU-API is a set of functions to manipulate MRU(Most Recently Used)
 * lists.
 *
 *
 */

typedef struct tagMRUINFO
{
    DWORD  dwParam1;
    DWORD  dwParam2;
    DWORD  dwParam3;
    HKEY   hkeyMain;
    LPCSTR lpszSubKey;
    DWORD  dwParam6;
} MRUINFO, *LPMRUINFO;

 
typedef struct tagMRU
{
    DWORD  dwParam1;  /* some kind of flag */
    DWORD  dwParam2;
    DWORD  dwParam3;
    HKEY   hkeyMRU;
    LPCSTR lpszSubKey;
    DWORD  dwParam6;
} MRU, *HMRU;

LPVOID WINAPI
CreateMRUListLazyA (LPMRUINFO lpmi, DWORD dwParam2,
		    DWORD dwParam3, DWORD dwParam4);


/**************************************************************************
 * CreateMRUListA [COMCTL32.151]
 *
 * PARAMS
 *     dwParam
 *
 * RETURNS
 */

LPVOID WINAPI
CreateMRUListA (LPMRUINFO lpmi)
{
     return CreateMRUListLazyA (lpmi, 0, 0, 0);
}


DWORD WINAPI
FreeMRUListA (HMRU hmru)
{
    FIXME("(%p) empty stub!\n", hmru);

#if 0
    if (!(hmru->dwParam1 & 1001)) {
	RegSetValueExA (hmru->hKeyMRU, "MRUList", 0, REG_SZ,
			  hmru->lpszMRUString,
			  lstrlenA (hmru->lpszMRUString));
    }


    RegClosKey (hmru->hkeyMRU
    COMCTL32_Free32 (hmru->lpszMRUString);
#endif

    return COMCTL32_Free (hmru);
}



DWORD WINAPI
AddMRUData (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{

    FIXME("(%lx %lx %lx) empty stub!\n",
	   dwParam1, dwParam2, dwParam3);

    return 0;
}


DWORD WINAPI
FindMRUData (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{

    FIXME("(%lx %lx %lx %lx) empty stub!\n",
	   dwParam1, dwParam2, dwParam3, dwParam4);

    return TRUE;
}


LPVOID WINAPI
CreateMRUListLazyA (LPMRUINFO lpmi, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{
    /* DWORD  dwLocal1;   *
     * HKEY   hkeyResult; *
     * DWORD  dwLocal3;   *
     * LPVOID lMRU;       *
     * DWORD  dwLocal5;   *
     * DWORD  dwLocal6;   *
     * DWORD  dwLocal7;   *
     * DWORD  dwDisposition; */

    /* internal variables */
    LPVOID ptr;

    FIXME("(%p) empty stub!\n", lpmi);

    if (lpmi) {
	FIXME("(%lx %lx %lx %lx \"%s\" %lx)\n",
	       lpmi->dwParam1, lpmi->dwParam2, lpmi->dwParam3,
	       (DWORD)lpmi->hkeyMain, lpmi->lpszSubKey, lpmi->dwParam6);
    }

    /* dummy pointer creation */
    ptr = COMCTL32_Alloc (32);

    FIXME("-- ret = %p\n", ptr);

    return ptr;
}




/**************************************************************************
 * Str_GetPtrA [COMCTL32.233]
 *
 * PARAMS
 *     lpSrc   [I]
 *     lpDest  [O]
 *     nMaxLen [I]
 *
 * RETURNS
 */

INT WINAPI
Str_GetPtrA (LPCSTR lpSrc, LPSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

    if (!lpDest && lpSrc)
	return lstrlenA (lpSrc);

    if (nMaxLen == 0)
	return 0;

    if (lpSrc == NULL) {
	lpDest[0] = '\0';
	return 0;
    }

    len = lstrlenA (lpSrc);
    if (len >= nMaxLen)
	len = nMaxLen - 1;

    RtlMoveMemory (lpDest, lpSrc, len);
    lpDest[len] = '\0';

    return len;
}


/**************************************************************************
 * Str_SetPtrA [COMCTL32.234]
 *
 * PARAMS
 *     lppDest [O]
 *     lpSrc   [I]
 *
 * RETURNS
 */

BOOL WINAPI
Str_SetPtrA (LPSTR *lppDest, LPCSTR lpSrc)
{
    TRACE("(%p %p)\n", lppDest, lpSrc);
 
    if (lpSrc) {
	LPSTR ptr = COMCTL32_ReAlloc (*lppDest, lstrlenA (lpSrc) + 1);
	if (!ptr)
	    return FALSE;
	lstrcpyA (ptr, lpSrc);
	*lppDest = ptr;
    }
    else {
	if (*lppDest) {
	    COMCTL32_Free (*lppDest);
	    *lppDest = NULL;
	}
    }

    return TRUE;
}


/**************************************************************************
 * Str_GetPtrW [COMCTL32.235]
 *
 * PARAMS
 *     lpSrc   [I]
 *     lpDest  [O]
 *     nMaxLen [I]
 *
 * RETURNS
 */

INT WINAPI
Str_GetPtrW (LPCWSTR lpSrc, LPWSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

    if (!lpDest && lpSrc)
	return lstrlenW (lpSrc);

    if (nMaxLen == 0)
	return 0;

    if (lpSrc == NULL) {
	lpDest[0] = L'\0';
	return 0;
    }

    len = lstrlenW (lpSrc);
    if (len >= nMaxLen)
	len = nMaxLen - 1;

    RtlMoveMemory (lpDest, lpSrc, len*sizeof(WCHAR));
    lpDest[len] = L'\0';

    return len;
}


/**************************************************************************
 * Str_SetPtrW [COMCTL32.236]
 *
 * PARAMS
 *     lpDest [O]
 *     lpSrc  [I]
 *
 * RETURNS
 */

BOOL WINAPI
Str_SetPtrW (LPWSTR *lppDest, LPCWSTR lpSrc)
{
    TRACE("(%p %p)\n", lppDest, lpSrc);
 
    if (lpSrc) {
	INT len = lstrlenW (lpSrc) + 1;
	LPWSTR ptr = COMCTL32_ReAlloc (*lppDest, len * sizeof(WCHAR));
	if (!ptr)
	    return FALSE;
	lstrcpyW (ptr, lpSrc);
	*lppDest = ptr;
    }
    else {
	if (*lppDest) {
	    COMCTL32_Free (*lppDest);
	    *lppDest = NULL;
	}
    }

    return TRUE;
}


/**************************************************************************
 * The DSA-API is a set of functions to create and manipulate arrays of
 * fix sized memory blocks. These arrays can store any kind of data
 * (strings, icons...).
 */

/**************************************************************************
 * DSA_Create [COMCTL32.320] Creates a dynamic storage array
 *
 * PARAMS
 *     nSize [I] size of the array elements
 *     nGrow [I] number of elements by which the array grows when it is filled
 *
 * RETURNS
 *     Success: pointer to a array control structure. use this like a handle.
 *     Failure: NULL
 */

HDSA WINAPI
DSA_Create (INT nSize, INT nGrow)
{
    HDSA hdsa;

    TRACE("(size=%d grow=%d)\n", nSize, nGrow);

    hdsa = (HDSA)COMCTL32_Alloc (sizeof(DSA));
    if (hdsa)
    {
	hdsa->nItemCount = 0;
        hdsa->pData = NULL;
	hdsa->nMaxCount = 0;
	hdsa->nItemSize = nSize;
	hdsa->nGrow = MAX(1, nGrow);
    }

    return hdsa;
}


/**************************************************************************
 * DSA_Destroy [COMCTL32.321] Destroys a dynamic storage array
 *
 * PARAMS
 *     hdsa [I] pointer to the array control structure
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_Destroy (const HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa)
	return FALSE;

    if (hdsa->pData && (!COMCTL32_Free (hdsa->pData)))
	return FALSE;

    return COMCTL32_Free (hdsa);
}


/**************************************************************************
 * DSA_GetItem [COMCTL32.322] 
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] number of the Item to get
 *     pDest  [O] destination buffer. Has to be >= dwElementSize.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_GetItem (const HDSA hdsa, INT nIndex, LPVOID pDest)
{
    LPVOID pSrc;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pDest);
    
    if (!hdsa)
	return FALSE;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return FALSE;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    memmove (pDest, pSrc, hdsa->nItemSize);

    return TRUE;
}


/**************************************************************************
 * DSA_GetItemPtr [COMCTL32.323] 
 *
 * Retrieves a pointer to the specified item.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index of the desired item
 *
 * RETURNS
 *     Success: pointer to an item
 *     Failure: NULL
 */

LPVOID WINAPI
DSA_GetItemPtr (const HDSA hdsa, INT nIndex)
{
    LPVOID pSrc;

    TRACE("(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return NULL;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return NULL;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    
    TRACE("-- ret=%p\n", pSrc);

    return  pSrc;
}


/**************************************************************************
 * DSA_SetItem [COMCTL32.325] 
 *
 * Sets the contents of an item in the array.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the item
 *     pSrc   [I] pointer to the new item data
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_SetItem (const HDSA hdsa, INT nIndex, LPVOID pSrc)
{
    INT  nSize, nNewItems;
    LPVOID pDest, lpTemp;
    
    TRACE("(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return FALSE;
      
    if (hdsa->nItemCount <= nIndex) {
	/* within the old array */
	if (hdsa->nMaxCount > nIndex) {
	    /* within the allocated space, set a new boundary */
	    hdsa->nItemCount = nIndex + 1;
	}
	else {
	    /* resize the block of memory */
	    nNewItems =
		hdsa->nGrow * ((INT)((nIndex - 1) / hdsa->nGrow) + 1);
	    nSize = hdsa->nItemSize * nNewItems;

	    lpTemp = (LPVOID)COMCTL32_ReAlloc (hdsa->pData, nSize);
	    if (!lpTemp)
		return FALSE;

	    hdsa->nMaxCount = nNewItems;
	    hdsa->nItemCount = nIndex + 1;
	    hdsa->pData = lpTemp;
	}    
    }

    /* put the new entry in */
    pDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE("-- move dest=%p src=%p size=%d\n",
	   pDest, pSrc, hdsa->nItemSize);
    memmove (pDest, pSrc, hdsa->nItemSize);

    return TRUE;
}


/**************************************************************************
 * DSA_InsertItem [COMCTL32.325] 
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the new item
 *     pSrc   [I] pointer to the element
 *
 * RETURNS
 *     Success: position of the new item
 *     Failure: -1
 */

INT WINAPI
DSA_InsertItem (const HDSA hdsa, INT nIndex, LPVOID pSrc)
{
    INT   nNewItems, nSize, i;
    LPVOID  lpTemp, lpDest;
    LPDWORD p;
    
    TRACE("(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return -1;

    for (i = 0; i < hdsa->nItemSize; i += 4) {
	p = *(DWORD**)((char *) pSrc + i);
	if (IsBadStringPtrA ((char*)p, 256))
	    TRACE("-- %d=%p\n", i, (DWORD*)p);
	else
	    TRACE("-- %d=%p [%s]\n", i, p, debugstr_a((char*)p));
    }
   
    /* when nIndex > nItemCount then append */
    if (nIndex >= hdsa->nItemCount)
 	nIndex = hdsa->nItemCount;

    /* do we need to resize ? */
    if (hdsa->nItemCount >= hdsa->nMaxCount) {
	nNewItems = hdsa->nMaxCount + hdsa->nGrow;
	nSize = hdsa->nItemSize * nNewItems;

	lpTemp = (LPVOID)COMCTL32_ReAlloc (hdsa->pData, nSize);
	if (!lpTemp)
	    return -1;

	hdsa->nMaxCount = nNewItems;
	hdsa->pData = lpTemp;         
    }

    /* do we need to move elements ? */
    if (nIndex < hdsa->nItemCount) {
	lpTemp = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
	lpDest = (char *) lpTemp + hdsa->nItemSize;
	nSize = (hdsa->nItemCount - nIndex) * hdsa->nItemSize;
	TRACE("-- move dest=%p src=%p size=%d\n",
	       lpDest, lpTemp, nSize);
	memmove (lpDest, lpTemp, nSize);
    }

    /* ok, we can put the new Item in */
    hdsa->nItemCount++;
    lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE("-- move dest=%p src=%p size=%d\n",
	   lpDest, pSrc, hdsa->nItemSize);
    memmove (lpDest, pSrc, hdsa->nItemSize);

    return hdsa->nItemCount;
}


/**************************************************************************
 * DSA_DeleteItem [COMCTL32.326] 
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the element to delete
 *
 * RETURNS
 *     Success: number of the deleted element
 *     Failure: -1
 */

INT WINAPI
DSA_DeleteItem (const HDSA hdsa, INT nIndex)
{
    LPVOID lpDest,lpSrc;
    INT  nSize;
    
    TRACE("(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return -1;
    if (nIndex < 0 || nIndex >= hdsa->nItemCount)
	return -1;

    /* do we need to move ? */
    if (nIndex < hdsa->nItemCount - 1) {
	lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
	lpSrc = (char *) lpDest + hdsa->nItemSize;
	nSize = hdsa->nItemSize * (hdsa->nItemCount - nIndex - 1);
	TRACE("-- move dest=%p src=%p size=%d\n",
	       lpDest, lpSrc, nSize);
	memmove (lpDest, lpSrc, nSize);
    }
    
    hdsa->nItemCount--;
    
    /* free memory ? */
    if ((hdsa->nMaxCount - hdsa->nItemCount) >= hdsa->nGrow) {
	nSize = hdsa->nItemSize * hdsa->nItemCount;

	lpDest = (LPVOID)COMCTL32_ReAlloc (hdsa->pData, nSize);
	if (!lpDest)
	    return -1;

	hdsa->nMaxCount = hdsa->nItemCount;
	hdsa->pData = lpDest;
    }

    return nIndex;
}


/**************************************************************************
 * DSA_DeleteAllItems [COMCTL32.326]
 *
 * Removes all items and reinitializes the array.
 *
 * PARAMS
 *     hdsa [I] pointer to the array control structure
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_DeleteAllItems (const HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa) 
	return FALSE;
    if (hdsa->pData && (!COMCTL32_Free (hdsa->pData)))
	return FALSE;

    hdsa->nItemCount = 0;
    hdsa->pData = NULL;
    hdsa->nMaxCount = 0;

    return TRUE;
}


/**************************************************************************
 * The DPA-API is a set of functions to create and manipulate arrays of
 * pointers.
 */

/**************************************************************************
 * DPA_Create [COMCTL32.328] Creates a dynamic pointer array
 *
 * PARAMS
 *     nGrow [I] number of items by which the array grows when it is filled
 *
 * RETURNS
 *     Success: handle (pointer) to the pointer array.
 *     Failure: NULL
 */

HDPA WINAPI
DPA_Create (INT nGrow)
{
    HDPA hdpa;

    TRACE("(%d)\n", nGrow);

    hdpa = (HDPA)COMCTL32_Alloc (sizeof(DPA));
    if (hdpa) {
	hdpa->nGrow = MAX(8, nGrow);
	hdpa->hHeap = COMCTL32_hHeap;
	hdpa->nMaxCount = hdpa->nGrow * 2;
	hdpa->ptrs =
	    (LPVOID*)COMCTL32_Alloc (hdpa->nMaxCount * sizeof(LPVOID));
    }

    TRACE("-- %p\n", hdpa);

    return hdpa;
}


/**************************************************************************
 * DPA_Destroy [COMCTL32.329] Destroys a dynamic pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_Destroy (const HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa)
	return FALSE;

    if (hdpa->ptrs && (!HeapFree (hdpa->hHeap, 0, hdpa->ptrs)))
	return FALSE;

    return HeapFree (hdpa->hHeap, 0, hdpa);
}


/**************************************************************************
 * DPA_Grow [COMCTL32.330]
 *
 * Sets the growth amount.
 *
 * PARAMS
 *     hdpa  [I] handle (pointer) to the existing (source) pointer array
 *     nGrow [I] number of items, the array grows, when it's too small
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_Grow (const HDPA hdpa, INT nGrow)
{
    TRACE("(%p %d)\n", hdpa, nGrow);

    if (!hdpa)
	return FALSE;

    hdpa->nGrow = MAX(8, nGrow);

    return TRUE;
}


/**************************************************************************
 * DPA_Clone [COMCTL32.331]
 *
 * Copies a pointer array to an other one or creates a copy
 *
 * PARAMS
 *     hdpa    [I] handle (pointer) to the existing (source) pointer array
 *     hdpaNew [O] handle (pointer) to the destination pointer array
 *
 * RETURNS
 *     Success: pointer to the destination pointer array.
 *     Failure: NULL
 *
 * NOTES
 *     - If the 'hdpaNew' is a NULL-Pointer, a copy of the source pointer
 *       array will be created and it's handle (pointer) is returned.
 *     - If 'hdpa' is a NULL-Pointer, the original implementation crashes,
 *       this implementation just returns NULL.
 */

HDPA WINAPI
DPA_Clone (const HDPA hdpa, const HDPA hdpaNew)
{
    INT nNewItems, nSize;
    HDPA hdpaTemp;

    if (!hdpa)
	return NULL;

    TRACE("(%p %p)\n", hdpa, hdpaNew);

    if (!hdpaNew) {
	/* create a new DPA */
	hdpaTemp = (HDPA)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				    sizeof(DPA));
	hdpaTemp->hHeap = hdpa->hHeap;
	hdpaTemp->nGrow = hdpa->nGrow;
    }
    else
	hdpaTemp = hdpaNew;

    if (hdpaTemp->ptrs) {
	/* remove old pointer array */
	HeapFree (hdpaTemp->hHeap, 0, hdpaTemp->ptrs);
	hdpaTemp->ptrs = NULL;
	hdpaTemp->nItemCount = 0;
	hdpaTemp->nMaxCount = 0;
    }

    /* create a new pointer array */
    nNewItems = hdpaTemp->nGrow *
		((INT)((hdpa->nItemCount - 1) / hdpaTemp->nGrow) + 1);
    nSize = nNewItems * sizeof(LPVOID);
    hdpaTemp->ptrs =
	(LPVOID*)HeapAlloc (hdpaTemp->hHeap, HEAP_ZERO_MEMORY, nSize);
    hdpaTemp->nMaxCount = nNewItems;

    /* clone the pointer array */
    hdpaTemp->nItemCount = hdpa->nItemCount;
    memmove (hdpaTemp->ptrs, hdpa->ptrs,
	     hdpaTemp->nItemCount * sizeof(LPVOID));

    return hdpaTemp;
}


/**************************************************************************
 * DPA_GetPtr [COMCTL32.332]
 *
 * Retrieves a pointer from a dynamic pointer array
 *
 * PARAMS
 *     hdpa   [I] handle (pointer) to the pointer array
 *     nIndex [I] array index of the desired pointer
 *
 * RETURNS
 *     Success: pointer
 *     Failure: NULL
 */

LPVOID WINAPI
DPA_GetPtr (const HDPA hdpa, INT i)
{
    TRACE("(%p %d)\n", hdpa, i);

    if (!hdpa)
	return NULL;
    if (!hdpa->ptrs)
	return NULL;
    if ((i < 0) || (i >= hdpa->nItemCount))
	return NULL;

    TRACE("-- %p\n", hdpa->ptrs[i]);

    return hdpa->ptrs[i];
}


/**************************************************************************
 * DPA_GetPtrIndex [COMCTL32.333]
 *
 * Retrieves the index of the specified pointer
 *
 * PARAMS
 *     hdpa   [I] handle (pointer) to the pointer array
 *     p      [I] pointer
 *
 * RETURNS
 *     Success: index of the specified pointer
 *     Failure: -1
 */

INT WINAPI
DPA_GetPtrIndex (const HDPA hdpa, LPVOID p)
{
    INT i;

    if (!hdpa->ptrs)
	return -1;

    for (i = 0; i < hdpa->nItemCount; i++) {
	if (hdpa->ptrs[i] == p)
	    return i;
    }

    return -1;
}


/**************************************************************************
 * DPA_InsertPtr [COMCTL32.334]
 *
 * Inserts a pointer into a dynamic pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the array
 *     i    [I] array index
 *     p    [I] pointer to insert
 *
 * RETURNS
 *     Success: index of the inserted pointer
 *     Failure: -1
 */

INT WINAPI
DPA_InsertPtr (const HDPA hdpa, INT i, LPVOID p)
{
    INT   nNewItems, nSize, nIndex = 0;
    LPVOID  *lpTemp, *lpDest;

    TRACE("(%p %d %p)\n", hdpa, i, p);

    if ((!hdpa) || (i < 0))
	return -1;

    if (!hdpa->ptrs) {
	hdpa->ptrs =
	    (LPVOID*)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				2 * hdpa->nGrow * sizeof(LPVOID));
	if (!hdpa->ptrs)
	    return -1;
	hdpa->nMaxCount = hdpa->nGrow * 2;
        nIndex = 0;
    }
    else {
	if (hdpa->nItemCount >= hdpa->nMaxCount) {
	    TRACE("-- resizing\n");
	    nNewItems = hdpa->nMaxCount + hdpa->nGrow;
	    nSize = nNewItems * sizeof(LPVOID);

	    lpTemp = (LPVOID*)HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
					   hdpa->ptrs, nSize);
	    if (!lpTemp)
		return -1;
	    hdpa->nMaxCount = nNewItems;
	    hdpa->ptrs = lpTemp;
	}

	if (i >= hdpa->nItemCount) {
	    nIndex = hdpa->nItemCount;
	    TRACE("-- appending at %d\n", nIndex);
	}
	else {
	    TRACE("-- inserting at %d\n", i);
	    lpTemp = hdpa->ptrs + i;
	    lpDest = lpTemp + 1;
	    nSize  = (hdpa->nItemCount - i) * sizeof(LPVOID);
	    TRACE("-- move dest=%p src=%p size=%x\n",
		   lpDest, lpTemp, nSize);
	    memmove (lpDest, lpTemp, nSize);
	    nIndex = i;
	}
    }

    /* insert item */
    hdpa->nItemCount++;
    hdpa->ptrs[nIndex] = p;

    return nIndex;
}


/**************************************************************************
 * DPA_SetPtr [COMCTL32.335]
 *
 * Sets a pointer in the pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *     i    [I] index of the pointer that will be set
 *     p    [I] pointer to be set
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_SetPtr (const HDPA hdpa, INT i, LPVOID p)
{
    LPVOID *lpTemp;
    
    TRACE("(%p %d %p)\n", hdpa, i, p);

    if ((!hdpa) || i < 0)
	return FALSE;
      
    if (hdpa->nItemCount <= i) {
	/* within the old array */
	if (hdpa->nMaxCount > i) {
	    /* within the allocated space, set a new boundary */
	    hdpa->nItemCount = i;
	}
	else {
	    /* resize the block of memory */
	    INT nNewItems =
		hdpa->nGrow * ((INT)((i - 1) / hdpa->nGrow) + 1);
	    INT nSize = nNewItems * sizeof(LPVOID);

	    lpTemp = (LPVOID*)HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
					   hdpa->ptrs, nSize);
	    if (!lpTemp)
		return FALSE;

	    hdpa->nItemCount = nNewItems;
	    hdpa->ptrs = lpTemp;        
	}    
    }

    /* put the new entry in */
    hdpa->ptrs[i] = p;

    return TRUE;
}


/**************************************************************************
 * DPA_DeletePtr [COMCTL32.336]
 *
 * Removes a pointer from the pointer array.
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *     i    [I] index of the pointer that will be deleted
 *
 * RETURNS
 *     Success: deleted pointer
 *     Failure: NULL
 */

LPVOID WINAPI
DPA_DeletePtr (const HDPA hdpa, INT i)
{
    LPVOID *lpDest, *lpSrc, lpTemp = NULL;
    INT  nSize;
    
    TRACE("(%p %d)\n", hdpa, i);

    if ((!hdpa) || i < 0 || i >= hdpa->nItemCount)
	return NULL;

    lpTemp = hdpa->ptrs[i];

    /* do we need to move ?*/
    if (i < hdpa->nItemCount - 1) {
	lpDest = hdpa->ptrs + i;
	lpSrc = lpDest + 1;
	nSize = (hdpa->nItemCount - i - 1) * sizeof(LPVOID);
	TRACE("-- move dest=%p src=%p size=%x\n",
	       lpDest, lpSrc, nSize);
	memmove (lpDest, lpSrc, nSize);
    }
    
    hdpa->nItemCount --;
    
    /* free memory ?*/
    if ((hdpa->nMaxCount - hdpa->nItemCount) >= hdpa->nGrow) {
	INT nNewItems = MIN(hdpa->nGrow * 2, hdpa->nItemCount);
	nSize = nNewItems * sizeof(LPVOID);
	lpDest = (LPVOID)HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				      hdpa->ptrs, nSize);
	if (!lpDest)
	    return NULL;

	hdpa->nMaxCount = nNewItems;
	hdpa->ptrs = (LPVOID*)lpDest;         
    }

    return lpTemp;
}


/**************************************************************************
 * DPA_DeleteAllPtrs [COMCTL32.337]
 *
 * Removes all pointers and reinitializes the array.
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_DeleteAllPtrs (const HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa) 
	return FALSE;

    if (hdpa->ptrs && (!HeapFree (hdpa->hHeap, 0, hdpa->ptrs)))
	return FALSE;

    hdpa->nItemCount = 0;
    hdpa->nMaxCount = hdpa->nGrow * 2;
    hdpa->ptrs = (LPVOID*)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				     hdpa->nMaxCount * sizeof(LPVOID));

    return TRUE;
}


/**************************************************************************
 * DPA_QuickSort [Internal]
 *
 * Ordinary quicksort (used by DPA_Sort).
 *
 * PARAMS
 *     lpPtrs     [I] pointer to the pointer array
 *     l          [I] index of the "left border" of the partition
 *     r          [I] index of the "right border" of the partition
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter in compare function)
 *
 * RETURNS
 *     NONE
 */

static VOID
DPA_QuickSort (LPVOID *lpPtrs, INT l, INT r,
	       PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    LPVOID t, v;
    INT  i, j;

    TRACE("l=%i r=%i\n", l, r);
 
    i = l;
    j = r;
    v = lpPtrs[(int)(l+r)/2];
    do {
	while ((pfnCompare)(lpPtrs[i], v, lParam) > 0) i++;
	while ((pfnCompare)(lpPtrs[j], v, lParam) < 0) j--;
	if (i <= j) 
	{
	    t = lpPtrs[i];
	    lpPtrs[i++] = lpPtrs[j];
	    lpPtrs[j--] = t;
	}
    } while (i <= j);
    if (l < j) DPA_QuickSort (lpPtrs, l, j, pfnCompare, lParam);
    if (i < r) DPA_QuickSort (lpPtrs, i, r, pfnCompare, lParam);
}


/**************************************************************************
 * DPA_Sort [COMCTL32.338]
 *
 * Sorts a pointer array using a user defined compare function
 *
 * PARAMS
 *     hdpa       [I] handle (pointer) to the pointer array
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter of compare function)
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_Sort (const HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    if (!hdpa || !pfnCompare)
	return FALSE;

    TRACE("(%p %p 0x%lx)\n", hdpa, pfnCompare, lParam);

    if ((hdpa->nItemCount > 1) && (hdpa->ptrs))
	DPA_QuickSort (hdpa->ptrs, 0, hdpa->nItemCount - 1,
		       pfnCompare, lParam);

    return TRUE;
}


/**************************************************************************
 * DPA_Search [COMCTL32.339]
 *
 * Searches a pointer array for a specified pointer
 *
 * PARAMS
 *     hdpa       [I] handle (pointer) to the pointer array
 *     pFind      [I] pointer to search for
 *     nStart     [I] start index
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter of compare function)
 *     uOptions   [I] search options
 *
 * RETURNS
 *     Success: index of the pointer in the array.
 *     Failure: -1
 *
 * NOTES
 *     Binary search taken from R.Sedgewick "Algorithms in C"!
 *     Function is NOT tested!
 *     If something goes wrong, blame HIM not ME! (Eric Kohl)
 */

INT WINAPI
DPA_Search (const HDPA hdpa, LPVOID pFind, INT nStart,
	    PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT uOptions)
{
    if (!hdpa || !pfnCompare || !pFind)
	return -1;

    TRACE("(%p %p %d %p 0x%08lx 0x%08x)\n",
	   hdpa, pFind, nStart, pfnCompare, lParam, uOptions);

    if (uOptions & DPAS_SORTED) {
	/* array is sorted --> use binary search */
	INT l, r, x, n;
	LPVOID *lpPtr;

	TRACE("binary search\n");

	l = (nStart == -1) ? 0 : nStart;
	r = hdpa->nItemCount - 1;
	lpPtr = hdpa->ptrs;
	while (r >= l) {
	    x = (l + r) / 2;
	    n = (pfnCompare)(pFind, lpPtr[x], lParam);
	    if (n < 0)
		r = x - 1;
	    else
		l = x + 1;
	    if (n == 0) {
		TRACE("-- ret=%d\n", n);
		return n;
	    }
	}

	if (uOptions & DPAS_INSERTBEFORE) {
	    TRACE("-- ret=%d\n", r);
	    return r;
	}

	if (uOptions & DPAS_INSERTAFTER) {
	    TRACE("-- ret=%d\n", l);
	    return l;
	}
    }
    else {
	/* array is not sorted --> use linear search */
	LPVOID *lpPtr;
	INT  nIndex;

	TRACE("linear search\n");
	
	nIndex = (nStart == -1)? 0 : nStart;
	lpPtr = hdpa->ptrs;
	for (; nIndex < hdpa->nItemCount; nIndex++) {
	    if ((pfnCompare)(pFind, lpPtr[nIndex], lParam) == 0) {
		TRACE("-- ret=%d\n", nIndex);
		return nIndex;
	    }
	}
    }

    TRACE("-- not found: ret=-1\n");
    return -1;
}


/**************************************************************************
 * DPA_CreateEx [COMCTL32.340]
 *
 * Creates a dynamic pointer array using the specified size and heap.
 *
 * PARAMS
 *     nGrow [I] number of items by which the array grows when it is filled
 *     hHeap [I] handle to the heap where the array is stored
 *
 * RETURNS
 *     Success: handle (pointer) to the pointer array.
 *     Failure: NULL
 */

HDPA WINAPI
DPA_CreateEx (INT nGrow, HANDLE hHeap)
{
    HDPA hdpa;

    TRACE("(%d 0x%x)\n", nGrow, hHeap);

    if (hHeap)
	hdpa = (HDPA)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, sizeof(DPA));
    else
	hdpa = (HDPA)COMCTL32_Alloc (sizeof(DPA));

    if (hdpa) {
	hdpa->nGrow = MIN(8, nGrow);
	hdpa->hHeap = hHeap ? hHeap : COMCTL32_hHeap;
	hdpa->nMaxCount = hdpa->nGrow * 2;
	hdpa->ptrs =
	    (LPVOID*)HeapAlloc (hHeap, HEAP_ZERO_MEMORY,
				hdpa->nMaxCount * sizeof(LPVOID));
    }

    TRACE("-- %p\n", hdpa);

    return hdpa;
}


/**************************************************************************
 * Notification functions
 */

typedef struct tagNOTIFYDATA
{
    HWND hwndFrom;
    HWND hwndTo;
    DWORD  dwParam3;
    DWORD  dwParam4;
    DWORD  dwParam5;
    DWORD  dwParam6;
} NOTIFYDATA, *LPNOTIFYDATA;


/**************************************************************************
 * DoNotify [Internal]
 */

static LRESULT
DoNotify (LPNOTIFYDATA lpNotify, UINT uCode, LPNMHDR lpHdr)
{
    NMHDR nmhdr;
    LPNMHDR lpNmh = NULL;
    UINT idFrom = 0;

    TRACE("(0x%04x 0x%04x %d %p 0x%08lx)\n",
	   lpNotify->hwndFrom, lpNotify->hwndTo, uCode, lpHdr,
	   lpNotify->dwParam5);

    if (!lpNotify->hwndTo)
	return 0;

    if (lpNotify->hwndFrom == -1) {
	lpNmh = lpHdr;
	idFrom = lpHdr->idFrom;
    }
    else {
	if (lpNotify->hwndFrom) {
	    HWND hwndParent = GetParent (lpNotify->hwndFrom);
	    if (hwndParent) {
		hwndParent = GetWindow (lpNotify->hwndFrom, GW_OWNER);
		if (hwndParent)
		    idFrom = GetDlgCtrlID (lpNotify->hwndFrom);
	    }
	}

	lpNmh = (lpHdr) ? lpHdr : &nmhdr;

	lpNmh->hwndFrom = lpNotify->hwndFrom;
	lpNmh->idFrom = idFrom;
	lpNmh->code = uCode;
    }

    return SendMessageA (lpNotify->hwndTo, WM_NOTIFY, idFrom, (LPARAM)lpNmh);
}


/**************************************************************************
 * SendNotify [COMCTL32.341]
 *
 * PARAMS
 *     hwndFrom [I]
 *     hwndTo   [I]
 *     uCode    [I]
 *     lpHdr    [I]
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 */

LRESULT WINAPI
COMCTL32_SendNotify (HWND hwndFrom, HWND hwndTo,
		     UINT uCode, LPNMHDR lpHdr)
{
    NOTIFYDATA notify;

    TRACE("(0x%04x 0x%04x %d %p)\n",
	   hwndFrom, hwndTo, uCode, lpHdr);

    notify.hwndFrom = hwndFrom;
    notify.hwndTo   = hwndTo;
    notify.dwParam5 = 0;
    notify.dwParam6 = 0;

    return DoNotify (&notify, uCode, lpHdr);
}


/**************************************************************************
 * SendNotifyEx [COMCTL32.342]
 *
 * PARAMS
 *     hwndFrom [I]
 *     hwndTo   [I]
 *     uCode    [I]
 *     lpHdr    [I]
 *     dwParam5 [I]
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 */

LRESULT WINAPI
COMCTL32_SendNotifyEx (HWND hwndTo, HWND hwndFrom, UINT uCode,
		       LPNMHDR lpHdr, DWORD dwParam5)
{
    NOTIFYDATA notify;
    HWND hwndNotify;

    TRACE("(0x%04x 0x%04x %d %p 0x%08lx)\n",
	   hwndFrom, hwndTo, uCode, lpHdr, dwParam5);

    hwndNotify = hwndTo;
    if (!hwndTo) {
	if (IsWindow (hwndFrom)) {
	    hwndNotify = GetParent (hwndFrom);
	    if (!hwndNotify)
		return 0;
	}
    }

    notify.hwndFrom = hwndFrom;
    notify.hwndTo   = hwndNotify;
    notify.dwParam5 = dwParam5;
    notify.dwParam6 = 0;

    return DoNotify (&notify, uCode, lpHdr);
}


/**************************************************************************
 * StrChrA [COMCTL32.350]
 *
 */

LPSTR WINAPI
COMCTL32_StrChrA (LPCSTR lpString, CHAR cChar)
{
    return strchr (lpString, cChar);
}


/**************************************************************************
 * StrStrIA [COMCTL32.355]
 */

LPSTR WINAPI
COMCTL32_StrStrIA (LPCSTR lpStr1, LPCSTR lpStr2)
{
    INT len1, len2, i;
    CHAR  first;

    if (*lpStr2 == 0)
	return ((LPSTR)lpStr1);
    len1 = 0;
    while (lpStr1[len1] != 0) ++len1;
    len2 = 0;
    while (lpStr2[len2] != 0) ++len2;
    if (len2 == 0)
	return ((LPSTR)(lpStr1 + len1));
    first = tolower (*lpStr2);
    while (len1 >= len2) {
	if (tolower(*lpStr1) == first) {
	    for (i = 1; i < len2; ++i)
		if (tolower (lpStr1[i]) != tolower(lpStr2[i]))
		    break;
	    if (i >= len2)
		return ((LPSTR)lpStr1);
        }
	++lpStr1; --len1;
    }
    return (NULL);
}


/**************************************************************************
 * StrToIntA [COMCTL32.357] Converts a string to a signed integer.
 */

INT WINAPI
COMCTL32_StrToIntA (LPSTR lpString)
{
    return atoi(lpString);
}


/**************************************************************************
 * DPA_EnumCallback [COMCTL32.385]
 *
 * Enumerates all items in a dynamic pointer array.
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *     enumProc [I]
 *     lParam   [I] 
 *
 * RETURNS
 *     none
 */

VOID WINAPI
DPA_EnumCallback (const HDPA hdpa, DPAENUMPROC enumProc, LPARAM lParam)
{
    INT i;

    TRACE("(%p %p %08lx)\n", hdpa, enumProc, lParam);

    if (!hdpa)
	return;
    if (hdpa->nItemCount <= 0)
	return;

    for (i = 0; i < hdpa->nItemCount; i++) {
	if ((enumProc)(hdpa->ptrs[i], lParam) == 0)
	    return;
    }

    return;
}


/**************************************************************************
 * DPA_DestroyCallback [COMCTL32.386]
 *
 * Enumerates all items in a dynamic pointer array and destroys it.
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_DestroyCallback (const HDPA hdpa, DPAENUMPROC enumProc, LPARAM lParam)
{
    TRACE("(%p %p %08lx)\n", hdpa, enumProc, lParam);

    DPA_EnumCallback (hdpa, enumProc, lParam);

    return DPA_Destroy (hdpa);
}


/**************************************************************************
 * DSA_EnumCallback [COMCTL32.387]
 *
 * Enumerates all items in a dynamic storage array.
 *
 * PARAMS
 *     hdsa     [I] handle to the dynamic storage array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */

VOID WINAPI
DSA_EnumCallback (const HDSA hdsa, DSAENUMPROC enumProc, LPARAM lParam)
{
    INT i;

    TRACE("(%p %p %08lx)\n", hdsa, enumProc, lParam);

    if (!hdsa)
	return;
    if (hdsa->nItemCount <= 0)
	return;

    for (i = 0; i < hdsa->nItemCount; i++) {
	LPVOID lpItem = DSA_GetItemPtr (hdsa, i);
	if ((enumProc)(lpItem, lParam) == 0)
	    return;
    }

    return;
}


/**************************************************************************
 * DSA_DestroyCallback [COMCTL32.388]
 *
 * Enumerates all items in a dynamic storage array and destroys it.
 *
 * PARAMS
 *     hdsa     [I] handle to the dynamic storage array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_DestroyCallback (const HDSA hdsa, DSAENUMPROC enumProc, LPARAM lParam)
{
    TRACE("(%p %p %08lx)\n", hdsa, enumProc, lParam);

    DSA_EnumCallback (hdsa, enumProc, lParam);

    return DSA_Destroy (hdsa);
}

/**************************************************************************
 * StrCSpnA [COMCTL32.356]
 *
 */
INT WINAPI COMCTL32_StrCSpnA( LPCSTR lpStr, LPCSTR lpSet) {
  return strcspn(lpStr, lpSet);
}

/**************************************************************************
 * StrChrW [COMCTL32.358]
 *
 */
LPWSTR WINAPI COMCTL32_StrChrW( LPCWSTR lpStart, WORD wMatch) {
  return CRTDLL_wcschr(lpStart, wMatch);
}

/**************************************************************************
 * StrCmpNA [COMCTL32.352]
 *
 */
INT WINAPI COMCTL32_StrCmpNA( LPCSTR lpStr1, LPCSTR lpStr2, int nChar) {
  return strncmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNA [COMCTL32.352]
 *
 */
INT WINAPI COMCTL32_StrCmpNIA( LPCSTR lpStr1, LPCSTR lpStr2, int nChar) {
  return strncasecmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNW [COMCTL32.360]
 *
 */
INT WINAPI COMCTL32_StrCmpNW( LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar) {
  return CRTDLL_wcsncmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrRChrA [COMCTL32.351]
 *
 */
LPSTR WINAPI COMCTL32_StrRChrA( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch) {
  return lstrrchr(lpStart, lpEnd, wMatch); 
}

/**************************************************************************
 * StrRChrW [COMCTL32.359]
 *
 */
LPWSTR WINAPI COMCTL32_StrRChrW( LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch) {
  return lstrrchrw(lpStart, lpEnd, wMatch); 
}

/**************************************************************************
 * StrStrA [COMCTL32.354]
 *
 */
LPSTR WINAPI COMCTL32_StrStrA( LPCSTR lpFirst, LPCSTR lpSrch) {
  return strstr(lpFirst, lpSrch);
}

/**************************************************************************
 * StrStrW [COMCTL32.362]
 *
 */
LPWSTR WINAPI COMCTL32_StrStrW( LPCWSTR lpFirst, LPCWSTR lpSrch) {
  return CRTDLL_wcsstr(lpFirst, lpSrch);
}

/**************************************************************************
 * StrSpnW [COMCTL32.364]
 *
 */
INT WINAPI COMCTL32_StrSpnW( LPWSTR lpStr, LPWSTR lpSet) {
  LPWSTR lpLoop = lpStr;

  /* validate ptr */
  if ((lpStr == 0) || (lpSet == 0)) return 0;

/* while(*lpLoop) { if lpLoop++; } */

  for(; (*lpLoop != 0); lpLoop++)
    if( CRTDLL_wcschr(lpSet, *(WORD*)lpLoop))
      return (INT)(lpLoop-lpStr);
  
  return (INT)(lpLoop-lpStr);
}

/**************************************************************************
 * comctl32_410 [COMCTL32.410]
 *
 * FIXME: What's this supposed to do?
 *        Parameter 1 is an HWND, you're on your own for the rest.
 */

BOOL WINAPI comctl32_410( HWND hw, DWORD b, DWORD c, DWORD d) {

   FIXME_(commctrl)("(%x, %lx, %lx, %lx): stub!\n", hw, b, c, d);

   return TRUE;
}

/*************************************************************************
 * InitMUILanguage [COMCTL32.70]
 *
 * FIXME: What's this supposed to do?  Apparently some i18n thing.
 *
 */

BOOL WINAPI InitMUILanguage( DWORD a ) {

   FIXME_(commctrl)("(%lx): stub!\n", a);

   return TRUE;
}

