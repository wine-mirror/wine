/*
 *	shell icon cache (SIC)
 *
 * Copyright 1998, 1999 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "wine/debug.h"

#include "shellapi.h"
#include "shlguid.h"
#include "pidl.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/********************** THE ICON CACHE ********************************/

#define INVALID_INDEX -1

typedef struct
{
	LPSTR sSourceFile;	/* file (not path!) containing the icon */
	DWORD dwSourceIndex;	/* index within the file, if it is a resoure ID it will be negated */
	DWORD dwListIndex;	/* index within the iconlist */
	DWORD dwFlags;		/* GIL_* flags */
	DWORD dwAccessTime;
} SIC_ENTRY, * LPSIC_ENTRY;

static HDPA		sic_hdpa = 0;
static CRITICAL_SECTION SHELL32_SicCS = CRITICAL_SECTION_INIT("SHELL32_SicCS");

/*****************************************************************************
 * SIC_CompareEntries
 *
 * NOTES
 *  Callback for DPA_Search
 */
static INT CALLBACK SIC_CompareEntries( LPVOID p1, LPVOID p2, LPARAM lparam)
{	TRACE("%p %p %8lx\n", p1, p2, lparam);

	if (((LPSIC_ENTRY)p1)->dwSourceIndex != ((LPSIC_ENTRY)p2)->dwSourceIndex) /* first the faster one*/
	  return 1;

	if (strcasecmp(((LPSIC_ENTRY)p1)->sSourceFile,((LPSIC_ENTRY)p2)->sSourceFile))
	  return 1;

	return 0;
}
/*****************************************************************************
 * SIC_IconAppend			[internal]
 *
 * NOTES
 *  appends a icon pair to the end of the cache
 */
static INT SIC_IconAppend (LPCSTR sSourceFile, INT dwSourceIndex, HICON hSmallIcon, HICON hBigIcon)
{	LPSIC_ENTRY lpsice;
	INT ret, index, index1;
	char *path;
	TRACE("%s %i %x %x\n", sSourceFile, dwSourceIndex, hSmallIcon ,hBigIcon);

	lpsice = (LPSIC_ENTRY) SHAlloc (sizeof (SIC_ENTRY));

        path = PathFindFileNameA(sSourceFile);
        lpsice->sSourceFile = HeapAlloc( GetProcessHeap(), 0, strlen(path)+1 );
        strcpy( lpsice->sSourceFile, path );

	lpsice->dwSourceIndex = dwSourceIndex;

	EnterCriticalSection(&SHELL32_SicCS);

	index = DPA_InsertPtr(sic_hdpa, 0x7fff, lpsice);
	if ( INVALID_INDEX == index )
	{
	  SHFree(lpsice);
	  ret = INVALID_INDEX;
	}
	else
	{
	  index = ImageList_AddIcon (ShellSmallIconList, hSmallIcon);
	  index1= ImageList_AddIcon (ShellBigIconList, hBigIcon);

	  if (index!=index1)
	  {
	    FIXME("iconlists out of sync 0x%x 0x%x\n", index, index1);
	  }
	  lpsice->dwListIndex = index;
	  ret = lpsice->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}
/****************************************************************************
 * SIC_LoadIcon				[internal]
 *
 * NOTES
 *  gets small/big icon by number from a file
 */
static INT SIC_LoadIcon (LPCSTR sSourceFile, INT dwSourceIndex)
{	HICON	hiconLarge=0;
	HICON	hiconSmall=0;

        PrivateExtractIconsA( sSourceFile, dwSourceIndex, 32, 32, &hiconLarge, 0, 1, 0 );
        PrivateExtractIconsA( sSourceFile, dwSourceIndex, 16, 16, &hiconSmall, 0, 1, 0 );

	if ( !hiconLarge ||  !hiconSmall)
	{
	  WARN("failure loading icon %i from %s (%x %x)\n", dwSourceIndex, sSourceFile, hiconLarge, hiconSmall);
	  return -1;
	}
	return SIC_IconAppend (sSourceFile, dwSourceIndex, hiconSmall, hiconLarge);
}
/*****************************************************************************
 * SIC_GetIconIndex			[internal]
 *
 * Parameters
 *	sSourceFile	[IN]	filename of file containing the icon
 *	index		[IN]	index/resID (negated) in this file
 *
 * NOTES
 *  look in the cache for a proper icon. if not available the icon is taken
 *  from the file and cached
 */
INT SIC_GetIconIndex (LPCSTR sSourceFile, INT dwSourceIndex )
{	SIC_ENTRY sice;
	INT ret, index = INVALID_INDEX;

	TRACE("%s %i\n", sSourceFile, dwSourceIndex);

	sice.sSourceFile = PathFindFileNameA(sSourceFile);
	sice.dwSourceIndex = dwSourceIndex;

	EnterCriticalSection(&SHELL32_SicCS);

	if (NULL != DPA_GetPtr (sic_hdpa, 0))
	{
	  /* search linear from position 0*/
	  index = DPA_Search (sic_hdpa, &sice, 0, SIC_CompareEntries, 0, 0);
	}

	if ( INVALID_INDEX == index )
	{
	  ret = SIC_LoadIcon (sSourceFile, dwSourceIndex);
	}
	else
	{
	  TRACE("-- found\n");
	  ret = ((LPSIC_ENTRY)DPA_GetPtr(sic_hdpa, index))->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}
/****************************************************************************
 * SIC_GetIcon				[internal]
 *
 * NOTES
 *  retrieves the specified icon from the iconcache. if not found tries to load the icon
 */
static HICON WINE_UNUSED SIC_GetIcon (LPCSTR sSourceFile, INT dwSourceIndex, BOOL bSmallIcon )
{	INT index;

	TRACE("%s %i\n", sSourceFile, dwSourceIndex);

	index = SIC_GetIconIndex(sSourceFile, dwSourceIndex);

	if (INVALID_INDEX == index)
	{
	  return (HICON)INVALID_INDEX;
	}

	if (bSmallIcon)
	  return ImageList_GetIcon(ShellSmallIconList, index, ILD_NORMAL);
	return ImageList_GetIcon(ShellBigIconList, index, ILD_NORMAL);

}
/*****************************************************************************
 * SIC_Initialize			[internal]
 *
 * NOTES
 *  hack to load the resources from the shell32.dll under a different dll name
 *  will be removed when the resource-compiler is ready
 */
BOOL SIC_Initialize(void)
{
	HICON		hSm, hLg;
	UINT		index;

	TRACE("\n");

	if (sic_hdpa)	/* already initialized?*/
	  return TRUE;

	sic_hdpa = DPA_Create(16);

	if (!sic_hdpa)
	{
	  return(FALSE);
	}

	ShellSmallIconList = ImageList_Create(16,16,ILC_COLORDDB | ILC_MASK,0,0x20);
	ShellBigIconList = ImageList_Create(32,32,ILC_COLORDDB | ILC_MASK,0,0x20);

	ImageList_SetBkColor(ShellSmallIconList, GetSysColor(COLOR_WINDOW));
	ImageList_SetBkColor(ShellBigIconList, GetSysColor(COLOR_WINDOW));

	for (index=1; index<39; index++)
	{
	  hSm = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(index), IMAGE_ICON, 16, 16,LR_SHARED);
	  hLg = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(index), IMAGE_ICON, 32, 32,LR_SHARED);

	  if(!hSm)
	  {
	    hSm = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(0), IMAGE_ICON, 16, 16,LR_SHARED);
	    hLg = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(0), IMAGE_ICON, 32, 32,LR_SHARED);
	  }
	  SIC_IconAppend ("shell32.dll", index, hSm, hLg);
	}

	TRACE("hIconSmall=%p hIconBig=%p\n",ShellSmallIconList, ShellBigIconList);

	return TRUE;
}
/*************************************************************************
 * SIC_Destroy
 *
 * frees the cache
 */
void SIC_Destroy(void)
{
	LPSIC_ENTRY lpsice;
	int i;

	TRACE("\n");

	EnterCriticalSection(&SHELL32_SicCS);

	if (sic_hdpa && NULL != DPA_GetPtr (sic_hdpa, 0))
	{
	  for (i=0; i < DPA_GetPtrCount(sic_hdpa); ++i)
	  {
	    lpsice = DPA_GetPtr(sic_hdpa, i);
	    SHFree(lpsice);
	  }
	  DPA_Destroy(sic_hdpa);
	}

	sic_hdpa = NULL;

	LeaveCriticalSection(&SHELL32_SicCS);
	DeleteCriticalSection(&SHELL32_SicCS);
}
/*************************************************************************
 * Shell_GetImageList			[SHELL32.71]
 *
 * PARAMETERS
 *  imglist[1|2] [OUT] pointer which recive imagelist handles
 *
 */
BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList)
{	TRACE("(%p,%p)\n",lpBigList,lpSmallList);
	if (lpBigList)
	{ *lpBigList = ShellBigIconList;
	}
	if (lpSmallList)
	{ *lpSmallList = ShellSmallIconList;
	}

	return TRUE;
}
/*************************************************************************
 * PidlToSicIndex			[INTERNAL]
 *
 * PARAMETERS
 *	sh	[IN]	IShellFolder
 *	pidl	[IN]
 *	bBigIcon [IN]
 *	uFlags	[IN]	GIL_*
 *	pIndex	[OUT]	index within the SIC
 *
 */
BOOL PidlToSicIndex (
	IShellFolder * sh,
	LPITEMIDLIST pidl,
	BOOL bBigIcon,
	UINT uFlags,
	UINT * pIndex)
{
	IExtractIconA	*ei;
	char		szIconFile[MAX_PATH];	/* file containing the icon */
	INT		iSourceIndex;		/* index or resID(negated) in this file */
	BOOL		ret = FALSE;
	UINT		dwFlags = 0;

	TRACE("sf=%p pidl=%p %s\n", sh, pidl, bBigIcon?"Big":"Small");

	if (SUCCEEDED (IShellFolder_GetUIObjectOf(sh, 0, 1, &pidl, &IID_IExtractIconA, 0, (void **)&ei)))
	{
	  if (SUCCEEDED(IExtractIconA_GetIconLocation(ei, uFlags, szIconFile, MAX_PATH, &iSourceIndex, &dwFlags)))
	  {
	    *pIndex = SIC_GetIconIndex(szIconFile, iSourceIndex);
	    ret = TRUE;
	  }
	  IExtractIconA_Release(ei);
	}

	if (INVALID_INDEX == *pIndex)	/* default icon when failed */
	  *pIndex = 1;

	return ret;

}

/*************************************************************************
 * SHMapPIDLToSystemImageListIndex	[SHELL32.77]
 *
 * PARAMETERS
 *	sh	[IN]		pointer to an instance of IShellFolder
 *	pidl	[IN]
 *	pIndex	[OUT][OPTIONAL]	SIC index for big icon
 *
 */
int WINAPI SHMapPIDLToSystemImageListIndex(
	LPSHELLFOLDER sh,
	LPCITEMIDLIST pidl,
	UINT * pIndex)
{
	UINT	Index;

	TRACE("(SF=%p,pidl=%p,%p)\n",sh,pidl,pIndex);
	pdump(pidl);

	if (pIndex)
	  PidlToSicIndex ( sh, pidl, 1, 0, pIndex);
	PidlToSicIndex ( sh, pidl, 0, 0, &Index);
	return Index;
}

/*************************************************************************
 * Shell_GetCachedImageIndex		[SHELL32.72]
 *
 */
INT WINAPI Shell_GetCachedImageIndexA(LPCSTR szPath, INT nIndex, BOOL bSimulateDoc)
{
	WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_a(szPath), nIndex, bSimulateDoc);
	return SIC_GetIconIndex(szPath, nIndex);
}

INT WINAPI Shell_GetCachedImageIndexW(LPCWSTR szPath, INT nIndex, BOOL bSimulateDoc)
{	INT ret;
	LPSTR sTemp = HEAP_strdupWtoA (GetProcessHeap(),0,szPath);

	WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_w(szPath), nIndex, bSimulateDoc);

	ret = SIC_GetIconIndex(sTemp, nIndex);
	HeapFree(GetProcessHeap(),0,sTemp);
	return ret;
}

INT WINAPI Shell_GetCachedImageIndexAW(LPCVOID szPath, INT nIndex, BOOL bSimulateDoc)
{	if( SHELL_OsIsUnicode())
	  return Shell_GetCachedImageIndexW(szPath, nIndex, bSimulateDoc);
	return Shell_GetCachedImageIndexA(szPath, nIndex, bSimulateDoc);
}

/*************************************************************************
 * ExtractIconEx			[SHELL32.@]
 */
HICON WINAPI ExtractIconExAW ( LPCVOID lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons )
{	if (SHELL_OsIsUnicode())
	  return ExtractIconExW ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	return ExtractIconExA ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}
/*************************************************************************
 * ExtractIconExA			[SHELL32.@]
 * RETURNS
 *  0 no icon found
 *  1 file is not valid
 *  HICON handle of a icon (phiconLarge/Small == NULL)
 */
HICON WINAPI ExtractIconExA ( LPCSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons )
{	HICON ret=0;

	TRACE("file=%s idx=%i %p %p num=%i\n", lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons );

	if (nIconIndex==-1)	/* Number of icons requested */
            return (HICON)PrivateExtractIconsA( lpszFile, -1, 0, 0, NULL, 0, 0, 0 );

	if (phiconLarge)
        {
          ret = (HICON)PrivateExtractIconsA( lpszFile, nIconIndex, 32, 32, phiconLarge, 0, nIcons, 0 );
	  if ( nIcons==1)
	  { ret = phiconLarge[0];
	  }
	}

	/* if no pointers given and one icon expected, return the handle directly*/
	if (!phiconLarge && !phiconSmall && nIcons==1 )
	  phiconSmall = &ret;

	if (phiconSmall)
        {
          ret = (HICON)PrivateExtractIconsA( lpszFile, nIconIndex, 16, 16, phiconSmall, 0, nIcons, 0 );
	  if ( nIcons==1 )
	  { ret = phiconSmall[0];
	  }
	}

	return ret;
}
/*************************************************************************
 * ExtractIconExW			[SHELL32.@]
 */
HICON WINAPI ExtractIconExW ( LPCWSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons )
{	LPSTR sFile;
	HICON ret;

	TRACE("file=%s idx=%i %p %p num=%i\n", debugstr_w(lpszFile), nIconIndex, phiconLarge, phiconSmall, nIcons );

	sFile = HEAP_strdupWtoA (GetProcessHeap(),0,lpszFile);
	ret = ExtractIconExA ( sFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	HeapFree(GetProcessHeap(),0,sFile);
	return ret;
}

/*************************************************************************
 *				ExtractAssociatedIconA (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{	
	HICON hIcon;
	WORD wDummyIcon = 0;
	
	TRACE("\n");

	if(lpiIcon == NULL)
	    lpiIcon = &wDummyIcon;

	hIcon = ExtractIconA(hInst, lpIconPath, *lpiIcon);

	if( hIcon < (HICON)2 )
	{ if( hIcon == (HICON)1 ) /* no icons found in given file */
	  { char  tempPath[0x80];
	    UINT16  uRet = FindExecutable16(lpIconPath,NULL,tempPath);

	    if( uRet > 32 && tempPath[0] )
	    { strcpy(lpIconPath,tempPath);
	      hIcon = ExtractIconA(hInst, lpIconPath, *lpiIcon);
	      if( hIcon > (HICON)2 )
	        return hIcon;
	    }
	    else hIcon = 0;
	  }

	  if( hIcon == (HICON)1 )
	    *lpiIcon = 2;   /* MSDOS icon - we found .exe but no icons in it */
	  else
	    *lpiIcon = 6;   /* generic icon - found nothing */

	  GetModuleFileName16(hInst, lpIconPath, 0x80);
	  hIcon = LoadIconA( hInst, MAKEINTRESOURCEA(*lpiIcon));
	}
	return hIcon;
}

/*************************************************************************
 *				ExtractAssociatedIconExA (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconExA(DWORD d1, DWORD d2, DWORD d3, DWORD d4)
{
  FIXME("(%lx %lx %lx %lx): stub\n", d1, d2, d3, d4);
  return 0;
}

/*************************************************************************
 *				ExtractAssociatedIconExW (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconExW(DWORD d1, DWORD d2, DWORD d3, DWORD d4)
{
  FIXME("(%lx %lx %lx %lx): stub\n", d1, d2, d3, d4);
  return 0;
}
