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

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct 
{ ICOM_VTABLE(IContextMenu)* lpvtbl;
  DWORD     ref;
  LPSHELLFOLDER pSFParent;
  LPITEMIDLIST  *aPidls;
  BOOL32    bAllValues;
} IContextMenuImpl;

static HRESULT WINAPI IContextMenu_fnQueryInterface(IContextMenu *,REFIID , LPVOID *);
static ULONG WINAPI IContextMenu_fnAddRef(IContextMenu *);
static ULONG WINAPI IContextMenu_fnRelease(IContextMenu *);
static HRESULT WINAPI IContextMenu_fnQueryContextMenu(IContextMenu *, HMENU32 ,UINT32 ,UINT32 ,UINT32 ,UINT32);
static HRESULT WINAPI IContextMenu_fnInvokeCommand(IContextMenu *, LPCMINVOKECOMMANDINFO32);
static HRESULT WINAPI IContextMenu_fnGetCommandString(IContextMenu *, UINT32 ,UINT32 ,LPUINT32 ,LPSTR ,UINT32);
static HRESULT WINAPI IContextMenu_fnHandleMenuMsg(IContextMenu *, UINT32, WPARAM32, LPARAM);

/* Private Methods */
BOOL32 IContextMenu_AllocPidlTable(IContextMenuImpl*, DWORD);
void IContextMenu_FreePidlTable(IContextMenuImpl*);
BOOL32 IContextMenu_CanRenameItems(IContextMenuImpl*);
BOOL32 IContextMenu_FillPidlTable(IContextMenuImpl*, LPCITEMIDLIST *, UINT32);

/**************************************************************************
* IContextMenu VTable
* 
*/
static struct ICOM_VTABLE(IContextMenu) cmvt = {	
	IContextMenu_fnQueryInterface,
	IContextMenu_fnAddRef,
	IContextMenu_fnRelease,
	IContextMenu_fnQueryContextMenu,
	IContextMenu_fnInvokeCommand,
	IContextMenu_fnGetCommandString,
	IContextMenu_fnHandleMenuMsg,
	(void *) 0xdeadbabe	/* just paranoia */
};

/**************************************************************************
*  IContextMenu_fnQueryInterface
*/
static HRESULT WINAPI IContextMenu_fnQueryInterface(IContextMenu *iface, REFIID riid, LPVOID *ppvObj)
{ ICOM_THIS(IContextMenuImpl, iface);
	char    xriid[50];
  WINE_StringFromCLSID((LPCLSID)riid,xriid);
  TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = This; 
  }
  else if(IsEqualIID(riid, &IID_IContextMenu))  /*IContextMenu*/
  { *ppvObj = This;
  }   
  else if(IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
  { FIXME (shell,"-- LPSHELLEXTINIT pointer requested\n");
  }   

  if(*ppvObj)
  { 
    (*(IContextMenuImpl**)ppvObj)->lpvtbl->fnAddRef(iface);      
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
  TRACE(shell,"-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}   

/**************************************************************************
*  IContextMenu_fnAddRef
*/
static ULONG WINAPI IContextMenu_fnAddRef(IContextMenu *iface)
{ ICOM_THIS(IContextMenuImpl, iface);
	TRACE(shell,"(%p)->(count=%lu)\n",This,(This->ref)+1);
	shell32_ObjCount++;
	return ++(This->ref);
}
/**************************************************************************
*  IContextMenu_fnRelease
*/
static ULONG WINAPI IContextMenu_fnRelease(IContextMenu *iface)
{	ICOM_THIS(IContextMenuImpl, iface);
	TRACE(shell,"(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(shell," destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    This->pSFParent->lpvtbl->fnRelease(This->pSFParent);

	  /*make sure the pidl is freed*/
	  if(This->aPidls)
	  { IContextMenu_FreePidlTable(This);
	  }

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**************************************************************************
*   IContextMenu_Constructor()
*/
IContextMenu *IContextMenu_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST *aPidls, UINT32 uItemCount)
{	IContextMenuImpl* cm;
	UINT32  u;
  FIXME(shell, "HELLO age\n")  ;
	cm = (IContextMenuImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IContextMenuImpl));
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
	return (IContextMenu*)cm;
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
* IContextMenu_fnQueryContextMenu()
*/

static HRESULT WINAPI IContextMenu_fnQueryContextMenu(IContextMenu *iface, HMENU32 hmenu, UINT32 indexMenu,
							UINT32 idCmdFirst,UINT32 idCmdLast,UINT32 uFlags)
{	ICOM_THIS(IContextMenuImpl, iface);
	BOOL32	fExplore ;

	TRACE(shell,"(%p)->(hmenu=%x indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	if(!(CMF_DEFAULTONLY & uFlags))
	{ if(!This->bAllValues)	
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
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_RENAME, MFT_STRING, "&Rename", (IContextMenu_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED));
	    }
	  }
	  else	/* file menu */
	  { _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_OPEN, MFT_STRING, "&Open", MFS_ENABLED|MFS_DEFAULT);
            if(uFlags & CMF_CANRENAME)
            { _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, idCmdFirst+IDM_RENAME, MFT_STRING, "&Rename", (IContextMenu_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED));
	    }
	  }
	  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (IDM_LAST + 1));
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* IContextMenu_fnInvokeCommand()
*/
static HRESULT WINAPI IContextMenu_fnInvokeCommand(IContextMenu *iface, LPCMINVOKECOMMANDINFO32 lpcmi)
{	ICOM_THIS(IContextMenuImpl, iface);
	LPITEMIDLIST	pidlTemp,pidlFQ;
	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;
	HWND32	hWndSV;
	SHELLEXECUTEINFO32A	sei;
	int   i;

 	TRACE(shell,"(%p)->(invcom=%p verb=%p wnd=%x)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);    

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

	    for(i = 0; This->aPidls[i]; i++)
	    { if(!_ILIsValue(This->aPidls[i]))
                break;
	    }
      
	    pidlTemp = ILCombine(This->pSFParent->mpidl, This->aPidls[i]);
	    pidlFQ = ILCombine(This->pSFParent->pMyPidl, pidlTemp);
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
*  IContextMenu_fnGetCommandString()
*/
static HRESULT WINAPI IContextMenu_fnGetCommandString(IContextMenu *iface, UINT32 idCommand,
		UINT32 uFlags,LPUINT32 lpReserved,LPSTR lpszName,UINT32 uMaxNameLen)
{	ICOM_THIS(IContextMenuImpl, iface);
	HRESULT  hr = E_INVALIDARG;

	TRACE(shell,"(%p)->(idcom=%x flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

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

	     /* NT 4.0 with IE 3.0x or no IE will always call This with GCS_VERBW. In This 
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
	TRACE(shell,"-- (%p)->(name=%s)\n",This, lpszName);
	return hr;
}

/**************************************************************************
* IContextMenu_fnHandleMenuMsg()
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI IContextMenu_fnHandleMenuMsg(IContextMenu *iface, UINT32 uMsg,WPARAM32 wParam,LPARAM lParam)
{	ICOM_THIS(IContextMenuImpl, iface);
	TRACE(shell,"(%p)->(msg=%x wp=%x lp=%lx)\n",This, uMsg, wParam, lParam);
	return E_NOTIMPL;
}



/**************************************************************************
*  IContextMenu_AllocPidlTable()
*/
BOOL32 IContextMenu_AllocPidlTable(IContextMenuImpl *This, DWORD dwEntries)
{	TRACE(shell,"(%p)->(entrys=%lu)\n",This, dwEntries);

	/*add one for NULL terminator */
	dwEntries++;

	This->aPidls = (LPITEMIDLIST*)SHAlloc(dwEntries * sizeof(LPITEMIDLIST));

	if(This->aPidls)
	{ ZeroMemory(This->aPidls, dwEntries * sizeof(LPITEMIDLIST));	/*set all of the entries to NULL*/
	}
	return (This->aPidls != NULL);
}

/**************************************************************************
* IContextMenu_FreePidlTable()
*/
void IContextMenu_FreePidlTable(IContextMenuImpl *This)
{	int   i;

	TRACE(shell,"(%p)->()\n",This);

	if(This->aPidls)
	{ for(i = 0; This->aPidls[i]; i++)
	  { SHFree(This->aPidls[i]);
	  }
   
	  SHFree(This->aPidls);
	  This->aPidls = NULL;
	}
}

/**************************************************************************
* IContextMenu_FillPidlTable()
*/
BOOL32 IContextMenu_FillPidlTable(IContextMenuImpl *This, LPCITEMIDLIST *aPidls, UINT32 uItemCount)
{   UINT32  i;
	TRACE(shell,"(%p)->(apidl=%p count=%u)\n",This, aPidls, uItemCount);
	if(This->aPidls)
	{ for(i = 0; i < uItemCount; i++)
	  { This->aPidls[i] = ILClone(aPidls[i]);
	  }
	  return TRUE;
 	}
	return FALSE;
}

/**************************************************************************
* IContextMenu_CanRenameItems()
*/
BOOL32 IContextMenu_CanRenameItems(IContextMenuImpl *This)
{	UINT32  i;
	DWORD dwAttributes;

	TRACE(shell,"(%p)->()\n",This);

	if(This->aPidls)
	{ for(i = 0; This->aPidls[i]; i++){} /*get the number of items assigned to This object*/
	    if(i > 1)	/*you can't rename more than one item at a time*/
	    { return FALSE;
	    }
	    dwAttributes = SFGAO_CANRENAME;
	    This->pSFParent->lpvtbl->fnGetAttributesOf(This->pSFParent, i,
        						 (LPCITEMIDLIST*)This->aPidls, &dwAttributes);
      
	    return dwAttributes & SFGAO_CANRENAME;
	}
	return FALSE;
}

