/*
 *	clipboard helper functions
 *
 *	Copyright 2000	Juergen Schmied <juergen.schmied@debitel.de>
 *
 * For copy & paste functions within contextmenus does the shell use
 * the OLE clipboard functions in combination with dataobjects.
 * The OLE32.DLL gets loaded with LoadLibrary
 *
 * - a right mousebutton-copy sets the following formats:
 *  classic:
 *	Shell IDList Array
 *	Prefered Drop Effect
 *	Shell Object Offsets
 *	HDROP
 *	FileName
 *  ole:
 *	OlePrivateData (ClipboardDataObjectInterface)
 *
 */

#include <string.h>

#include "debugtools.h"

#include "pidl.h"
#include "wine/undocshell.h"
#include "shell32_main.h"
#include "shell.h" /* DROPFILESTRUCT */

DEFAULT_DEBUG_CHANNEL(shell)

static int refClipCount = 0;
static HINSTANCE hShellOle32 = 0;

/**************************************************************************
 * InitShellOle
 *
 * 
 */
void InitShellOle(void)
{
}

/**************************************************************************
 * FreeShellOle
 *
 * unload OLE32.DLL
 */
void FreeShellOle(void)
{
	if (!--refClipCount)
	{
	  pOleUninitialize();
	  FreeLibrary(hShellOle32);
	}
}

/**************************************************************************
 * LoadShellOle
 *
 * make sure OLE32.DLL is loaded
 */
BOOL GetShellOle(void)
{
	if(!refClipCount)
	{
	  hShellOle32 = LoadLibraryA("ole32.dll");
	  if(hShellOle32)
	  {
	    pOleInitialize=(void*)GetProcAddress(hShellOle32,"OleInitialize");
	    pOleUninitialize=(void*)GetProcAddress(hShellOle32,"OleUninitialize");
	    pRegisterDragDrop=(void*)GetProcAddress(hShellOle32,"RegisterDragDrop");
	    pRevokeDragDrop=(void*)GetProcAddress(hShellOle32,"RevokeDragDrop");
	    pDoDragDrop=(void*)GetProcAddress(hShellOle32,"DoDragDrop");
	    pReleaseStgMedium=(void*)GetProcAddress(hShellOle32,"ReleaseStgMedium");
	    pOleSetClipboard=(void*)GetProcAddress(hShellOle32,"OleSetClipboard");
	    pOleGetClipboard=(void*)GetProcAddress(hShellOle32,"OleGetClipboard");

	    pOleInitialize(NULL);
	    refClipCount++;
	  }
	}
	return TRUE;
}

/**************************************************************************
 * RenderHDROP
 *
 * creates a CF_HDROP structure
 */
HGLOBAL RenderHDROP(LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl)
{
	int i;
	int rootsize = 0,size = 0;
	char szRootPath[MAX_PATH];
	char szFileName[MAX_PATH];
	HGLOBAL hGlobal;
	LPDROPFILESTRUCT pDropFiles;
	int offset;
	
	TRACE("(%p,%p,%u)\n", pidlRoot, apidl, cidl);

	/* get the size needed */
	size = sizeof(DROPFILESTRUCT);

	SHGetPathFromIDListA(pidlRoot, szRootPath);
	PathAddBackslashA(szRootPath);
	rootsize = strlen(szRootPath);
	
	for (i=0; i<cidl;i++)
	{
	  _ILSimpleGetText(apidl[i], szFileName, MAX_PATH);
	  size += rootsize + strlen(szFileName) + 1;
	}

	size++;

	/* Fill the structure */
	hGlobal = GlobalAlloc(GHND|GMEM_SHARE, size);
	if(!hGlobal) return hGlobal;

        pDropFiles = (LPDROPFILESTRUCT)GlobalLock(hGlobal);
        pDropFiles->lSize = sizeof(DROPFILESTRUCT);
        pDropFiles->fWideChar = FALSE;

	offset = pDropFiles->lSize;
	strcpy(szFileName, szRootPath);
	
	for (i=0; i<cidl;i++)
	{
	  
	  _ILSimpleGetText(apidl[i], szFileName + rootsize, MAX_PATH - rootsize);
	  size = strlen(szFileName) + 1;
	  lstrcpyA(((char*)pDropFiles)+offset, szFileName);
	  offset += size;
	}

	((char*)pDropFiles)[offset] = 0;
	GlobalUnlock(hGlobal);

	return hGlobal;
}

HGLOBAL RenderSHELLIDLIST (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl)
{
	int i,offset = 0, sizePidl, size;
	HGLOBAL hGlobal;
	LPCIDA	pcida;

	TRACE("(%p,%p,%u)\n", pidlRoot, apidl, cidl);

	/* get the size needed */
	size = sizeof(CIDA) + sizeof (UINT)*(cidl);	/* header */
	size += ILGetSize (pidlRoot);			/* root pidl */
	for(i=0; i<cidl; i++)
	{
	  size += ILGetSize(apidl[i]);			/* child pidls */
	}

	/* fill the structure */
	hGlobal = GlobalAlloc(GHND|GMEM_SHARE, size);		
	if(!hGlobal) return hGlobal;
	pcida = GlobalLock (hGlobal);
	pcida->cidl = cidl;

	/* root pidl */
	offset = sizeof(CIDA) + sizeof (UINT)*(cidl);
	pcida->aoffset[0] = offset;			/* first element */
	sizePidl = ILGetSize (pidlRoot);
	memcpy(((LPBYTE)pcida)+offset, pidlRoot, sizePidl);
	offset += sizePidl;

	for(i=0; i<cidl; i++)				/* child pidls */
	{
	  pcida->aoffset[i+1] = offset;
	  sizePidl = ILGetSize(apidl[i]);
	  memcpy(((LPBYTE)pcida)+offset, apidl[i], sizePidl);
	  offset += sizePidl;
	}

	GlobalUnlock(hGlobal);
	return hGlobal;
}

HGLOBAL RenderSHELLIDLISTOFFSET (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl)
{
	FIXME("\n");
	return 0;
}

HGLOBAL RenderFILECONTENTS (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl)
{
	FIXME("\n");
	return 0;
}

HGLOBAL RenderFILEDESCRIPTOR (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl)
{
	FIXME("\n");
	return 0;
}

HGLOBAL RenderFILENAME (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl)
{
	int len, size = 0;
	char szTemp[MAX_PATH], *szFileName;
	HGLOBAL hGlobal;
	
	TRACE("(%p,%p,%u)\n", pidlRoot, apidl, cidl);

	/* build name of first file */
	SHGetPathFromIDListA(pidlRoot, szTemp);
	PathAddBackslashA(szTemp);
	len = strlen(szTemp);
	_ILSimpleGetText(apidl[0], szTemp+len, MAX_PATH - len);
	size = strlen(szTemp) + 1;

	/* fill the structure */
	hGlobal = GlobalAlloc(GHND|GMEM_SHARE, size);
	if(!hGlobal) return hGlobal;
        szFileName = (char *)GlobalLock(hGlobal);
	GlobalUnlock(hGlobal);
	return hGlobal;
}

HGLOBAL RenderPREFEREDDROPEFFECT (DWORD dwFlags)
{
	DWORD * pdwFlag;
	HGLOBAL hGlobal;
	
	TRACE("(0x%08lx)\n", dwFlags);

	hGlobal = GlobalAlloc(GHND|GMEM_SHARE, sizeof(DWORD));
	if(!hGlobal) return hGlobal;
        pdwFlag = (DWORD*)GlobalLock(hGlobal);
	*pdwFlag = dwFlags;
	GlobalUnlock(hGlobal);
	return hGlobal;
}

/**************************************************************************
 * IsDataInClipboard
 *
 * checks if there is something in the clipboard we can use
 */
BOOL IsDataInClipboard (HWND hwnd)
{
	BOOL ret = FALSE;
	
	if (OpenClipboard(hwnd))
	{
	  if (GetOpenClipboardWindow())
	  {
	    ret = IsClipboardFormatAvailable(CF_TEXT);
	  }
	  CloseClipboard();
	}
	return ret;
}
