/*
 *	IContextMenu
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
 */
#include <string.h>

#include "winerror.h"
#include "debug.h"

#include "pidl.h"
#include "wine/obj_base.h"
#include "if_macros.h"
#include "shlguid.h"
#include "shell32_main.h"
#include "shresdef.h"

static HRESULT WINAPI IContextMenu_QueryInterface(LPCONTEXTMENU ,REFIID , LPVOID *);
static ULONG WINAPI IContextMenu_AddRef(LPCONTEXTMENU);
static ULONG WINAPI IContextMenu_Release(LPCONTEXTMENU);
static HRESULT WINAPI IContextMenu_QueryContextMenu(LPCONTEXTMENU , HMENU32 ,UINT32 ,UINT32 ,UINT32 ,UINT32);
static HRESULT WINAPI IContextMenu_InvokeCommand(LPCONTEXTMENU, LPCMINVOKECOMMANDINFO32);
static HRESULT WINAPI IContextMenu_GetCommandString(LPCONTEXTMENU , UINT32 ,UINT32 ,LPUINT32 ,LPSTR ,UINT32);
static HRESULT WINAPI IContextMenu_HandleMenuMsg(LPCONTEXTMENU, UINT32, WPARAM32, LPARAM);

BOOL32 IContextMenu_AllocPidlTable(LPCONTEXTMENU, DWORD);
void IContextMenu_FreePidlTable(LPCONTEXTMENU);
BOOL32 IContextMenu_CanRenameItems(LPCONTEXTMENU);
BOOL32 IContextMenu_FillPidlTable(LPCONTEXTMENU, LPCITEMIDLIST *, UINT32);

static struct IContextMenu_VTable cmvt = 
{	IContextMenu_QueryInterface,
	IContextMenu_AddRef,
	IContextMenu_Release,
	IContextMenu_QueryContextMenu,
	IContextMenu_InvokeCommand,
	IContextMenu_GetCommandString,
	IContextMenu_HandleMenuMsg,
	(void *) 0xdeadbabe	/* just paranoia */
};
/**************************************************************************
*  IContextMenu_QueryInterface
*/
static HRESULT WINAPI IContextMenu_QueryInterface(LPCONTEXTMENU this,REFIID riid, LPVOID *ppvObj)
{ char    xriid[50];
  WINE_StringFromCLSID((LPCLSID)riid,xriid);
  TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = (LPUNKNOWN)(LPCONTEXTMENU)this; 
  }
  else if(IsEqualIID(riid, &IID_IContextMenu))  /*IContextMenu*/
  { *ppvObj = (LPCONTEXTMENU)this;
  }   
  else if(IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
  { FIXME (shell,"-- LPSHELLEXTINIT pointer requested\n");
  }   

  if(*ppvObj)
  { (*(LPCONTEXTMENU *)ppvObj)->lpvtbl->fnAddRef(this);      
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
  TRACE(shell,"-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}   

/**************************************************************************
*  IContextMenu_AddRef
*/
static ULONG WINAPI IContextMenu_AddRef(LPCONTEXTMENU this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
	shell32_ObjCount++;
	return ++(this->ref);
}
/**************************************************************************
*  IContextMenu_Release
*/
static ULONG WINAPI IContextMenu_Release(LPCONTEXTMENU this)
{	TRACE(shell,"(%p)->()\n",this);

	shell32_ObjCount--;

	if (!--(this->ref)) 
	{ TRACE(shell," destroying IContextMenu(%p)\n",this);

	  if(this->pSFParent)
	    this->pSFParent->lpvtbl->fnRelease(this->pSFParent);

	  /*make sure the pidl is freed*/
	  if(this->aPidls)
	  { IContextMenu_FreePidlTable(this);
	  }

	  HeapFree(GetProcessHeap(),0,this);
	  return 0;
	}
	return this->ref;
}

/**************************************************************************
*   IContextMenu_Constructor()
*/
LPCONTEXTMENU IContextMenu_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST *aPidls, UINT32 uItemCount)
{	LPCONTEXTMENU cm;
	UINT32  u;
    
	cm = (LPCONTEXTMENU)HeapAlloc(GetProcessHeap(),0,sizeof(IContextMenu));
	cm->lpvtbl=&cmvt;
	cm->ref = 1;

	cm->pSFParent = pSFParent;
	if(cm->pSFParent)
	   cm->pSFParent->lpvtbl->fnAddRef(cm->pSFParent);

	cm->aPidls = NULL;

	IContextMenu_AllocPidlTable(cm, uItemCount);
    
	if(cm->aPidls)
	{ IContextMenu_FillPidlTable(cm, aPidls, uItemCount);
	}

	cm->bAllValues = 1;
	for(u = 0; u < uItemCount; u++)
	{ cm->bAllValues &= (_ILIsValue(aPidls[u]) ? 1 : 0);
	}
	TRACE(shell,"(%p)->()\n",cm);
	shell32_ObjCount++;
	return cm;
}
/**************************************************************************
*  ICM_InsertItem()
*/ 
void WINAPI _InsertMenuItem (HMENU32 hmenu, UINT32 indexMenu, BOOL32 fByPosition, 
			UINT32 wID, UINT32 fType, LPSTR dwTypeData, UINT32 fState)
{	MENUITEMINFO32A	mii;

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	if (fType == MFT_SEPARATOR)
	{ mii.fMask = MIIM_ID | MIIM_TYPE;
	}
	else
	{ mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.dwTypeData = dwTypeData;
	  mii.fState = MFS_ENABLED | MFS_DEFAULT;
	}
	mii.wID = wID;
	mii.fType = fType;
	InsertMenuItem32A( hmenu, indexMenu, fByPosition, &mii);
}
/**************************************************************************
* IContextMenu_QueryContextMenu()
*/

static HRESULT WINAPI IContextMenu_QueryContextMenu( LPCONTEXTMENU this, HMENU32 hmenu, UINT32 indexMenu,
							UINT32 idCmdFirst,UINT32 idCmdLast,UINT32 uFlags)
{	BOOL32	fExplore ;

	TRACE(shell,"(%p)->(hmenu=%x indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",this, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	if(!(CMF_DEFAULTONLY & uFlags))
	{ if(!this->bAllValues)	
	  { /* folder menu */
	    fExplore = uFlags & CMF_EXPLORE;
	    if(fExplore) 
	    { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED|MFS_DEFAULT);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED);
	    }
	    else
            { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED|MFS_DEFAULT);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED);
            }

            if(uFlags & CMF_CANRENAME)
            { _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_RENAME, MFT_STRING, "&Rename", (IContextMenu_CanRenameItems(this) ? MFS_ENABLED : MFS_DISABLED));
	    }
	  }
	  else	/* file menu */
	  { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED|MFS_DEFAULT);
            if(uFlags & CMF_CANRENAME)
            { _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_RENAME, MFT_STRING, "&Rename", (IContextMenu_CanRenameItems(this) ? MFS_ENABLED : MFS_DISABLED));
	    }
	  }
	  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (IDM_LAST + 1));
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* IContextMenu_InvokeCommand()
*/
static HRESULT WINAPI IContextMenu_InvokeCommand(LPCONTEXTMENU this, LPCMINVOKECOMMANDINFO32 lpcmi)
{	LPITEMIDLIST	pidlTemp,pidlFQ;
	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;
	HWND32	hWndSV;
	SHELLEXECUTEINFO32A	sei;
	int   i;

 	TRACE(shell,"(%p)->(invcom=%p verb=%p wnd=%x)\n",this,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);    

	if(HIWORD(lpcmi->lpVerb))
	{ /* get the active IShellView */
	  lpSB = (LPSHELLBROWSER)SendMessage32A(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0);
	  IShellBrowser_QueryActiveShellView(lpSB, &lpSV);	/* does AddRef() on lpSV */
	  lpSV->lpvtbl->fnGetWindow(lpSV, &hWndSV);
	  
	  /* these verbs are used by the filedialogs*/
	  TRACE(shell,"%s\n",lpcmi->lpVerb);
	  if (! strcmp(lpcmi->lpVerb,CMDSTR_NEWFOLDER))
	  { FIXME(shell,"%s not implemented\n",lpcmi->lpVerb);
	  }
	  else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWLIST))
	  { SendMessage32A(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW,0),0 );
	  }
	  else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWDETAILS))
	  { SendMessage32A(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW,0),0 );
	  } 
	  else
	  { FIXME(shell,"please report: unknown verb %s\n",lpcmi->lpVerb);
	  }
	  lpSV->lpvtbl->fnRelease(lpSV);
	  return NOERROR;
	}

	if(LOWORD(lpcmi->lpVerb) > IDM_LAST)
	  return E_INVALIDARG;

	switch(LOWORD(lpcmi->lpVerb))
	{ case IDM_EXPLORE:
	  case IDM_OPEN:
            /* Find the first item in the list that is not a value. These commands 
      	    should never be invoked if there isn't at least one folder item in the list.*/

	    for(i = 0; this->aPidls[i]; i++)
	    { if(!_ILIsValue(this->aPidls[i]))
                break;
	    }
      
	    pidlTemp = ILCombine(this->pSFParent->mpidl, this->aPidls[i]);
	    pidlFQ = ILCombine(this->pSFParent->pMyPidl, pidlTemp);
	    SHFree(pidlTemp);
      
	    ZeroMemory(&sei, sizeof(sei));
	    sei.cbSize = sizeof(sei);
	    sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
	    sei.lpIDList = pidlFQ;
	    sei.lpClass = "folder";
	    sei.hwnd = lpcmi->hwnd;
	    sei.nShow = SW_SHOWNORMAL;
      
	    if(LOWORD(lpcmi->lpVerb) == IDM_EXPLORE)
	    { sei.lpVerb = "explore";
	    }
	    else
	    { sei.lpVerb = "open";
	    }
	    ShellExecuteEx32A(&sei);
	    SHFree(pidlFQ);
	    break;
		
	  case IDM_RENAME:
	    MessageBeep32(MB_OK);
	    /*handle rename for the view here*/
	    break;	    
	}
	return NOERROR;
}

/**************************************************************************
*  IContextMenu_GetCommandString()
*/
static HRESULT WINAPI IContextMenu_GetCommandString( LPCONTEXTMENU this, UINT32 idCommand,
		UINT32 uFlags,LPUINT32 lpReserved,LPSTR lpszName,UINT32 uMaxNameLen)
{	HRESULT  hr = E_INVALIDARG;

	TRACE(shell,"(%p)->(idcom=%x flags=%x %p name=%p len=%x)\n",this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	switch(uFlags)
	{ case GCS_HELPTEXT:
	    hr = E_NOTIMPL;
	    break;
   
	  case GCS_VERBA:
	    switch(idCommand)
	    { case IDM_RENAME:
	        strcpy((LPSTR)lpszName, "rename");
	        hr = NOERROR;
	        break;
	    }
	    break;

	     /* NT 4.0 with IE 3.0x or no IE will always call this with GCS_VERBW. In this 
	     case, you need to do the lstrcpyW to the pointer passed.*/
	  case GCS_VERBW:
	    switch(idCommand)
	    { case IDM_RENAME:
	        lstrcpyAtoW((LPWSTR)lpszName, "rename");
	        hr = NOERROR;
	        break;
	    }
	    break;

	  case GCS_VALIDATE:
	    hr = NOERROR;
	    break;
	}
	TRACE(shell,"-- (%p)->(name=%s)\n",this, lpszName);
	return hr;
}
/**************************************************************************
* IContextMenu_HandleMenuMsg()
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI IContextMenu_HandleMenuMsg(LPCONTEXTMENU this, UINT32 uMsg,WPARAM32 wParam,LPARAM lParam)
{	TRACE(shell,"(%p)->(msg=%x wp=%x lp=%lx)\n",this, uMsg, wParam, lParam);
	return E_NOTIMPL;
}
/**************************************************************************
*  IContextMenu_AllocPidlTable()
*/
BOOL32 IContextMenu_AllocPidlTable(LPCONTEXTMENU this, DWORD dwEntries)
{	TRACE(shell,"(%p)->(entrys=%lu)\n",this, dwEntries);

	/*add one for NULL terminator */
	dwEntries++;

	this->aPidls = (LPITEMIDLIST*)SHAlloc(dwEntries * sizeof(LPITEMIDLIST));

	if(this->aPidls)
	{ ZeroMemory(this->aPidls, dwEntries * sizeof(LPITEMIDLIST));	/*set all of the entries to NULL*/
	}
	return (this->aPidls != NULL);
}

/**************************************************************************
* IContextMenu_FreePidlTable()
*/
void IContextMenu_FreePidlTable(LPCONTEXTMENU this)
{	int   i;

	TRACE(shell,"(%p)->()\n",this);

	if(this->aPidls)
	{ for(i = 0; this->aPidls[i]; i++)
	  { SHFree(this->aPidls[i]);
	  }
   
	  SHFree(this->aPidls);
	  this->aPidls = NULL;
	}
}

/**************************************************************************
* IContextMenu_FillPidlTable()
*/
BOOL32 IContextMenu_FillPidlTable(LPCONTEXTMENU this, LPCITEMIDLIST *aPidls, UINT32 uItemCount)
{   UINT32  i;
	TRACE(shell,"(%p)->(apidl=%p count=%u)\n",this, aPidls, uItemCount);
	if(this->aPidls)
	{ for(i = 0; i < uItemCount; i++)
	  { this->aPidls[i] = ILClone(aPidls[i]);
	  }
	  return TRUE;
 	}
	return FALSE;
}

/**************************************************************************
* IContextMenu_CanRenameItems()
*/
BOOL32 IContextMenu_CanRenameItems(LPCONTEXTMENU this)
{	UINT32  i;
	DWORD dwAttributes;

	TRACE(shell,"(%p)->()\n",this);

	if(this->aPidls)
	{ for(i = 0; this->aPidls[i]; i++){} /*get the number of items assigned to this object*/
	    if(i > 1)	/*you can't rename more than one item at a time*/
	    { return FALSE;
	    }
	    dwAttributes = SFGAO_CANRENAME;
	    this->pSFParent->lpvtbl->fnGetAttributesOf(this->pSFParent, i,
        						 (LPCITEMIDLIST*)this->aPidls, &dwAttributes);
      
	    return dwAttributes & SFGAO_CANRENAME;
	}
	return FALSE;
}

