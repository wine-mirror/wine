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
#include "commctrl.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(commctrl)


extern HANDLE COMCTL32_hHeap; /* handle to the private heap */

/*
 * We put some function prototypes here that don't seem to belong in
 * any header file. When they find their place, we can remove them.
 */
extern LPWSTR __cdecl CRTDLL_wcschr(LPCWSTR, WCHAR);
extern LPSTR WINAPI lstrrchr(LPCSTR, LPCSTR, WORD);
extern LPWSTR WINAPI lstrrchrw(LPCWSTR, LPCWSTR, WORD);
extern LPWSTR WINAPI strstrw(LPCWSTR, LPCWSTR);


/**************************************************************************
 * DPA_Merge [COMCTL32.11]
 *
 * PARAMS
 *     hdpa1    [I] handle to a dynamic pointer array
 *     hdpa2    [I] handle to a dynamic pointer array
 *     dwFlags  [I] flags
 *     pfnSort  [I] pointer to sort function
 *     dwParam5 [I]
 *     lParam   [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

BOOL WINAPI
DPA_Merge (const HDPA hdpa1, const HDPA hdpa2, DWORD dwFlags,
	   PFNDPACOMPARE pfnCompare, LPVOID pfnParam5, LPARAM lParam)
{
    /* LPVOID *pWork1, *pWork2; */
    INT  nCount1, nCount2;

    TRACE (commctrl, "(%p %p %08lx %p %p %08lx): stub!\n",
	   hdpa1, hdpa2, dwFlags, pfnCompare, pfnParam5, lParam);

    if (IsBadWritePtr (hdpa1, sizeof(DPA)))
	return FALSE;

    if (IsBadWritePtr (hdpa2, sizeof(DPA)))
	return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnCompare))
	return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnParam5))
	return FALSE;

    if (dwFlags & DPAM_SORT) {
	TRACE (commctrl, "sorting dpa's!\n");
	DPA_Sort (hdpa1, pfnCompare, lParam);
	DPA_Sort (hdpa2, pfnCompare, lParam);
    }

    if (hdpa2->nItemCount <= 0)
	return TRUE;

    nCount1 = hdpa1->nItemCount - 1;

    nCount2 = hdpa2->nItemCount - 1;

    FIXME (commctrl, "nCount1=%d nCount2=%d\n", nCount1, nCount2);
    FIXME (commctrl, "semi stub!\n");
#if 0

    do {


	if (nResult == 0) {

	}
	else if (nResult > 0) {

	}
	else {

	}

    }
    while (nCount2 >= 0);

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

    TRACE (commctrl, "(0x%lx)\n", dwSize);

    lpPtr = HeapAlloc (COMCTL32_hHeap, HEAP_ZERO_MEMORY, dwSize);

    TRACE (commctrl, "-- ret=%p\n", lpPtr);

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

    TRACE (commctrl, "(%p 0x%08lx)\n", lpSrc, dwSize);

    if (lpSrc)
	lpDest = HeapReAlloc (COMCTL32_hHeap, HEAP_ZERO_MEMORY, lpSrc, dwSize);
    else
	lpDest = HeapAlloc (COMCTL32_hHeap, HEAP_ZERO_MEMORY, dwSize);

    TRACE (commctrl, "-- ret=%p\n", lpDest);

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
    TRACE (commctrl, "(%p)\n", lpMem);

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
    TRACE (commctrl, "(%p)\n", lpMem);

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
    FIXME (commctrl, "(%p) empty stub!\n", hmru);

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

    FIXME (commctrl, "(%lx %lx %lx) empty stub!\n",
	   dwParam1, dwParam2, dwParam3);

    return 0;
}


DWORD WINAPI
FindMRUData (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{

    FIXME (commctrl, "(%lx %lx %lx %lx) empty stub!\n",
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

    FIXME (commctrl, "(%p) empty stub!\n", lpmi);

    if (lpmi) {
	FIXME (commctrl, "(%lx %lx %lx %lx \"%s\" %lx)\n",
	       lpmi->dwParam1, lpmi->dwParam2, lpmi->dwParam3,
	       (DWORD)lpmi->hkeyMain, lpmi->lpszSubKey, lpmi->dwParam6);
    }

    /* dummy pointer creation */
    ptr = COMCTL32_Alloc (32);

    FIXME (commctrl, "-- ret = %p\n", ptr);

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

    TRACE (commctrl, "(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

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
    TRACE (commctrl, "(%p %p)\n", lppDest, lpSrc);
 
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

    TRACE (commctrl, "(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

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
    TRACE (commctrl, "(%p %p)\n", lppDest, lpSrc);
 
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

    TRACE (commctrl, "(size=%d grow=%d)\n", nSize, nGrow);

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
    TRACE (commctrl, "(%p)\n", hdsa);

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

    TRACE (commctrl, "(%p %d %p)\n", hdsa, nIndex, pDest);
    
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

    TRACE (commctrl, "(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return NULL;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return NULL;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    
    TRACE (commctrl, "-- ret=%p\n", pSrc);

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
    
    TRACE (commctrl, "(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return FALSE;
      
    if (hdsa->nItemCount <= nIndex) {
	/* within the old array */
	if (hdsa->nMaxCount > nIndex) {
	    /* within the allocated space, set a new boundary */
	    hdsa->nItemCount = nIndex;
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
	    hdsa->pData = lpTemp;
	}    
    }

    /* put the new entry in */
    pDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE (commctrl, "-- move dest=%p src=%p size=%d\n",
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
    
    TRACE(commctrl, "(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return -1;

    for (i = 0; i < hdsa->nItemSize; i += 4) {
	p = *(DWORD**)((char *) pSrc + i);
	if (IsBadStringPtrA ((char*)p, 256))
	    TRACE (commctrl, "-- %d=%p\n", i, (DWORD*)p);
	else
	    TRACE (commctrl, "-- %d=%p [%s]\n", i, p, debugstr_a((char*)p));
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
	TRACE (commctrl, "-- move dest=%p src=%p size=%d\n",
	       lpDest, lpTemp, nSize);
	memmove (lpDest, lpTemp, nSize);
    }

    /* ok, we can put the new Item in */
    hdsa->nItemCount++;
    lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE (commctrl, "-- move dest=%p src=%p size=%d\n",
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
    
    TRACE (commctrl, "(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return -1;
    if (nIndex < 0 || nIndex >= hdsa->nItemCount)
	return -1;

    /* do we need to move ? */
    if (nIndex < hdsa->nItemCount - 1) {
	lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
	lpSrc = (char *) lpDest + hdsa->nItemSize;
	nSize = hdsa->nItemSize * (hdsa->nItemCount - nIndex - 1);
	TRACE (commctrl, "-- move dest=%p src=%p size=%d\n",
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
    TRACE (commctrl, "(%p)\n", hdsa);

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

    TRACE (commctrl, "(%d)\n", nGrow);

    hdpa = (HDPA)COMCTL32_Alloc (sizeof(DPA));
    if (hdpa) {
	hdpa->nGrow = MAX(8, nGrow);
	hdpa->hHeap = COMCTL32_hHeap;
	hdpa->nMaxCount = hdpa->nGrow * 2;
	hdpa->ptrs =
	    (LPVOID*)COMCTL32_Alloc (hdpa->nMaxCount * sizeof(LPVOID));
    }

    TRACE (commctrl, "-- %p\n", hdpa);

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
    TRACE (commctrl, "(%p)\n", hdpa);

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
    TRACE (commctrl, "(%p %d)\n", hdpa, nGrow);

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

    TRACE (commctrl, "(%p %p)\n", hdpa, hdpaNew);

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
    TRACE (commctrl, "(%p %d)\n", hdpa, i);

    if (!hdpa)
	return NULL;
    if (!hdpa->ptrs)
	return NULL;
    if ((i < 0) || (i >= hdpa->nItemCount))
	return NULL;

    TRACE (commctrl, "-- %p\n", hdpa->ptrs[i]);

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

    TRACE (commctrl, "(%p %d %p)\n", hdpa, i, p);

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
	    TRACE (commctrl, "-- resizing\n");
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
	    TRACE (commctrl, "-- appending at %d\n", nIndex);
	}
	else {
	    TRACE (commctrl, "-- inserting at %d\n", i);
	    lpTemp = hdpa->ptrs + i;
	    lpDest = lpTemp + 1;
	    nSize  = (hdpa->nItemCount - i) * sizeof(LPVOID);
	    TRACE (commctrl, "-- move dest=%p src=%p size=%x\n",
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
    
    TRACE (commctrl, "(%p %d %p)\n", hdpa, i, p);

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
    
    TRACE (commctrl, "(%p %d)\n", hdpa, i);

    if ((!hdpa) || i < 0 || i >= hdpa->nItemCount)
	return NULL;

    lpTemp = hdpa->ptrs[i];

    /* do we need to move ?*/
    if (i < hdpa->nItemCount - 1) {
	lpDest = hdpa->ptrs + i;
	lpSrc = lpDest + 1;
	nSize = (hdpa->nItemCount - i - 1) * sizeof(LPVOID);
	TRACE (commctrl,"-- move dest=%p src=%p size=%x\n",
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
    TRACE (commctrl, "(%p)\n", hdpa);

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

    TRACE (commctrl, "l=%i r=%i\n", l, r);
 
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

    TRACE (commctrl, "(%p %p 0x%lx)\n", hdpa, pfnCompare, lParam);

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

    TRACE (commctrl, "(%p %p %d %p 0x%08lx 0x%08x)\n",
	   hdpa, pFind, nStart, pfnCompare, lParam, uOptions);

    if (uOptions & DPAS_SORTED) {
	/* array is sorted --> use binary search */
	INT l, r, x, n;
	LPVOID *lpPtr;

	TRACE (commctrl, "binary search\n");

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
		TRACE (commctrl, "-- ret=%d\n", n);
		return n;
	    }
	}

	if (uOptions & DPAS_INSERTBEFORE) {
	    TRACE (commctrl, "-- ret=%d\n", r);
	    return r;
	}

	if (uOptions & DPAS_INSERTAFTER) {
	    TRACE (commctrl, "-- ret=%d\n", l);
	    return l;
	}
    }
    else {
	/* array is not sorted --> use linear search */
	LPVOID *lpPtr;
	INT  nIndex;

	TRACE (commctrl, "linear search\n");
	
	nIndex = (nStart == -1)? 0 : nStart;
	lpPtr = hdpa->ptrs;
	for (; nIndex < hdpa->nItemCount; nIndex++) {
	    if ((pfnCompare)(pFind, lpPtr[nIndex], lParam) == 0) {
		TRACE (commctrl, "-- ret=%d\n", nIndex);
		return nIndex;
	    }
	}
    }

    TRACE (commctrl, "-- not found: ret=-1\n");
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

    TRACE (commctrl, "(%d 0x%x)\n", nGrow, hHeap);

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

    TRACE (commctrl, "-- %p\n", hdpa);

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

    TRACE (commctrl, "(0x%04x 0x%04x %d %p 0x%08lx)\n",
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

    TRACE (commctrl, "(0x%04x 0x%04x %d %p)\n",
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

    TRACE (commctrl, "(0x%04x 0x%04x %d %p 0x%08lx)\n",
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

    TRACE (commctrl, "(%p %p %08lx)\n", hdpa, enumProc, lParam);

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
    TRACE (commctrl, "(%p %p %08lx)\n", hdpa, enumProc, lParam);

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

    TRACE (commctrl, "(%p %p %08lx)\n", hdsa, enumProc, lParam);

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
    TRACE (commctrl, "(%p %p %08lx)\n", hdsa, enumProc, lParam);

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
  return lstrncmpA(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNW [COMCTL32.360]
 *
 */
INT WINAPI COMCTL32_StrCmpNW( LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar) {
  return lstrncmpW(lpStr1, lpStr2, nChar);
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
  return strstrw(lpFirst, lpSrch);
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

/*************************************************************************
 * DPA_LoadStream [COMCTL32.9]
 *
 * NOTE: Ordinal is only accurate for Win98 / IE 4 and later
 */

DWORD WINAPI DPA_LoadStream(HDPA *hDpa, DWORD pfnDpaLoadCallback, DWORD param3, DWORD param4)
{
  FIXME(commctrl, "(%p %lx %lx %lx): partial stub!\n", hDpa, pfnDpaLoadCallback, param3, param4);

 *hDpa = DPA_Create(8);

  return(0);
}

/************************************************************************
 * DPA_SaveStream [COMCTL32.10]
 *
 * NOTE: Ordinal is only accurate for Win98 / IE 4 and later
 */

DWORD WINAPI DPA_SaveStream(DWORD param1, DWORD param2, DWORD param3, DWORD param4)
{
  FIXME(commctrl, "(%lx %lx %lx %lx): stub!\n", param1, param2, param3, param4);

  return(0);
}
