/*
 *  Common Dialog Boxes interface (32 bit)
 *  Find/Replace
 *
 * Copyright 1999 Bertho A. Stultiens
 */

#include "winbase.h"
#include "commdlg.h"
#include "cderr.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"


HINSTANCE	COMDLG32_hInstance = 0;
HINSTANCE16	COMDLG32_hInstance16 = 0;

static DWORD	COMDLG32_TlsIndex;
static int	COMDLG32_Attach = 0;

/***********************************************************************
 *	COMDLG32_DllEntryPoint			(COMDLG32.entry)
 *
 *    Initialization code for the COMDLG32 DLL
 *
 * RETURNS:
 *	FALSE if sibling could not be loaded or instantiated twice, TRUE
 *	otherwise.
 */
BOOL WINAPI COMDLG32_DllEntryPoint(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
	TRACE(commdlg, "(%08x, %08lx, %p)\n", hInstance, Reason, Reserved);

	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		COMDLG32_Attach++;
		if(COMDLG32_hInstance)
		{
			ERR(commdlg, "comdlg32.dll instantiated twice in one address space!\n");
			/*
			 * We should return FALSE here, but that will break
			 * most apps that use CreateProcess because we do
			 * not yet support seperate address spaces.
			 */
			return TRUE;
		}

		COMDLG32_hInstance = hInstance;
		DisableThreadLibraryCalls(hInstance);

		if(!COMDLG32_hInstance16)
		{
			if(!(COMDLG32_hInstance16 = LoadLibrary16("commdlg.dll")))
			{
				ERR(commdlg, "Could not load sibling commdlg.dll\n");
				return FALSE;
			}
		}

		if((COMDLG32_TlsIndex = TlsAlloc()) == 0xffffffff)
		{
			ERR(commdlg, "No space for COMDLG32 TLS\n");
			return FALSE;
		}
		break;

	case DLL_PROCESS_DETACH:
		if(!--COMDLG32_Attach)
		{
			TlsFree(COMDLG32_TlsIndex);
			COMDLG32_hInstance = 0;
			if(COMDLG32_hInstance16)
				FreeLibrary16(COMDLG32_hInstance16);
		}
		break;
	}
	return TRUE;
}


/***********************************************************************
 *	COMDLG32_AllocMem 			(internal)
 * Get memory for internal datastructure plus stringspace etc.
 *	RETURNS
 *		Pointer to a heap block: Succes
 *		NULL: Failure
 */
LPVOID COMDLG32_AllocMem(
	int size	/* [in] Block size to allocate */
) {
        LPVOID ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if(!ptr)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
                return NULL;
        }
        return ptr;
}


/***********************************************************************
 *	COMDLG32_SetCommDlgExtendedError	(internal)
 *
 * Used to set the thread's local error value if a comdlg32 function fails.
 */
void COMDLG32_SetCommDlgExtendedError(DWORD err)
{
	TRACE(commdlg, "(%08lx)\n", err);
	TlsSetValue(COMDLG32_TlsIndex, (void *)err);
}


/***********************************************************************
 *	CommDlgExtendedError			(COMDLG32.5)
 *
 * Get the thread's local error value if a comdlg32 function fails.
 *	RETURNS
 *		Current error value which might not be valid
 *		if a previous call succeeded.
 */
DWORD WINAPI CommDlgExtendedError(void)
{
	return (DWORD)TlsGetValue(COMDLG32_TlsIndex);
}
