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
#include "ole2.h"
#include "debug.h"
#include "shlobj.h"
#include "shell.h"
#include "winerror.h"
#include "winnls.h"
#include "winproc.h"
#include "commctrl.h"
#include "winversion.h"
#include "shell32_main.h"

#include "pidl.h"

void pdump (LPCITEMIDLIST pidl)
{	DWORD type;
	CHAR * szData;
	CHAR * szShortName;
	LPITEMIDLIST pidltemp = pidl;
	if (! pidltemp)
	{ TRACE(pidl,"-------- pidl = NULL (Root)\n");
	  return;
	}
	TRACE(pidl,"-------- pidl=%p \n", pidl);
	if (pidltemp->mkid.cb)
	{ do
	  { type   = _ILGetDataPointer(pidltemp)->type;
	    szData = _ILGetTextPointer(type, _ILGetDataPointer(pidltemp));
	    szShortName = _ILGetSTextPointer(type, _ILGetDataPointer(pidltemp));

	    TRACE(pidl,"---- pidl=%p size=%u type=%lx %s, (%s)\n",
	               pidltemp, pidltemp->mkid.cb,type,debugstr_a(szData), debugstr_a(szShortName));

	    pidltemp = ILGetNext(pidltemp);
	  } while (pidltemp->mkid.cb);
	  return;
	}
	else
	  TRACE(pidl,"empty pidl (Desktop)\n");	
}
/*************************************************************************
 * ILGetDisplayName			[SHELL32.15]
 */
BOOL WINAPI ILGetDisplayName(LPCITEMIDLIST pidl,LPSTR path)
{	FIXME(shell,"pidl=%p %p semi-stub\n",pidl,path);
	return SHGetPathFromIDListA(pidl, path);
}
/*************************************************************************
 * ILFindLastID [SHELL32.16]
 */
LPITEMIDLIST WINAPI ILFindLastID(LPITEMIDLIST pidl) 
{	LPITEMIDLIST   pidlLast = NULL;

	TRACE(pidl,"(pidl=%p)\n",pidl);

	if(pidl)
	{ while(pidl->mkid.cb)
	  { pidlLast = (LPITEMIDLIST)pidl;
	    pidl = ILGetNext(pidl);
	  }  
	}
	return pidlLast;		
}
/*************************************************************************
 * ILRemoveLastID [SHELL32.17]
 * NOTES
 *  Removes the last item 
 */
BOOL WINAPI ILRemoveLastID(LPCITEMIDLIST pidl)
{	TRACE(shell,"pidl=%p\n",pidl);
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

  TRACE(pidl,"pidl=%p newpidl=%p\n",pidl, newpidl);
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
	
	TRACE(pidl,"pidl=%p \n",pidl);
	pdump(pidl);
	
	if (pidl)
	{ len = pidl->mkid.cb;	
	  newpidl = (LPITEMIDLIST) SHAlloc (len+2);
	  if (newpidl)
	  { memcpy(newpidl,pidl,len);
   	    ILGetNext(newpidl)->mkid.cb = 0x00;
	  }
	}
	TRACE(pidl,"-- newpidl=%p\n",newpidl);

  	return newpidl;
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
	WARN(shell,"(hwnd=0x%x,csidl=0x%lx,0x%lx):semi-stub.\n",
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

	TRACE(pidl,"pidl=%p newpidl=%p\n",pidl, newpidl);
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

	TRACE(pidl,"pidl1=%p pidl2=%p\n",pidl1, pidl2);

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

	    if (strcmp ( szData1, szData2 )!=0 )
	      return FALSE;

	    pidltemp1 = ILGetNext(pidltemp1);
	    pidltemp2 = ILGetNext(pidltemp2);

	  } while (pidltemp1->mkid.cb && pidltemp2->mkid.cb);
	}	
	if (!pidltemp1->mkid.cb && !pidltemp2->mkid.cb)
	{ TRACE(shell, "--- equal\n");
	  return TRUE;
	}

	return FALSE;
}
/*************************************************************************
 * ILIsParent [SHELL32.23]
 *
 */
DWORD WINAPI ILIsParent( DWORD x, DWORD y, DWORD z)
{	FIXME(pidl,"0x%08lx 0x%08lx 0x%08lx stub\n",x,y,z);
	return 0;
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

	TRACE(pidl,"pidl1=%p pidl2=%p\n",pidl1, pidl2);

	pdump (pidl1);
	pdump (pidl2);

	if ( !pidl1 || !pidl1->mkid.cb)		/* pidl 1 is desktop (root) */
	{ TRACE(shell, "--- %p\n", pidl2);
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

	TRACE(shell, "--- %p\n", ret);
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
  
  TRACE(pidl,"pidl=%p pidl=%p\n",pidl1,pidl2);

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
{	FIXME(pidl,"sf=%p pidl=%p 0x%04lx\n",lpsf,pidl,z);
	pdump (pidl);
	return 0;
}

/*************************************************************************
 *  SHLogILFromFSIL [SHELL32.95]
 *
 * NOTES
 */
LPITEMIDLIST WINAPI SHLogILFromFSIL(LPITEMIDLIST pidl)
{	FIXME(pidl,"(pidl=%p)\n",pidl);
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
	TRACE(pidl,"pidl=%p size=%lu\n",pidl, len);
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
 *
 */
LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl)
{	LPITEMIDLIST nextpidl;

	/* TRACE(pidl,"(pidl=%p)\n",pidl);*/
	if(pidl)
	{ nextpidl = (LPITEMIDLIST)(LPBYTE)(((LPBYTE)pidl) + pidl->mkid.cb);
	  return nextpidl;
	}
	else
	{  return (NULL);
	}
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
	WARN(pidl,"(pidl=%p,pidl=%p,%08u)semi-stub\n",pidl,item,bEnd);
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
{	TRACE(pidl,"(pidl=0x%08lx)\n",(DWORD)pidl);

	if (!pidl)
	  return FALSE;

	return SHFree(pidl);
}
/*************************************************************************
 * ILGlobalFree [SHELL32.156]
 *
 */
DWORD WINAPI ILGlobalFree( LPITEMIDLIST pidl)
{	TRACE(pidl,"%p\n",pidl);

	if (!pidl)
	  return FALSE;

	return pCOMCTL32_Free (pidl);
}
/*************************************************************************
 * ILCreateFromPath [SHELL32.157]
 *
 */
LPITEMIDLIST WINAPI ILCreateFromPath(LPVOID path) 
{	LPSHELLFOLDER shellfolder;
	LPITEMIDLIST pidlnew;
	WCHAR lpszDisplayName[MAX_PATH];
	DWORD pchEaten;
	
	if ( !VERSION_OsIsUnicode())
	{ TRACE(pidl,"(path=%s)\n",(LPSTR)path);
	  LocalToWideChar(lpszDisplayName, path, MAX_PATH);
  	}
	else
	{ TRACE(pidl,"(path=L%s)\n",debugstr_w(path));
	  lstrcpyW(lpszDisplayName, path);
	}

	if (SHGetDesktopFolder(&shellfolder)==S_OK)
	{ shellfolder->lpvtbl->fnParseDisplayName(shellfolder,0, NULL,lpszDisplayName,&pchEaten,&pidlnew,NULL);
	  shellfolder->lpvtbl->fnRelease(shellfolder);
	}
	return pidlnew;
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
	{ TRACE(pidl,"(path=L%s)\n",debugstr_w((LPWSTR)lpszPath));
	  WideCharToLocal(lpszTemp, lpszPath, MAX_PATH);
  	}
	else
	{ TRACE(pidl,"(path=%s)\n",(LPSTR)lpszPath);
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
{	FIXME(shell,"sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);
	switch (nFormat)
	{ case SHGDFIL_FINDDATA:
	  case SHGDFIL_NETRESOURCE:
	  case SHGDFIL_DESCRIPTIONID:
	    break;
	  default:
	    ERR(shell,"Unknown SHGDFIL %i, please report\n", nFormat);
	}
	return E_INVALIDARG;
}
/*************************************************************************
 * SHGetDataFromIDListW [SHELL32.247]
 *
 */
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len)
{	FIXME(shell,"sf=%p pidl=%p 0x%04x %p 0x%04x stub\n",psf,pidl,nFormat,dest,len);
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
{	TRACE(pidl,"()\n");
	return _ILCreate(PT_DESKTOP, NULL, 0);
}
LPITEMIDLIST WINAPI _ILCreateMyComputer()
{	TRACE(pidl,"()\n");
	return _ILCreate(PT_MYCOMP, (void *)"My Computer", strlen ("My Computer")+1);
}
LPITEMIDLIST WINAPI _ILCreateDrive( LPCSTR lpszNew)
{	char sTemp[4];
	strncpy (sTemp,lpszNew,4);
	sTemp[2]='\\';
	sTemp[3]=0x00;
	TRACE(pidl,"(%s)\n",sTemp);
	return _ILCreate(PT_DRIVE,(LPVOID)&sTemp[0],4);
}
LPITEMIDLIST WINAPI _ILCreateFolder( LPCSTR lpszShortName, LPCSTR lpszName)
{	char	buff[MAX_PATH];
	char *	pbuff = buff;
	ULONG	len, len1;
	
	TRACE(pidl,"(%s, %s)\n",lpszShortName, lpszName);

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
	
	TRACE(pidl,"(%s, %s)\n", lpszShortName, lpszName);

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
{	TRACE(pidl,"(%p,%p,%u)\n",pidl,pOut,uSize);

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

	TRACE(pidl,"(pidl=%p %p %d)\n",pidl,lpszText,uSize);
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
	TRACE(pidl,"(-- %s)\n",debugstr_a(lpszText));
	return ret;
}
/**************************************************************************
 *  _ILIsDesktop()
 *  _ILIsDrive()
 *  _ILIsFolder()
 *  _ILIsValue()
 */
BOOL WINAPI _ILIsDesktop(LPCITEMIDLIST pidl)
{	TRACE(pidl,"(%p)\n",pidl);
	return ( !pidl || (pidl && pidl->mkid.cb == 0x00) );
}

BOOL WINAPI _ILIsMyComputer(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE(pidl,"(%p)\n",pidl);
	return (pidl && lpPData && PT_MYCOMP == lpPData->type);
}

BOOL WINAPI _ILIsDrive(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE(pidl,"(%p)\n",pidl);
	return (pidl && lpPData && PT_DRIVE == lpPData->type);
}

BOOL WINAPI _ILIsFolder(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE(pidl,"(%p)\n",pidl);
	return (pidl && lpPData && PT_FOLDER == lpPData->type);
}

BOOL WINAPI _ILIsValue(LPCITEMIDLIST pidl)
{	LPPIDLDATA lpPData = _ILGetDataPointer(pidl);
	TRACE(pidl,"(%p)\n",pidl);
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
 
	TRACE(pidl,"(%p path=%p)\n",pidl, lpszPath);
 
	if(!pidl)
	  return 0;

	if(_ILIsMyComputer(pidl))
	{ pidlTemp = ILGetNext(pidl);
	  TRACE(pidl,"-- skip My Computer\n");
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

	TRACE(pidl,"-- (size=%lu path=%s)\n",dwCopied, debugstr_a(lpszPath));
	return dwCopied;
}


/**************************************************************************
 *  _ILGetValueText()
 *  Gets the text for the last item in the list
 */
DWORD WINAPI _ILGetValueText(LPCITEMIDLIST pidl, LPSTR lpszValue, DWORD dwSize)
{	LPITEMIDLIST  pidlTemp=pidl;
	CHAR          szText[MAX_PATH];

	TRACE(pidl,"(pidl=%p %p 0x%08lx)\n",pidl,lpszValue,dwSize);

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

	TRACE(pidl,"-- (pidl=%p %p=%s 0x%08lx)\n",pidl,lpszValue,lpszValue,dwSize);
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
	
	TRACE(pidl,"(%p,%lu)\n",lpszOut,dwOutSize);

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

	TRACE(pidl,"-- (%p=%s,%u)\n",lpszTemp,lpszTemp,len);

	return len;
}

/**************************************************************************
 *  _ILCreate()
 *  Creates a new PIDL
 *  type = PT_DESKTOP | PT_DRIVE | PT_FOLDER | PT_VALUE
 *  pIn = data
 *  uInSize = size of data (raw)
 */

LPITEMIDLIST WINAPI _ILCreate(PIDLTYPE type, LPVOID pIn, UINT16 uInSize)
{	LPITEMIDLIST   pidlOut=NULL;
	UINT16         uSize;
	LPITEMIDLIST   pidlTemp=NULL;
	LPPIDLDATA     pData;
	LPSTR	pszDest;
	
	TRACE(pidl,"(0x%02x %p %i)\n",type,pIn,uInSize);

	if ( type == PT_DESKTOP)
	{ pidlOut = SHAlloc(2);
	  pidlOut->mkid.cb=0x0000;
	  return pidlOut;
	}

	if (! pIn)
	{ return NULL;
	}	

	/* the sizes of: cb(2), pidldata-1(26), szText+1, next cb(2) */
	switch (type)
	{ case PT_DRIVE:
	    uSize = 4 + 9;
	    break;
	  default:
	    uSize = 4 + (sizeof(PIDLDATA)) + uInSize; 
	 }   
	pidlOut = SHAlloc(uSize);
	pidlTemp = pidlOut;
	if(pidlOut)
	{ pidlTemp->mkid.cb = uSize - 2;
	  pData =_ILGetDataPointer(pidlTemp);
	  pszDest =  _ILGetTextPointer(type, pData);
	  pData->type = type;
	  switch(type)
	  { case PT_MYCOMP:
	      memcpy(pszDest, pIn, uInSize);
	      TRACE(pidl,"- create My Computer: %s\n",debugstr_a(pszDest));
	      break;
	    case PT_DRIVE:
	      memcpy(pszDest, pIn, uInSize);
	      TRACE(pidl,"- create Drive: %s\n",debugstr_a(pszDest));
	      break;
	    case PT_FOLDER:
	    case PT_VALUE:   
	      memcpy(pszDest, pIn, uInSize);
	      TRACE(pidl,"- create Value: %s\n",debugstr_a(pszDest));
	      break;
	    default: 
	      FIXME(pidl,"-- wrong argument\n");
	      break;
	  }
   
	  pidlTemp = ILGetNext(pidlTemp);
	  pidlTemp->mkid.cb = 0x00;
	}
	TRACE(pidl,"-- (pidl=%p, size=%u)\n",pidlOut,uSize-2);
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
	
	TRACE(pidl,"(%x %p %p %x)\n",type,pidl,pOut,uOutSize);
	
	if(!pidl)
	{  return 0;
	}

	*(LPSTR)pOut = 0;	 

	pData = _ILGetDataPointer(pidl);
	if ( pData->type != type)
	{ ERR(pidl,"-- wrong type\n");
	  return 0;
	}
	pszSrc = _ILGetTextPointer(pData->type, pData);

	switch(type)
	{ case PT_MYCOMP:
	    if(uOutSize < 1)
	      return 0;
	    strncpy((LPSTR)pOut, "My Computer", uOutSize);
	    dwReturn = strlen((LPSTR)pOut)+1;
	    break;

	  case PT_DRIVE:
	    if(uOutSize < 1)
	      return 0;
	    strncpy((LPSTR)pOut, pszSrc, uOutSize);
	    dwReturn = strlen((LPSTR)pOut)+1;
	    break;

	  case PT_FOLDER:
	  case PT_VALUE: 
	     strncpy((LPSTR)pOut, pszSrc, uOutSize);
	     dwReturn = strlen((LPSTR)pOut)+1;
	     break;
	  default:
	    ERR(pidl,"-- unknown type\n");
	    break;										 
	}
	TRACE(pidl,"-- (%p=%s 0x%08lx)\n",pOut,(char*)pOut,dwReturn);
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
	    return (LPSTR)&(pidldata->u.drive.szDriveName);
	  case PT_MYCOMP:
	  case PT_FOLDER:
	  case PT_VALUE:
	    return (LPSTR)&(pidldata->u.file.szNames);
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
	{ case PT_MYCOMP:
	  case PT_DRIVE:
	    return NULL;
	  case PT_FOLDER:
	  case PT_VALUE:
	    return (LPSTR)(pidldata->u.file.szNames + strlen (pidldata->u.file.szNames) + 1);
	}
	return NULL;
}
BOOL WINAPI _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{	LPPIDLDATA pdata =_ILGetDataPointer(pidl);
	FILETIME ft;
	SYSTEMTIME time;

	switch (pdata->type)
	{ case PT_DRIVE:
	  case PT_MYCOMP:
	    return FALSE;
	  case PT_FOLDER:
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
	{ case PT_DRIVE:
	  case PT_MYCOMP:
	  case PT_FOLDER:
	    return FALSE;
	  case PT_VALUE:
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
	int i;

	TRACE(pidl,"pidl=%p\n",pidl);

	if ( ! _ILGetValueText(pidl, pTemp, MAX_PATH))
	{ return FALSE;
	}

	for (i=0; pTemp[i]!='.' && pTemp[i];i++);

	if (!pTemp[i])
	  return FALSE;
	  
	strncpy(pOut, &pTemp[i], uOutSize);
	TRACE(pidl,"%s\n",pOut);

	return TRUE;
}

/**************************************************************************
 *  IDLList "Item ID List List"
 * 
 */
static UINT WINAPI IDLList_GetState(LPIDLLIST this);
static LPITEMIDLIST WINAPI IDLList_GetElement(LPIDLLIST this, UINT nIndex);
static UINT WINAPI IDLList_GetCount(LPIDLLIST this);
static BOOL WINAPI IDLList_StoreItem(LPIDLLIST this, LPITEMIDLIST pidl);
static BOOL WINAPI IDLList_AddItems(LPIDLLIST this, LPITEMIDLIST *apidl, UINT cidl);
static BOOL WINAPI IDLList_InitList(LPIDLLIST this);
static void WINAPI IDLList_CleanList(LPIDLLIST this);

static IDLList_VTable idllvt = 
{	IDLList_GetState,
	IDLList_GetElement,
	IDLList_GetCount,
	IDLList_StoreItem,
	IDLList_AddItems,
	IDLList_InitList,
	IDLList_CleanList
};

LPIDLLIST IDLList_Constructor (UINT uStep)
{	LPIDLLIST lpidll;
	if (!(lpidll = (LPIDLLIST)HeapAlloc(GetProcessHeap(),0,sizeof(IDLList))))
	  return NULL;

	lpidll->lpvtbl=&idllvt;
	lpidll->uStep=uStep;
	lpidll->dpa=NULL;

	TRACE (shell,"(%p)\n",lpidll);
	return lpidll;
}
void IDLList_Destructor(LPIDLLIST this)
{	TRACE (shell,"(%p)\n",this);
	IDLList_CleanList(this);
}
 
static UINT WINAPI IDLList_GetState(LPIDLLIST this)
{	TRACE (shell,"(%p)->(uStep=%u dpa=%p)\n",this, this->uStep, this->dpa);

	if (this->uStep == 0)
	{ if (this->dpa)
	    return(State_Init);
          return(State_OutOfMem);
        }
        return(State_UnInit);
}
static LPITEMIDLIST WINAPI IDLList_GetElement(LPIDLLIST this, UINT nIndex)
{	TRACE (shell,"(%p)->(index=%u)\n",this, nIndex);
	return((LPITEMIDLIST)pDPA_GetPtr(this->dpa, nIndex));
}
static UINT WINAPI IDLList_GetCount(LPIDLLIST this)
{	TRACE (shell,"(%p)\n",this);
	return(IDLList_GetState(this)==State_Init ? DPA_GetPtrCount(this->dpa) : 0);
}
static BOOL WINAPI IDLList_StoreItem(LPIDLLIST this, LPITEMIDLIST pidl)
{	TRACE (shell,"(%p)->(pidl=%p)\n",this, pidl);
	if (pidl)
        { if (IDLList_InitList(this) && pDPA_InsertPtr(this->dpa, 0x7fff, (LPSTR)pidl)>=0)
	    return(TRUE);
	  ILFree(pidl);
        }
        IDLList_CleanList(this);
        return(FALSE);
}
static BOOL WINAPI IDLList_AddItems(LPIDLLIST this, LPITEMIDLIST *apidl, UINT cidl)
{	INT i;
	TRACE (shell,"(%p)->(apidl=%p cidl=%u)\n",this, apidl, cidl);

	for (i=0; i<cidl; ++i)
        { if (!IDLList_StoreItem(this, ILClone((LPCITEMIDLIST)apidl[i])))
	    return(FALSE);
        }
        return(TRUE);
}
static BOOL WINAPI IDLList_InitList(LPIDLLIST this)
{	TRACE (shell,"(%p)\n",this);
	switch (IDLList_GetState(this))
        { case State_Init:
	    return(TRUE);

	  case State_OutOfMem:
	    return(FALSE);

	  case State_UnInit:
	  default:
	    this->dpa = pDPA_Create(this->uStep);
	    this->uStep = 0;
	    return(IDLList_InitList(this));
        }
}
static void WINAPI IDLList_CleanList(LPIDLLIST this)
{	INT i;
	TRACE (shell,"(%p)\n",this);

	if (this->uStep != 0)
        { this->dpa = NULL;
	  this->uStep = 0;
	  return;
        }

        if (!this->dpa)
        { return;
        }

        for (i=DPA_GetPtrCount(this->dpa)-1; i>=0; --i)
        { ILFree(IDLList_GetElement(this,i));
        }

        pDPA_Destroy(this->dpa);
        this->dpa=NULL;
}        
