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
BOOL32 WINAPI ILGetDisplayName(LPCITEMIDLIST iil,LPSTR path)
{	FIXME(pidl,"(%p,%p),stub, return e:!\n",iil,path);
	strcpy(path,"e:\\");
	return TRUE;
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

  TRACE(pidl,"%p\n",pidl);

  pdump(pidl);

  if (!pidl)
    return NULL;
    
  len = ILGetSize(pidl);
  newpidl = (LPITEMIDLIST)SHAlloc(len);
  if (newpidl)
    memcpy(newpidl,pidl,len);
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
	TRACE(pidl,"pidl=%p\n",pidl);
	
	if (pidl)
	{ len = pidl->mkid.cb;	
	  newpidl = (LPITEMIDLIST) SHAlloc (len+2);
	  if (newpidl)
	  { memcpy(newpidl,pidl,len);
   	    ILGetNext(newpidl)->mkid.cb = 0x00;
	  }
	 }

  	return newpidl;
}
/*************************************************************************
 * ILIsEqual [SHELL32.21]
 *
 */
BOOL32 WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{	FIXME(pidl,"pidl1=%p pidl2=%p stub\n",pidl1, pidl2);
	pdump (pidl1);
	pdump (pidl2);
	return FALSE;
}
/*************************************************************************
 * ILFindChild [SHELL32.24]
 *
 */
DWORD WINAPI ILFindChild(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2)
{	FIXME(pidl,"%p %p stub\n",pidl1,pidl2);
	pdump (pidl1);
	pdump (pidl2);
	return 0;
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
 *  SHLogILFromFSIL [SHELL32.95]
 *
 * NOTES
 *  might be the prepending of MyComputer to a filesystem pidl (?)
 */
LPITEMIDLIST WINAPI SHLogILFromFSIL(LPITEMIDLIST pidl)
{	FIXME(pidl,"(pidl=%p)\n",pidl);
	pdump(pidl);
	return ILClone(pidl);
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

	/*TRACE(pidl,"pidl=%p\n",pidl);*/

	if (pidl)
	{ while (si->cb) 
	  { len += si->cb;
	    si  = (LPSHITEMID)(((LPBYTE)si)+si->cb);
	  }
	  len += 2;
	}
	/*TRACE(pidl,"-- size=%lu\n",len);*/
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

/*	TRACE(pidl,"(pidl=%p)\n",pidl);*/
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
 *  otherwise adds the item to the end.
 *  Destroys the passed in idlist!
 */
LPITEMIDLIST WINAPI ILAppend(LPITEMIDLIST pidl,LPCITEMIDLIST item,BOOL32 bEnd)
{	FIXME(pidl,"(pidl=%p,pidl=%p,%08u)stub\n",pidl,item,bEnd);
	return NULL;
}
/*************************************************************************
 * ILFree [SHELL32.155]
 *
 * NOTES
 *     free_check_ptr - frees memory (if not NULL)
 *     allocated by SHMalloc allocator
 *     exported by ordinal
 */
DWORD WINAPI ILFree(LPVOID pidl) 
{	TRACE(pidl,"(pidl=0x%08lx)\n",(DWORD)pidl);
	if (!pidl)
	  return 0;
	return SHFree(pidl);
}
/*************************************************************************
 * ILCreateFromPath [SHELL32.157]
 *
 */
LPITEMIDLIST WINAPI ILCreateFromPath(LPSTR path) 
{	LPSHELLFOLDER shellfolder;
	LPITEMIDLIST pidlnew;
	CHAR pszTemp[MAX_PATH*2];
	LPWSTR lpszDisplayName = (LPWSTR)&pszTemp[0];
	DWORD pchEaten;
	
	TRACE(pidl,"(path=%s)\n",path);
	
	LocalToWideChar32(lpszDisplayName, path, MAX_PATH);
  
	if (SHGetDesktopFolder(&shellfolder)==S_OK)
	{ shellfolder->lpvtbl->fnParseDisplayName(shellfolder,0, NULL,lpszDisplayName,&pchEaten,&pidlnew,NULL);
	  shellfolder->lpvtbl->fnRelease(shellfolder);
	}
	return pidlnew;
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
 
	TRACE(pidl,"(%p)\n",pidl);
 
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
	  PathAddBackslash(lpszPath);
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
	    uSize = 4 + 10;
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
DWORD WINAPI _ILGetData(PIDLTYPE type, LPCITEMIDLIST pidl, LPVOID pOut, UINT16 uOutSize)
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
