/*
 *	pidl Handling
 *
 *	Copyright 1998	Juergen Schmied
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
 *
 * NOTES
 *  a pidl == NULL means desktop and is legal
 *
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "winreg.h"
#include "shlguid.h"
#include "winerror.h"
#include "winnls.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shellapi.h"
#include "shlwapi.h"

#include "pidl.h"
#include "debughlp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pidl);
WINE_DECLARE_DEBUG_CHANNEL(shell);

/* from comctl32.dll */
extern LPVOID WINAPI Alloc(INT);
extern BOOL WINAPI Free(LPVOID);

/*************************************************************************
 * ILGetDisplayName			[SHELL32.15]
 */
BOOL WINAPI ILGetDisplayName(LPCITEMIDLIST pidl,LPSTR path)
{
	TRACE_(shell)("pidl=%p %p semi-stub\n",pidl,path);
	return SHGetPathFromIDListA(pidl, path);
}
/*************************************************************************
 * ILFindLastID [SHELL32.16]
 *
 * NOTES
 *   observed: pidl=Desktop return=pidl
 */
LPITEMIDLIST WINAPI ILFindLastID(LPITEMIDLIST pidl)
{	LPITEMIDLIST   pidlLast = pidl;

	TRACE("(pidl=%p)\n",pidl);

	while (pidl->mkid.cb)
	{
	  pidlLast = pidl;
	  pidl = ILGetNext(pidl);
	}
	return pidlLast;
}
/*************************************************************************
 * ILRemoveLastID [SHELL32.17]
 *
 * NOTES
 *   when pidl=Desktop return=FALSE
 */
BOOL WINAPI ILRemoveLastID(LPCITEMIDLIST pidl)
{
	TRACE_(shell)("pidl=%p\n",pidl);

	if (!pidl || !pidl->mkid.cb)
	  return 0;
	ILFindLastID(pidl)->mkid.cb = 0;
	return 1;
}

/*************************************************************************
 * ILClone [SHELL32.18]
 *
 * NOTES
 *    duplicate an idlist
 */
LPITEMIDLIST WINAPI ILClone (LPCITEMIDLIST pidl)
{ DWORD    len;
  LPITEMIDLIST  newpidl;

  if (!pidl)
    return NULL;

  len = ILGetSize(pidl);
  newpidl = (LPITEMIDLIST)SHAlloc(len);
  if (newpidl)
    memcpy(newpidl,pidl,len);

  TRACE("pidl=%p newpidl=%p\n",pidl, newpidl);
  pdump(pidl);

  return newpidl;
}
/*************************************************************************
 * ILCloneFirst [SHELL32.19]
 *
 * NOTES
 *  duplicates the first idlist of a complex pidl
 */
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST pidl)
{	DWORD len;
	LPITEMIDLIST pidlNew = NULL;

	TRACE("pidl=%p \n",pidl);
	pdump(pidl);

	if (pidl)
	{
	  len = pidl->mkid.cb;
	  pidlNew = (LPITEMIDLIST) SHAlloc (len+2);
	  if (pidlNew)
	  {
	    memcpy(pidlNew,pidl,len+2);		/* 2 -> mind a desktop pidl */

	    if (len)
	      ILGetNext(pidlNew)->mkid.cb = 0x00;
	  }
	}
	TRACE("-- newpidl=%p\n",pidlNew);

	return pidlNew;
}

/*************************************************************************
 * ILLoadFromStream (SHELL32.26)
 *
 * NOTES
 *   the first two bytes are the len, the pidl is following then
 */
HRESULT WINAPI ILLoadFromStream (IStream * pStream, LPITEMIDLIST * ppPidl)
{	WORD		wLen = 0;
	DWORD		dwBytesRead;
	HRESULT		ret = E_FAIL;


	TRACE_(shell)("%p %p\n", pStream ,  ppPidl);

	if (*ppPidl)
	{ SHFree(*ppPidl);
	  *ppPidl = NULL;
	}

	IStream_AddRef (pStream);

	if (SUCCEEDED(IStream_Read(pStream, (LPVOID)&wLen, 2, &dwBytesRead)))
	{ *ppPidl = SHAlloc (wLen);
	  if (SUCCEEDED(IStream_Read(pStream, *ppPidl , wLen, &dwBytesRead)))
	  { ret = S_OK;
	  }
	  else
	  { SHFree(*ppPidl);
	    *ppPidl = NULL;
	  }
	}

	/* we are not yet fully compatible */
	if (!pcheck(*ppPidl))
	{ SHFree(*ppPidl);
	  *ppPidl = NULL;
	}


	IStream_Release (pStream);

	return ret;
}

/*************************************************************************
 * ILSaveToStream (SHELL32.27)
 *
 * NOTES
 *   the first two bytes are the len, the pidl is following then
 */
HRESULT WINAPI ILSaveToStream (IStream * pStream, LPCITEMIDLIST pPidl)
{
	LPITEMIDLIST	pidl;
	WORD		wLen = 0;
	HRESULT		ret = E_FAIL;

	TRACE_(shell)("%p %p\n", pStream, pPidl);

	IStream_AddRef (pStream);

	pidl = pPidl;
        while (pidl->mkid.cb)
        {
          wLen += sizeof(WORD) + pidl->mkid.cb;
          pidl = ILGetNext(pidl);
        }

	if (SUCCEEDED(IStream_Write(pStream, (LPVOID)&wLen, 2, NULL)))
	{
	  if (SUCCEEDED(IStream_Write(pStream, pPidl, wLen, NULL)))
	  { ret = S_OK;
	  }
	}


	IStream_Release (pStream);

	return ret;
}

HRESULT WINAPI SHILCreateFromPathA (LPCSTR path, LPITEMIDLIST * ppidl, DWORD * attributes)
{	LPSHELLFOLDER sf;
	WCHAR lpszDisplayName[MAX_PATH];
	DWORD pchEaten;
	HRESULT ret = E_FAIL;

	TRACE_(shell)("%s %p 0x%08lx\n",path,ppidl,attributes?*attributes:0);

        if (!MultiByteToWideChar( CP_ACP, 0, path, -1, lpszDisplayName, MAX_PATH ))
            lpszDisplayName[MAX_PATH-1] = 0;

	if (SUCCEEDED (SHGetDesktopFolder(&sf)))
	{
	  ret = IShellFolder_ParseDisplayName(sf,0, NULL,lpszDisplayName,&pchEaten,ppidl,attributes);
	  IShellFolder_Release(sf);
	}
	return ret;
}

HRESULT WINAPI SHILCreateFromPathW (LPCWSTR path, LPITEMIDLIST * ppidl, DWORD * attributes)
{	LPSHELLFOLDER sf;
	DWORD pchEaten;
	HRESULT ret = E_FAIL;

	TRACE_(shell)("%s %p 0x%08lx\n",debugstr_w(path),ppidl,attributes?*attributes:0);

	if (SUCCEEDED (SHGetDesktopFolder(&sf)))
	{
	  ret = IShellFolder_ParseDisplayName(sf,0, NULL, (LPWSTR) path, &pchEaten, ppidl, attributes);
	  IShellFolder_Release(sf);
	}
	return ret;
}

/*************************************************************************
 * SHILCreateFromPath   [SHELL32.28]
 *
 * NOTES
 *   Wrapper for IShellFolder_ParseDisplayName().
 */
HRESULT WINAPI SHILCreateFromPathAW (LPCVOID path, LPITEMIDLIST * ppidl, DWORD * attributes)
{
	if ( SHELL_OsIsUnicode())
	  return SHILCreateFromPathW (path, ppidl, attributes);
	return SHILCreateFromPathA (path, ppidl, attributes);
}

/*************************************************************************
 * SHCloneSpecialIDList [SHELL32.89]
 *
 * PARAMETERS
 *  hwndOwner 	[in]
 *  nFolder 	[in]	CSIDL_xxxxx ??
 *
 * RETURNS
 *  pidl ??
 * NOTES
 *     exported by ordinal
 */
LPITEMIDLIST WINAPI SHCloneSpecialIDList(HWND hwndOwner,DWORD nFolder,DWORD x3)
{	LPITEMIDLIST ppidl;
	WARN_(shell)("(hwnd=%p,csidl=0x%lx,0x%lx):semi-stub.\n",
					 hwndOwner,nFolder,x3);

	SHGetSpecialFolderLocation(hwndOwner, nFolder, &ppidl);

	return ppidl;
}

/*************************************************************************
 * ILGlobalClone [SHELL32.20]
 *
 */
LPITEMIDLIST WINAPI ILGlobalClone(LPCITEMIDLIST pidl)
{	DWORD    len;
	LPITEMIDLIST  newpidl;

	if (!pidl)
	  return NULL;

	len = ILGetSize(pidl);
	newpidl = (LPITEMIDLIST)Alloc(len);
	if (newpidl)
	  memcpy(newpidl,pidl,len);

	TRACE("pidl=%p newpidl=%p\n",pidl, newpidl);
	pdump(pidl);

	return newpidl;
}

/*************************************************************************
 * ILIsEqual [SHELL32.21]
 *
 */
BOOL WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
	char	szData1[MAX_PATH];
	char	szData2[MAX_PATH];

	LPITEMIDLIST pidltemp1 = pidl1;
	LPITEMIDLIST pidltemp2 = pidl2;

	TRACE("pidl1=%p pidl2=%p\n",pidl1, pidl2);

	/* explorer reads from registry directly (StreamMRU),
	   so we can only check here */
	if ((!pcheck (pidl1)) || (!pcheck (pidl2))) return FALSE;

	pdump (pidl1);
	pdump (pidl2);

	if ( (!pidl1) || (!pidl2) ) return FALSE;

	while (pidltemp1->mkid.cb && pidltemp2->mkid.cb)
	{
	    _ILSimpleGetText(pidltemp1, szData1, MAX_PATH);
	    _ILSimpleGetText(pidltemp2, szData2, MAX_PATH);

	    if (strcasecmp ( szData1, szData2 )!=0 )
	      return FALSE;

	    pidltemp1 = ILGetNext(pidltemp1);
	    pidltemp2 = ILGetNext(pidltemp2);
	}

	if (!pidltemp1->mkid.cb && !pidltemp2->mkid.cb)
	{
	  return TRUE;
	}

	return FALSE;
}
/*************************************************************************
 * ILIsParent [SHELL32.23]
 *
 * parent=a/b	child=a/b/c -> true, c is in folder a/b
 *		child=a/b/c/d -> false if bImmediate is true, d is not in folder a/b
 *		child=a/b/c/d -> true if bImmediate is false, d is in a subfolder of a/b
 */
BOOL WINAPI ILIsParent( LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild, BOOL bImmediate)
{
	char	szData1[MAX_PATH];
	char	szData2[MAX_PATH];

	LPITEMIDLIST pParent = pidlParent;
	LPITEMIDLIST pChild = pidlChild;

	TRACE("%p %p %x\n", pidlParent, pidlChild, bImmediate);

	while (pParent->mkid.cb && pChild->mkid.cb)
	{
	  _ILSimpleGetText(pParent, szData1, MAX_PATH);
	  _ILSimpleGetText(pChild, szData2, MAX_PATH);

	  if (strcasecmp ( szData1, szData2 )!=0 )
	    return FALSE;

	  pParent = ILGetNext(pParent);
	  pChild = ILGetNext(pChild);
	}

	if ( pParent->mkid.cb || ! pChild->mkid.cb)	/* child shorter or has equal length to parent */
	  return FALSE;

	if ( ILGetNext(pChild)->mkid.cb && bImmediate)	/* not immediate descent */
	  return FALSE;

	return TRUE;
}

/*************************************************************************
 * ILFindChild [SHELL32.24]
 *
 * NOTES
 *  Compares elements from pidl1 and pidl2.
 *
 *  pidl1 is desktop		pidl2
 *  pidl1 shorter pidl2		pointer to first different element of pidl2
 *				if there was at least one equal element
 *  pidl2 shorter pidl1		0
 *  pidl2 equal pidl1		pointer to last 0x00-element of pidl2
 */
LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2)
{
	char	szData1[MAX_PATH];
	char	szData2[MAX_PATH];

	LPITEMIDLIST pidltemp1 = pidl1;
	LPITEMIDLIST pidltemp2 = pidl2;
	LPITEMIDLIST ret=NULL;

	TRACE("pidl1=%p pidl2=%p\n",pidl1, pidl2);

	/* explorer reads from registry directly (StreamMRU),
	   so we can only check here */
	if ((!pcheck (pidl1)) || (!pcheck (pidl2)))
	  return FALSE;

	pdump (pidl1);
	pdump (pidl2);

	if ( _ILIsDesktop(pidl1) )
	{
	  ret = pidl2;
	}
	else
	{
	  while (pidltemp1->mkid.cb && pidltemp2->mkid.cb)
	  {
	    _ILSimpleGetText(pidltemp1, szData1, MAX_PATH);
	    _ILSimpleGetText(pidltemp2, szData2, MAX_PATH);

	    if (strcasecmp(szData1,szData2))
	      break;

	    pidltemp1 = ILGetNext(pidltemp1);
	    pidltemp2 = ILGetNext(pidltemp2);
	    ret = pidltemp2;
	  }

	  if (pidltemp1->mkid.cb)
	  {
	    ret = NULL; /* elements of pidl1 left*/
	  }
	}
	TRACE_(shell)("--- %p\n", ret);
	return ret; /* pidl 1 is shorter */
}

/*************************************************************************
 * ILCombine [SHELL32.25]
 *
 * NOTES
 *  Concatenates two complex idlists.
 *  The pidl is the first one, pidlsub the next one
 *  Does not destroy the passed in idlists!
 */
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2)
{
	DWORD    len1,len2;
	LPITEMIDLIST  pidlNew;

	TRACE("pidl=%p pidl=%p\n",pidl1,pidl2);

	if(!pidl1 && !pidl2) return NULL;

	pdump (pidl1);
	pdump (pidl2);

	if(!pidl1)
	{
	  pidlNew = ILClone(pidl2);
	  return pidlNew;
	}

	if(!pidl2)
	{
	  pidlNew = ILClone(pidl1);
	  return pidlNew;
	}

	len1  = ILGetSize(pidl1)-2;
	len2  = ILGetSize(pidl2);
	pidlNew  = SHAlloc(len1+len2);

	if (pidlNew)
	{
	  memcpy(pidlNew,pidl1,len1);
	  memcpy(((BYTE *)pidlNew)+len1,pidl2,len2);
	}

	/*  TRACE(pidl,"--new pidl=%p\n",pidlNew);*/
	return pidlNew;
}
/*************************************************************************
 *  SHGetRealIDL [SHELL32.98]
 *
 * NOTES
 */
LPITEMIDLIST WINAPI SHGetRealIDL(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl, DWORD z)
{
	FIXME("sf=%p pidl=%p 0x%04lx\n",lpsf,pidl,z);

	pdump (pidl);
	return 0;
}

/*************************************************************************
 *  SHLogILFromFSIL [SHELL32.95]
 *
 * NOTES
 *  pild = CSIDL_DESKTOP	ret = 0
 *  pild = CSIDL_DRIVES		ret = 0
 */
LPITEMIDLIST WINAPI SHLogILFromFSIL(LPITEMIDLIST pidl)
{
	FIXME("(pidl=%p)\n",pidl);

	pdump(pidl);

	return 0;
}

/*************************************************************************
 * ILGetSize [SHELL32.152]
 *  gets the byte size of an idlist including zero terminator (pidl)
 *
 * PARAMETERS
 *  pidl ITEMIDLIST
 *
 * RETURNS
 *  size of pidl
 *
 * NOTES
 *  exported by ordinal
 */
DWORD WINAPI ILGetSize(LPITEMIDLIST pidl)
{
	LPSHITEMID si = &(pidl->mkid);
	DWORD  len=0;

	if (pidl)
	{ while (si->cb)
	  { len += si->cb;
	    si  = (LPSHITEMID)(((LPBYTE)si)+si->cb);
	  }
	  len += 2;
	}
	TRACE("pidl=%p size=%lu\n",pidl, len);
	return len;
}

/*************************************************************************
 * ILGetNext [SHELL32.153]
 *  gets the next simple pidl of a complex pidl
 *
 * observed return values:
 *  null -> null
 *  desktop -> null
 *  simple pidl -> pointer to 0x0000 element
 *
 */
LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl)
{
	WORD len;

	TRACE("%p\n", pidl);

	if(pidl)
	{
	  len =  pidl->mkid.cb;
	  if (len)
	  {
	    pidl = (LPITEMIDLIST) (((LPBYTE)pidl)+len);
	    TRACE("-- %p\n", pidl);
	    return pidl;
	  }
	}
	return NULL;
}
/*************************************************************************
 * ILAppend [SHELL32.154]
 *
 * NOTES
 *  Adds the single item to the idlist indicated by pidl.
 *  if bEnd is 0, adds the item to the front of the list,
 *  otherwise adds the item to the end. (???)
 *  Destroys the passed in idlist! (???)
 */
LPITEMIDLIST WINAPI ILAppend(LPITEMIDLIST pidl,LPCITEMIDLIST item,BOOL bEnd)
{
	LPITEMIDLIST idlRet;

	WARN("(pidl=%p,pidl=%p,%08u)semi-stub\n",pidl,item,bEnd);

	pdump (pidl);
	pdump (item);

	if (_ILIsDesktop(pidl))
	{
	   idlRet = ILClone(item);
	   if (pidl)
	     SHFree (pidl);
	   return idlRet;
	}

	if (bEnd)
	{
	  idlRet=ILCombine(pidl,item);
	}
	else
	{
	  idlRet=ILCombine(item,pidl);
	}

	SHFree(pidl);
	return idlRet;
}
/*************************************************************************
 * ILFree [SHELL32.155]
 *
 * NOTES
 *     free_check_ptr - frees memory (if not NULL)
 *     allocated by SHMalloc allocator
 *     exported by ordinal
 */
DWORD WINAPI ILFree(LPITEMIDLIST pidl)
{
	TRACE("(pidl=0x%08lx)\n",(DWORD)pidl);

	if(!pidl) return FALSE;
	SHFree(pidl);
	return TRUE;
}
/*************************************************************************
 * ILGlobalFree [SHELL32.156]
 *
 */
void WINAPI ILGlobalFree( LPCITEMIDLIST pidl)
{
	TRACE("%p\n",pidl);

	if(!pidl) return;
	Free(pidl);
}
/*************************************************************************
 * ILCreateFromPath [SHELL32.157]
 *
 */
LPITEMIDLIST WINAPI ILCreateFromPathA (LPCSTR path)
{
	LPITEMIDLIST pidlnew;
	DWORD attributes = 0;

	TRACE_(shell)("%s\n",path);

	if (SUCCEEDED (SHILCreateFromPathA (path, &pidlnew, &attributes)))
	  return pidlnew;
	return FALSE;
}
LPITEMIDLIST WINAPI ILCreateFromPathW (LPCWSTR path)
{
	LPITEMIDLIST pidlnew;
	DWORD attributes = 0;

	TRACE_(shell)("%s\n",debugstr_w(path));

	if (SUCCEEDED (SHILCreateFromPathW (path, &pidlnew, &attributes)))
	  return pidlnew;
	return FALSE;
}
LPITEMIDLIST WINAPI ILCreateFromPathAW (LPCVOID path)
{
	if ( SHELL_OsIsUnicode())
	  return ILCreateFromPathW (path);
	return ILCreateFromPathA (path);
}
/*************************************************************************
 *  SHSimpleIDListFromPath [SHELL32.162]
 */
LPITEMIDLIST WINAPI SHSimpleIDListFromPathA (LPCSTR lpszPath)
{
	LPITEMIDLIST	pidl=NULL;
	HANDLE	hFile;
	WIN32_FIND_DATAA	stffile;

	TRACE("path=%s\n", lpszPath);

	if (!lpszPath) return NULL;

	hFile = FindFirstFileA(lpszPath, &stffile);

	if ( hFile != INVALID_HANDLE_VALUE )
	{
	  if (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	  {
	    pidl = _ILCreateFolder (&stffile);
	  }
	  else
	  {
	    pidl = _ILCreateValue (&stffile);
	  }
	  FindClose (hFile);
	}
	return pidl;
}
LPITEMIDLIST WINAPI SHSimpleIDListFromPathW (LPCWSTR lpszPath)
{
	char	lpszTemp[MAX_PATH];
	TRACE("path=%s\n",debugstr_w(lpszPath));

        if (!WideCharToMultiByte( CP_ACP, 0, lpszPath, -1, lpszTemp, sizeof(lpszTemp), NULL, NULL ))
            lpszTemp[sizeof(lpszTemp)-1] = 0;

	return SHSimpleIDListFromPathA (lpszTemp);
}

LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW (LPCVOID lpszPath)
{
	if ( SHELL_OsIsUnicode())
	  return SHSimpleIDListFromPathW (lpszPath);
	return SHSimpleIDListFromPathA (lpszPath);
}

/*************************************************************************
 * SHGetSpecialFolderLocation		[SHELL32.@]
 *
 * gets the folder locations from the registry and creates a pidl
 * creates missing reg keys and directories
 *
 * PARAMS
 *   hwndOwner [I]
 *   nFolder   [I] CSIDL_xxxxx
 *   ppidl     [O] PIDL of a special folder
 *
 */
HRESULT WINAPI SHGetSpecialFolderLocation(
	HWND hwndOwner,
	INT nFolder,
	LPITEMIDLIST * ppidl)
{
	CHAR		szPath[MAX_PATH];
	HRESULT		hr = E_INVALIDARG;

	TRACE_(shell)("(%p,0x%x,%p)\n", hwndOwner,nFolder,ppidl);

	if (ppidl)
	{
	  *ppidl = NULL;
	  switch (nFolder)
	  {
	    case CSIDL_DESKTOP:
	      *ppidl = _ILCreateDesktop();
	      break;

	    case CSIDL_DRIVES:
	      *ppidl = _ILCreateMyComputer();
	      break;

	    case CSIDL_NETWORK:
	      *ppidl = _ILCreateNetwork ();
	      break;

	    case CSIDL_CONTROLS:
	      *ppidl = _ILCreateControl ();
	      break;

	    case CSIDL_PRINTERS:
	      *ppidl = _ILCreatePrinter ();
	      break;

	    case CSIDL_BITBUCKET:
	      *ppidl = _ILCreateBitBucket ();
	      break;

	    default:
	      if (SHGetSpecialFolderPathA(hwndOwner, szPath, nFolder, TRUE))
	      {
		DWORD attributes=0;
		TRACE_(shell)("Value=%s\n",szPath);
		hr = SHILCreateFromPathA(szPath, ppidl, &attributes);
	      }
	  }
	  if(*ppidl) hr = NOERROR;
	}

	TRACE_(shell)("-- (new pidl %p)\n",*ppidl);
	return hr;
}

/*************************************************************************
 * SHGetFolderLocation [SHELL32.@]
 *
 * NOTES
 *  the pidl can be a simple one. since we cant get the path out of the pidl
 *  we have to take all data from the pidl
 */
HRESULT WINAPI SHGetFolderLocation(
	HWND hwnd,
	int csidl,
	HANDLE hToken,
	DWORD dwFlags,
	LPITEMIDLIST *ppidl)
{
	FIXME("%p 0x%08x %p 0x%08lx %p\n",
	 hwnd, csidl, hToken, dwFlags, ppidl);
	return SHGetSpecialFolderLocation(hwnd, csidl, ppidl);
}

/*************************************************************************
 * SHGetDataFromIDListA [SHELL32.247]
 *
 * NOTES
 *  the pidl can be a simple one. since we cant get the path out of the pidl
 *  we have to take all data from the pidl
 */
HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len)
{
	TRACE_(shell)("sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);

	pdump(pidl);
	if (!psf || !dest )  return E_INVALIDARG;

	switch (nFormat)
	{
	  case SHGDFIL_FINDDATA:
	    {
	       WIN32_FIND_DATAA * pfd = dest;

	       if ( len < sizeof (WIN32_FIND_DATAA)) return E_INVALIDARG;

	       ZeroMemory(pfd, sizeof (WIN32_FIND_DATAA));
	       _ILGetFileDateTime( pidl, &(pfd->ftLastWriteTime));
	       pfd->dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);
	       pfd->nFileSizeLow = _ILGetFileSize ( pidl, NULL, 0);
	       lstrcpynA(pfd->cFileName,_ILGetTextPointer(pidl), MAX_PATH);
	       lstrcpynA(pfd->cAlternateFileName,_ILGetSTextPointer(pidl), 14);
	    }
	    return NOERROR;

	  case SHGDFIL_NETRESOURCE:
	  case SHGDFIL_DESCRIPTIONID:
	    FIXME_(shell)("SHGDFIL %i stub\n", nFormat);
	    break;

	  default:
	    ERR_(shell)("Unknown SHGDFIL %i, please report\n", nFormat);
	}

	return E_INVALIDARG;
}
/*************************************************************************
 * SHGetDataFromIDListW [SHELL32.248]
 *
 */
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len)
{
	TRACE_(shell)("sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);

	pdump(pidl);

	if (! psf || !dest )  return E_INVALIDARG;

	switch (nFormat)
	{
	  case SHGDFIL_FINDDATA:
	    {
	       WIN32_FIND_DATAW * pfd = dest;

	       if ( len < sizeof (WIN32_FIND_DATAW)) return E_INVALIDARG;

	       ZeroMemory(pfd, sizeof (WIN32_FIND_DATAA));
	       _ILGetFileDateTime( pidl, &(pfd->ftLastWriteTime));
	       pfd->dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);
	       pfd->nFileSizeLow = _ILGetFileSize ( pidl, NULL, 0);
               if (!MultiByteToWideChar( CP_ACP, 0, _ILGetTextPointer(pidl), -1,
                                         pfd->cFileName, MAX_PATH ))
                   pfd->cFileName[MAX_PATH-1] = 0;
               if (!MultiByteToWideChar( CP_ACP, 0, _ILGetSTextPointer(pidl), -1,
                                         pfd->cAlternateFileName, 14 ))
                   pfd->cFileName[13] = 0;
	    }
	    return NOERROR;
	  case SHGDFIL_NETRESOURCE:
	  case SHGDFIL_DESCRIPTIONID:
	    FIXME_(shell)("SHGDFIL %i stub\n", nFormat);
	    break;

	  default:
	    ERR_(shell)("Unknown SHGDFIL %i, please report\n", nFormat);
	}

	return E_INVALIDARG;
}

/*************************************************************************
 * SHGetPathFromIDListA		[SHELL32.@][NT 4.0: SHELL32.220]
 *
 * PARAMETERS
 *  pidl,   [IN] pidl
 *  pszPath [OUT] path
 *
 * RETURNS
 *  path from a passed PIDL.
 *
 * NOTES
 *	NULL returns FALSE
 *	desktop pidl gives path to desktopdirectory back
 *	special pidls returning FALSE
 */
BOOL WINAPI SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath)
{
	HRESULT hr;
	STRRET str;
	LPSHELLFOLDER shellfolder;

	TRACE_(shell)("(pidl=%p,%p)\n",pidl,pszPath);
	pdump(pidl);

	if (!pidl) return FALSE;

	hr = SHGetDesktopFolder(&shellfolder);
	if (SUCCEEDED (hr)) {
	    hr = IShellFolder_GetDisplayNameOf(shellfolder,pidl,SHGDN_FORPARSING,&str);
	    if(SUCCEEDED(hr)) {
	        StrRetToStrNA (pszPath, MAX_PATH, &str, pidl);
	    }
	    IShellFolder_Release(shellfolder);
	}

	TRACE_(shell)("-- %s, 0x%08lx\n",pszPath, hr);
	return SUCCEEDED(hr);
}
/*************************************************************************
 * SHGetPathFromIDListW 			[SHELL32.@]
 */
BOOL WINAPI SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath)
{
	HRESULT hr;
	STRRET str;
	LPSHELLFOLDER shellfolder;

	TRACE_(shell)("(pidl=%p,%p)\n", pidl, debugstr_w(pszPath));
	pdump(pidl);

	if (!pidl) return FALSE;

	hr = SHGetDesktopFolder(&shellfolder);
	if (SUCCEEDED(hr)) {
	    hr = IShellFolder_GetDisplayNameOf(shellfolder, pidl, SHGDN_FORPARSING, &str);
	    if (SUCCEEDED(hr)) {
	        StrRetToStrNW(pszPath, MAX_PATH, &str, pidl);
	    }
	    IShellFolder_Release(shellfolder);
	}

	TRACE_(shell)("-- %s, 0x%08lx\n",debugstr_w(pszPath), hr);
	return SUCCEEDED(hr);
}

/*************************************************************************
 *	SHBindToParent		[shell version 5.0]
 */
HRESULT WINAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast)
{
	IShellFolder	* psf;
	LPITEMIDLIST	pidlChild, pidlParent;
	HRESULT		hr=E_FAIL;

	TRACE_(shell)("pidl=%p\n", pidl);
	pdump(pidl);

	*ppv = NULL;
	if (ppidlLast) *ppidlLast = NULL;

	if (_ILIsPidlSimple(pidl))
	{
	  /* we are on desktop level */
	  if (ppidlLast)
	    *ppidlLast = ILClone(pidl);
	  hr = SHGetDesktopFolder((IShellFolder**)ppv);
	}
	else
	{
	  pidlChild =  ILClone(ILFindLastID(pidl));
	  pidlParent = ILClone(pidl);
	  ILRemoveLastID(pidlParent);

	  hr = SHGetDesktopFolder(&psf);

	  if (SUCCEEDED(hr))
	    hr = IShellFolder_BindToObject(psf, pidlParent, NULL, riid, ppv);

	  if (SUCCEEDED(hr) && ppidlLast)
	    *ppidlLast = pidlChild;
	  else
	    ILFree (pidlChild);

	  SHFree (pidlParent);
	  if (psf) IShellFolder_Release(psf);
	}


	TRACE_(shell)("-- psf=%p pidl=%p ret=0x%08lx\n", *ppv, (ppidlLast)?*ppidlLast:NULL, hr);
	return hr;
}

/*************************************************************************
 * SHGetPathFromIDList		[SHELL32.@][NT 4.0: SHELL32.219]
 */
BOOL WINAPI SHGetPathFromIDListAW(LPCITEMIDLIST pidl,LPVOID pszPath)
{
	TRACE_(shell)("(pidl=%p,%p)\n",pidl,pszPath);

	if (SHELL_OsIsUnicode())
	  return SHGetPathFromIDListW(pidl,pszPath);
	return SHGetPathFromIDListA(pidl,pszPath);
}

/**************************************************************************
 *
 *		internal functions
 *
 *	### 1. section creating pidls ###
 *
 *************************************************************************
 *  _ILCreateDesktop()
 *  _ILCreateIExplore()
 *  _ILCreateMyComputer()
 *  _ILCreateDrive()
 *  _ILCreateFolder()
 *  _ILCreateValue()
 */
LPITEMIDLIST _ILCreateDesktop()
{	TRACE("()\n");
	return _ILCreate(PT_DESKTOP, NULL, 0);
}

LPITEMIDLIST _ILCreateMyComputer()
{	TRACE("()\n");
	return _ILCreate(PT_MYCOMP, &CLSID_MyComputer, sizeof(GUID));
}

LPITEMIDLIST _ILCreateIExplore()
{	TRACE("()\n");
	return _ILCreate(PT_MYCOMP, &CLSID_Internet, sizeof(GUID));
}

LPITEMIDLIST _ILCreateControl()
{	TRACE("()\n");
	return _ILCreate(PT_SPECIAL, &CLSID_ControlPanel, sizeof(GUID));
}

LPITEMIDLIST _ILCreatePrinter()
{	TRACE("()\n");
	return _ILCreate(PT_SPECIAL, &CLSID_Printers, sizeof(GUID));
}

LPITEMIDLIST _ILCreateNetwork()
{	TRACE("()\n");
	return _ILCreate(PT_MYCOMP, &CLSID_NetworkPlaces, sizeof(GUID));
}

LPITEMIDLIST _ILCreateBitBucket()
{	TRACE("()\n");
	return _ILCreate(PT_MYCOMP, &CLSID_RecycleBin, sizeof(GUID));
}

LPITEMIDLIST _ILCreateDrive( LPCSTR lpszNew)
{	char sTemp[4];
	lstrcpynA (sTemp,lpszNew,4);
	sTemp[2]='\\';
	sTemp[3]=0x00;
	TRACE("(%s)\n",sTemp);
	return _ILCreate(PT_DRIVE,(LPVOID)&sTemp[0],4);
}

LPITEMIDLIST _ILCreateFolder( WIN32_FIND_DATAA * stffile )
{
	char	buff[MAX_PATH + 14 +1]; /* see WIN32_FIND_DATA */
	char *	pbuff = buff;
	ULONG	len, len1;
	LPITEMIDLIST pidl;

	TRACE("(%s, %s)\n",stffile->cAlternateFileName, stffile->cFileName);

	/* prepare buffer with both names */
	len = strlen (stffile->cFileName) + 1;
	memcpy (pbuff, stffile->cFileName, len);
	pbuff += len;

	if (stffile->cAlternateFileName)
	{
	  len1 = strlen (stffile->cAlternateFileName)+1;
	  memcpy (pbuff, stffile->cAlternateFileName, len1);
	}
	else
	{
	  len1 = 1;
	  *pbuff = 0x00;
	}

	pidl = _ILCreate(PT_FOLDER, (LPVOID)buff, len + len1);

	/* set attributes */
	if (pidl)
	{
	  LPPIDLDATA pData;
	  pData = _ILGetDataPointer(pidl);
	  FileTimeToDosDateTime(&(stffile->ftLastWriteTime),&pData->u.folder.uFileDate,&pData->u.folder.uFileTime);
	  pData->u.folder.dwFileSize = stffile->nFileSizeLow;
	  pData->u.folder.uFileAttribs = stffile->dwFileAttributes;
	}

	return pidl;
}

LPITEMIDLIST _ILCreateValue(WIN32_FIND_DATAA * stffile)
{
	char	buff[MAX_PATH + 14 +1]; /* see WIN32_FIND_DATA */
	char *	pbuff = buff;
	ULONG	len, len1;
	LPITEMIDLIST pidl;

	TRACE("(%s, %s)\n",stffile->cAlternateFileName, stffile->cFileName);

	/* prepare buffer with both names */
	len = strlen (stffile->cFileName) + 1;
	memcpy (pbuff, stffile->cFileName, len);
	pbuff += len;

	if (stffile->cAlternateFileName)
	{
	  len1 = strlen (stffile->cAlternateFileName)+1;
	  memcpy (pbuff, stffile->cAlternateFileName, len1);
	}
	else
	{
	  len1 = 1;
	  *pbuff = 0x00;
	}

	pidl = _ILCreate(PT_VALUE, (LPVOID)buff, len + len1);

	/* set attributes */
	if (pidl)
	{
	  LPPIDLDATA pData;
	  pData = _ILGetDataPointer(pidl);
	  FileTimeToDosDateTime(&(stffile->ftLastWriteTime),&pData->u.folder.uFileDate,&pData->u.folder.uFileTime);
	  pData->u.folder.dwFileSize = stffile->nFileSizeLow;
	  pData->u.folder.uFileAttribs=stffile->dwFileAttributes;
	}

	return pidl;
}

LPITEMIDLIST _ILCreateFromPathA(LPCSTR szPath)
{
	HANDLE hFile;
	WIN32_FIND_DATAA stffile;
	LPITEMIDLIST pidl = NULL;
	
	hFile = FindFirstFileA(szPath, &stffile);
	if (hFile != INVALID_HANDLE_VALUE) 
	{
	  if (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    pidl = _ILCreateFolder(&stffile);
	  else
	    pidl = _ILCreateValue(&stffile);
	  FindClose(hFile);
	}
	return pidl;
}

LPITEMIDLIST _ILCreateSpecial(LPCSTR szGUID)
{
	IID iid;

	if (!SUCCEEDED(SHCLSIDFromStringA(szGUID, &iid)))
	{
	  ERR("%s is not a GUID\n", szGUID);
	  return NULL;
	}
	return _ILCreate(PT_MYCOMP, &iid, sizeof(IID));
}

/**************************************************************************
 *  _ILCreate()
 *  Creates a new PIDL
 *  type = PT_DESKTOP | PT_DRIVE | PT_FOLDER | PT_VALUE
 *  pIn = data
 *  uInSize = size of data (raw)
 */

LPITEMIDLIST _ILCreate(PIDLTYPE type, LPCVOID pIn, UINT uInSize)
{
	LPITEMIDLIST   pidlOut = NULL, pidlTemp = NULL;
	LPPIDLDATA     pData;
	UINT           uSize = 0;
	LPSTR	pszDest;

	TRACE("(0x%02x %p %i)\n",type,pIn,uInSize);

	switch (type)
	{
	  case PT_DESKTOP:
	    uSize = 0;
	    break;
	  case PT_SPECIAL:
	  case PT_MYCOMP:
	    uSize = 2 + 2 + sizeof(GUID);
	    break;
	  case PT_DRIVE:
	    uSize = 2 + 23;
	    break;
	  case PT_FOLDER:
	  case PT_VALUE:
	    uSize = 2 + 12 + uInSize;
	    break;
	  default:
	    FIXME("can't create type: 0x%08x\n",type);
	    return NULL;
	}

	if(!(pidlOut = SHAlloc(uSize + 2))) return NULL;
	ZeroMemory(pidlOut, uSize + 2);
	pidlOut->mkid.cb = uSize;

	switch (type)
	{
	  case PT_DESKTOP:
	    TRACE("- create Desktop\n");
	    break;

	  case PT_SPECIAL:
	  case PT_MYCOMP:
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    memcpy(&(pData->u.mycomp.guid), pIn, uInSize);
	    TRACE("-- create GUID-pidl %s\n", debugstr_guid(&(pData->u.mycomp.guid)));
	    break;

	  case PT_DRIVE:
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    pszDest = _ILGetTextPointer(pidlOut);
	    memcpy(pszDest, pIn, uInSize);
	    TRACE("-- create Drive: %s\n",debugstr_a(pszDest));
	    break;

	  case PT_FOLDER:
	  case PT_VALUE:
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    pszDest =  _ILGetTextPointer(pidlOut);
	    memcpy(pszDest, pIn, uInSize);
	    TRACE("-- create Value: %s\n",debugstr_a(pszDest));
	    break;
	}

	pidlTemp = ILGetNext(pidlOut);
	if (pidlTemp)
	  pidlTemp->mkid.cb = 0x00;

	TRACE("-- (pidl=%p, size=%u)\n", pidlOut, uSize);
	return pidlOut;
}

/**************************************************************************
 *  _ILGetDrive()
 *
 *  Gets the text for the drive eg. 'c:\'
 *
 * RETURNS
 *  strlen (lpszText)
 */
DWORD _ILGetDrive(LPCITEMIDLIST pidl,LPSTR pOut, UINT uSize)
{	TRACE("(%p,%p,%u)\n",pidl,pOut,uSize);

	if(_ILIsMyComputer(pidl))
	  pidl = ILGetNext(pidl);

	if (pidl && _ILIsDrive(pidl))
	  return _ILSimpleGetText(pidl, pOut, uSize);

	return 0;
}

/**************************************************************************
 *
 *	### 2. section testing pidls ###
 *
 **************************************************************************
 *  _ILIsDesktop()
 *  _ILIsMyComputer()
 *  _ILIsSpecialFolder()
 *  _ILIsDrive()
 *  _ILIsFolder()
 *  _ILIsValue()
 *  _ILIsPidlSimple()
 */
BOOL _ILIsDesktop(LPCITEMIDLIST pidl)
{	TRACE("(%p)\n",pidl);
	return pidl && pidl->mkid.cb  ? 0 : 1;
}

BOOL _ILIsMyComputer(LPCITEMIDLIST pidl)
{
	REFIID iid = _ILGetGUIDPointer(pidl);

	TRACE("(%p)\n",pidl);

	if (iid)
	  return IsEqualIID(iid, &CLSID_MyComputer);
	return FALSE;
}

BOOL _ILIsSpecialFolder (LPCITEMIDLIST pidl)
{
	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE("(%p)\n",pidl);
	return (pidl && ( (lpPData && (PT_MYCOMP== lpPData->type || PT_SPECIAL== lpPData->type)) ||
			  (pidl && pidl->mkid.cb == 0x00)
			));
}

BOOL _ILIsDrive(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE("(%p)\n",pidl);
	return (pidl && lpPData && (PT_DRIVE == lpPData->type ||
				    PT_DRIVE1 == lpPData->type ||
				    PT_DRIVE2 == lpPData->type ||
				    PT_DRIVE3 == lpPData->type));
}

BOOL _ILIsFolder(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE("(%p)\n",pidl);
	return (pidl && lpPData && (PT_FOLDER == lpPData->type || PT_FOLDER1 == lpPData->type));
}

BOOL _ILIsValue(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE("(%p)\n",pidl);
	return (pidl && lpPData && PT_VALUE == lpPData->type);
}

/**************************************************************************
 *	_ILIsPidlSimple
 */
BOOL _ILIsPidlSimple ( LPCITEMIDLIST pidl)
{
	BOOL ret = TRUE;

	if(! _ILIsDesktop(pidl))	/* pidl=NULL or mkid.cb=0 */
	{
	  WORD len = pidl->mkid.cb;
	  LPCITEMIDLIST pidlnext = (LPCITEMIDLIST) (((LPBYTE)pidl) + len );
	  if (pidlnext->mkid.cb)
	    ret = FALSE;
	}

	TRACE("%s\n", ret ? "Yes" : "No");
	return ret;
}

/**************************************************************************
 *
 *	### 3. section getting values from pidls ###
 */

 /**************************************************************************
 *  _ILSimpleGetText
 *
 * gets the text for the first item in the pidl (eg. simple pidl)
 *
 * returns the length of the string
 */
DWORD _ILSimpleGetText (LPCITEMIDLIST pidl, LPSTR szOut, UINT uOutSize)
{
	DWORD		dwReturn=0;
	LPSTR		szSrc;
	GUID const * 	riid;
	char szTemp[MAX_PATH];

	TRACE("(%p %p %x)\n",pidl,szOut,uOutSize);

	if (!pidl) return 0;

	if (szOut)
	  *szOut = 0;

	if (_ILIsDesktop(pidl))
	{
	 /* desktop */
	  if (HCR_GetClassNameA(&CLSID_ShellDesktop, szTemp, MAX_PATH))
	  {
	    if (szOut)
	      lstrcpynA(szOut, szTemp, uOutSize);

	    dwReturn = strlen (szTemp);
	  }
	}
	else if (( szSrc = _ILGetTextPointer(pidl) ))
	{
	  /* filesystem */
	  if (szOut)
	    lstrcpynA(szOut, szSrc, uOutSize);

	  dwReturn = strlen(szSrc);
	}
	else if (( riid = _ILGetGUIDPointer(pidl) ))
	{
	  /* special folder */
	  if ( HCR_GetClassNameA(riid, szTemp, MAX_PATH) )
	  {
	    if (szOut)
	      lstrcpynA(szOut, szTemp, uOutSize);

	    dwReturn = strlen (szTemp);
	  }
	}
	else
	{
	  ERR("-- no text\n");
	}

	TRACE("-- (%p=%s 0x%08lx)\n",szOut,debugstr_a(szOut),dwReturn);
	return dwReturn;
}

/**************************************************************************
 *
 *	### 4. getting pointers to parts of pidls ###
 *
 **************************************************************************
 *  _ILGetDataPointer()
 */
LPPIDLDATA _ILGetDataPointer(LPITEMIDLIST pidl)
{
	if(pidl && pidl->mkid.cb != 0x00)
	  return (LPPIDLDATA) &(pidl->mkid.abID);
	return NULL;
}

/**************************************************************************
 *  _ILGetTextPointer()
 * gets a pointer to the long filename string stored in the pidl
 */
LPSTR _ILGetTextPointer(LPCITEMIDLIST pidl)
{/*	TRACE(pidl,"(pidl%p)\n", pidl);*/

	LPPIDLDATA pdata =_ILGetDataPointer(pidl);

	if (pdata)
	{
	  switch (pdata->type)
	  {
	    case PT_MYCOMP:
	    case PT_SPECIAL:
	      return NULL;

	    case PT_DRIVE:
	    case PT_DRIVE1:
	    case PT_DRIVE2:
	    case PT_DRIVE3:
	      return (LPSTR)&(pdata->u.drive.szDriveName);

	    case PT_FOLDER:
	    case PT_FOLDER1:
	    case PT_VALUE:
	    case PT_IESPECIAL1:
	    case PT_IESPECIAL2:
	      return (LPSTR)&(pdata->u.file.szNames);

	    case PT_WORKGRP:
	    case PT_COMP:
	    case PT_NETWORK:
	    case PT_SHARE:
	      return (LPSTR)&(pdata->u.network.szNames);
	  }
	}
	return NULL;
}

/**************************************************************************
 *  _ILGetSTextPointer()
 * gets a pointer to the short filename string stored in the pidl
 */
LPSTR _ILGetSTextPointer(LPCITEMIDLIST pidl)
{/*	TRACE(pidl,"(pidl%p)\n", pidl);*/

	LPPIDLDATA pdata =_ILGetDataPointer(pidl);

	if (pdata)
	{
	  switch (pdata->type)
	  {
	    case PT_FOLDER:
	    case PT_VALUE:
	    case PT_IESPECIAL1:
	    case PT_IESPECIAL2:
	      return (LPSTR)(pdata->u.file.szNames + strlen (pdata->u.file.szNames) + 1);

	    case PT_WORKGRP:
	      return (LPSTR)(pdata->u.network.szNames + strlen (pdata->u.network.szNames) + 1);
	  }
	}
	return NULL;
}

/**************************************************************************
 * _ILGetGUIDPointer()
 *
 * returns reference to guid stored in some pidls
 */
REFIID _ILGetGUIDPointer(LPCITEMIDLIST pidl)
{
	LPPIDLDATA pdata =_ILGetDataPointer(pidl);

	TRACE("%p\n", pidl);

	if (pdata)
	{
	  TRACE("pdata->type 0x%04x\n", pdata->type);
	  switch (pdata->type)
	  {
	    case PT_SPECIAL:
	    case PT_MYCOMP:
	      return (REFIID) &(pdata->u.mycomp.guid);

	    default:
		TRACE("Unknown pidl type 0x%04x\n", pdata->type);
		break;
	  }
	}
	return NULL;
}

/*************************************************************************
 * _ILGetFileDateTime
 *
 * Given the ItemIdList, get the FileTime
 *
 * PARAMS
 *      pidl        [I] The ItemIDList
 *      pFt         [I] the resulted FILETIME of the file
 *
 * RETURNS
 *     True if Successful
 *
 * NOTES
 *
 */
BOOL _ILGetFileDateTime(LPCITEMIDLIST pidl, FILETIME *pFt)
{
    LPPIDLDATA pdata =_ILGetDataPointer(pidl);

    if(! pdata) return FALSE;

    switch (pdata->type)
    {
        case PT_FOLDER:
            DosDateTimeToFileTime(pdata->u.folder.uFileDate, pdata->u.folder.uFileTime, pFt);
            break;
        case PT_VALUE:
            DosDateTimeToFileTime(pdata->u.file.uFileDate, pdata->u.file.uFileTime, pFt);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
	FILETIME ft,lft;
	SYSTEMTIME time;
	BOOL ret;

	if (_ILGetFileDateTime( pidl, &ft )) {
	    FileTimeToLocalFileTime(&ft, &lft);
	    FileTimeToSystemTime (&lft, &time);
	    ret = GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  pOut, uOutSize);
	} else {
	    pOut[0] = '\0';
	    ret = FALSE;
	}
	return ret;

}

/*************************************************************************
 * _ILGetFileSize
 *
 * Given the ItemIdList, get the FileSize
 *
 * PARAMS
 *      pidl    [I] The ItemIDList
 *	pOut	[I] The buffer to save the result
 *      uOutsize [I] The size of the buffer
 *
 * RETURNS
 *     The FileSize
 *
 * NOTES
 *	pOut can be null when no string is needed
 *
 */
DWORD _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
	LPPIDLDATA pdata =_ILGetDataPointer(pidl);
	DWORD dwSize;

	if(! pdata) return 0;

	switch (pdata->type)
	{
	  case PT_VALUE:
	    dwSize = pdata->u.file.dwFileSize;
	    if (pOut) StrFormatByteSizeA(dwSize, pOut, uOutSize);
	    return dwSize;
	}
	if (pOut) *pOut = 0x00;
	return 0;
}

BOOL _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
	char szTemp[MAX_PATH];
	const char * pPoint;
	LPITEMIDLIST  pidlTemp=pidl;

	TRACE("pidl=%p\n",pidl);

	if (!pidl) return FALSE;

	pidlTemp = ILFindLastID(pidl);

	if (!_ILIsValue(pidlTemp)) return FALSE;
	if (!_ILSimpleGetText(pidlTemp, szTemp, MAX_PATH)) return FALSE;

	pPoint = PathFindExtensionA(szTemp);

	if (! *pPoint) return FALSE;

	pPoint++;
	lstrcpynA(pOut, pPoint, uOutSize);
	TRACE("%s\n",pOut);

	return TRUE;
}

/*************************************************************************
 * _ILGetFileType
 *
 * Given the ItemIdList, get the file type description
 *
 * PARAMS
 *      pidl        [I] The ItemIDList (simple)
 *      pOut        [I] The buffer to save the result
 *      uOutsize    [I] The size of the buffer
 *
 * RETURNS
 *	nothing
 *
 * NOTES
 *	This function copies as much as possible into the buffer.
 */
void _ILGetFileType(LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
	if(_ILIsValue(pidl))
	{
	  char sTemp[64];
          if(uOutSize > 0)
          {
            pOut[0] = 0;
          }
	  if (_ILGetExtension (pidl, sTemp, 64))
	  {
	    if (!( HCR_MapTypeToValueA(sTemp, sTemp, 64, TRUE)
	        && HCR_MapTypeToValueA(sTemp, pOut, uOutSize, FALSE )))
	    {
	      lstrcpynA (pOut, sTemp, uOutSize - 6);
	      strcat (pOut, "-file");
	    }
	  }
	}
	else
	{
	  lstrcpynA(pOut, "Folder", uOutSize);
	}
}

/*************************************************************************
 * _ILGetFileAttributes
 *
 * Given the ItemIdList, get the Attrib string format
 *
 * PARAMS
 *      pidl        [I] The ItemIDList
 *      pOut        [I] The buffer to save the result
 *      uOutsize    [I] The size of the Buffer
 *
 * RETURNS
 *     Attributes
 *
 * FIXME
 *  return value 0 in case of error is a valid return value
 *
 */
DWORD _ILGetFileAttributes(LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
	LPPIDLDATA pData =_ILGetDataPointer(pidl);
	WORD wAttrib = 0;
	int i;

	if(! pData) return 0;

	switch(pData->type)
	{
	  case PT_FOLDER:
	    wAttrib = pData->u.folder.uFileAttribs;
	    break;
	  case PT_VALUE:
	    wAttrib = pData->u.file.uFileAttribs;
	    break;
	}

	if(uOutSize >= 6)
	{
	  i=0;
	  if(wAttrib & FILE_ATTRIBUTE_READONLY)
	  {
	    pOut[i++] = 'R';
	  }
	  if(wAttrib & FILE_ATTRIBUTE_HIDDEN)
	  {
	    pOut[i++] = 'H';
	  }
	  if(wAttrib & FILE_ATTRIBUTE_SYSTEM)
	  {
	    pOut[i++] = 'S';
	  }
	  if(wAttrib & FILE_ATTRIBUTE_ARCHIVE)
	  {
	    pOut[i++] = 'A';
	  }
	  if(wAttrib & FILE_ATTRIBUTE_COMPRESSED)
	  {
	    pOut[i++] = 'C';
	  }
	  pOut[i] = 0x00;
	}
	return wAttrib;
}

/*************************************************************************
* ILFreeaPidl
*
* free a aPidl struct
*/
void _ILFreeaPidl(LPITEMIDLIST * apidl, UINT cidl)
{
	UINT   i;

	if (apidl)
	{
	  for (i = 0; i < cidl; i++) SHFree(apidl[i]);
	  SHFree(apidl);
	}
}

/*************************************************************************
* ILCopyaPidl
*
* copies an aPidl struct
*/
LPITEMIDLIST *  _ILCopyaPidl(LPITEMIDLIST * apidlsrc, UINT cidl)
{
	UINT i;
	LPITEMIDLIST * apidldest = (LPITEMIDLIST*)SHAlloc(cidl * sizeof(LPITEMIDLIST));
	if(!apidlsrc) return NULL;

	for (i = 0; i < cidl; i++)
	  apidldest[i] = ILClone(apidlsrc[i]);

	return apidldest;
}

/*************************************************************************
* _ILCopyCidaToaPidl
*
* creates aPidl from CIDA
*/
LPITEMIDLIST * _ILCopyCidaToaPidl(LPITEMIDLIST* pidl, LPIDA cida)
{
	UINT i;
	LPITEMIDLIST * dst = (LPITEMIDLIST*)SHAlloc(cida->cidl * sizeof(LPITEMIDLIST));

	if(!dst) return NULL;

	if (pidl)
	  *pidl = ILClone((LPITEMIDLIST)(&((LPBYTE)cida)[cida->aoffset[0]]));

	for (i = 0; i < cida->cidl; i++)
	  dst[i] = ILClone((LPITEMIDLIST)(&((LPBYTE)cida)[cida->aoffset[i + 1]]));

	return dst;
}
