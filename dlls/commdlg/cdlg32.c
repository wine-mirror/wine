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
HINSTANCE	SHELL32_hInstance = 0;
HINSTANCE       SHLWAPI_hInstance = 0;

/* DPA */
HDPA	(WINAPI* COMDLG32_DPA_Create) (INT);  
LPVOID	(WINAPI* COMDLG32_DPA_GetPtr) (const HDPA, INT);   
LPVOID	(WINAPI* COMDLG32_DPA_DeletePtr) (const HDPA hdpa, INT i);
LPVOID	(WINAPI* COMDLG32_DPA_DeleteAllPtrs) (const HDPA hdpa);
INT	(WINAPI* COMDLG32_DPA_InsertPtr) (const HDPA, INT, LPVOID); 
BOOL	(WINAPI* COMDLG32_DPA_Destroy) (const HDPA); 

/* IMAGELIST */
HICON	(WINAPI* COMDLG32_ImageList_GetIcon) (HIMAGELIST, INT, UINT);
HIMAGELIST (WINAPI *COMDLG32_ImageList_LoadImageA) (HINSTANCE, LPCSTR, INT, INT, COLORREF, UINT, UINT);
BOOL	(WINAPI* COMDLG32_ImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);
BOOL	(WINAPI* COMDLG32_ImageList_Destroy) (HIMAGELIST himl);

/* ITEMIDLIST */
LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILClone) (LPCITEMIDLIST);
LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILCombine)(LPCITEMIDLIST,LPCITEMIDLIST);
LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILGetNext)(LPITEMIDLIST);
BOOL (WINAPI *COMDLG32_PIDL_ILRemoveLastID)(LPCITEMIDLIST);
BOOL (WINAPI *COMDLG32_PIDL_ILIsEqual)(LPCITEMIDLIST, LPCITEMIDLIST);

/* SHELL */
BOOL (WINAPI *COMDLG32_SHGetPathFromIDListA) (LPCITEMIDLIST,LPSTR);
HRESULT (WINAPI *COMDLG32_SHGetSpecialFolderLocation)(HWND,INT,LPITEMIDLIST *);
DWORD (WINAPI *COMDLG32_SHGetDesktopFolder)(IShellFolder **);
DWORD	(WINAPI *COMDLG32_SHGetFileInfoA)(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
HRESULT (WINAPI *COMDLG32_StrRetToBufW)(LPSTRRET,LPITEMIDLIST,LPVOID,DWORD);
HRESULT (WINAPI *COMDLG32_StrRetToBufA)(LPSTRRET,LPITEMIDLIST,LPVOID,DWORD);

/* PATH */
BOOL (WINAPI *COMDLG32_PathIsRootA)(LPCSTR x);
LPCSTR (WINAPI *COMDLG32_PathFindFilenameA)(LPCSTR path);
DWORD (WINAPI *COMDLG32_PathRemoveFileSpecA)(LPSTR fn);
BOOL (WINAPI *COMDLG32_PathMatchSpecW)(LPCWSTR x, LPCWSTR y);
LPSTR (WINAPI *COMDLG32_PathAddBackslashA)(LPSTR path);

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
		SHELL32_hInstance = LoadLibraryA("SHELL32.DLL");
		SHLWAPI_hInstance = LoadLibraryA("SHLWAPI.DLL");
		
		if (!COMCTL32_hInstance || !SHELL32_hInstance || !SHLWAPI_hInstance)
		{
			ERR("loading of comctl32 or shell32 or shlwapi failed\n");
			return FALSE;
		}
		/* DPA */
		COMDLG32_DPA_Create=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)328L);
		COMDLG32_DPA_Destroy=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)329L);
		COMDLG32_DPA_GetPtr=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)332L);
		COMDLG32_DPA_InsertPtr=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)334L);
		COMDLG32_DPA_DeletePtr=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)336L);
		COMDLG32_DPA_DeleteAllPtrs=(void*)GetProcAddress(COMCTL32_hInstance, (LPCSTR)337L);

		/* IMAGELIST */
		COMDLG32_ImageList_GetIcon=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_GetIcon");
		COMDLG32_ImageList_LoadImageA=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_LoadImageA");
		COMDLG32_ImageList_Draw=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_Draw");
		COMDLG32_ImageList_Destroy=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_Destroy");
		
		/* ITEMISLIST */
		
		COMDLG32_PIDL_ILIsEqual =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)21L);
		COMDLG32_PIDL_ILCombine =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)25L);
		COMDLG32_PIDL_ILGetNext =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)153L);
		COMDLG32_PIDL_ILClone =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)18L);
		COMDLG32_PIDL_ILRemoveLastID =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)17L);
		
		/* SHELL */
		
		COMDLG32_SHFree = (void*)GetProcAddress(SHELL32_hInstance,"SHFree");
		COMDLG32_SHGetSpecialFolderLocation = (void*)GetProcAddress(SHELL32_hInstance,"SHGetSpecialFolderLocation");
		COMDLG32_SHGetPathFromIDListA = (void*)GetProcAddress(SHELL32_hInstance,"SHGetPathFromIDListA");
		COMDLG32_SHGetDesktopFolder = (void*)GetProcAddress(SHELL32_hInstance,"SHGetDesktopFolder");
		COMDLG32_SHGetFileInfoA = (void*)GetProcAddress(SHELL32_hInstance,"SHGetFileInfoA");

		/* FIXME - change the followings to call GetProcAddress
		   when shlwapi.dll will work */
		COMDLG32_StrRetToBufW = (void*)GetProcAddress(SHLWAPI_hInstance,"StrRetToBufW"); 
		COMDLG32_StrRetToBufA = (void*)GetProcAddress(SHLWAPI_hInstance,"StrRetToBufA");
		/* PATH */
		COMDLG32_PathMatchSpecW = (void*)GetProcAddress(SHLWAPI_hInstance,"PathMatchSpecW");
		COMDLG32_PathIsRootA = (void*)GetProcAddress(SHLWAPI_hInstance,"PathIsRootA");
		COMDLG32_PathRemoveFileSpecA = (void*)GetProcAddress(SHLWAPI_hInstance,"PathRemoveFileSpecA");
		COMDLG32_PathFindFilenameA = (void*)GetProcAddress(SHLWAPI_hInstance,"PathFindFileNameA");
		COMDLG32_PathAddBackslashA = (void*)GetProcAddress(SHLWAPI_hInstance,"PathAddBackslashA");
		break;

	case DLL_PROCESS_DETACH:
		if(!--COMDLG32_Attach)
		{
			TlsFree(COMDLG32_TlsIndex);
			COMDLG32_hInstance = 0;
			if(COMDLG32_hInstance16)
				FreeLibrary16(COMDLG32_hInstance16);

		}
		FreeLibrary(COMCTL32_hInstance);
		FreeLibrary(SHELL32_hInstance);
		FreeLibrary(SHLWAPI_hInstance);
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
