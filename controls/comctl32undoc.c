/*
 * Undocumented functions from COMCTL32.DLL
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *
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


typedef struct _DSA_DATA
{
    DWORD   dwEntryCount;
    DWORD   dwMaxCount;
    DWORD   dwInitial;
    DWORD   dwGrow;
    LPSTR   *ptrs;
} DSA_DATA, *LPDSA_DATA;


typedef struct _DPA_DATA
{
    DWORD   dwEntryCount;
    DWORD   dwMaxCount;
    DWORD   dwGrow;
    LPDWORD ptrs; 
} DPA_DATA, *LPDPA_DATA;


DWORD WINAPI DSA_Create (DWORD, DWORD);


DWORD WINAPI DPA_Create (DWORD);
DWORD WINAPI DPA_GetPtr (DWORD, DWORD);
DWORD WINAPI DPA_InsertPtr (DWORD, DWORD, DWORD);




/**************************************************************************
 * Alloc [COMCTL32.71]
 *
 */

LPVOID WINAPI
COMCTL32_Alloc (DWORD dwParam)
{
    LPVOID lpPtr;

    lpPtr = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, dwParam);

    TRACE (commctrl, "(0x%08lx) ret=0x%08lx\n", dwParam, (DWORD)lpPtr);

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

    if (dwParam1 == 0)
	dwPtr = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
			   dwParam2);
    else
	dwPtr = HeapReAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
			     dwParam1, dwParam2);

    TRACE (commctrl, "(0x%08lx 0x%08lx) ret=0x%08lx\n",
	   (DWORD)dwParam1, dwParam2, (DWORD)dwPtr);

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
    HeapFree (GetProcessHeap (), 0, dwParam);

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
    return (HeapSize (GetProcessHeap (), 0, dwParam));
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



/**************************************************************************
 * DSA_Create [COMCTL32.320] Creates a dynamic string array
 *
 * PARAMS
 *     dwParam1 [I]
 *     dwParam2 [I]
 */

DWORD WINAPI
DSA_Create (DWORD dwParam1, DWORD dwParam2)
{
    LPDSA_DATA dsaPtr;

    dsaPtr = (LPDSA_DATA)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
				    sizeof(DSA_DATA));
    dsaPtr->dwInitial = dwParam1;
    dsaPtr->dwGrow = dwParam2;

    TRACE (commctrl, "(0x%08lx 0x%08lx) ret=0x%08lx\n",
	   dwParam1, dwParam2, (DWORD)dsaPtr);

    return (DWORD)dsaPtr;
}


/**************************************************************************
 * DSA_Destroy [COMCTL32.321] Destroys a dynamic string array
 *
 * PARAMS
 *     dwParam1 [I]
 */

DWORD WINAPI
DSA_Destroy (DWORD dwParam1)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;
    DWORD i;

    TRACE (commctrl, "(0x%08lx):semi-stub!\n", dwParam1);

    if (dsaPtr->ptrs) {
	for (i = 0; i < dsaPtr->dwEntryCount; i++) {
	    if (dsaPtr->ptrs[i])
		HeapFree (GetProcessHeap (), 0, (LPSTR)dsaPtr->ptrs[i]);
	}
    }

    HeapFree (GetProcessHeap (), 0, dsaPtr);

    return 0;
}


DWORD WINAPI
DSA_GetItem (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;

    FIXME (commctrl, "(0x%08lx 0x%08lx 0x%08lx): stub!\n",
	   dwParam1, dwParam2, dwParam3);

    if (dsaPtr == NULL)
	return 0;
    if (dsaPtr->ptrs == NULL)
	return 0;
    if ((dwParam2 < 0) || (dwParam2 >= dsaPtr->dwEntryCount))
	return 0;

//    FIXME (commctrl, "\"%s\"\n", (LPSTR)dsaPtr->ptrs[dwParam2]);

    return (DWORD)lstrcpy32A ((LPSTR)dwParam3, (LPSTR)dsaPtr->ptrs[dwParam2]);
}


DWORD WINAPI
DSA_GetItemPtr (DWORD dwParam1, DWORD dwParam2)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;

    TRACE (commctrl, "(0x%08lx 0x%08lx)\n", dwParam1, dwParam2);

    if (dsaPtr == NULL)
	return 0;
    if (dsaPtr->ptrs == NULL)
	return 0;
    if ((dwParam2 < 0) || (dwParam2 >= dsaPtr->dwEntryCount))
	return 0;

    TRACE (commctrl, "ret=0x%08lx\n", (DWORD)dsaPtr->ptrs[dwParam2]);

    return (DWORD)dsaPtr->ptrs[dwParam2];
}


DWORD WINAPI
DSA_InsertItem (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;
    DWORD dwIndex;
    INT32 len;

    TRACE (commctrl, "(0x%08lx 0x%08lx \"%s\"):semi-stub!\n",
	   dwParam1, dwParam2, (LPSTR)dwParam3);

    if (dsaPtr->ptrs == NULL) {
	dsaPtr->ptrs = (LPSTR*)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
					  dsaPtr->dwInitial * sizeof(LPVOID));
	dsaPtr->dwMaxCount = dsaPtr->dwInitial;
        dwIndex = 0;
	len = lstrlen32A ((LPSTR)dwParam3);
	dsaPtr->ptrs[dwIndex] =
	    (LPSTR)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len+1);
	lstrcpy32A (dsaPtr->ptrs[dwIndex], (LPSTR)dwParam3);
    }
    else {
	TRACE (commctrl, "(0x%08lx 0x%08lx)\n",
	       dsaPtr->dwEntryCount, dsaPtr->dwMaxCount);
	if (dwParam2 >= dsaPtr->dwEntryCount) {
	    if (dsaPtr->dwEntryCount < dsaPtr->dwMaxCount) {
		dwIndex = dsaPtr->dwEntryCount;
		len = lstrlen32A ((LPSTR)dwParam3);
		dsaPtr->ptrs[dwIndex] =
		    (LPSTR)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len+1);
		lstrcpy32A (dsaPtr->ptrs[dwIndex], (LPSTR)dwParam3);
	    }
	    else {
		/* allocate new pointer list and copy all pointers */
		LPSTR *lpOldPtrs = dsaPtr->ptrs;
		dsaPtr->ptrs = (LPSTR*)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
				(dsaPtr->dwInitial + dsaPtr->dwGrow) *
				sizeof(LPVOID));
		memcpy (dsaPtr->ptrs, lpOldPtrs,
			dsaPtr->dwMaxCount * sizeof(LPVOID));
		dsaPtr->dwMaxCount += dsaPtr->dwGrow;
		HeapFree (GetProcessHeap (), 0, lpOldPtrs);

		/* add new string */
		dwIndex = dsaPtr->dwEntryCount;
		len = lstrlen32A ((LPSTR)dwParam3);
		dsaPtr->ptrs[dwIndex] =
		    (LPSTR)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len+1);
		lstrcpy32A (dsaPtr->ptrs[dwIndex], (LPSTR)dwParam3);
	    }
	}
	else {
	    FIXME (commctrl, "inserting! stub!\n");

	    dwIndex = dwParam2;
	}
    }

    dsaPtr->dwEntryCount++;

    TRACE (commctrl, "ret=0x%08lx\n", dwIndex);

    return (dwIndex);
}




DWORD WINAPI
DSA_DeleteItem (DWORD dwParam1, DWORD dwParam2)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;

    TRACE (commctrl, "(0x%08lx 0x%08lx):semi-stub!\n",
	   dwParam1, dwParam2);

    if (dsaPtr->ptrs) {
	if (dsaPtr->dwEntryCount == 1) {
	    if (dsaPtr->ptrs[dwParam2])
		HeapFree (GetProcessHeap (), 0, dsaPtr->ptrs[dwParam2]);
	    dsaPtr->dwEntryCount--;
	}
	else {
	    LPSTR *oldPtrs = dsaPtr->ptrs;
	    TRACE (commctrl, "complex delete!\n");

	    if (dsaPtr->ptrs[dwParam2])
		HeapFree (GetProcessHeap (), 0, dsaPtr->ptrs[dwParam2]);

	    dsaPtr->dwEntryCount--;
	    dsaPtr->ptrs = 
		(LPSTR*)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
				   dsaPtr->dwEntryCount * sizeof(LPVOID));
	    if (dwParam2 > 0) {
		memcpy (&dsaPtr->ptrs[0], &oldPtrs[0],
			dwParam2 * sizeof(LPSTR));
	    }

	    if (dwParam2 < dsaPtr->dwEntryCount) {
		memcpy (&dsaPtr->ptrs[dwParam2], &oldPtrs[dwParam2+1],
			(dsaPtr->dwEntryCount - dwParam2) * sizeof(LPSTR));
	    }
	    HeapFree (GetProcessHeap (), 0, oldPtrs);
	}

	if (dsaPtr->dwEntryCount == 0) {
	    HeapFree (GetProcessHeap (), 0, dsaPtr->ptrs);
	    dsaPtr->ptrs = NULL;
	}
    }

    return 0;
}





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

