/*
 * Undocumented functions from COMCTL32.DLL
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *
 * NOTES
 *     All of these functions are UNDOCUMENTED!! And I mean UNDOCUMENTED!!!!
 *     Do NOT rely on names of undocumented structures and types!!!
 *     These functions are used by EXPLORER.EXE, IEXPLORE.EXE and
 *     COMCTL32.DLL (internally).
 *
 * TODO
 *     - Fix DSA_InsertItem.
 *     - Write documentation.
 */

#include <string.h>
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


DWORD WINAPI Alloc (DWORD);
DWORD WINAPI ReAlloc (DWORD, DWORD);
DWORD WINAPI Free (DWORD);

DWORD WINAPI DSA_Create (DWORD, DWORD);


DWORD WINAPI DPA_Create (DWORD);
DWORD WINAPI DPA_GetPtr (DWORD, DWORD);
DWORD WINAPI DPA_InsertPtr (DWORD, DWORD, DWORD);


LPSTR WINAPI COMCTL32_StrChrA (LPSTR lpString, CHAR cChar);


DWORD WINAPI
Alloc (DWORD dwParam1)
{
    DWORD dwPtr;

    dwPtr = (DWORD)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY, dwParam1);

    TRACE (commctrl, "(0x%08lx) ret=0x%08lx\n", dwParam1, dwPtr);

    return dwPtr;
}


DWORD WINAPI
ReAlloc (DWORD dwParam1, DWORD dwParam2)
{
    DWORD dwPtr;

    if (dwParam1 == 0)
	dwPtr = (DWORD)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY, dwParam2);
    else
	dwPtr = (DWORD)HeapReAlloc (SystemHeap, HEAP_ZERO_MEMORY, 
				    (LPVOID)dwParam1, dwParam2);

    TRACE (commctrl, "(0x%08lx 0x%08lx) ret=0x%08lx\n",
	   dwParam1, dwParam2, dwPtr);

    return dwPtr;
}


DWORD WINAPI
Free (DWORD dwParam1)
{
    TRACE (commctrl, "(0x%08lx)\n", dwParam1);
    HeapFree (SystemHeap, 0, (LPVOID)dwParam1);

    return 0;
}





DWORD WINAPI
DSA_Create (DWORD dwParam1, DWORD dwParam2)
{
    LPDSA_DATA dsaPtr;

    dsaPtr = (LPDSA_DATA)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY, sizeof(DSA_DATA));
    dsaPtr->dwInitial = dwParam1;
    dsaPtr->dwGrow = dwParam2;

    TRACE (commctrl, "(0x%08lx 0x%08lx) ret=0x%08lx\n",
	   dwParam1, dwParam2, (DWORD)dsaPtr);

    return (DWORD)dsaPtr;
}


DWORD WINAPI
DSA_Destroy (DWORD dwParam1)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;
    DWORD i;

    TRACE (commctrl, "(0x%08lx):semi-stub!\n", dwParam1);

    if (dsaPtr->ptrs) {
	for (i = 0; i < dsaPtr->dwEntryCount; i++) {
	    if (dsaPtr->ptrs[i])
		HeapFree (SystemHeap, 0, (LPSTR)dsaPtr->ptrs[i]);
	}
    }

    HeapFree (SystemHeap, 0, dsaPtr);

    return 0;
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
	dsaPtr->ptrs = (LPSTR*)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
					  dsaPtr->dwInitial * sizeof(LPVOID));
	dsaPtr->dwMaxCount = dsaPtr->dwInitial;
        dwIndex = 0;
	len = lstrlen32A ((LPSTR)dwParam3);
	dsaPtr->ptrs[dwIndex] =
	    (LPSTR)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY, len+1);
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
		    (LPSTR)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY, len+1);
		lstrcpy32A (dsaPtr->ptrs[dwIndex], (LPSTR)dwParam3);
	    }
	    else {
		FIXME (commctrl, "resizing array! stub!\n");

		dwIndex = dwParam2;

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
DSA_DeleteItem (DWORD dwParam1, DWORD dwParam2)
{
    LPDSA_DATA dsaPtr = (LPDSA_DATA)dwParam1;

    TRACE (commctrl, "(0x%08lx 0x%08lx):semi-stub!\n",
	   dwParam1, dwParam2);

    if (dsaPtr->ptrs) {
	if (dsaPtr->dwEntryCount == 1) {
	    if (dsaPtr->ptrs[dwParam2])
		HeapFree (SystemHeap, 0, dsaPtr->ptrs[dwParam2]);
	    dsaPtr->dwEntryCount--;
	}
	else {
	    LPSTR *oldPtrs = dsaPtr->ptrs;
	    TRACE (commctrl, "complex delete!\n");

	    if (dsaPtr->ptrs[dwParam2])
		HeapFree (SystemHeap, 0, dsaPtr->ptrs[dwParam2]);

	    dsaPtr->dwEntryCount--;
	    dsaPtr->ptrs = 
		(LPSTR*)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
				   dsaPtr->dwEntryCount * sizeof(LPVOID));
	    if (dwParam2 > 0) {
		memcpy (&dsaPtr->ptrs[0], &oldPtrs[0],
			dwParam2 * sizeof(LPSTR));
	    }

	    if (dwParam2 < dsaPtr->dwEntryCount) {
		memcpy (&dsaPtr->ptrs[dwParam2], &oldPtrs[dwParam2+1],
			(dsaPtr->dwEntryCount - dwParam2) * sizeof(LPSTR));
	    }
	    HeapFree (SystemHeap, 0, oldPtrs);
	}

	if (dsaPtr->dwEntryCount == 0) {
	    HeapFree (SystemHeap, 0, dsaPtr->ptrs);
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


LPSTR WINAPI
COMCTL32_StrChrA (LPSTR lpString, CHAR cChar)
{
    return strchr (lpString, cChar);
}

