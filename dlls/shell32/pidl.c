/*
 *	pidl Handling
 *
 *	Copyright 1998	Juergen Schmied
 *
 *  !!! currently work in progress on all classes !!!
 *  <contact juergen.schmied@metronet.de, 980801>
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
{ DWORD type;
  CHAR * szData;
  LPITEMIDLIST pidltemp = pidl;
  TRACE(pidl,"---------- pidl=%p \n", pidl);
  do
  { szData = ((LPPIDLDATA )(pidltemp->mkid.abID))->szText;
    type   = ((LPPIDLDATA )(pidltemp->mkid.abID))->type;
    TRACE(pidl,"---- pidl=%p size=%u type=%lx %s\n",pidltemp, pidltemp->mkid.cb,type,debugstr_a(szData));
    pidltemp = (LPITEMIDLIST)(((BYTE*)pidltemp)+pidltemp->mkid.cb);
  } while (pidltemp->mkid.cb);
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
LPSHITEMID WINAPI ILFindLastID(LPITEMIDLIST iil) 
{	LPSHITEMID	lastsii,sii;

 	TRACE(pidl,"%p\n",iil);
	if (!iil)
	{ return NULL;
    }
	sii = &(iil->mkid);
	lastsii = sii;
	while (sii->cb)
    { lastsii = sii;
	  sii = (LPSHITEMID)(((char*)sii)+sii->cb);
	}
	return lastsii;
}
/*************************************************************************
 * ILRemoveLastID [SHELL32.17]
 * NOTES
 *  Removes the last item 
 */
BOOL32 WINAPI ILRemoveLastID(LPCITEMIDLIST pidl)
{ LPCITEMIDLIST	xpidl;
   
  TRACE(shell,"pidl=%p\n",pidl);
  if (!pidl || !pidl->mkid.cb)
    return 0;
  ILFindLastID(pidl)->cb = 0;
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
{ FIXME(pidl,"pidl=%p\n",pidl);
  return NULL;
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
{ LPSHITEMID si = &(pidl->mkid);
  DWORD  len=0;

  TRACE(pidl,"pidl=%p\n",pidl);

  if (pidl)
  { while (si->cb) 
    { len += si->cb;
      si  = (LPSHITEMID)(((LPBYTE)si)+si->cb);
    }
    len += 2;
	}
/*  TRACE(pidl,"-- size=%lu\n",len);*/
	return len;
}
/*************************************************************************
 * ILGetNext [SHELL32.153]
 *  gets the next simple pidl ot of a complex pidl
 *
 * PARAMETERS
 *  pidl ITEMIDLIST
 *
 * RETURNS
 *  pointer to next element
 *
 */
LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl)
{ LPITEMIDLIST nextpidl;

  TRACE(pidl,"(pidl=%p)\n",pidl);
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
{ TRACE(pidl,"(pidl=%p,pidl=%p,%08u)\n",pidl,item,bEnd);
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
{ TRACE(pidl,"(pidl=0x%08lx)\n",(DWORD)pidl);
  if (!pidl)
		return 0;
  return SHFree(pidl);
}

/**************************************************************************
*	  INTERNAL CLASS pidlmgr
*/

static struct PidlMgr_VTable pmgrvt = {
    PidlMgr_CreateDesktop,
    PidlMgr_CreateMyComputer,
	PidlMgr_CreateDrive,
    PidlMgr_CreateFolder,
    PidlMgr_CreateValue,
    PidlMgr_GetDesktop,
	PidlMgr_GetDrive,
    PidlMgr_GetLastItem,
    PidlMgr_GetItemText,
    PidlMgr_IsDesktop,
    PidlMgr_IsMyComputer,
	PidlMgr_IsDrive,
    PidlMgr_IsFolder,
    PidlMgr_IsValue,
    PidlMgr_HasFolders,
    PidlMgr_GetFolderText,
    PidlMgr_GetValueText,
    PidlMgr_GetValueType,
    PidlMgr_GetDataText,
    PidlMgr_GetPidlPath,
    PidlMgr_Create,
    PidlMgr_GetData,
    PidlMgr_GetDataPointer,
    PidlMgr_SeparatePathAndValue
};
/**************************************************************************
 *	  PidlMgr_Constructor
 */
LPPIDLMGR PidlMgr_Constructor() 
{	LPPIDLMGR pmgr;
	pmgr = (LPPIDLMGR)HeapAlloc(GetProcessHeap(),0,sizeof(pidlmgr));
	pmgr->lpvtbl = &pmgrvt;
	TRACE(pidl,"(%p)->()\n",pmgr);
  /** FIXMEDllRefCount++;*/
	return pmgr;
}
/**************************************************************************
 *	  PidlMgr_Destructor
 */
void PidlMgr_Destructor(LPPIDLMGR this) 
{	HeapFree(GetProcessHeap(),0,this);
 	TRACE(pidl,"(%p)->()\n",this);
  /** FIXMEDllRefCount--;*/
}

/**************************************************************************
 *  PidlMgr_CreateDesktop()
 *  PidlMgr_CreateMyComputer()
 *  PidlMgr_CreateDrive()
 *  PidlMgr_CreateFolder() 
 *  PidlMgr_CreateValue()
 */
LPITEMIDLIST PidlMgr_CreateDesktop(LPPIDLMGR this)
{ TRACE(pidl,"(%p)->()\n",this);
    return PidlMgr_Create(this,PT_DESKTOP, NULL, 0);
}
LPITEMIDLIST PidlMgr_CreateMyComputer(LPPIDLMGR this)
{ TRACE(pidl,"(%p)->()\n",this);
  return PidlMgr_Create(this,PT_MYCOMP, (void *)"My Computer", strlen ("My Computer")+1);
}
LPITEMIDLIST PidlMgr_CreateDrive(LPPIDLMGR this, LPCSTR lpszNew)
{ char sTemp[4];
  strncpy (sTemp,lpszNew,4);
	sTemp[2]='\\';
	sTemp[3]=0x00;
  TRACE(pidl,"(%p)->(%s)\n",this,sTemp);
  return PidlMgr_Create(this,PT_DRIVE,(LPVOID)&sTemp[0],4);
}
LPITEMIDLIST PidlMgr_CreateFolder(LPPIDLMGR this, LPCSTR lpszNew)
{ TRACE(pidl,"(%p)->(%s)\n",this,lpszNew);
  return PidlMgr_Create(this,PT_FOLDER, (LPVOID)lpszNew, strlen(lpszNew)+1);
}
LPITEMIDLIST PidlMgr_CreateValue(LPPIDLMGR this,LPCSTR lpszNew)
{ TRACE(pidl,"(%p)->(%s)\n",this,lpszNew);
  return PidlMgr_Create(this,PT_VALUE, (LPVOID)lpszNew, strlen(lpszNew)+1);
}

/**************************************************************************
 *  PidlMgr_GetDesktop()
 * 
 *  FIXME: quick hack
 */
BOOL32 PidlMgr_GetDesktop(LPPIDLMGR this,LPCITEMIDLIST pidl,LPSTR pOut)
{ TRACE(pidl,"(%p)->(%p %p)\n",this,pidl,pOut);
  return (BOOL32)PidlMgr_GetData(this,PT_DESKTOP, pidl, (LPVOID)pOut, 255);
}
/**************************************************************************
 *  PidlMgr_GetDrive()
 *
 *  FIXME: quick hack
 */
BOOL32 PidlMgr_GetDrive(LPPIDLMGR this,LPCITEMIDLIST pidl,LPSTR pOut, UINT16 uSize)
{ LPITEMIDLIST   pidlTemp=NULL;

  TRACE(pidl,"(%p)->(%p,%p,%u)\n",this,pidl,pOut,uSize);
  if(PidlMgr_IsMyComputer(this,pidl))
  { pidlTemp = ILGetNext(pidl);
	}
  else if (pidlTemp && PidlMgr_IsDrive(this,pidlTemp))
  { return (BOOL32)PidlMgr_GetData(this,PT_DRIVE, pidlTemp, (LPVOID)pOut, uSize);
	}
	return FALSE;
}
/**************************************************************************
 *  PidlMgr_GetLastItem()
 *  Gets the last item in the list
 */
LPITEMIDLIST PidlMgr_GetLastItem(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPITEMIDLIST   pidlLast = NULL;

  TRACE(pidl,"(%p)->(pidl=%p)\n",this,pidl);

  if(pidl)
  { while(pidl->mkid.cb)
    { pidlLast = (LPITEMIDLIST)pidl;
      pidl = ILGetNext(pidl);
    }  
  }
  return pidlLast;
}
/**************************************************************************
 *  PidlMgr_GetItemText()
 *  Gets the text for only this item
 */
DWORD PidlMgr_GetItemText(LPPIDLMGR this,LPCITEMIDLIST pidl, LPSTR lpszText, UINT16 uSize)
{ TRACE(pidl,"(%p)->(pidl=%p %p %x)\n",this,pidl,lpszText,uSize);
  if (PidlMgr_IsMyComputer(this, pidl))
  { return PidlMgr_GetData(this,PT_MYCOMP, pidl, (LPVOID)lpszText, uSize);
	}
	if (PidlMgr_IsDrive(this, pidl))
	{ return PidlMgr_GetData(this,PT_DRIVE, pidl, (LPVOID)lpszText, uSize);
	}
	return PidlMgr_GetData(this,PT_TEXT, pidl, (LPVOID)lpszText, uSize);	
}
/**************************************************************************
 *  PidlMgr_IsDesktop()
 *  PidlMgr_IsDrive()
 *  PidlMgr_IsFolder()
 *  PidlMgr_IsValue()
*/
BOOL32 PidlMgr_IsDesktop(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ TRACE(pidl,"%p->(%p)\n",this,pidl);

  if (! pidl)
    return FALSE;

  return (  pidl->mkid.cb == 0x00 );
}

BOOL32 PidlMgr_IsMyComputer(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"%p->(%p)\n",this,pidl);

  if (! pidl)
    return FALSE;

  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_MYCOMP == pData->type);
}

BOOL32 PidlMgr_IsDrive(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"%p->(%p)\n",this,pidl);

  if (! pidl)
    return FALSE;

  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_DRIVE == pData->type);
}

BOOL32 PidlMgr_IsFolder(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"%p->(%p)\n",this,pidl);

  if (! pidl)
    return FALSE;

  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_FOLDER == pData->type);
}

BOOL32 PidlMgr_IsValue(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(pidl,"%p->(%p)\n",this,pidl);

  if (! pidl)
    return FALSE;

  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_VALUE == pData->type);
}
/**************************************************************************
 * PidlMgr_HasFolders()
 * fixme: quick hack
 */
BOOL32 PidlMgr_HasFolders(LPPIDLMGR this, LPSTR pszPath, LPCITEMIDLIST pidl)
{ BOOL32 bResult= FALSE;
  WIN32_FIND_DATA32A stffile;	
  HANDLE32 hFile;

  TRACE(pidl,"(%p)->%p %p\n",this, pszPath, pidl);
	
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
 *  PidlMgr_GetFolderText()
 *  Creates a Path string from a PIDL, filtering out the special Folders 
 */
DWORD PidlMgr_GetFolderText(LPPIDLMGR this,LPCITEMIDLIST pidl,
   LPSTR lpszPath, DWORD dwSize)
{ LPITEMIDLIST   pidlTemp;
  DWORD          dwCopied = 0;
 
  TRACE(pidl,"(%p)->(%p)\n",this,pidl);
 
  if(!pidl)
  { return 0;
  }

  if(PidlMgr_IsMyComputer(this,pidl))
  { pidlTemp = ILGetNext(pidl);
    TRACE(pidl,"-- (%p)->skip My Computer\n",this);
  }
  else
  { pidlTemp = (LPITEMIDLIST)pidl;
  }

  //if this is NULL, return the required size of the buffer
  if(!lpszPath)
  { while(pidlTemp->mkid.cb)
    { LPPIDLDATA  pData = PidlMgr_GetDataPointer(this,pidlTemp);
      
      //add the length of this item plus one for the backslash
      dwCopied += strlen(pData->szText) + 1;  /* FIXME pData->szText is not every time a string*/

      pidlTemp = ILGetNext(pidlTemp);
    }

    //add one for the NULL terminator
	TRACE(pidl,"-- (%p)->(size=%lu)\n",this,dwCopied);
    return dwCopied + 1;
  }

  *lpszPath = 0;

  while(pidlTemp->mkid.cb && (dwCopied < dwSize))
  { LPPIDLDATA  pData = PidlMgr_GetDataPointer(this,pidlTemp);

    //if this item is a value, then skip it and finish
    if(PT_VALUE == pData->type)
    { break;
	}
   
    strcat(lpszPath, pData->szText);
    strcat(lpszPath, "\\");
    dwCopied += strlen(pData->szText) + 1;
    pidlTemp = ILGetNext(pidlTemp);

	TRACE(pidl,"-- (%p)->(size=%lu,%s)\n",this,dwCopied,lpszPath);
  }

  //remove the last backslash if necessary
  if(dwCopied)
  { if(*(lpszPath + strlen(lpszPath) - 1) == '\\')
    { *(lpszPath + strlen(lpszPath) - 1) = 0;
      dwCopied--;
    }
  }
  TRACE(pidl,"-- (%p)->(path=%s)\n",this,lpszPath);
  return dwCopied;
}


/**************************************************************************
 *  PidlMgr_GetValueText()
 *  Gets the text for the last item in the list
 */
DWORD PidlMgr_GetValueText(LPPIDLMGR this,
    LPCITEMIDLIST pidl, LPSTR lpszValue, DWORD dwSize)
{ LPITEMIDLIST  pidlTemp=pidl;
  CHAR          szText[MAX_PATH];

  TRACE(pidl,"(%p)->(pidl=%p %p 0x%08lx)\n",this,pidl,lpszValue,dwSize);

  if(!pidl)
  { return 0;
	}
		
  while(pidlTemp->mkid.cb && !PidlMgr_IsValue(this,pidlTemp))
  { pidlTemp = ILGetNext(pidlTemp);
  }

  if(!pidlTemp->mkid.cb)
  { return 0;
	}

  PidlMgr_GetItemText(this, pidlTemp, szText, sizeof(szText));

  if(!lpszValue)
  { return strlen(szText) + 1;
  }
  strcpy(lpszValue, szText);
	TRACE(pidl,"-- (%p)->(pidl=%p %p=%s 0x%08lx)\n",this,pidl,lpszValue,lpszValue,dwSize);
  return strlen(lpszValue);
}
/**************************************************************************
 *  PidlMgr_GetValueType()
 */
BOOL32 PidlMgr_GetValueType( LPPIDLMGR this,
   LPCITEMIDLIST pidlPath,
   LPCITEMIDLIST pidlValue,
   LPDWORD pdwType)
{ LPSTR    lpszFolder,
           lpszValueName;
  DWORD    dwNameSize;
 
  FIXME(pidl,"(%p)->(%p %p %p) stub\n",this,pidlPath,pidlValue,pdwType);
	
  if(!pidlPath)
  { return FALSE;
	}

  if(!pidlValue)
  { return FALSE;
	}

  if(!pdwType)
  { return FALSE;
	}

  //get the Desktop
  //PidlMgr_GetDesktop(this,pidlPath);

  /* fixme: add the driveletter here*/
	
  //assemble the Folder string
  dwNameSize = PidlMgr_GetFolderText(this,pidlPath, NULL, 0);
  lpszFolder = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszFolder)
  {  return FALSE;
	}
  PidlMgr_GetFolderText(this,pidlPath, lpszFolder, dwNameSize);

  //assemble the value name
  dwNameSize = PidlMgr_GetValueText(this,pidlValue, NULL, 0);
  lpszValueName = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszValueName)
  { HeapFree(GetProcessHeap(),0,lpszFolder);
    return FALSE;
  }
  PidlMgr_GetValueText(this,pidlValue, lpszValueName, dwNameSize);

  /* fixme: we've got the path now do something with it
	  -like get the filetype*/
	
	pdwType=NULL;
	
  HeapFree(GetProcessHeap(),0,lpszFolder);
  HeapFree(GetProcessHeap(),0,lpszValueName);
  return TRUE;
}
/**************************************************************************
 *  PidlMgr_GetDataText()
 */
DWORD PidlMgr_GetDataText( LPPIDLMGR this,
 LPCITEMIDLIST pidlPath, LPCITEMIDLIST pidlValue, LPSTR lpszOut, DWORD dwOutSize)
{ LPSTR    lpszFolder,
           lpszValueName;
  DWORD    dwNameSize;

  FIXME(pidl,"(%p)->(pidl=%p pidl=%p) stub\n",this,pidlPath,pidlValue);

  if(!lpszOut || !pidlPath || !pidlValue)
  { return FALSE;
	}

  /* fixme: get the driveletter*/

  //assemble the Folder string
  dwNameSize = PidlMgr_GetFolderText(this,pidlPath, NULL, 0);
  lpszFolder = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszFolder)
  {  return FALSE;
	}
  PidlMgr_GetFolderText(this,pidlPath, lpszFolder, dwNameSize);

  //assemble the value name
  dwNameSize = PidlMgr_GetValueText(this,pidlValue, NULL, 0);
  lpszValueName = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszValueName)
  { HeapFree(GetProcessHeap(),0,lpszFolder);
    return FALSE;
  }
  PidlMgr_GetValueText(this,pidlValue, lpszValueName, dwNameSize);

  /* fixme: we've got the path now do something with it*/
	
  HeapFree(GetProcessHeap(),0,lpszFolder);
  HeapFree(GetProcessHeap(),0,lpszValueName);

  TRACE(pidl,"-- (%p)->(%p=%s 0x%08lx)\n",this,lpszOut,lpszOut,dwOutSize);

	return TRUE;
}

/**************************************************************************
 *   CPidlMgr::GetPidlPath()
 *  Create a string that includes the Drive name, the folder text and 
 *  the value text.
 */
DWORD PidlMgr_GetPidlPath(LPPIDLMGR this,
    LPCITEMIDLIST pidl, LPSTR lpszOut, DWORD dwOutSize)
{ LPSTR lpszTemp;
  WORD	len;

  TRACE(pidl,"(%p)->(%p,%lu)\n",this,lpszOut,dwOutSize);

  if(!lpszOut)
  { return 0;
	}

  *lpszOut = 0;
  lpszTemp = lpszOut;

  dwOutSize -= PidlMgr_GetFolderText(this,pidl, lpszTemp, dwOutSize);

  //add a backslash if necessary
  len = strlen(lpszTemp);
  if (len && lpszTemp[len-1]!='\\')
	{ lpszTemp[len+0]='\\';
    lpszTemp[len+1]='\0';
		dwOutSize--;
  }

  lpszTemp = lpszOut + strlen(lpszOut);

  //add the value string
  PidlMgr_GetValueText(this,pidl, lpszTemp, dwOutSize);

  //remove the last backslash if necessary
  if(*(lpszOut + strlen(lpszOut) - 1) == '\\')
  { *(lpszOut + strlen(lpszOut) - 1) = 0;
  }

  TRACE(pidl,"-- (%p)->(%p=%s,%lu)\n",this,lpszOut,lpszOut,dwOutSize);

  return strlen(lpszOut);

}

/**************************************************************************
 *  PidlMgr_Create()
 *  Creates a new PIDL
 *  type = PT_DESKTOP | PT_DRIVE | PT_FOLDER | PT_VALUE
 *  pIn = data
 *  uInSize = size of data
 */

LPITEMIDLIST PidlMgr_Create(LPPIDLMGR this,PIDLTYPE type, LPVOID pIn, UINT16 uInSize)
{ LPITEMIDLIST   pidlOut=NULL;
  UINT16         uSize;
  LPITEMIDLIST   pidlTemp=NULL;
  LPPIDLDATA     pData;

  TRACE(pidl,"(%p)->(%x %p %x)\n",this,type,pIn,uInSize);

  if ( type == PT_DESKTOP)
  {   pidlOut = SHAlloc(2);
      pidlOut->mkid.cb=0x0000;
      return pidlOut;
	}

  if (! pIn)
	{ return NULL;
	}	

  uSize = 2 + (sizeof(PIDLTYPE)) + uInSize + 2;  /* cb + PIDLTYPE + uInSize +2 */
  pidlOut = SHAlloc(uSize);
  pidlTemp = pidlOut;
  if(pidlOut)
  { pidlTemp->mkid.cb = uSize - 2;
    pData =(LPPIDLDATA) &(pidlTemp->mkid.abID[0]);
    pData->type = type;
    switch(type)
    { case PT_MYCOMP:
        memcpy(pData->szText, pIn, uInSize);
                       TRACE(pidl,"- (%p)->create My Computer: %s\n",this,debugstr_a(pData->szText));
                       break;
      case PT_DRIVE:
        memcpy(pData->szText, pIn, uInSize);
                       TRACE(pidl,"- (%p)->create Drive: %s\n",this,debugstr_a(pData->szText));
											 break;
      case PT_FOLDER:
      case PT_VALUE:   
        memcpy(pData->szText, pIn, uInSize);
                       TRACE(pidl,"- (%p)->create Value: %s\n",this,debugstr_a(pData->szText));
											 break;
      default: 
        FIXME(pidl,"- (%p) wrong argument\n",this);
			                 break;
    }
   
    pidlTemp = ILGetNext(pidlTemp);
  pidlTemp->mkid.cb = 0x00;
  }
  TRACE(pidl,"-- (%p)->(pidl=%p, size=%u)\n",this,pidlOut,uSize-2);
  return pidlOut;
}
/**************************************************************************
 *  PidlMgr_GetData(PIDLTYPE, LPCITEMIDLIST, LPVOID, UINT16)
 */
DWORD PidlMgr_GetData(
    LPPIDLMGR this,
		PIDLTYPE type, 
    LPCITEMIDLIST pidl,
		LPVOID pOut,
		UINT16 uOutSize)
{ LPPIDLDATA  pData;
  DWORD       dwReturn=0; 

	TRACE(pidl,"(%p)->(%x %p %p %x)\n",this,type,pidl,pOut,uOutSize);
	
  if(!pidl)
  {  return 0;
	}

  pData = PidlMgr_GetDataPointer(this,pidl);

  //copy the data
  switch(type)
  { case PT_MYCOMP:  if(uOutSize < 1)
                       return 0;
                     if(PT_MYCOMP != pData->type)
                       return 0;
										 *(LPSTR)pOut = 0;	 
                     strncpy((LPSTR)pOut, "My Computer", uOutSize);
										 dwReturn = strlen((LPSTR)pOut);
                     break;

	 case PT_DRIVE:    if(uOutSize < 1)
                       return 0;
                     if(PT_DRIVE != pData->type)
                       return 0;
										 *(LPSTR)pOut = 0;	 
                     strncpy((LPSTR)pOut, pData->szText, uOutSize);
										 dwReturn = strlen((LPSTR)pOut);
                     break;

   case PT_FOLDER:
   case PT_VALUE: 
	 case PT_TEXT:     *(LPSTR)pOut = 0;
                     strncpy((LPSTR)pOut, pData->szText, uOutSize);
                     dwReturn = strlen((LPSTR)pOut);
                     break;
   default:     break;										 
  }
	TRACE(pidl,"-- (%p)->(%p=%s 0x%08lx)\n",this,pOut,(char*)pOut,dwReturn);
  return dwReturn;
}


/**************************************************************************
 *  PidlMgr_GetDataPointer()
 */
LPPIDLDATA PidlMgr_GetDataPointer(LPPIDLMGR this,LPITEMIDLIST pidl)
{ if(!pidl)
  {  return NULL;
	}
	TRACE(pidl,"(%p)->(%p)\n"	,this, pidl);
  return (LPPIDLDATA)(pidl->mkid.abID);
}

/**************************************************************************
 *  CPidlMgr_SeparatePathAndValue)
 *  Creates a separate path and value PIDL from a fully qualified PIDL.
 */
BOOL32 PidlMgr_SeparatePathAndValue(LPPIDLMGR this, 
   LPITEMIDLIST pidlFQ, LPITEMIDLIST *ppidlPath, LPITEMIDLIST *ppidlValue)
{ LPITEMIDLIST   pidlTemp;
	TRACE(pidl,"(%p)->(pidl=%p pidl=%p pidl=%p)",this,pidlFQ,ppidlPath,ppidlValue);
  if(!pidlFQ)
  {  return FALSE;
	}

  *ppidlValue = PidlMgr_GetLastItem(this,pidlFQ);

  if(!PidlMgr_IsValue(this,*ppidlValue))
  { return FALSE;
	}

  *ppidlValue = ILClone(*ppidlValue);
  *ppidlPath = ILClone(pidlFQ);

  pidlTemp = PidlMgr_GetLastItem(this,*ppidlPath);
  pidlTemp->mkid.cb = 0x00;

  return TRUE;
}
