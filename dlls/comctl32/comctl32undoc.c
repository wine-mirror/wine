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
 *     - Write documentation.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "windows.h"
#include "commctrl.h"
#include "heap.h"
#include "debug.h"


CRITICAL_SECTION cs_comctl_alloc;
HANDLE32 hComctl32Heap = 0;


/**************************************************************************
 * Alloc [COMCTL32.71]
 *
 * Allocates memory block from the dll's local heap
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

    if (hComctl32Heap == 0) {
	EnterCriticalSection ((void*)&cs_comctl_alloc);
	hComctl32Heap = HeapCreate (0, 1, 0x4000000);
	LeaveCriticalSection ((void*)&cs_comctl_alloc);
	TRACE (commctrl, "Heap created: 0x%x\n", hComctl32Heap);
	if (!hComctl32Heap)
	    return NULL;        
    }

    lpPtr = HeapAlloc (hComctl32Heap, HEAP_ZERO_MEMORY, dwSize);
    TRACE (commctrl, "-- ret=%p\n", lpPtr);
    return lpPtr;
}


/**************************************************************************
 * ReAlloc [COMCTL32.72]
 *
 * Changes the size of an allocated memory block or allocates a memory
 * block using the dll's local heap.
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

    if (hComctl32Heap == 0) {
	EnterCriticalSection ((void*)&cs_comctl_alloc);
	hComctl32Heap = HeapCreate (0, 1, 0x4000000);
	LeaveCriticalSection ((void*)&cs_comctl_alloc);
	TRACE (commctrl, "Heap created: 0x%x\n", hComctl32Heap);
	if (!hComctl32Heap)
	    return NULL;        
    }
    
    if (lpSrc)
	lpDest = HeapReAlloc (hComctl32Heap, HEAP_ZERO_MEMORY, lpSrc, dwSize);
    else
	lpDest = HeapAlloc (hComctl32Heap, HEAP_ZERO_MEMORY, dwSize);

    TRACE (commctrl, "-- ret=%p\n", lpDest);

    return lpDest;
}


/**************************************************************************
 * Free [COMCTL32.73]
.*
 * Frees an allocated memory block from the dll's local heap.
 *
 * PARAMS
 *     lpMem [I] pointer to memory block which will be freed
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL32 WINAPI
COMCTL32_Free (LPVOID lpMem)
{
    TRACE (commctrl, "(%p)\n", lpMem);

    return HeapFree (hComctl32Heap, 0, lpMem);
}


/**************************************************************************
 * GetSize [COMCTL32.74]
 *
 * Retrieves the size of the specified memory block from the dll's
 * local heap.
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

    return HeapSize (hComctl32Heap, 0, lpMem);
}


/**************************************************************************
 * CreateMRUListA [COMCTL32.151]
 *
 * PARAMS
 *     dwParam
 *
 * RETURNS
 */

DWORD WINAPI
CreateMRUList32A (DWORD dwParam)
{

    FIXME (commctrl, "(%lx)\n", dwParam);

    return 1;
}




/**************************************************************************
 * Str_SetPtrA [COMCTL32.234]
 *
 * PARAMS
 *     dwParam1 [I]
 *     dwParam2 [I]
 *
 * RETURNS
 */

BOOL32 WINAPI
COMCTL32_Str_SetPtrA (LPSTR lpStr, LPVOID *lpPtr)
{
    INT32 len;
    LPSTR ptr;

    TRACE (commctrl, "(%p %p)\n", lpStr, lpPtr);
 
    if (lpStr) {
	len = lstrlen32A (lpStr);
	ptr = COMCTL32_ReAlloc (lpPtr, len + 1);
	if (!(ptr))
	    return FALSE;
	lstrcpy32A (ptr, lpStr);
	if (!lpPtr)
	    return FALSE;
	*lpPtr = ptr;
	return TRUE;
    }

    if (*lpPtr) {
	COMCTL32_Free (*lpPtr);
	return TRUE;
    }

    return FALSE;
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
DSA_Create (INT32 nSize, INT32 nGrow)
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
	hdsa->nGrow = MIN(1, nGrow);
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

BOOL32 WINAPI
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

BOOL32 WINAPI
DSA_GetItem (const HDSA hdsa, INT32 nIndex, LPVOID pDest)
{
    LPVOID pSrc;

    TRACE (commctrl, "(%p %d %p)\n", hdsa, nIndex, pDest);
    
    if (!hdsa)
	return FALSE;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return FALSE;

    pSrc = hdsa->pData + (hdsa->nItemSize * nIndex);
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
DSA_GetItemPtr (const HDSA hdsa, INT32 nIndex)
{
    LPVOID pSrc;

    TRACE (commctrl, "(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return NULL;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return NULL;

    pSrc = hdsa->pData + (hdsa->nItemSize * nIndex);
    
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

BOOL32 WINAPI
DSA_SetItem (const HDSA hdsa, INT32 nIndex, LPVOID pSrc)
{
    INT32  nSize, nNewItems;
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
		hdsa->nGrow * ((INT32)((nIndex - 1) / hdsa->nGrow) + 1);
	    nSize = hdsa->nItemSize * nNewItems;

	    lpTemp = (LPVOID)COMCTL32_ReAlloc (hdsa->pData, nSize);
	    if (!lpTemp)
		return FALSE;

	    hdsa->nMaxCount = nNewItems;
	    hdsa->pData = lpTemp;
	}    
    }

    /* put the new entry in */
    pDest = hdsa->pData + (hdsa->nItemSize * nIndex);
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

INT32 WINAPI
DSA_InsertItem (const HDSA hdsa, INT32 nIndex, LPVOID pSrc)
{
    INT32   nNewItems, nSize, i;
    LPVOID  lpTemp, lpDest;
    LPDWORD p;
    
    TRACE(commctrl, "(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return -1;

    for (i = 0; i < hdsa->nItemSize; i += 4) {
	p = *(DWORD**)(pSrc + i);
	if (IsBadStringPtr32A ((char*)p, 256))
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
	lpTemp = hdsa->pData + (hdsa->nItemSize * nIndex);
	lpDest = lpTemp + hdsa->nItemSize;
	nSize = (hdsa->nItemCount - nIndex) * hdsa->nItemSize;
	TRACE (commctrl, "-- move dest=%p src=%p size=%d\n",
	       lpDest, lpTemp, nSize);
	memmove (lpDest, lpTemp, nSize);
    }

    /* ok, we can put the new Item in */
    hdsa->nItemCount++;
    lpDest = hdsa->pData + (hdsa->nItemSize * nIndex);
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

INT32 WINAPI
DSA_DeleteItem (const HDSA hdsa, INT32 nIndex)
{
    LPVOID lpDest,lpSrc;
    INT32  nSize;
    
    TRACE (commctrl, "(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return -1;
    if (nIndex < 0 || nIndex >= hdsa->nItemCount)
	return -1;

    /* do we need to move ? */
    if (nIndex < hdsa->nItemCount - 1) {
	lpDest = hdsa->pData + (hdsa->nItemSize * nIndex);
	lpSrc = lpDest + hdsa->nItemSize;
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

BOOL32 WINAPI
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
DPA_Create (INT32 nGrow)
{
    HDPA hdpa;

    TRACE (commctrl, "(%d)\n", nGrow);

    hdpa = (HDPA)COMCTL32_Alloc (sizeof(DPA));
    if (hdpa) {
	hdpa->nGrow = MIN(8, nGrow);
	hdpa->hHeap = hComctl32Heap;
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

BOOL32 WINAPI
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

BOOL32 WINAPI
DPA_Grow (const HDPA hdpa, INT32 nGrow)
{
    FIXME (commctrl, "(%p %d) stub!\n", hdpa, nGrow);

    if (!hdpa)
	return FALSE;

    hdpa->nGrow = MIN(8, nGrow);

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
    INT32 nNewItems, nSize;
    HDPA hdpaTemp;

    if (!hdpa)
	return NULL;

    FIXME (commctrl, "(%p %p) stub!\n", hdpa, hdpaNew);

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
		((INT32)((hdpa->nItemCount - 1) / hdpaTemp->nGrow) + 1);
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
DPA_GetPtr (const HDPA hdpa, INT32 i)
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

INT32 WINAPI
DPA_GetPtrIndex (const HDPA hdpa, LPVOID p)
{
    INT32 i;

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

INT32 WINAPI
DPA_InsertPtr (const HDPA hdpa, INT32 i, LPVOID p)
{
    INT32   nNewItems, nSize, nIndex = 0;
    LPVOID  *lpTemp, *lpDest;

    TRACE (commctrl, "(%p %d %p)\n", hdpa, i, p);

    if ((!hdpa) || (i < 0))
	return -1;

    if (!hdpa->ptrs) {
	hdpa->ptrs =
	    (LPVOID*)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				hdpa->nGrow * sizeof(LPVOID));
	if (!hdpa->ptrs)
	    return -1;
	hdpa->nMaxCount = hdpa->nGrow;
        nIndex = 0;
    }
    else {
	if (hdpa->nItemCount > hdpa->nMaxCount) {
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
	    TRACE (commctrl, "-- appending at %d\n", nIndex);
	    nIndex = hdpa->nItemCount;
	}
	else {
	    TRACE (commctrl, "-- inserting at %d\n", i);
	    lpTemp = hdpa->ptrs + (sizeof(LPVOID) * i);
	    lpDest = lpTemp + sizeof(LPVOID);
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

BOOL32 WINAPI
DPA_SetPtr (const HDPA hdpa, INT32 i, LPVOID p)
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
	    INT32 nNewItems =
		hdpa->nGrow * ((INT32)((i - 1) / hdpa->nGrow) + 1);
	    INT32 nSize = nNewItems * sizeof(LPVOID);

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
DPA_DeletePtr (const HDPA hdpa, INT32 i)
{
    LPVOID lpDest, lpSrc, lpTemp = NULL;
    INT32  nSize;
    
    TRACE (commctrl, "(%p %d)\n", hdpa, i);

    if ((!hdpa) || i < 0 || i >= hdpa->nItemCount)
	return NULL;

    lpTemp = hdpa->ptrs[i];

    /* do we need to move ?*/
    if (i < hdpa->nItemCount - 1) {
	lpDest = hdpa->ptrs + (i * sizeof (LPVOID));
	lpSrc = lpDest + sizeof(LPVOID);
	nSize = (hdpa->nItemCount - i - 1) * sizeof(LPVOID);
	TRACE (commctrl,"-- move dest=%p src=%p size=%x\n",
	       lpDest, lpSrc, nSize);
	memmove (lpDest, lpSrc, nSize);
    }
    
    hdpa->nItemCount --;
    
    /* free memory ?*/
    if ((hdpa->nMaxCount - hdpa->nItemCount) >= hdpa->nGrow) {
	INT32 nNewItems = MIN(hdpa->nGrow * 2, hdpa->nItemCount);
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

BOOL32 WINAPI
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
 * Modified version of quicksort (used by DPA_Sort).
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
 *
 * NOTES
 *     Taken from R.Sedgewick "Algorithms in C"!
 *     If something goes wrong, blame HIM not ME! (Eric Kohl)
 */

static VOID
DPA_QuickSort (LPVOID *lpPtrs , INT32 l, INT32 r,
	       PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    LPVOID t, v;
    INT32  i, j;

    if (l > r) {
	v = lpPtrs[r];
	i = l - 1;
	j = r;
	for (;;) {
	    while ((pfnCompare)(lpPtrs[++i], v, lParam) < 0);
	    while ((pfnCompare)(lpPtrs[--j], v, lParam) > 0);
	    if (i >= j)
		break;
	    t = lpPtrs[i];
	    lpPtrs[i] = lpPtrs[j];
	    lpPtrs[j] = t;
	}
	t = lpPtrs[i];
	lpPtrs[i] = lpPtrs[r];
	lpPtrs[r] = t;
	DPA_QuickSort (lpPtrs, l, i - 1, pfnCompare, lParam);
	DPA_QuickSort (lpPtrs, i + 1, r, pfnCompare, lParam);
    }
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

BOOL32 WINAPI
DPA_Sort (const HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    if (!hdpa || !pfnCompare)
	return FALSE;

    TRACE (commctrl, "(%p %p 0x%lx)\n", hdpa, pfnCompare, lParam);

    DPA_QuickSort (hdpa->ptrs, 0, hdpa->nItemCount - 1, pfnCompare, lParam);

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
 *     If something goes wrong, blame HIM not ME! (Eric Kohl)
 */

INT32 WINAPI
DPA_Search (const HDPA hdpa, LPVOID pFind, INT32 nStart,
	    PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT32 uOptions)
{
    if (!hdpa || !pfnCompare || !pFind)
	return -1;

    TRACE (commctrl, "(%p %p %d %p 0x%08lx 0x%08x)\n",
	   hdpa, pFind, nStart, pfnCompare, lParam, uOptions);

    if (uOptions & DPAS_SORTED) {
	/* array is sorted --> use binary search */
	INT32 l, r, x, n;
	LPVOID *lpPtr;

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
	    TRACE (commctrl, "-- ret=%d\n", r);
	    return l;
	}
    }
    else {
	/* array is not sorted --> use linear search */
	LPVOID *lpPtr;
	INT32  nIndex;

	FIXME (commctrl, "linear search\n");
	
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
DPA_CreateEx (INT32 nGrow, HANDLE32 hHeap)
{
    HDPA hdpa;

    TRACE (commctrl, "(%d 0x%x)\n", nGrow, hHeap);

    if (hHeap)
	hdpa = (HDPA)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, sizeof(DPA));
    else
	hdpa = (HDPA)COMCTL32_Alloc (sizeof(DPA));

    if (hdpa) {
	hdpa->nGrow = MIN(8, nGrow);
	hdpa->hHeap = hHeap ? hHeap : hComctl32Heap;
	hdpa->nMaxCount = hdpa->nGrow * 2;
	hdpa->ptrs =
	    (LPVOID*)HeapAlloc (hHeap, HEAP_ZERO_MEMORY,
				hdpa->nMaxCount * sizeof(LPVOID));
    }

    TRACE (commctrl, "-- %p\n", hdpa);

    return hdpa;
}


/**************************************************************************
 * SendNotify [COMCTL32.341]
 *
 */

DWORD WINAPI
COMCTL32_SendNotify (DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4)
{
    FIXME (commctrl, "(0x%08lx 0x%08lx 0x%08lx 0x%08lx)\n",
	   dw1, dw2, dw3, dw4);

    return 0;
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
    INT32 len1, len2, i;
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

INT32 WINAPI
COMCTL32_StrToIntA (LPSTR lpString)
{
    return atoi(lpString);
}

