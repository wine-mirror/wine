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
#include "ole.h"
#include "ole2.h"
#include "debug.h"
#include "compobj.h"
#include "interfaces.h"
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

	    TRACE(pidl,"---- pidl=%p size=%u type=%lx %s\n",pidltemp, pidltemp->mkid.cb,type,debugstr_a(szData));

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
BOOL32 WINAPI ILGetDisplayName(LPCITEMIDLIST pidl,LPSTR path)
{	FIXME(shell,"pidl=%p %p semi-stub\n",pidl,path);
	return SHGetPathFromIDList32A(pidl, path);
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
BOOL32 WINAPI ILRemoveLastID(LPCITEMIDLIST pidl)
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
BOOL32 WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
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
LPITEMIDLIST WINAPI SHGetRealIDL(DWORD x, DWORD y, DWORD z)
{	FIXME(pidl,"0x%04lx 0x%04lx 0x%04lx\n",x,y,z);
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
LPITEMIDLIST WINAPI ILAppend(LPITEMIDLIST pidl,LPCITEMIDLIST item,BOOL32 bEnd)
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
	  LocalToWideChar32(lpszDisplayName, path, MAX_PATH);
  	}
	else
	{ TRACE(pidl,"(path=L%s)\n",debugstr_w(path));
	  lstrcpy32W(lpszDisplayName, path);
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
LPITEMIDLIST WINAPI SHSimpleIDListFromPath32AW (LPVOID lpszPath)
{	LPCSTR	lpszElement;
	char	lpszTemp[MAX_PATH];

	if (!lpszPath)
	  return 0;

	if ( VERSION_OsIsUnicode())
	{ TRACE(pidl,"(path=L%s)\n",debugstr_w((LPWSTR)lpszPath));
	  WideCharToLocal32(lpszTemp, lpszPath, MAX_PATH);
  	}
	else
	{ TRACE(pidl,"(path=%s)\n",(LPSTR)lpszPath);
	  strcpy(lpszTemp, lpszPath);
	}
		
	lpszElement = PathFindFilename32A(lpszTemp);
	if( GetFileAttributes32A(lpszTemp) & FILE_ATTRIBUTE_DIRECTORY )
	{ return _ILCreateFolder(lpszElement);
	}
	return _ILCreateValue(lpszElement);
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
{ TRACE(pidl,"()\n");
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
LPITEMIDLIST WINAPI _ILCreateFolder( LPCSTR lpszNew)
{ TRACE(pidl,"(%s)\n",lpszNew);
  return _ILCreate(PT_FOLDER, (LPVOID)lpszNew, strlen(lpszNew)+1);
}
LPITEMIDLIST WINAPI _ILCreateValue(LPCSTR lpszNew)
{ TRACE(pidl,"(%s)\n",lpszNew);
  return _ILCreate(PT_VALUE, (LPVOID)lpszNew, strlen(lpszNew)+1);
}

/**************************************************************************
 *  _ILGetDrive()
 *
 *  FIXME: quick hack
 */
BOOL32 WINAPI _ILGetDrive(LPCITEMIDLIST pidl,LPSTR pOut, UINT16 uSize)
{	LPITEMIDLIST   pidlTemp=NULL;

	TRACE(pidl,"(%p,%p,%u)\n",pidl,pOut,uSize);
	if(_ILIsMyComputer(pidl))
	{ pidlTemp = ILGetNext(pidl);
	}
	else if (pidlTemp && _ILIsDrive(pidlTemp))
	{ return (BOOL32)_ILGetData(PT_DRIVE, pidlTemp, (LPVOID)pOut, uSize);
	}
	return FALSE;
}
/**************************************************************************
 *  _ILGetItemText()
 *  Gets the text for only this item
 */
DWORD WINAPI _ILGetItemText(LPCITEMIDLIST pidl, LPSTR lpszText, UINT16 uSize)
{	TRACE(pidl,"(pidl=%p %p %x)\n",pidl,lpszText,uSize);
	if (_ILIsMyComputer(pidl))
	{ return _ILGetData(PT_MYCOMP, pidl, (LPVOID)lpszText, uSize);
	}
	if (_ILIsDrive(pidl))
	{ return _ILGetData(PT_DRIVE, pidl, (LPVOID)lpszText, uSize);
	}
	if (_ILIsFolder (pidl))
	{ return _ILGetData(PT_FOLDER, pidl, (LPVOID)lpszText, uSize);
	}
	return _ILGetData(PT_VALUE, pidl, (LPVOID)lpszText, uSize);	
}
/**************************************************************************
 *  _ILIsDesktop()
 *  _ILIsDrive()
 *  _ILIsFolder()
 *  _ILIsValue()
 */
BOOL32 WINAPI _ILIsDesktop(LPCITEMIDLIST pidl)
{ TRACE(pidl,"(%p)\n",pidl);

  if (! pidl)
    return TRUE;

  return (  pidl->mkid.cb == 0x00 );
}

BOOL32 WINAPI _ILIsMyComputer(LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"(%p)\n",pidl);

  if (! pidl)
    return FALSE;

  pData = _ILGetDataPointer(pidl);
  return (PT_MYCOMP == pData->type);
}

BOOL32 WINAPI _ILIsDrive(LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"(%p)\n",pidl);

  if (! pidl)
    return FALSE;

  pData = _ILGetDataPointer(pidl);
  return (PT_DRIVE == pData->type);
}

BOOL32 WINAPI _ILIsFolder(LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"(%p)\n",pidl);

  if (! pidl)
    return FALSE;

  pData = _ILGetDataPointer(pidl);
  return (PT_FOLDER == pData->type);
}

BOOL32 WINAPI _ILIsValue(LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"(%p)\n",pidl);

  if (! pidl)
    return FALSE;

  pData = _ILGetDataPointer(pidl);
  return (PT_VALUE == pData->type);
}
/**************************************************************************
 * _ILHasFolders()
 * fixme: quick hack
 */
BOOL32 WINAPI _ILHasFolders( LPSTR pszPath, LPCITEMIDLIST pidl)
{	BOOL32 bResult= FALSE;
	WIN32_FIND_DATA32A stffile;	
	HANDLE32 hFile;

	TRACE(pidl,"%p %p\n", pszPath, pidl);
	
	hFile = FindFirstFile32A(pszPath,&stffile);
	do
	{ if (! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	  {  bResult= TRUE;	  
	  }
	} while( FindNextFile32A(hFile,&stffile));
	FindClose32 (hFile);
	
	return bResult;
}

/**************************************************************************
 *  _ILGetFolderText()
 *  Creates a Path string from a PIDL, filtering out the special Folders 
 */
DWORD WINAPI _ILGetFolderText(LPCITEMIDLIST pidl,LPSTR lpszPath, DWORD dwSize)
{	LPITEMIDLIST	pidlTemp;
	DWORD		dwCopied = 0;
	LPSTR		pText;
 
	TRACE(pidl,"(%p path=%p)\n",pidl, lpszPath);
 
	if(!pidl)
	{ return 0;
	}

	if(_ILIsMyComputer(pidl))
	{ pidlTemp = ILGetNext(pidl);
	  TRACE(pidl,"-- skip My Computer\n");
	}
	else
	{ pidlTemp = (LPITEMIDLIST)pidl;
	}

	//if this is NULL, return the required size of the buffer
	if(!lpszPath)
	{ while(pidlTemp->mkid.cb)
	  { LPPIDLDATA  pData = _ILGetDataPointer(pidlTemp);
	    pText = _ILGetTextPointer(pData->type,pData);         

	    /*add the length of this item plus one for the backslash
	    fixme: is one to much, drive has its own backslash*/
	    dwCopied += strlen(pText) + 1;  	    
	    pidlTemp = ILGetNext(pidlTemp);
	  }

	  //add one for the NULL terminator
	  TRACE(pidl,"-- (size=%lu)\n",dwCopied);
	  return dwCopied + 1;
	}

	*lpszPath = 0;

	while(pidlTemp->mkid.cb && (dwCopied < dwSize))
	{ LPPIDLDATA  pData = _ILGetDataPointer(pidlTemp);

	  //if this item is a value, then skip it and finish
	  if(PT_VALUE == pData->type)
	  { break;
	  }
	  pText = _ILGetTextPointer(pData->type,pData);   
	  strcat(lpszPath, pText);
	  PathAddBackslash32A(lpszPath);
	  dwCopied += strlen(pText) + 1;
	  pidlTemp = ILGetNext(pidlTemp);

	  TRACE(pidl,"-- (size=%lu,%s)\n",dwCopied,lpszPath);
	}

	//remove the last backslash if necessary
	if(dwCopied)
	{ if(*(lpszPath + strlen(lpszPath) - 1) == '\\')
	  { *(lpszPath + strlen(lpszPath) - 1) = 0;
	    dwCopied--;
	  }
	}
	TRACE(pidl,"-- (path=%s)\n",lpszPath);
	return dwCopied;
}


/**************************************************************************
 *  _ILGetValueText()
 *  Gets the text for the last item in the list
 */
DWORD WINAPI _ILGetValueText(
    LPCITEMIDLIST pidl, LPSTR lpszValue, DWORD dwSize)
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
	{ return strlen(szText) + 1;
	}
	strcpy(lpszValue, szText);
	TRACE(pidl,"-- (pidl=%p %p=%s 0x%08lx)\n",pidl,lpszValue,lpszValue,dwSize);
	return strlen(lpszValue);
}
/**************************************************************************
 *  _ILGetDataText()
 * NOTES
 *  used from ShellView
 */
DWORD WINAPI _ILGetDataText( LPCITEMIDLIST pidlPath, LPCITEMIDLIST pidlValue, LPSTR lpszOut, DWORD dwOutSize)
{ LPSTR    lpszFolder,
           lpszValueName;
  DWORD    dwNameSize;

  FIXME(pidl,"(pidl=%p pidl=%p) stub\n",pidlPath,pidlValue);

  if(!lpszOut || !pidlPath || !pidlValue)
  { return FALSE;
	}

  /* fixme: get the driveletter*/

  //assemble the Folder string
  dwNameSize = _ILGetFolderText(pidlPath, NULL, 0);
  lpszFolder = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszFolder)
  {  return FALSE;
	}
  _ILGetFolderText(pidlPath, lpszFolder, dwNameSize);

  //assemble the value name
  dwNameSize = _ILGetValueText(pidlValue, NULL, 0);
  lpszValueName = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszValueName)
  { HeapFree(GetProcessHeap(),0,lpszFolder);
    return FALSE;
  }
  _ILGetValueText(pidlValue, lpszValueName, dwNameSize);

  /* fixme: we've got the path now do something with it*/
	
  HeapFree(GetProcessHeap(),0,lpszFolder);
  HeapFree(GetProcessHeap(),0,lpszValueName);

  TRACE(pidl,"-- (%p=%s 0x%08lx)\n",lpszOut,lpszOut,dwOutSize);

	return TRUE;
}

/**************************************************************************
 *  _ILGetPidlPath()
 *  Create a string that includes the Drive name, the folder text and 
 *  the value text.
 */
DWORD WINAPI _ILGetPidlPath( LPCITEMIDLIST pidl, LPSTR lpszOut, DWORD dwOutSize)
{ LPSTR lpszTemp;
  WORD	len;

  TRACE(pidl,"(%p,%lu)\n",lpszOut,dwOutSize);

  if(!lpszOut)
  { return 0;
	}

  *lpszOut = 0;
  lpszTemp = lpszOut;

  dwOutSize -= _ILGetFolderText(pidl, lpszTemp, dwOutSize);

  //add a backslash if necessary
  len = strlen(lpszTemp);
  if (len && lpszTemp[len-1]!='\\')
	{ lpszTemp[len+0]='\\';
    lpszTemp[len+1]='\0';
		dwOutSize--;
  }

  lpszTemp = lpszOut + strlen(lpszOut);

  //add the value string
  _ILGetValueText(pidl, lpszTemp, dwOutSize);

  //remove the last backslash if necessary
  if(*(lpszOut + strlen(lpszOut) - 1) == '\\')
  { *(lpszOut + strlen(lpszOut) - 1) = 0;
  }

  TRACE(pidl,"-- (%p=%s,%lu)\n",lpszOut,lpszOut,dwOutSize);

  return strlen(lpszOut);

}

/**************************************************************************
 *  _ILCreate()
 *  Creates a new PIDL
 *  type = PT_DESKTOP | PT_DRIVE | PT_FOLDER | PT_VALUE
 *  pIn = data
 *  uInSize = size of data
 */

LPITEMIDLIST WINAPI _ILCreate(PIDLTYPE type, LPVOID pIn, UINT16 uInSize)
{	LPITEMIDLIST   pidlOut=NULL;
	UINT16         uSize;
	LPITEMIDLIST   pidlTemp=NULL;
	LPPIDLDATA     pData;
	LPSTR	pszDest;
	
	TRACE(pidl,"(%x %p %x)\n",type,pIn,uInSize);

	if ( type == PT_DESKTOP)
	{ pidlOut = SHAlloc(2);
	  pidlOut->mkid.cb=0x0000;
	  return pidlOut;
	}

	if (! pIn)
	{ return NULL;
	}	

	/* the sizes of: cb(2), pidldata-1, szText+1, next cb(2) */
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
 */
DWORD WINAPI _ILGetData(PIDLTYPE type, LPCITEMIDLIST pidl, LPVOID pOut, UINT32 uOutSize)
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
	    dwReturn = strlen((LPSTR)pOut);
	    break;

	  case PT_DRIVE:
	    if(uOutSize < 1)
	      return 0;
	    strncpy((LPSTR)pOut, pszSrc, uOutSize);
	    dwReturn = strlen((LPSTR)pOut);
	    break;

	  case PT_FOLDER:
	  case PT_VALUE: 
	     strncpy((LPSTR)pOut, pszSrc, uOutSize);
	     dwReturn = strlen((LPSTR)pOut);
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
{	if(!pidl)
	{ return NULL;
	}
/*	TRACE(pidl,"(%p)\n",  pidl);*/
	return (LPPIDLDATA)(&pidl->mkid.abID);
}
/**************************************************************************
 *  _ILGetTextPointer()
 * gets a pointer to the string stored in the pidl
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
	    return (LPSTR)&(pidldata->u.file.szText);
	}
	return NULL;
}
BOOL32 WINAPI _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize)
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
	return GetDateFormat32A(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  pOut, uOutSize);
}
BOOL32 WINAPI _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize)
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
	StrFormatByteSize32A(pdata->u.file.dwFileSize, stemp, 20);
	strncpy( pOut, stemp, 20);
	return TRUE;
}

BOOL32 WINAPI _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize)
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
static UINT32 WINAPI IDLList_GetState(LPIDLLIST this);
static LPITEMIDLIST WINAPI IDLList_GetElement(LPIDLLIST this, UINT32 nIndex);
static UINT32 WINAPI IDLList_GetCount(LPIDLLIST this);
static BOOL32 WINAPI IDLList_StoreItem(LPIDLLIST this, LPITEMIDLIST pidl);
static BOOL32 WINAPI IDLList_AddItems(LPIDLLIST this, LPITEMIDLIST *apidl, UINT32 cidl);
static BOOL32 WINAPI IDLList_InitList(LPIDLLIST this);
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

LPIDLLIST IDLList_Constructor (UINT32 uStep)
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
 
static UINT32 WINAPI IDLList_GetState(LPIDLLIST this)
{	TRACE (shell,"(%p)->(uStep=%u dpa=%p)\n",this, this->uStep, this->dpa);

	if (this->uStep == 0)
	{ if (this->dpa)
	    return(State_Init);
          return(State_OutOfMem);
        }
        return(State_UnInit);
}
static LPITEMIDLIST WINAPI IDLList_GetElement(LPIDLLIST this, UINT32 nIndex)
{	TRACE (shell,"(%p)->(index=%u)\n",this, nIndex);
	return((LPITEMIDLIST)pDPA_GetPtr(this->dpa, nIndex));
}
static UINT32 WINAPI IDLList_GetCount(LPIDLLIST this)
{	TRACE (shell,"(%p)\n",this);
	return(IDLList_GetState(this)==State_Init ? DPA_GetPtrCount(this->dpa) : 0);
}
static BOOL32 WINAPI IDLList_StoreItem(LPIDLLIST this, LPITEMIDLIST pidl)
{	TRACE (shell,"(%p)->(pidl=%p)\n",this, pidl);
	if (pidl)
        { if (IDLList_InitList(this) && pDPA_InsertPtr(this->dpa, 0x7fff, (LPSTR)pidl)>=0)
	    return(TRUE);
	  ILFree(pidl);
        }
        IDLList_CleanList(this);
        return(FALSE);
}
static BOOL32 WINAPI IDLList_AddItems(LPIDLLIST this, LPITEMIDLIST *apidl, UINT32 cidl)
{	INT32 i;
	TRACE (shell,"(%p)->(apidl=%p cidl=%u)\n",this, apidl, cidl);

	for (i=0; i<cidl; ++i)
        { if (!IDLList_StoreItem(this, ILClone((LPCITEMIDLIST)apidl[i])))
	    return(FALSE);
        }
        return(TRUE);
}
static BOOL32 WINAPI IDLList_InitList(LPIDLLIST this)
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
{	INT32 i;
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
