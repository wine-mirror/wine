/*
 *  Common Dialog Boxes interface (32 bit)
 *  Find/Replace
 *
 * Copyright 1999 Bertho A. Stultiens
 */

#include "winbase.h"
#include "wine/winbase16.h"
#include "commdlg.h"
#include "cderr.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"


HINSTANCE	COMDLG32_hInstance = 0;
HINSTANCE16	COMDLG32_hInstance16 = 0;

static DWORD	COMDLG32_TlsIndex;
static int	COMDLG32_Attach = 0;

HINSTANCE	COMCTL32_hInstance = 0;
HINSTANCE	SHELL32_hInstance = 0;
HINSTANCE       SHLWAPI_hInstance = 0;
HINSTANCE	SHFOLDER_hInstance = 0;

/* IMAGELIST */
BOOL (WINAPI* COMDLG32_ImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);

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
DWORD (WINAPI *COMDLG32_SHGetFileInfoA)(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
LPVOID (WINAPI *COMDLG32_SHAlloc)(DWORD);
DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
HRESULT (WINAPI *COMDLG32_SHGetDataFromIDListA)(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len);
HRESULT (WINAPI *COMDLG32_StrRetToBufA)(LPSTRRET,LPITEMIDLIST,LPSTR,DWORD);
HRESULT (WINAPI *COMDLG32_StrRetToBufW)(LPSTRRET,LPITEMIDLIST,LPWSTR,DWORD);
BOOL (WINAPI *COMDLG32_SHGetFolderPathA)(HWND,int,HANDLE,DWORD,LPSTR);

/* PATH */
BOOL (WINAPI *COMDLG32_PathIsRootA)(LPCSTR x);
LPSTR (WINAPI *COMDLG32_PathFindFileNameA)(LPCSTR path);
DWORD (WINAPI *COMDLG32_PathRemoveFileSpecA)(LPSTR fn);
BOOL (WINAPI *COMDLG32_PathMatchSpecW)(LPCWSTR x, LPCWSTR y);
LPSTR (WINAPI *COMDLG32_PathAddBackslashA)(LPSTR path);
BOOL (WINAPI *COMDLG32_PathCanonicalizeA)(LPSTR pszBuf, LPCSTR pszPath);
int (WINAPI *COMDLG32_PathGetDriveNumberA)(LPCSTR lpszPath);
BOOL (WINAPI *COMDLG32_PathIsRelativeA) (LPCSTR lpszPath);
LPSTR (WINAPI *COMDLG32_PathFindNextComponentA)(LPCSTR pszPath);
LPWSTR (WINAPI *COMDLG32_PathAddBackslashW)(LPWSTR lpszPath);
LPVOID (WINAPI *COMDLG32_PathFindExtensionA)(LPCVOID lpszPath);
BOOL (WINAPI *COMDLG32_PathAddExtensionA)(LPSTR  pszPath,LPCSTR pszExtension);

/***********************************************************************
 *	COMDLG32_DllEntryPoint			(COMDLG32.entry)
 *
 *    Initialization code for the COMDLG32 DLL
 *
 * RETURNS:
 *	FALSE if sibling could not be loaded or instantiated twice, TRUE
 *	otherwise.
 */
static char * GPA_string = "Failed to get entry point %s for hinst = 0x%08x\n";
#define GPA(dest, hinst, name) \
	if(!(dest = (void*)GetProcAddress(hinst,name)))\
	{ \
	  ERR(GPA_string, debugres_a(name), hinst); \
	  return FALSE; \
	}

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

		COMCTL32_hInstance = GetModuleHandleA("COMCTL32.DLL");	
		SHELL32_hInstance = GetModuleHandleA("SHELL32.DLL");
		SHLWAPI_hInstance = GetModuleHandleA("SHLWAPI.DLL");
		
		if (!COMCTL32_hInstance || !SHELL32_hInstance || !SHLWAPI_hInstance)
		{
			ERR("loading of comctl32 or shell32 or shlwapi failed\n");
			return FALSE;
		}

		/* IMAGELIST */
		GPA(COMDLG32_ImageList_Draw, COMCTL32_hInstance,"ImageList_Draw");
		
		/* ITEMIDLIST */
		GPA(COMDLG32_PIDL_ILIsEqual, SHELL32_hInstance, (LPCSTR)21L);
		GPA(COMDLG32_PIDL_ILCombine, SHELL32_hInstance, (LPCSTR)25L);
		GPA(COMDLG32_PIDL_ILGetNext, SHELL32_hInstance, (LPCSTR)153L);
		GPA(COMDLG32_PIDL_ILClone, SHELL32_hInstance, (LPCSTR)18L);
		GPA(COMDLG32_PIDL_ILRemoveLastID, SHELL32_hInstance, (LPCSTR)17L);
		
		/* SHELL */
		
		GPA(COMDLG32_SHAlloc, SHELL32_hInstance, (LPCSTR)196L);
		GPA(COMDLG32_SHFree, SHELL32_hInstance, (LPCSTR)195L);
		GPA(COMDLG32_SHGetSpecialFolderLocation, SHELL32_hInstance,"SHGetSpecialFolderLocation");
		GPA(COMDLG32_SHGetPathFromIDListA, SHELL32_hInstance,"SHGetPathFromIDListA");
		GPA(COMDLG32_SHGetDesktopFolder, SHELL32_hInstance,"SHGetDesktopFolder");
		GPA(COMDLG32_SHGetFileInfoA, SHELL32_hInstance,"SHGetFileInfoA");
		GPA(COMDLG32_SHGetDataFromIDListA, SHELL32_hInstance,"SHGetDataFromIDListA");

		/* for the first versions of shell32 SHGetFolderPathA is in SHFOLDER.DLL */
		COMDLG32_SHGetFolderPathA = (void*)GetProcAddress(SHELL32_hInstance,"SHGetFolderPathA");
		if (!COMDLG32_SHGetFolderPathA)
		{
		  SHFOLDER_hInstance = LoadLibraryA("SHFOLDER.DLL");
		  GPA(COMDLG32_SHGetFolderPathA, SHFOLDER_hInstance,"SHGetFolderPathA");
		}

		/* ### WARINIG ### 
		We can't do a GetProcAddress to link to  StrRetToBuf[A|W] sine not all 
		versions of the shlwapi are exporting these functions. When we are 
		seperating the dlls then we have to dublicate code from shell32 into comdlg32. 
		Till then just call these functions. These functions don't have any side effects 
		so it won't break the use of any combination of native and buildin dll's (jsch) */

		/* PATH */
		GPA(COMDLG32_PathMatchSpecW, SHLWAPI_hInstance,"PathMatchSpecW");
		GPA(COMDLG32_PathIsRootA, SHLWAPI_hInstance,"PathIsRootA");
		GPA(COMDLG32_PathRemoveFileSpecA, SHLWAPI_hInstance,"PathRemoveFileSpecA");
		GPA(COMDLG32_PathFindFileNameA, SHLWAPI_hInstance,"PathFindFileNameA");
		GPA(COMDLG32_PathAddBackslashA, SHLWAPI_hInstance,"PathAddBackslashA");
		GPA(COMDLG32_PathCanonicalizeA, SHLWAPI_hInstance,"PathCanonicalizeA");
		GPA(COMDLG32_PathGetDriveNumberA, SHLWAPI_hInstance,"PathGetDriveNumberA");
		GPA(COMDLG32_PathIsRelativeA, SHLWAPI_hInstance,"PathIsRelativeA");
		GPA(COMDLG32_PathFindNextComponentA, SHLWAPI_hInstance,"PathFindNextComponentA");
		GPA(COMDLG32_PathAddBackslashW, SHLWAPI_hInstance,"PathAddBackslashW");
		GPA(COMDLG32_PathFindExtensionA, SHLWAPI_hInstance,"PathFindExtensionA");
		GPA(COMDLG32_PathAddExtensionA, SHLWAPI_hInstance,"PathAddExtensionA");
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
			if(SHFOLDER_hInstance)
				FreeLibrary(SHFOLDER_hInstance);

		}
		break;
	}
	return TRUE;
}
#undef GPA

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
