/*
 *  Common Dialog Boxes interface (32 bit)
 *  Find/Replace
 *
 * Copyright 1999 Bertho A. Stultiens
 */

#include "winbase.h"
#include "commdlg.h"
#include "cderr.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"


HINSTANCE	COMDLG32_hInstance = 0;
HINSTANCE16	COMDLG32_hInstance16 = 0;

static DWORD	COMDLG32_TlsIndex;
static int	COMDLG32_Attach = 0;

HINSTANCE	COMCTL32_hInstance = 0;
HDPA	(WINAPI* COMDLG32_DPA_Create) (INT);  
LPVOID	(WINAPI* COMDLG32_DPA_GetPtr) (const HDPA, INT);   
LPVOID	(WINAPI* COMDLG32_DPA_DeletePtr) (const HDPA hdpa, INT i);
LPVOID	(WINAPI* COMDLG32_DPA_DeleteAllPtrs) (const HDPA hdpa);
INT	(WINAPI* COMDLG32_DPA_InsertPtr) (const HDPA, INT, LPVOID); 
BOOL	(WINAPI* COMDLG32_DPA_Destroy) (const HDPA); 

HICON	(WINAPI* COMDLG32_ImageList_GetIcon) (HIMAGELIST, INT, UINT);
HIMAGELIST (WINAPI *COMDLG32_ImageList_LoadImageA) (HINSTANCE, LPCSTR, INT, INT, COLORREF, UINT, UINT);
BOOL	(WINAPI* COMDLG32_ImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);
BOOL	(WINAPI* COMDLG32_ImageList_Destroy) (HIMAGELIST himl);

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
	TRACE("(%08x, %08lx, %p)\n", hInstance, Reason, Reserved);

	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		COMDLG32_Attach++;
		if(COMDLG32_hInstance)
		{
			ERR("comdlg32.dll instantiated twice in one address space!\n");
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
				ERR("Could not load sibling commdlg.dll\n");
				return FALSE;
			}
		}

		if((COMDLG32_TlsIndex = TlsAlloc()) == 0xffffffff)
		{
			ERR("No space for COMDLG32 TLS\n");
			return FALSE;
		}

		COMCTL32_hInstance = LoadLibraryA("COMCTL32.DLL");	

		COMDLG32_DPA_Create=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)328L);
		COMDLG32_DPA_Destroy=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)329L);
		COMDLG32_DPA_GetPtr=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)332L);
		COMDLG32_DPA_InsertPtr=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)334L);
		COMDLG32_DPA_DeletePtr=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)336L);
		COMDLG32_DPA_DeleteAllPtrs=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)337L);

		COMDLG32_ImageList_GetIcon=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_GetIcon");
		COMDLG32_ImageList_LoadImageA=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_LoadImageA");
		COMDLG32_ImageList_Draw=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_Draw");
		COMDLG32_ImageList_Destroy=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_Destroy");
		break;

	case DLL_PROCESS_DETACH:
		if(!--COMDLG32_Attach)
		{
			TlsFree(COMDLG32_TlsIndex);
			COMDLG32_hInstance = 0;
			if(COMDLG32_hInstance16)
				FreeLibrary16(COMDLG32_hInstance16);

			FreeLibrary(COMCTL32_hInstance);
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
	TRACE("(%08lx)\n", err);
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
