/*
 *	pidl Handling
 *
 *	Copyright 1998	Juergen Schmied
 *
 * NOTES
 *  a pidl == NULL means desktop and is legal
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "debugtools.h"
#include "shell.h"
#include "shlguid.h"
#include "winerror.h"
#include "winnls.h"
#include "winversion.h"
#include "shell32_main.h"

#include "pidl.h"
#include "wine/undocshell.h"

DECLARE_DEBUG_CHANNEL(pidl)
DECLARE_DEBUG_CHANNEL(shell)

void pdump (LPCITEMIDLIST pidl)
{	DWORD type;
	char * szData;
	char * szShortName;
	char szName[MAX_PATH];
	BOOL bIsShellDebug;
	
	LPITEMIDLIST pidltemp = pidl;
	if (!TRACE_ON(pidl))
	  return;

	/* silence the sub-functions */
	bIsShellDebug = TRACE_ON(shell);
	__SET_DEBUGGING(__DBCL_TRACE, dbch_shell, FALSE);
	__SET_DEBUGGING(__DBCL_TRACE, dbch_pidl, FALSE);

	if (! pidltemp)
	{
	  MESSAGE ("-------- pidl=NULL (Desktop)\n");
	}
	else
	{
	  MESSAGE ("-------- pidl=%p\n", pidl);
	  if (pidltemp->mkid.cb)
	  { 
	    do
	    {
	      type   = _ILGetDataPointer(pidltemp)->type;
	      szData = _ILGetTextPointer(type, _ILGetDataPointer(pidltemp));
	      szShortName = _ILGetSTextPointer(type, _ILGetDataPointer(pidltemp));
	      _ILSimpleGetText(pidltemp, szName, MAX_PATH);

	      MESSAGE ("-- pidl=%p size=%u type=%lx name=%s (%s,%s)\n",
	               pidltemp, pidltemp->mkid.cb,type,szName,debugstr_a(szData), debugstr_a(szShortName));

	      pidltemp = ILGetNext(pidltemp);

	    } while (pidltemp->mkid.cb);
	  }
	  else
	  {
	    MESSAGE ("empty pidl (Desktop)\n");
	  }
	}

	__SET_DEBUGGING(__DBCL_TRACE, dbch_shell, bIsShellDebug);
	__SET_DEBUGGING(__DBCL_TRACE, dbch_pidl, TRUE);

}
#define BYTES_PRINTED 32
BOOL pcheck (LPCITEMIDLIST pidl)
{       DWORD type, ret=TRUE;
	BOOL bIsPidlDebug;

        LPITEMIDLIST pidltemp = pidl;

	bIsPidlDebug = TRACE_ON(shell);
	__SET_DEBUGGING(__DBCL_TRACE, dbch_pidl, FALSE);

        if (pidltemp && pidltemp->mkid.cb)
        { do
          { type   = _ILGetDataPointer(pidltemp)->type;
            switch (type)
	    { case PT_DESKTOP:
	      case PT_MYCOMP:
	      case PT_SPECIAL:
	      case PT_DRIVE:
	      case PT_DRIVE1:
	      case PT_DRIVE2:
	      case PT_DRIVE3:
	      case PT_FOLDER:
	      case PT_VALUE:
	      case PT_FOLDER1:
	      case PT_WORKGRP:
	      case PT_COMP:
	      case PT_NETWORK:
	      case PT_SHARE:
	      case PT_IESPECIAL:
		break;
	      default:
	      {
		char szTemp[BYTES_PRINTED*4 + 1];
		int i;
		unsigned char c;

		memset(szTemp, ' ', BYTES_PRINTED*4 + 1);
		for ( i = 0; (i<pidltemp->mkid.cb) && (i<BYTES_PRINTED); i++)
		{
		  c = ((unsigned char *)pidltemp)[i];

		  szTemp[i*3+0] = ((c>>4)>9)? (c>>4)+55 : (c>>4)+48;
		  szTemp[i*3+1] = ((0x0F&c)>9)? (0x0F&c)+55 : (0x0F&c)+48;
		  szTemp[i*3+2] = ' ';
		  szTemp[i+BYTES_PRINTED*3]  =  (c>=0x20 && c <=0x80) ? c : '.';
		}
		szTemp[BYTES_PRINTED*4] = 0x00;
		ERR_(pidl)("unknown IDLIST type size=%u type=%lx\n%s\n",pidltemp->mkid.cb,type, szTemp);
		ret = FALSE;
	      }
	    }
	    pidltemp = ILGetNext(pidltemp);
	  } while (pidltemp->mkid.cb);
	}
	__SET_DEBUGGING(__DBCL_TRACE, dbch_pidl, bIsPidlDebug);
	return ret;
}

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

	TRACE_(pidl)("(pidl=%p)\n",pidl);

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
 *    dupicate an idlist
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

  TRACE_(pidl)("pidl=%p newpidl=%p\n",pidl, newpidl);
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
	
	TRACE_(pidl)("pidl=%p \n",pidl);
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
	TRACE_(pidl)("-- newpidl=%p\n",pidlNew);

	return pidlNew;
}
/*************************************************************************
 * ILLoadFromStream
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
	
	/* we are not jet fully compatible */
	if (!pcheck(*ppPidl))
	{ SHFree(*ppPidl);
	  *ppPidl = NULL;
	}
	

	IStream_Release (pStream);

	return ret;
}
/*************************************************************************
 * SHILCreateFromPath	[SHELL32.28]
 *
 * NOTES
 *   wraper for IShellFolder::ParseDisplayName()
 */
HRESULT WINAPI SHILCreateFromPathA (LPCSTR path, LPITEMIDLIST * ppidl, DWORD * attributes)
{	LPSHELLFOLDER sf;
	WCHAR lpszDisplayName[MAX_PATH];
	DWORD pchEaten;
	HRESULT ret = E_FAIL;
	
	TRACE_(shell)("%s %p 0x%08lx\n",path,ppidl,attributes?*attributes:0);

	LocalToWideChar(lpszDisplayName, path, MAX_PATH);

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
	  ret = IShellFolder_ParseDisplayName(sf,0, NULL, path, &pchEaten, ppidl, attributes);
	  IShellFolder_Release(sf);
	}
	return ret;
}
HRESULT WINAPI SHILCreateFromPathAW (LPCVOID path, LPITEMIDLIST * ppidl, DWORD * attributes)
{
	if ( VERSION_OsIsUnicode())
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
	WARN_(shell)("(hwnd=0x%x,csidl=0x%lx,0x%lx):semi-stub.\n",
					 hwndOwner,nFolder,x3);

	SHGetSpecialFolderLocation(hwndOwner, nFolder, &ppidl);

	return ppidl;
}

/*************************************************************************
 * ILGlobalClone [SHELL32.97]
 *
 */
LPITEMIDLIST WINAPI ILGlobalClone(LPCITEMIDLIST pidl)
{	DWORD    len;
	LPITEMIDLIST  newpidl;

	if (!pidl)
	  return NULL;
    
	len = ILGetSize(pidl);
	newpidl = (LPITEMIDLIST)pCOMCTL32_Alloc(len);
	if (newpidl)
	  memcpy(newpidl,pidl,len);

	TRACE_(pidl)("pidl=%p newpidl=%p\n",pidl, newpidl);
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

	TRACE_(pidl)("pidl1=%p pidl2=%p\n",pidl1, pidl2);

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
	
	TRACE_(pidl)("%p %p %x\n", pidlParent, pidlChild, bImmediate);

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

	TRACE_(pidl)("pidl1=%p pidl2=%p\n",pidl1, pidl2);

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
	
	TRACE_(pidl)("pidl=%p pidl=%p\n",pidl1,pidl2);

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
	FIXME_(pidl)("sf=%p pidl=%p 0x%04lx\n",lpsf,pidl,z);

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
	FIXME_(pidl)("(pidl=%p)\n",pidl);

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
	TRACE_(pidl)("pidl=%p size=%lu\n",pidl, len);
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
	
	TRACE_(pidl)("(pidl=%p)\n",pidl);

	if(pidl)
	{
	  len =  pidl->mkid.cb;
	  if (len)
	  {
	    pidl = (LPITEMIDLIST) (((LPBYTE)pidl)+len);
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

	WARN_(pidl)("(pidl=%p,pidl=%p,%08u)semi-stub\n",pidl,item,bEnd);

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
{	TRACE_(pidl)("(pidl=0x%08lx)\n",(DWORD)pidl);

	if (!pidl)
	  return FALSE;

	return SHFree(pidl);
}
/*************************************************************************
 * ILGlobalFree [SHELL32.156]
 *
 */
DWORD WINAPI ILGlobalFree( LPITEMIDLIST pidl)
{
	TRACE_(pidl)("%p\n",pidl);

	if (!pidl)
	  return FALSE;

	return pCOMCTL32_Free (pidl);
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
	if ( VERSION_OsIsUnicode())
	  return ILCreateFromPathW (path);
	return ILCreateFromPathA (path);
}
/*************************************************************************
 *  SHSimpleIDListFromPath [SHELL32.162]
 */
LPITEMIDLIST WINAPI SHSimpleIDListFromPathA (LPSTR lpszPath)
{
	LPITEMIDLIST	pidl=NULL;
	HANDLE	hFile;
	WIN32_FIND_DATAA	stffile;

	TRACE_(pidl)("path=%s\n", lpszPath);

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
LPITEMIDLIST WINAPI SHSimpleIDListFromPathW (LPWSTR lpszPath)
{
	char	lpszTemp[MAX_PATH];
	TRACE_(pidl)("path=L%s\n",debugstr_w(lpszPath));

	WideCharToLocal(lpszTemp, lpszPath, MAX_PATH);

	return SHSimpleIDListFromPathA (lpszTemp);
}

LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW (LPVOID lpszPath)
{
	if ( VERSION_OsIsUnicode())
	  return SHSimpleIDListFromPathW (lpszPath);
	return SHSimpleIDListFromPathA (lpszPath);
}

/*************************************************************************
 * SHGetSpecialFolderLocation		[SHELL32.223]
 *
 * gets the folder locations from the registry and creates a pidl
 * creates missing reg keys and directorys
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
	CHAR		szPath[256];
	HRESULT		hr = E_INVALIDARG;

	TRACE_(shell)("(%04x,0x%x,%p)\n", hwndOwner,nFolder,ppidl);

	*ppidl = NULL;

	if (ppidl)
	{
	  switch (nFolder)
	  {
	    case CSIDL_DESKTOP:
	      *ppidl = _ILCreateDesktop();
	      hr = NOERROR;
	      break;

	    case CSIDL_DRIVES:
	      *ppidl = _ILCreateMyComputer();
	      hr = NOERROR;
	      break;

	    default:
	      if (SHGetSpecialFolderPathA(hwndOwner, szPath, nFolder, TRUE))
	      {
		DWORD attributes=0;
		TRACE_(shell)("Value=%s\n",szPath);
		hr = SHILCreateFromPathA(szPath, ppidl, &attributes);
	      }
	  }
	}

	TRACE_(shell)("-- (new pidl %p)\n",*ppidl);
	return hr;
}

/*************************************************************************
 * SHGetDataFromIDListA [SHELL32.247]
 *
 */
HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len)
{
	TRACE_(shell)("sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);
	
	if (! psf || !dest )  return E_INVALIDARG;

	switch (nFormat)
	{
	  case SHGDFIL_FINDDATA:
	    {
	       WIN32_FIND_DATAA * pfd = dest;
	       CHAR	pszPath[MAX_PATH];
	       HANDLE	handle;

	       if ( len < sizeof (WIN32_FIND_DATAA))
	         return E_INVALIDARG;

	       SHGetPathFromIDListA(pidl, pszPath);

	       if ((handle  = FindFirstFileA ( pszPath, pfd)))
	         FindClose (handle);
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
 * SHGetDataFromIDListW [SHELL32.247]
 *
 */
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len)
{
	FIXME_(shell)("sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);

	return SHGetDataFromIDListA( psf, pidl, nFormat, dest, len);
}

/*************************************************************************
 * SHGetPathFromIDListA		[SHELL32.261][NT 4.0: SHELL32.220]
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
 *
 * FIXME
 *  fnGetDisplayNameOf can return different types of OLEString
 */
BOOL WINAPI SHGetPathFromIDListA (LPCITEMIDLIST pidl,LPSTR pszPath)
{	STRRET str;
	LPSHELLFOLDER shellfolder;

	TRACE_(shell)("(pidl=%p,%p)\n",pidl,pszPath);

	if (!pidl) return FALSE;

	pdump(pidl);

	if(_ILIsDesktop(pidl))
	{
	   SHGetSpecialFolderPathA(0, pszPath, CSIDL_DESKTOPDIRECTORY, FALSE);	
	}
	else
	{
	  if (SHGetDesktopFolder(&shellfolder)==S_OK)
	  {
	    IShellFolder_GetDisplayNameOf(shellfolder,pidl,SHGDN_FORPARSING,&str);
	    StrRetToStrNA (pszPath, MAX_PATH, &str, pidl);
	    IShellFolder_Release(shellfolder);
	  }
	}
	TRACE_(shell)("-- (%s)\n",pszPath);

	return TRUE;
}
/*************************************************************************
 * SHGetPathFromIDListW 			[SHELL32.262]
 */
BOOL WINAPI SHGetPathFromIDListW (LPCITEMIDLIST pidl,LPWSTR pszPath)
{	char sTemp[MAX_PATH];

	TRACE_(shell)("(pidl=%p)\n", pidl);

	SHGetPathFromIDListA (pidl, sTemp);
	lstrcpyAtoW(pszPath, sTemp);

	TRACE_(shell)("-- (%s)\n",debugstr_w(pszPath));

	return TRUE;
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
	}


	TRACE_(shell)("-- psf=%p pidl=%p ret=0x%08lx\n", *ppv, (ppidlLast)?*ppidlLast:NULL, hr);
	return hr;
}

/*************************************************************************
 * SHGetPathFromIDListAW		[SHELL32.221][NT 4.0: SHELL32.219]
 */
BOOL WINAPI SHGetPathFromIDListAW(LPCITEMIDLIST pidl,LPVOID pszPath)
{
	TRACE_(shell)("(pidl=%p,%p)\n",pidl,pszPath);

	if (VERSION_OsIsUnicode())
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
LPITEMIDLIST WINAPI _ILCreateDesktop()
{	TRACE_(pidl)("()\n");
	return _ILCreate(PT_DESKTOP, NULL, 0);
}

LPITEMIDLIST WINAPI _ILCreateMyComputer()
{	TRACE_(pidl)("()\n");
	return _ILCreate(PT_MYCOMP, &IID_MyComputer, sizeof(GUID));
}

LPITEMIDLIST WINAPI _ILCreateIExplore()
{	TRACE_(pidl)("()\n");
	return _ILCreate(PT_MYCOMP, &IID_IExplore, sizeof(GUID));
}

LPITEMIDLIST WINAPI _ILCreateDrive( LPCSTR lpszNew)
{	char sTemp[4];
	lstrcpynA (sTemp,lpszNew,4);
	sTemp[2]='\\';
	sTemp[3]=0x00;
	TRACE_(pidl)("(%s)\n",sTemp);
	return _ILCreate(PT_DRIVE,(LPVOID)&sTemp[0],4);
}

LPITEMIDLIST WINAPI _ILCreateFolder( WIN32_FIND_DATAA * stffile )
{
	char	buff[MAX_PATH + 14 +1]; /* see WIN32_FIND_DATA */
	char *	pbuff = buff;
	ULONG	len, len1;
	LPITEMIDLIST pidl;
	
	TRACE_(pidl)("(%s, %s)\n",stffile->cAlternateFileName, stffile->cFileName);

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
	  pData->u.folder.uFileAttribs=stffile->dwFileAttributes;
	}

	return pidl;
}

LPITEMIDLIST WINAPI _ILCreateValue(WIN32_FIND_DATAA * stffile)
{
	char	buff[MAX_PATH + 14 +1]; /* see WIN32_FIND_DATA */
	char *	pbuff = buff;
	ULONG	len, len1;
	LPITEMIDLIST pidl;
	
	TRACE_(pidl)("(%s, %s)\n",stffile->cAlternateFileName, stffile->cFileName);

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

LPITEMIDLIST WINAPI _ILCreateSpecial(LPCSTR szGUID)
{
	IID	iid;
	CLSIDFromString16(szGUID,&iid);
	return _ILCreate(PT_MYCOMP, &iid, sizeof(IID));
}

/**************************************************************************
 *  _ILCreate()
 *  Creates a new PIDL
 *  type = PT_DESKTOP | PT_DRIVE | PT_FOLDER | PT_VALUE
 *  pIn = data
 *  uInSize = size of data (raw)
 */

LPITEMIDLIST WINAPI _ILCreate(PIDLTYPE type, LPCVOID pIn, UINT16 uInSize)
{	LPITEMIDLIST   pidlOut = NULL, pidlTemp = NULL;
	LPPIDLDATA     pData;
	UINT16         uSize = 0;
	LPSTR	pszDest;
	
	TRACE_(pidl)("(0x%02x %p %i)\n",type,pIn,uInSize);

	switch (type)
	{ case PT_DESKTOP:
	    uSize = 0;
	    pidlOut = SHAlloc(uSize + 2);
	    pidlOut->mkid.cb = uSize;
	    TRACE_(pidl)("- create Desktop\n");
	    break;

	  case PT_MYCOMP:
	    uSize = 2 + 2 + sizeof(GUID);
	    pidlOut = SHAlloc(uSize + 2);
	    ZeroMemory(pidlOut, uSize + 2);
	    pidlOut->mkid.cb = uSize;
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    memcpy(&(pData->u.mycomp.guid), pIn, uInSize);
	    TRACE_(pidl)("- create GUID-pidl\n");
	    break;

	  case PT_DRIVE:
	    uSize = 2 + 23;
	    pidlOut = SHAlloc(uSize + 2);
	    ZeroMemory(pidlOut, uSize + 2);
	    pidlOut->mkid.cb = uSize;
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    pszDest =  _ILGetTextPointer(type, pData);
	    memcpy(pszDest, pIn, uInSize);
	    TRACE_(pidl)("- create Drive: %s\n",debugstr_a(pszDest));
	    break;

	  case PT_FOLDER:
	  case PT_VALUE:   
	    uSize = 2 + 12 + uInSize;
	    pidlOut = SHAlloc(uSize + 2);
	    ZeroMemory(pidlOut, uSize + 2);
	    pidlOut->mkid.cb = uSize;
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    pszDest =  _ILGetTextPointer(type, pData);
	    memcpy(pszDest, pIn, uInSize);
	    TRACE_(pidl)("- create Value: %s\n",debugstr_a(pszDest));
	    break;
	}
	
	pidlTemp = ILGetNext(pidlOut);
	if (pidlTemp)
	  pidlTemp->mkid.cb = 0x00;

	TRACE_(pidl)("-- (pidl=%p, size=%u)\n", pidlOut, uSize);
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
DWORD WINAPI _ILGetDrive(LPCITEMIDLIST pidl,LPSTR pOut, UINT16 uSize)
{	TRACE_(pidl)("(%p,%p,%u)\n",pidl,pOut,uSize);

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
BOOL WINAPI _ILIsDesktop(LPCITEMIDLIST pidl)
{	TRACE_(pidl)("(%p)\n",pidl);
	return ( !pidl || (pidl && pidl->mkid.cb == 0x00) );
}

BOOL WINAPI _ILIsMyComputer(LPCITEMIDLIST pidl)
{
	REFIID iid = _ILGetGUIDPointer(pidl);

	TRACE_(pidl)("(%p)\n",pidl);

	if (iid)
	  return IsEqualIID(iid, &IID_MyComputer);
	return FALSE;
}

BOOL WINAPI _ILIsSpecialFolder (LPCITEMIDLIST pidl)
{
	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE_(pidl)("(%p)\n",pidl);
	return (pidl && ( (lpPData && (PT_MYCOMP== lpPData->type || PT_SPECIAL== lpPData->type)) || 
			  (pidl && pidl->mkid.cb == 0x00)
			));
}

BOOL WINAPI _ILIsDrive(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE_(pidl)("(%p)\n",pidl);
	return (pidl && lpPData && (PT_DRIVE == lpPData->type ||
				    PT_DRIVE1 == lpPData->type ||
				    PT_DRIVE2 == lpPData->type ||
				    PT_DRIVE3 == lpPData->type));
}

BOOL WINAPI _ILIsFolder(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE_(pidl)("(%p)\n",pidl);
	return (pidl && lpPData && (PT_FOLDER == lpPData->type || PT_FOLDER1 == lpPData->type));
}

BOOL WINAPI _ILIsValue(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE_(pidl)("(%p)\n",pidl);
	return (pidl && lpPData && PT_VALUE == lpPData->type);
}

/**************************************************************************
 *	_ILIsPidlSimple
 */
BOOL WINAPI _ILIsPidlSimple ( LPCITEMIDLIST pidl)
{
	BOOL ret = TRUE;

	if(! _ILIsDesktop(pidl))	/* pidl=NULL or mkid.cb=0 */
	{
	  WORD len = pidl->mkid.cb;
	  LPCITEMIDLIST pidlnext = (LPCITEMIDLIST) (((LPBYTE)pidl) + len );
	  if (pidlnext->mkid.cb)
	    ret = FALSE;
	}

	TRACE_(pidl)("%s\n", ret ? "Yes" : "No");
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
 * returns the lenght of the string
 */
DWORD WINAPI _ILSimpleGetText (LPCITEMIDLIST pidl, LPSTR szOut, UINT uOutSize)
{
	LPPIDLDATA	pData;
	DWORD		dwReturn=0; 
	LPSTR		szSrc;
	GUID const * 	riid;
	char szTemp[MAX_PATH];
	
	TRACE_(pidl)("(%p %p %x)\n",pidl,szOut,uOutSize);
	
	if (!pidl) return 0;

	if (szOut)
	  *szOut = 0;

	pData = _ILGetDataPointer(pidl);

	if (!pData)					
	{
	 /* desktop */
	  if (HCR_GetClassName(&CLSID_ShellDesktop, szTemp, MAX_PATH))
	  {
	    if (szOut)
	      lstrcpynA(szOut, szTemp, uOutSize);

	    dwReturn = strlen (szTemp);
	  }
	}
	else if (( szSrc = _ILGetTextPointer(pData->type, pData) ))
	{
	  /* filesystem */
	  if (szOut)
	    lstrcpynA(szOut, szSrc, MAX_PATH);

	  dwReturn = strlen(szSrc);
	}
	else if (( riid = _ILGetGUIDPointer(pidl) ))
	{
	  /* special folder */
	  if ( HCR_GetClassName(riid, szTemp, MAX_PATH) )
	  {
	    if (szOut)
	      lstrcpynA(szOut, szTemp, uOutSize);

	    dwReturn = strlen (szTemp);
	  }
	}
	else
	{
	  ERR_(pidl)("-- no text\n");
	}

	TRACE_(pidl)("-- (%p=%s 0x%08lx)\n",szOut,(char*)szOut,dwReturn);
	return dwReturn;
}

/**************************************************************************
 *
 *	### 4. getting pointers to parts of pidls ###
 *
 **************************************************************************
 *  _ILGetDataPointer()
 */
LPPIDLDATA WINAPI _ILGetDataPointer(LPITEMIDLIST pidl)
{
	if(pidl && pidl->mkid.cb != 0x00)
	  return (LPPIDLDATA) &(pidl->mkid.abID);
	return NULL;
}

/**************************************************************************
 *  _ILGetTextPointer()
 * gets a pointer to the long filename string stored in the pidl
 */
LPSTR WINAPI _ILGetTextPointer(PIDLTYPE type, LPPIDLDATA pidldata)
{/*	TRACE(pidl,"(type=%x data=%p)\n", type, pidldata);*/

	if(!pidldata)
	{ return NULL;
	}

	switch (type)
	{
	  case PT_MYCOMP:
	  case PT_SPECIAL:
	    return NULL;

	  case PT_DRIVE:
	  case PT_DRIVE1:
	  case PT_DRIVE2:
	  case PT_DRIVE3:
	    return (LPSTR)&(pidldata->u.drive.szDriveName);

	  case PT_FOLDER:
	  case PT_FOLDER1:
	  case PT_VALUE:
	  case PT_IESPECIAL:
	    return (LPSTR)&(pidldata->u.file.szNames);

	  case PT_WORKGRP:
	  case PT_COMP:
	  case PT_NETWORK:
	  case PT_SHARE:
	    return (LPSTR)&(pidldata->u.network.szNames);
	}
	return NULL;
}

/**************************************************************************
 *  _ILGetSTextPointer()
 * gets a pointer to the short filename string stored in the pidl
 */
LPSTR WINAPI _ILGetSTextPointer(PIDLTYPE type, LPPIDLDATA pidldata)
{/*	TRACE(pidl,"(type=%x data=%p)\n", type, pidldata);*/

	if(!pidldata)
	  return NULL;

	switch (type)
	{
	  case PT_FOLDER:
	  case PT_VALUE:
	  case PT_IESPECIAL:
	    return (LPSTR)(pidldata->u.file.szNames + strlen (pidldata->u.file.szNames) + 1);

	  case PT_WORKGRP:
	    return (LPSTR)(pidldata->u.network.szNames + strlen (pidldata->u.network.szNames) + 1);
	}
	return NULL;
}

/**************************************************************************
 * _ILGetGUIDPointer()
 *
 * returns reference to guid stored in some pidls
 */
REFIID WINAPI _ILGetGUIDPointer(LPCITEMIDLIST pidl)
{
	LPPIDLDATA pdata =_ILGetDataPointer(pidl);

	if (pdata)
	{
	  switch (pdata->type)
	  {
	    case PT_SPECIAL:
	    case PT_MYCOMP:
	      return (REFIID) &(pdata->u.mycomp.guid);
	  }
	}
	return NULL;
}

BOOL WINAPI _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{	LPPIDLDATA pdata =_ILGetDataPointer(pidl);
	FILETIME ft;
	SYSTEMTIME time;

	switch (pdata->type)
	{ case PT_FOLDER:
	    DosDateTimeToFileTime(pdata->u.folder.uFileDate, pdata->u.folder.uFileTime, &ft);
	    break;	    
	  case PT_VALUE:
	    DosDateTimeToFileTime(pdata->u.file.uFileDate, pdata->u.file.uFileTime, &ft);
	    break;
	  default:
	    return FALSE;
	}
	FileTimeToSystemTime (&ft, &time);
	return GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  pOut, uOutSize);
}

BOOL WINAPI _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{	LPPIDLDATA pdata =_ILGetDataPointer(pidl);
	
	switch (pdata->type)
	{ case PT_VALUE:
	    break;
	  default:
	    return FALSE;
	}
	StrFormatByteSizeA(pdata->u.file.dwFileSize, pOut, uOutSize);
	return TRUE;
}

BOOL WINAPI _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
	char szTemp[MAX_PATH];
	const char * pPoint;
	LPITEMIDLIST  pidlTemp=pidl;
	
	TRACE_(pidl)("pidl=%p\n",pidl);

	if (!pidl) return FALSE;
	
	pidlTemp = ILFindLastID(pidl);
	
	if (!_ILIsValue(pidlTemp)) return FALSE;
	if (!_ILSimpleGetText(pidlTemp, szTemp, MAX_PATH)) return FALSE;

	pPoint = PathFindExtensionA(szTemp);

	if (! *pPoint) return FALSE;

	pPoint++;
	lstrcpynA(pOut, pPoint, uOutSize);
	TRACE_(pidl)("%s\n",pOut);

	return TRUE;
}

