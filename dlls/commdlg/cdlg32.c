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

/* IMAGELIST */
BOOL	(WINAPI* COMDLG32_ImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);

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
LPVOID (WINAPI *COMDLG32_SHAlloc)(DWORD);
DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
HRESULT (WINAPI *COMDLG32_StrRetToBufA)(LPSTRRET,LPITEMIDLIST,LPSTR,DWORD);
HRESULT (WINAPI *COMDLG32_StrRetToBufW)(LPSTRRET,LPITEMIDLIST,LPWSTR,DWORD);

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

		COMDLG32_TlsIndex = 0xffffffff;

		COMCTL32_hInstance = LoadLibraryA("COMCTL32.DLL");	
		SHELL32_hInstance = LoadLibraryA("SHELL32.DLL");
		SHLWAPI_hInstance = LoadLibraryA("SHLWAPI.DLL");
		
		if (!COMCTL32_hInstance || !SHELL32_hInstance || !SHLWAPI_hInstance)
		{
			ERR("loading of comctl32 or shell32 or shlwapi failed\n");
			return FALSE;
		}

		/* IMAGELIST */
		COMDLG32_ImageList_Draw=(void*)GetProcAddress(COMCTL32_hInstance,"ImageList_Draw");
		
		/* ITEMISLIST */
		
		COMDLG32_PIDL_ILIsEqual =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)21L);
		COMDLG32_PIDL_ILCombine =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)25L);
		COMDLG32_PIDL_ILGetNext =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)153L);
		COMDLG32_PIDL_ILClone =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)18L);
		COMDLG32_PIDL_ILRemoveLastID =(void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)17L);
		
		/* SHELL */
		
		COMDLG32_SHAlloc = (void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)196L);
		COMDLG32_SHFree = (void*)GetProcAddress(SHELL32_hInstance, (LPCSTR)195L);
		COMDLG32_SHGetSpecialFolderLocation = (void*)GetProcAddress(SHELL32_hInstance,"SHGetSpecialFolderLocation");
		COMDLG32_SHGetPathFromIDListA = (void*)GetProcAddress(SHELL32_hInstance,"SHGetPathFromIDListA");
		COMDLG32_SHGetDesktopFolder = (void*)GetProcAddress(SHELL32_hInstance,"SHGetDesktopFolder");
		COMDLG32_SHGetFileInfoA = (void*)GetProcAddress(SHELL32_hInstance,"SHGetFileInfoA");

		/* ### WARINIG ### 
		We can't do a GetProcAddress to link to  StrRetToBuf[A|W] sine not all 
		versions of the shlwapi are exporting these functions. When we are 
		seperating the dlls then we have to dublicate code from shell32 into comdlg32. 
		Till then just call these functions. These functions don't have any side effects 
		so it won't break the use of any combination of native and buildin dll's (jsch) */

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
		        if (COMDLG32_TlsIndex != 0xffffffff)
			  TlsFree(COMDLG32_TlsIndex);
			COMDLG32_TlsIndex = 0xffffffff;
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
        if (COMDLG32_TlsIndex == 0xffffffff)
	  COMDLG32_TlsIndex = TlsAlloc();
	if (COMDLG32_TlsIndex != 0xffffffff)
	  TlsSetValue(COMDLG32_TlsIndex, (void *)err);
	else
	  FIXME("No Tls Space\n");
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
        if (COMDLG32_TlsIndex != 0xffffffff) 
	  return (DWORD)TlsGetValue(COMDLG32_TlsIndex);
	else
	  {
	    FIXME("No Tls Space\n");
	    return 0;
	  }
}
