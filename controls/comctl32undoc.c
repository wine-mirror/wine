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
 *     - Fix DSA_InsertItem.
 *     - Fix DSA_GetItem.
 *     - Write documentation.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "windows.h"
#include "heap.h"
#include "debug.h"

typedef struct _DPA_DATA
{
    DWORD   dwEntryCount;
    DWORD   dwMaxCount;
    DWORD   dwGrow;
    LPDWORD ptrs; 
} DPA_DATA, *LPDPA_DATA;


DWORD WINAPI DPA_Create (DWORD);
DWORD WINAPI DPA_GetPtr (DWORD, DWORD);
DWORD WINAPI DPA_InsertPtr (DWORD, DWORD, DWORD);


CRITICAL_SECTION cs_comctl_alloc;
HANDLE32 hComctl32Heap=0;
/**************************************************************************
 * Alloc [COMCTL32.71]
 *
 */

LPVOID WINAPI COMCTL32_Alloc (DWORD dwParam)
{   LPVOID lpPtr;

    TRACE (commctrl, "(0x%08lx)\n", dwParam);

	if (hComctl32Heap==0)
	{ EnterCriticalSection((void*)&cs_comctl_alloc);
      hComctl32Heap=HeapCreate(0,1,0x4000000);
      LeaveCriticalSection((void*)&cs_comctl_alloc);
      TRACE (commctrl, "Heap created: 0x%08x\n", hComctl32Heap);
      if (! hComctl32Heap)
        return FALSE;        
	}

//    lpPtr = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, dwParam);
    lpPtr = HeapAlloc (hComctl32Heap, HEAP_ZERO_MEMORY, dwParam);
    TRACE (commctrl, "-- ret=%p\n", lpPtr);
    return lpPtr;
}


/**************************************************************************
 * ReAlloc [COMCTL32.72]
 *
 */

LPVOID WINAPI
COMCTL32_ReAlloc (LPVOID dwParam1, DWORD dwParam2)
{
    LPVOID dwPtr;
    TRACE (commctrl, "(0x%08lx 0x%08lx)\n",(DWORD)dwParam1, dwParam2);
    
    if (dwParam1 == 0)
	dwPtr = HeapAlloc (hComctl32Heap, HEAP_ZERO_MEMORY,
			   dwParam2);
    else
	dwPtr = HeapReAlloc (hComctl32Heap, HEAP_ZERO_MEMORY,
			     dwParam1, dwParam2);

    TRACE (commctrl, "-- ret=0x%08lx\n", (DWORD)dwPtr);

    return dwPtr;
}


/**************************************************************************
 * Free [COMCTL32.73]
 *
 */

DWORD WINAPI
COMCTL32_Free (LPVOID dwParam)
{
    TRACE (commctrl, "(0x%08lx)\n", (DWORD)dwParam);
    HeapFree (hComctl32Heap, 0, dwParam);

    return 0;
}


/**************************************************************************
 * GetSize [COMCTL32.74]
 *
 */

DWORD WINAPI
COMCTL32_GetSize (LPVOID dwParam)
{
    TRACE (commctrl, "(0x%08lx)\n", (DWORD)dwParam);
    return (HeapSize (hComctl32Heap, 0, dwParam));
}



/**************************************************************************
 * Str_SetPtrA [COMCTL32.234]
 *
 * PARAMS
 *     dwParam1 [I]
 *     dwParam2 [I]
 */

BOOL32 WINAPI
COMCTL32_Str_SetPtrA (LPSTR lpStr, LPVOID *lpPtr)
{
    INT32 len;
    LPSTR ptr;

    FIXME (commctrl, "(0x%08lx 0x%08lx)\n", (DWORD)lpStr, (DWORD)lpPtr);
    FIXME (commctrl, "(\"%s\" \"%s\")\n", lpStr, (LPSTR)*lpPtr);

    if (lpStr) {
	len = lstrlen32A (lpStr);
	ptr = COMCTL32_ReAlloc (lpPtr, len + 1);
	if (!(ptr))
	    return FALSE;
	lstrcpy32A (ptr, lpStr);
	*lpPtr = ptr;
	return TRUE;
    }

    if (*lpPtr) {
	COMCTL32_Free (*lpPtr);
	return TRUE;
    }

    return FALSE;
}

/*************************************************************************
* The DSA-API is a set of functions to create and manipulate arrays of
* fix sized memory blocks. This arrays can store any kind of data (strings,
* icons...) so the name "dynamic string array" is a bit misleading.
*
* STATUS 
*  complete
*/
typedef struct _DSA_DATA
{   DWORD   dwEntryCount;
    BYTE    * pData;
    DWORD   dwMaxCount;
    DWORD   dwElementSize;
    DWORD   dwGrow;
} DSA_DATA, *LPDSA_DATA;

/**************************************************************************
 * DSA_Create [COMCTL32.320] Creates a dynamic string array
 *
 * PARAMS
 *     dwSize [I] size of the array elements
 *     dwGrow [I] 
 * RETURNS
 *     pointer to a array control structure. use this like a handle.
 */

LPDSA_DATA WINAPI DSA_Create (DWORD dwSize, DWORD dwGrow)
{   LPDSA_DATA dsaPtr;

    TRACE (commctrl, "(size=0x%08lx grow=0x%08lx)\n", dwSize, dwGrow);

    if ((dsaPtr=(LPDSA_DATA)COMCTL32_Alloc(sizeof(DSA_DATA))));
    {  dsaPtr->dwEntryCount=0x00;
       dsaPtr->pData=NULL;
       dsaPtr->dwMaxCount=0x00;
       dsaPtr->dwElementSize=dwSize;
       if ( dwGrow == 0 )
         dsaPtr->dwGrow=1;
       else
         dsaPtr->dwGrow=dwGrow;
       return dsaPtr;
    }
    return FALSE;   
}

/**************************************************************************
 * DSA_Destroy [COMCTL32.321] Destroys a dynamic string array
 *
 * PARAMS
 *     dsaPtr [I] pointer to the array control structure
 * RETURNS
 *  TRUE if dsaPtr = NULL or success
 *  FALSE if failure
 */

BOOL32 WINAPI DSA_Destroy (const LPDSA_DATA dsaPtr )
{   TRACE (commctrl, "(%p)\n", dsaPtr);

	if (! dsaPtr)
      return FALSE;

    if (dsaPtr->pData && (! COMCTL32_Free(dsaPtr->pData)))
    { return FALSE;
    }
    return COMCTL32_Free (dsaPtr);
}

/**************************************************************************
 * DSA_GetItem [COMCTL32.322] 
 *
 * PARAMS
 *  dsaPtr [I] pointer to the array control structure
 *  dwItem [I] number of the Item to get
+ *  pDest  [O] destination buffer. Has to be >= dwElementSize.
 */

BOOL32 WINAPI DSA_GetItem (const LPDSA_DATA dsaPtr, DWORD dwItem, LPBYTE pDest)
{   BYTE * pSrc;

    TRACE (commctrl, "(%p 0x%08lx %p)\n", dsaPtr, dwItem, pDest);
    
    if ( (!dsaPtr) || (dwItem < 0) || (dwItem >= dsaPtr->dwEntryCount))
      return FALSE;
    
    pSrc = dsaPtr->pData + (dsaPtr->dwElementSize * dwItem);
    memmove(pDest,pSrc,dsaPtr->dwElementSize);
    return TRUE;
}

/**************************************************************************
 * DSA_GetItemPtr [COMCTL32.323] 
 *
 * PARAMS
 *  dsaPtr [I] pointer to the array control structure
 *  dwItem [I] number of the Item to get
 * RETURNS
 *  pointer ti a item 
 */
LPBYTE WINAPI DSA_GetItemPtr (const LPDSA_DATA dsaPtr, DWORD dwItem)
{   BYTE * pSrc;

	TRACE (commctrl, "(%p 0x%08lx)\n", dsaPtr, dwItem);

    if ((!dsaPtr) || (dwItem < 0) || (dwItem >= dsaPtr->dwEntryCount))
	  return FALSE;
    pSrc = dsaPtr->pData + (dsaPtr->dwElementSize * dwItem);
    
    TRACE (commctrl, "-- ret=%p\n", pSrc);

    return  pSrc;
}

/**************************************************************************
 * DSA_SetItem [COMCTL32.325] 
 *
 * PARAMS
 *  dsaPtr [I] pointer to the array control structure
 *  dwItem [I] index for the new element
 *  pSrc   [I] the element
 */
BOOL32 WINAPI DSA_SetItem (const LPDSA_DATA dsaPtr, DWORD dwItem, LPBYTE pSrc)
{   LPBYTE pDest;
 	DWORD  dwSize, dwNewItems;
	LPBYTE lpTemp;
    
    TRACE (commctrl, "(%p 0x%08lx %p)\n", dsaPtr, dwItem, pSrc);

	if ((!dsaPtr) || dwItem<0 )
      return FALSE;
      
    if (dsaPtr->dwEntryCount <= dwItem)	/* within the old array */
    { if ( dsaPtr->dwMaxCount > dwItem) 
      { dsaPtr->dwEntryCount = dwItem; /* within the allocated space, set a new boundary */
      }
      else
      { /* resize the block of memory*/
        dwNewItems = dsaPtr->dwGrow * ( (WORD)((dwItem-1)/dsaPtr->dwGrow) +1);
        dwSize = dsaPtr->dwElementSize * dwNewItems;
        lpTemp = (LPBYTE) COMCTL32_ReAlloc(dsaPtr->pData,dwSize);
        if (! lpTemp )
        { return FALSE;
        }
        dsaPtr->dwMaxCount = dwNewItems;
        dsaPtr->pData = lpTemp;        
      }    
    }
	/* put the new entry in */
	pDest = dsaPtr->pData +  (dsaPtr->dwElementSize * dwItem);
    TRACE (commctrl,"move dest=%p src=%p size=%x",pDest,pSrc,dsaPtr->dwElementSize);
	memmove(pDest,pSrc,dsaPtr->dwElementSize);
    return TRUE;
}

/**************************************************************************
 * DSA_InsertItem [COMCTL32.325] 
 *
 * PARAMS
 *  dsaPtr [I] pointer to the array control structure
 *  dwItem [I] index for the new element
 *  pSrc   [I] the element
 *
 * RETURNS
 *  the position of the new element
 */
DWORD WINAPI DSA_InsertItem (const LPDSA_DATA dsaPtr, DWORD dwItem, LPBYTE pSrc)
{	DWORD dwNewItems, dwSize,i;
	LPBYTE lpTemp, lpDest;
    LPDWORD p;
    
	TRACE(commctrl, "(%p 0x%08lx %p)\n", dsaPtr, dwItem, pSrc);

	if ( (!dsaPtr) || dwItem<0 )
      return -1;

	for (i=0; i<dsaPtr->dwElementSize;i+=4)
    { p = *(DWORD**)(pSrc+i);
      if ( IsBadStringPtr32A ((char*)p,256))
      { TRACE(commctrl,"-- 0x%04lx=%p\n",i,(DWORD*)p);
      }
      else
      { TRACE(commctrl,"-- 0x%04lx=%p [%s]\n",i,p,debugstr_a((char*)p));
      }
    }
    
    if (dwItem > dsaPtr->dwEntryCount)		/* when dwItem > dwEntryCount then append*/
      dwItem = dsaPtr->dwEntryCount+1;
    
    if (dwItem >= dsaPtr->dwMaxCount)		/* do we need to resize ? */
    { dwNewItems = dsaPtr->dwMaxCount + dsaPtr->dwGrow;
      dwSize = dsaPtr->dwElementSize * dwNewItems;
	  lpTemp = (LPBYTE)COMCTL32_ReAlloc(dsaPtr->pData,dwSize);
      if (!lpTemp)
      { return -1;
      }
      dsaPtr->dwMaxCount = dwNewItems;
      dsaPtr->pData = lpTemp;         
    }

    if (dwItem < dsaPtr->dwEntryCount)		/* do we need to move elements ?*/
	{ lpTemp = dsaPtr->pData + (dsaPtr->dwElementSize * dwItem);
      lpDest = lpTemp + dsaPtr->dwElementSize;
      TRACE (commctrl,"-- move dest=%p src=%p size=%x\n",lpDest,lpTemp,dsaPtr->dwElementSize);
      memmove (lpDest,lpTemp,dsaPtr->dwElementSize);
    } 
    /* ok, we can put the new Item in*/
    dsaPtr->dwEntryCount++;
    lpDest = dsaPtr->pData + (dsaPtr->dwElementSize * dwItem);
    TRACE (commctrl,"-- move dest=%p src=%p size=%x\n",lpDest,pSrc,dsaPtr->dwElementSize);
	memmove (lpDest,pSrc,dsaPtr->dwElementSize);
	return dsaPtr->dwEntryCount;
}
/**************************************************************************
 * DSA_DeleteItem [COMCTL32.326] 
 *
 * PARAMS
 *  dsaPtr [I] pointer to the array control structure
 *  dwItem [I] index for the element to delete
 * RETURNS
 *  number of the element deleted
 */
DWORD WINAPI DSA_DeleteItem (const LPDSA_DATA dsaPtr, DWORD dwItem)
{	LPBYTE	lpDest,lpSrc;
	DWORD	dwSize;
    
    TRACE (commctrl, "(%p 0x%08lx)\n", dsaPtr, dwItem);

	if ( (! dsaPtr) || dwItem<0 || dwItem>=dsaPtr->dwEntryCount)
      return FALSE;

    if ( dwItem < dsaPtr->dwEntryCount-1 )	/* do we need to move ?*/
	{ lpDest = dsaPtr->pData + (dsaPtr->dwElementSize * dwItem);
      lpSrc = lpDest + dsaPtr->dwElementSize;
      dwSize = dsaPtr->dwElementSize * (dsaPtr->dwEntryCount-dwItem-1);
      TRACE (commctrl,"-- move dest=%p src=%p size=%x\n",lpDest,lpSrc,dwSize);
      memmove (lpDest,lpSrc,dwSize);
	}
    
    dsaPtr->dwEntryCount--;
    
    if ( (dsaPtr->dwMaxCount-dsaPtr->dwEntryCount) >= dsaPtr->dwGrow) /* free memory ?*/
    { dwSize = dsaPtr->dwElementSize * dsaPtr->dwEntryCount;
      lpDest = (LPBYTE) COMCTL32_ReAlloc(dsaPtr->pData,dwSize);
      if (!lpDest)
      { return FALSE;
      }
      dsaPtr->dwMaxCount = dsaPtr->dwEntryCount;
      dsaPtr->pData = lpDest;         

    }
    return dwItem;
}

/**************************************************************************
 * DSA_DeleteAllItems [COMCTL32.326] 
 *  deletes all elements and initializes array
 *
 * PARAMS
 *  dsaPtr [I] pointer to the array control structure
 *
 * RETURNS
 *  TRUE/FALSE
 */
BOOL32 WINAPI DSA_DeleteAllItems (const LPDSA_DATA dsaPtr)
{   TRACE (commctrl, "(%p)\n", dsaPtr);

	if (! dsaPtr) 
      return FALSE;

    if (dsaPtr->pData && (! COMCTL32_Free(dsaPtr->pData)))
    { return FALSE;
    }
	dsaPtr->dwEntryCount=0x00;
    dsaPtr->pData=NULL;
    dsaPtr->dwMaxCount=0x00;
    return TRUE;
}
/**************************************************************************/


DWORD WINAPI
DPA_Create (DWORD dwParam1)
{
    LPDPA_DATA dpaPtr;

    TRACE (commctrl, "(0x%08lx)\n", dwParam1);

    dpaPtr = (LPDPA_DATA)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY, sizeof(DPA_DATA));
    dpaPtr->dwGrow = dwParam1;

    TRACE (commctrl, "ret=0x%08lx\n", (DWORD)dpaPtr);

    return (DWORD)dpaPtr;
}



DWORD WINAPI
DPA_GetPtr (DWORD dwParam1, DWORD dwParam2)
{
    LPDPA_DATA dpaPtr = (LPDPA_DATA)dwParam1;

    TRACE (commctrl, "(0x%08lx 0x%08lx)\n", dwParam1, dwParam2);

    if (dpaPtr == NULL)
	return 0;
    if (dpaPtr->ptrs == NULL)
	return 0;
    if ((dwParam2 < 0) || (dwParam2 >= dpaPtr->dwEntryCount))
	return 0;

    TRACE (commctrl, "ret=0x%08lx\n", (DWORD)dpaPtr->ptrs[dwParam2]);

    return (DWORD)dpaPtr->ptrs[dwParam2];
}



DWORD WINAPI
DPA_InsertPtr (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
    LPDPA_DATA dpaPtr = (LPDPA_DATA)dwParam1;
    DWORD dwIndex;

    TRACE (commctrl, "(0x%08lx 0x%08lx 0x%lx)\n",
	   dwParam1, dwParam2, dwParam3);

    if (dpaPtr->ptrs == NULL) {
	dpaPtr->ptrs = (LPDWORD)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
					   dpaPtr->dwGrow * sizeof(LPVOID));
	dpaPtr->dwMaxCount = dpaPtr->dwGrow;
        dwIndex = 0;
        dpaPtr->ptrs[dwIndex] = dwParam3;
    }
    else {
	FIXME (commctrl, "adding to existing array! stub!\n");


	dwIndex = dwParam2;
    }

    dpaPtr->dwEntryCount++;

    return (dwIndex);
}


/**************************************************************************
 * DPA_CreateEx [COMCTL32.340]
 *
 */

DWORD WINAPI
DPA_CreateEx (DWORD dwParam1, DWORD dwParam2)
{
    FIXME (commctrl, "(0x%08lx 0x%08lx)\n",
	   dwParam1, dwParam2);

    return 0;
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

