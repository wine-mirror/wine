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

DECLARE_DEBUG_CHANNEL(pidl)
DECLARE_DEBUG_CHANNEL(shell)

static char * szMyComp = "My Computer";	/* for comparing */
static char * szNetHood = "Network Neighbourhood";	/* for comparing */

void pdump (LPCITEMIDLIST pidl)
{	DWORD type;
	CHAR * szData;
	CHAR * szShortName;
	LPITEMIDLIST pidltemp = pidl;
	if (! pidltemp)
	{ TRACE_(pidl)("-------- pidl = NULL (Root)\n");
	  return;
	}
	TRACE_(pidl)("-------- pidl=%p \n", pidl);
	if (pidltemp->mkid.cb)
	{ do
	  { type   = _ILGetDataPointer(pidltemp)->type;
	    szData = _ILGetTextPointer(type, _ILGetDataPointer(pidltemp));
	    szShortName = _ILGetSTextPointer(type, _ILGetDataPointer(pidltemp));

	    TRACE_(pidl)("---- pidl=%p size=%u type=%lx %s, (%s)\n",
	               pidltemp, pidltemp->mkid.cb,type,debugstr_a(szData), debugstr_a(szShortName));

	    pidltemp = ILGetNext(pidltemp);
	  } while (pidltemp->mkid.cb);
	  return;
	}
	else
	  TRACE_(pidl)("empty pidl (Desktop)\n");	
}
#define BYTES_PRINTED 32
BOOL pcheck (LPCITEMIDLIST pidl)
{       DWORD type, ret=TRUE;

        LPITEMIDLIST pidltemp = pidl;

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
	return ret;
}

/*************************************************************************
 * ILGetDisplayName			[SHELL32.15]
 */
BOOL WINAPI ILGetDisplayName(LPCITEMIDLIST pidl,LPSTR path)
{	TRACE_(shell)("pidl=%p %p semi-stub\n",pidl,path);
	return SHGetPathFromIDListA(pidl, path);
}
/*************************************************************************
 * ILFindLastID [SHELL32.16]
 */
LPITEMIDLIST WINAPI ILFindLastID(LPITEMIDLIST pidl) 
{	LPITEMIDLIST   pidlLast = NULL;

	TRACE_(pidl)("(pidl=%p)\n",pidl);

	while (pidl->mkid.cb)
	{ pidlLast = pidl;
	  pidl = ILGetNext(pidl);
	}
	return pidlLast;		
}
/*************************************************************************
 * ILRemoveLastID [SHELL32.17]
 * NOTES
 *  Removes the last item 
 */
BOOL WINAPI ILRemoveLastID(LPCITEMIDLIST pidl)
{	TRACE_(shell)("pidl=%p\n",pidl);
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
	LPITEMIDLIST newpidl=NULL;
	
	TRACE_(pidl)("pidl=%p \n",pidl);
	pdump(pidl);
	
	if (pidl)
	{ len = pidl->mkid.cb;	
	  newpidl = (LPITEMIDLIST) SHAlloc (len+2);
	  if (newpidl)
	  { memcpy(newpidl,pidl,len);
	    ILGetNext(newpidl)->mkid.cb = 0x00;
	  }
	}
	TRACE_(pidl)("-- newpidl=%p\n",newpidl);

  	return newpidl;
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
HRESULT WINAPI SHILCreateFromPathA (LPSTR path, LPITEMIDLIST * ppidl, DWORD attributes)
{	LPSHELLFOLDER sf;
	WCHAR lpszDisplayName[MAX_PATH];
	DWORD pchEaten;
	HRESULT ret = E_FAIL;
	
	TRACE_(shell)("%s %p 0x%08lx\n",path,ppidl,attributes);

	LocalToWideChar(lpszDisplayName, path, MAX_PATH);

	if (SUCCEEDED (SHGetDesktopFolder(&sf)))
	{ ret = sf->lpvtbl->fnParseDisplayName(sf,0, NULL,lpszDisplayName,&pchEaten,ppidl,&attributes);
	  sf->lpvtbl->fnRelease(sf);
	}
	return ret; 	
}
HRESULT WINAPI SHILCreateFromPathW (LPWSTR path, LPITEMIDLIST * ppidl, DWORD attributes)
{	LPSHELLFOLDER sf;
	DWORD pchEaten;
	HRESULT ret = E_FAIL;
	
	TRACE_(shell)("%s %p 0x%08lx\n",debugstr_w(path),ppidl,attributes);

	if (SUCCEEDED (SHGetDesktopFolder(&sf)))
	{ ret = sf->lpvtbl->fnParseDisplayName(sf,0, NULL, path, &pchEaten, ppidl, &attributes);
	  sf->lpvtbl->fnRelease(sf);
	}
	return ret;
}
HRESULT WINAPI SHILCreateFromPathAW (LPVOID path, LPITEMIDLIST * ppidl, DWORD attributes)
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
{	LPPIDLDATA ppidldata;
	CHAR * szData1;
	CHAR * szData2;

	LPITEMIDLIST pidltemp1 = pidl1;
	LPITEMIDLIST pidltemp2 = pidl2;

	TRACE_(pidl)("pidl1=%p pidl2=%p\n",pidl1, pidl2);

	/* explorer reads from registry directly (StreamMRU),
	   so we can only check here */
	if ((!pcheck (pidl1)) || (!pcheck (pidl2)))
	  return FALSE;

	pdump (pidl1);
	pdump (pidl2);

	if ( (!pidl1) || (!pidl2) )
	{ return FALSE;
	}
	
	if (pidltemp1->mkid.cb && pidltemp2->mkid.cb)
	{ do
	  { ppidldata = _ILGetDataPointer(pidltemp1);
	    szData1 = _ILGetTextPointer(ppidldata->type, ppidldata);
	    
	    ppidldata = _ILGetDataPointer(pidltemp2);    
	    szData2 = _ILGetTextPointer(ppidldata->type, ppidldata);

	    if (!szData1 || !szData2)
	    { FIXME_(pidl)("Failure getting text pointer");
	      return FALSE;
	    }
	    
	    if (strcmp ( szData1, szData2 )!=0 )
	      return FALSE;

	    pidltemp1 = ILGetNext(pidltemp1);
	    pidltemp2 = ILGetNext(pidltemp2);

	  } while (pidltemp1->mkid.cb && pidltemp2->mkid.cb);
	}	
 	if (!pidltemp1 && !pidltemp2)
	{ TRACE_(shell)("--- equal\n");
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
	LPPIDLDATA ppidldata;
	CHAR * szData1;
	CHAR * szData2;

	LPITEMIDLIST pParent = pidlParent;
	LPITEMIDLIST pChild = pidlChild;
	
	TRACE_(pidl)("%p %p %x\n", pidlParent, pidlChild, bImmediate);

	if (pParent->mkid.cb && pChild->mkid.cb)
	{ do
	  { ppidldata = _ILGetDataPointer(pParent);
	    szData1 = _ILGetTextPointer(ppidldata->type, ppidldata);
	    
	    ppidldata = _ILGetDataPointer(pChild);    
	    szData2 = _ILGetTextPointer(ppidldata->type, ppidldata);

	    if (!szData1 || !szData2)
	    { FIXME_(pidl)("Failure getting text pointer");
	      return FALSE;
	    }
	    
	    if (strcmp ( szData1, szData2 )!=0 )
	      return FALSE;

	    pParent = ILGetNext(pParent);
	    pChild = ILGetNext(pChild);

	  } while (pParent->mkid.cb && pChild->mkid.cb);
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
 *  When at least the first element is equal, it gives a pointer
 *  to the first different element of pidl 2 back.
 *  Returns 0 if pidl 2 is shorter.
 */
LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2)
{	LPPIDLDATA ppidldata;
	CHAR * szData1;
	CHAR * szData2;

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

	if ( !pidl1 || !pidl1->mkid.cb)		/* pidl 1 is desktop (root) */
	{ TRACE_(shell)("--- %p\n", pidl2);
	  return pidl2;
	}

	if (pidltemp1->mkid.cb && pidltemp2->mkid.cb)
	{ do
	  { ppidldata = _ILGetDataPointer(pidltemp1);
	    szData1 = _ILGetTextPointer(ppidldata->type, ppidldata);
	    
	    ppidldata = _ILGetDataPointer(pidltemp2);    
	    szData2 = _ILGetTextPointer(ppidldata->type, ppidldata);

	    pidltemp2 = ILGetNext(pidltemp2);	/* points behind the pidl2 */

	    if (strcmp(szData1,szData2) == 0)
	    { ret = pidltemp2;	/* found equal element */
	    }
	    else
	    { if (ret)		/* different element after equal -> break */
	      { ret = NULL;
	        break;
	      }
	    }
	    pidltemp1 = ILGetNext(pidltemp1);
	  } while (pidltemp1->mkid.cb && pidltemp2->mkid.cb);
	}	

	if (!pidltemp2->mkid.cb)
	{ return NULL; /* complete equal or pidl 2 is shorter */
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
{ DWORD    len1,len2;
  LPITEMIDLIST  pidlNew;
  
  TRACE_(pidl)("pidl=%p pidl=%p\n",pidl1,pidl2);

  if(!pidl1 && !pidl2)
  {  return NULL;
  }

  pdump (pidl1);
  pdump (pidl2);
 
  if(!pidl1)
  { pidlNew = ILClone(pidl2);
    return pidlNew;
  }

  if(!pidl2)
  { pidlNew = ILClone(pidl1);
    return pidlNew;
  }

  len1  = ILGetSize(pidl1)-2;
  len2  = ILGetSize(pidl2);
  pidlNew  = SHAlloc(len1+len2);
  
  if (pidlNew)
  { memcpy(pidlNew,pidl1,len1);
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
{	FIXME_(pidl)("sf=%p pidl=%p 0x%04lx\n",lpsf,pidl,z);
	pdump (pidl);
	return 0;
}

/*************************************************************************
 *  SHLogILFromFSIL [SHELL32.95]
 *
 * NOTES
 */
LPITEMIDLIST WINAPI SHLogILFromFSIL(LPITEMIDLIST pidl)
{	FIXME_(pidl)("(pidl=%p)\n",pidl);
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
{	LPSHITEMID si = &(pidl->mkid);
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
 * PARAMETERS
 *  pidl ITEMIDLIST
 *
 * RETURNS
 *  pointer to next element
 *  NULL when last element ist reached
 *
 */
LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl)
{	LPITEMIDLIST nextpidl;
	WORD len;
	
	TRACE_(pidl)("(pidl=%p)\n",pidl);
	if(pidl)
	{ len =  pidl->mkid.cb;
	  if (len)
	  { nextpidl = (LPITEMIDLIST) (((LPBYTE)pidl)+len);
	    return nextpidl;
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
{	LPITEMIDLIST idlRet;
	WARN_(pidl)("(pidl=%p,pidl=%p,%08u)semi-stub\n",pidl,item,bEnd);
	pdump (pidl);
	pdump (item);
	
	if (_ILIsDesktop(pidl))
	{  idlRet = ILClone(item);
	   if (pidl)
	     SHFree (pidl);
	   return idlRet;
	}  
	if (bEnd)
	{ idlRet=ILCombine(pidl,item);
	}
	else
	{ idlRet=ILCombine(item,pidl);
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
{	TRACE_(pidl)("%p\n",pidl);

	if (!pidl)
	  return FALSE;

	return pCOMCTL32_Free (pidl);
}
/*************************************************************************
 * ILCreateFromPath [SHELL32.157]
 *
 */
LPITEMIDLIST WINAPI ILCreateFromPathA (LPSTR path) 
{	LPITEMIDLIST pidlnew;

	TRACE_(shell)("%s\n",path);
	if (SUCCEEDED (SHILCreateFromPathA (path, &pidlnew, 0)))
	  return pidlnew;
	return FALSE;
}
LPITEMIDLIST WINAPI ILCreateFromPathW (LPWSTR path) 
{	LPITEMIDLIST pidlnew;

	TRACE_(shell)("%s\n",debugstr_w(path));

	if (SUCCEEDED (SHILCreateFromPathW (path, &pidlnew, 0)))
	  return pidlnew;
	return FALSE;
}
LPITEMIDLIST WINAPI ILCreateFromPathAW (LPVOID path) 
{
	if ( VERSION_OsIsUnicode())
	  return ILCreateFromPathW (path);
	return ILCreateFromPathA (path);
}
/*************************************************************************
 *  SHSimpleIDListFromPath [SHELL32.162]
 *
 */
LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW (LPVOID lpszPath)
{	LPCSTR	lpszElement;
	char	lpszTemp[MAX_PATH];

	if (!lpszPath)
	  return 0;

	if ( VERSION_OsIsUnicode())
	{ TRACE_(pidl)("(path=L%s)\n",debugstr_w((LPWSTR)lpszPath));
	  WideCharToLocal(lpszTemp, lpszPath, MAX_PATH);
  	}
	else
	{ TRACE_(pidl)("(path=%s)\n",(LPSTR)lpszPath);
	  strcpy(lpszTemp, lpszPath);
	}
		
	lpszElement = PathFindFilenameA(lpszTemp);
	if( GetFileAttributesA(lpszTemp) & FILE_ATTRIBUTE_DIRECTORY )
	{ return _ILCreateFolder(NULL, lpszElement);	/*FIXME: fill shortname */
	}
	return _ILCreateValue(NULL, lpszElement);	/*FIXME: fill shortname */
}
/*************************************************************************
 * SHGetDataFromIDListA [SHELL32.247]
 *
 */
HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len)
{	TRACE_(shell)("sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);
	
	if (! psf || !dest )
	  return E_INVALIDARG;

	switch (nFormat)
	{ case SHGDFIL_FINDDATA:
	    {  WIN32_FIND_DATAA * pfd = dest;
	       STRRET	lpName;
	       CHAR	pszPath[MAX_PATH];
	       HANDLE handle;

	       if ( len < sizeof (WIN32_FIND_DATAA))
	         return E_INVALIDARG;

	       psf->lpvtbl->fnAddRef(psf);
	       psf->lpvtbl->fnGetDisplayNameOf( psf, pidl, SHGDN_FORPARSING, &lpName);
	       psf->lpvtbl->fnRelease(psf);

	       strcpy(pszPath,lpName.u.cStr);
	       if ((handle  = FindFirstFileA ( pszPath, pfd)))
	         FindClose (handle);
	    }
	    break;
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
{	FIXME_(shell)("sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);
	return SHGetDataFromIDListA( psf, pidl, nFormat, dest, len);
}

/**************************************************************************
* internal functions
*/

/**************************************************************************
 *  _ILCreateDesktop()
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
LPITEMIDLIST WINAPI _ILCreateDrive( LPCSTR lpszNew)
{	char sTemp[4];
	strncpy (sTemp,lpszNew,4);
	sTemp[2]='\\';
	sTemp[3]=0x00;
	TRACE_(pidl)("(%s)\n",sTemp);
	return _ILCreate(PT_DRIVE,(LPVOID)&sTemp[0],4);
}
LPITEMIDLIST WINAPI _ILCreateFolder( LPCSTR lpszShortName, LPCSTR lpszName)
{	char	buff[MAX_PATH];
	char *	pbuff = buff;
	ULONG	len, len1;
	
	TRACE_(pidl)("(%s, %s)\n",lpszShortName, lpszName);

	len = strlen (lpszName)+1;
	memcpy (pbuff, lpszName, len);
	pbuff += len;

	if (lpszShortName)
	{ len1 = strlen (lpszShortName)+1;
	  memcpy (pbuff, lpszShortName, len1);
	}
	else
	{ len1 = 1;
	  *pbuff = 0x00;
	}
	return _ILCreate(PT_FOLDER, (LPVOID)buff, len + len1);
}
LPITEMIDLIST WINAPI _ILCreateValue(LPCSTR lpszShortName, LPCSTR lpszName)
{	char	buff[MAX_PATH];
	char *	pbuff = buff;
	ULONG	len, len1;
	
	TRACE_(pidl)("(%s, %s)\n", lpszShortName, lpszName);

	len = strlen (lpszName)+1;
	memcpy (pbuff, lpszName, len);
	pbuff += len;

	if (lpszShortName)
	{ len1 = strlen (lpszShortName)+1;
	  memcpy (pbuff, lpszShortName, len1);
	}
	else
	{ len1 = 1;
	  *pbuff = 0x00;
	}
	return _ILCreate(PT_VALUE, (LPVOID)buff, len + len1);
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
	  return _ILGetData(PT_DRIVE, pidl, (LPVOID)pOut, uSize)-1;

	return 0;
}
/**************************************************************************
 *  _ILGetItemText()
 *  Gets the text for only the first item
 *
 * RETURNS
 *  strlen (lpszText)
 */
DWORD WINAPI _ILGetItemText(LPCITEMIDLIST pidl, LPSTR lpszText, UINT16 uSize)
{	DWORD ret = 0;

	TRACE_(pidl)("(pidl=%p %p %d)\n",pidl,lpszText,uSize);
	if (_ILIsMyComputer(pidl))
	{ ret = _ILGetData(PT_MYCOMP, pidl, (LPVOID)lpszText, uSize)-1;
	}
	else if (_ILIsDrive(pidl))
	{ ret = _ILGetData(PT_DRIVE, pidl, (LPVOID)lpszText, uSize)-1;
	}
	else if (_ILIsFolder (pidl))
	{ ret = _ILGetData(PT_FOLDER, pidl, (LPVOID)lpszText, uSize)-1;
	}
	else if (_ILIsValue (pidl))
	{ ret = _ILGetData(PT_VALUE, pidl, (LPVOID)lpszText, uSize)-1;
	}
	TRACE_(pidl)("(-- %s)\n",debugstr_a(lpszText));
	return ret;
}
/**************************************************************************
 *  _ILIsDesktop()
 *  _ILIsDrive()
 *  _ILIsFolder()
 *  _ILIsValue()
 */
BOOL WINAPI _ILIsDesktop(LPCITEMIDLIST pidl)
{	TRACE_(pidl)("(%p)\n",pidl);
	return ( !pidl || (pidl && pidl->mkid.cb == 0x00) );
}

BOOL WINAPI _ILIsMyComputer(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE_(pidl)("(%p)\n",pidl);
	return (pidl && lpPData && PT_MYCOMP == lpPData->type);
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
 *  _ILGetFolderText()
 *  Creates a Path string from a PIDL, filtering out the special Folders and values
 *  There is no trailing backslash
 *  When lpszPath is NULL the needed size is returned
 * 
 * RETURNS
 *  strlen(lpszPath)
 */
DWORD WINAPI _ILGetFolderText(LPCITEMIDLIST pidl,LPSTR lpszPath, DWORD dwSize)
{	LPITEMIDLIST	pidlTemp;
	LPPIDLDATA	pData;
	DWORD		dwCopied = 0;
	LPSTR		pText;
 
	TRACE_(pidl)("(%p path=%p)\n",pidl, lpszPath);
 
	if(!pidl)
	  return 0;

	if(_ILIsMyComputer(pidl))
	{ pidlTemp = ILGetNext(pidl);
	  TRACE_(pidl)("-- skip My Computer\n");
	}
	else
	{ pidlTemp = (LPITEMIDLIST)pidl;
	}

	if(lpszPath)
	  *lpszPath = 0;

	pData = _ILGetDataPointer(pidlTemp);

	while(pidlTemp->mkid.cb && !(PT_VALUE == pData->type))
	{ 
	  if (!(pText = _ILGetTextPointer(pData->type,pData)))
	    return 0;				/* foreign pidl */
	    	  
	  dwCopied += strlen(pText);

	  pidlTemp = ILGetNext(pidlTemp);
	  pData = _ILGetDataPointer(pidlTemp);

	  if (lpszPath)
	  { strcat(lpszPath, pText);

	    if (pidlTemp->mkid.cb 		/* last element ? */
	    	&& (pText[2] != '\\')	 	/* drive has own '\' */
		&& (PT_VALUE != pData->type))	/* next element is value */
	    { lpszPath[dwCopied] = '\\';
	      lpszPath[dwCopied+1] = '\0';
	      dwCopied++;
	    }
	  }
	  else						/* only length */
	  { if (pidlTemp->mkid.cb 
	        && (pText[2] != '\\')
		&& (PT_VALUE != pData->type))
	      dwCopied++;				/* backslash between elements */
	  }
	}

	TRACE_(pidl)("-- (size=%lu path=%s)\n",dwCopied, debugstr_a(lpszPath));
	return dwCopied;
}


/**************************************************************************
 *  _ILGetValueText()
 *  Gets the text for the last item in the list
 */
DWORD WINAPI _ILGetValueText(LPCITEMIDLIST pidl, LPSTR lpszValue, DWORD dwSize)
{	LPITEMIDLIST  pidlTemp=pidl;
	CHAR          szText[MAX_PATH];

	TRACE_(pidl)("(pidl=%p %p 0x%08lx)\n",pidl,lpszValue,dwSize);

	if(!pidl)
	{ return 0;
	}
		
	while(pidlTemp->mkid.cb && !_ILIsValue(pidlTemp))
	{ pidlTemp = ILGetNext(pidlTemp);
	}

	if(!pidlTemp->mkid.cb)
	{ return 0;
	}

	_ILGetItemText( pidlTemp, szText, sizeof(szText));

	if(!lpszValue)
	{ return strlen(szText);
	}
	
	strcpy(lpszValue, szText);

	TRACE_(pidl)("-- (pidl=%p %p=%s 0x%08lx)\n",pidl,lpszValue,lpszValue,dwSize);
	return strlen(lpszValue);
}

/**************************************************************************
 *  _ILGetPidlPath()
 *  Create a string that includes the Drive name, the folder text and 
 *  the value text.
 *
 * RETURNS
 *  strlen(lpszOut)
 */
DWORD WINAPI _ILGetPidlPath( LPCITEMIDLIST pidl, LPSTR lpszOut, DWORD dwOutSize)
{	int	len = 0;
	LPSTR	lpszTemp = lpszOut;
	
	TRACE_(pidl)("(%p,%lu)\n",lpszOut,dwOutSize);

	if(!lpszOut)
	{ return 0;
	}

	*lpszOut = 0;

	len = _ILGetFolderText(pidl, lpszOut, dwOutSize);

	lpszOut += len;
	strcpy (lpszOut,"\\");
	len++; lpszOut++; dwOutSize -= len;

	len += _ILGetValueText(pidl, lpszOut, dwOutSize );

	/*remove the last backslash if necessary */
	if( lpszTemp[len-1]=='\\')
	{ lpszTemp[len-1] = 0;
	  len--;
	}

	TRACE_(pidl)("-- (%p=%s,%u)\n",lpszTemp,lpszTemp,len);

	return len;
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
	    pidlOut->mkid.cb = uSize;
	    pData =_ILGetDataPointer(pidlOut);
	    pData->type = type;
	    memcpy(&(pData->u.mycomp.guid), pIn, uInSize);
	    TRACE_(pidl)("- create My Computer\n");
	    break;

	  case PT_DRIVE:
	    uSize = 2 + 23;
	    pidlOut = SHAlloc(uSize + 2);
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
 *  _ILGetData(PIDLTYPE, LPCITEMIDLIST, LPVOID, UINT16)
 *
 * RETURNS
 *  length of data (raw)
 */
DWORD WINAPI _ILGetData(PIDLTYPE type, LPCITEMIDLIST pidl, LPVOID pOut, UINT uOutSize)
{	LPPIDLDATA  pData;
	DWORD       dwReturn=0; 
	LPSTR	    pszSrc;
	
	TRACE_(pidl)("(%x %p %p %x)\n",type,pidl,pOut,uOutSize);
	
	if( (!pidl) || (!pOut) || (uOutSize < 1))
	{  return 0;
	}

	*(LPSTR)pOut = 0;

	pData = _ILGetDataPointer(pidl);

	assert ( pData->type == type);

	pszSrc = _ILGetTextPointer(pData->type, pData);

	if (pszSrc)
	{ strncpy((LPSTR)pOut, pszSrc, uOutSize);
	  dwReturn = strlen((LPSTR)pOut)+1;
	}
	else
	{ ERR_(pidl)("-- no data\n");
	}

	TRACE_(pidl)("-- (%p=%s 0x%08lx)\n",pOut,(char*)pOut,dwReturn);
	return dwReturn;
}


/**************************************************************************
 *  _ILGetDataPointer()
 */
LPPIDLDATA WINAPI _ILGetDataPointer(LPITEMIDLIST pidl)
{	if(pidl && pidl->mkid.cb != 0x00)
	  return (LPPIDLDATA)(&pidl->mkid.abID);
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
	{ case PT_DRIVE:
	  case PT_DRIVE1:
	  case PT_DRIVE2:
	  case PT_DRIVE3:
	    return (LPSTR)&(pidldata->u.drive.szDriveName);

	  case PT_MYCOMP:
	    return szMyComp;

	  case PT_SPECIAL:
	    return szNetHood;

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
 * gets a pointer to the long filename string stored in the pidl
 */
LPSTR WINAPI _ILGetSTextPointer(PIDLTYPE type, LPPIDLDATA pidldata)
{/*	TRACE(pidl,"(type=%x data=%p)\n", type, pidldata);*/

	if(!pidldata)
	{ return NULL;
	}
	switch (type)
	{ case PT_FOLDER:
	  case PT_VALUE:
	  case PT_IESPECIAL:
	    return (LPSTR)(pidldata->u.file.szNames + strlen (pidldata->u.file.szNames) + 1);
	  case PT_WORKGRP:
	    return (LPSTR)(pidldata->u.network.szNames + strlen (pidldata->u.network.szNames) + 1);
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
	char stemp[20]; /* for filesize */
	
	switch (pdata->type)
	{ case PT_VALUE:
	    break;
	  default:
	    return FALSE;
	}
	StrFormatByteSizeA(pdata->u.file.dwFileSize, stemp, 20);
	strncpy( pOut, stemp, 20);
	return TRUE;
}

BOOL WINAPI _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{	char pTemp[MAX_PATH];
	const char * pPoint;

	TRACE_(pidl)("pidl=%p\n",pidl);

	if ( ! _ILGetValueText(pidl, pTemp, MAX_PATH))
	{ return FALSE;
	}

	pPoint = PathFindExtensionA(pTemp);

	if (! *pPoint)
	  return FALSE;

	pPoint++;
	strncpy(pOut, pPoint, uOutSize);
	TRACE_(pidl)("%s\n",pOut);

	return TRUE;
}
