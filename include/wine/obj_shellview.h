/************************************************************
 *    IShellView
 */

#ifndef __WINE_WINE_OBJ_ISHELLVIEW_H
#define __WINE_WINE_OBJ_ISHELLVIEW_H

#include "winbase.h"
#include "winuser.h"
#include "wine/obj_base.h"
#include "wine/obj_inplace.h"
#include "wine/obj_shellfolder.h"
#include "prsht.h"	/* LPFNADDPROPSHEETPAGE */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/****************************************************************************
 * IShellBrowser is here defined because of a cyclic dependance between 
 * IShellBrowser and IShellView
 */
typedef struct IShellBrowser IShellBrowser, *LPSHELLBROWSER;

DEFINE_SHLGUID(IID_IShellView,          0x000214E3L, 0, 0);
typedef struct IShellView IShellView, *LPSHELLVIEW;

/* shellview select item flags*/
#define SVSI_DESELECT   0x0000
#define SVSI_SELECT     0x0001
#define SVSI_EDIT       0x0003  /* includes select */
#define SVSI_DESELECTOTHERS 0x0004
#define SVSI_ENSUREVISIBLE  0x0008
#define SVSI_FOCUSED        0x0010

/* shellview get item object flags */
#define SVGIO_BACKGROUND    0x00000000
#define SVGIO_SELECTION     0x00000001
#define SVGIO_ALLVIEW       0x00000002

/* The explorer dispatches WM_COMMAND messages based on the range of
 command/menuitem IDs. All the IDs of menuitems that the view (right
 pane) inserts must be in FCIDM_SHVIEWFIRST/LAST (otherwise, the explorer
 won't dispatch them). The view should not deal with any menuitems
 in FCIDM_BROWSERFIRST/LAST (otherwise, it won't work with the future
 version of the shell).

  FCIDM_SHVIEWFIRST/LAST      for the right pane (IShellView)
  FCIDM_BROWSERFIRST/LAST     for the explorer frame (IShellBrowser)
  FCIDM_GLOBAL/LAST           for the explorer's submenu IDs
*/
#define FCIDM_SHVIEWFIRST	0x0000
/* undocumented */
#define FCIDM_SHVIEW_ARRANGE	0x7001
#define FCIDM_SHVIEW_DELETE	0x7011
#define FCIDM_SHVIEW_PROPERTIES	0x7013
#define FCIDM_SHVIEW_CUT	0x7018
#define FCIDM_SHVIEW_COPY	0x7019
#define FCIDM_SHVIEW_INSERT	0x701A
#define FCIDM_SHVIEW_UNDO	0x701B
#define FCIDM_SHVIEW_INSERTLINK	0x701C
#define FCIDM_SHVIEW_SELECTALL	0x7021
#define FCIDM_SHVIEW_INVERTSELECTION	0x7022
#define FCIDM_SHVIEW_BIGICON	0x7029
#define FCIDM_SHVIEW_SMALLICON	0x702A
#define FCIDM_SHVIEW_LISTVIEW	0x702B	
#define FCIDM_SHVIEW_REPORTVIEW	0x702C
#define FCIDM_SHVIEW_AUTOARRANGE	0x7031  
#define FCIDM_SHVIEW_SNAPTOGRID	0x7032
#define FCIDM_SHVIEW_HELP	0x7041

#define FCIDM_SHVIEWLAST	0x7fff
#define FCIDM_BROWSERFIRST	0xA000
/* undocumented toolbar items from stddlg's*/
#define FCIDM_TB_SMALLICON	0xA003
#define FCIDM_TB_REPORTVIEW	0xA004

#define FCIDM_BROWSERLAST	0xbf00
#define FCIDM_GLOBALFIRST	0x8000
#define FCIDM_GLOBALLAST	0x9fff

/*
* Global submenu IDs and separator IDs
*/
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)
#define FCIDM_MENU_EXPLORE          (FCIDM_GLOBALFIRST+0x0150)
#define FCIDM_MENU_FAVORITES        (FCIDM_GLOBALFIRST+0x0170)

/* control IDs known to the view */
#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)

/* uState values for IShellView::UIActivate */
typedef enum 
{ SVUIA_DEACTIVATE       = 0,
  SVUIA_ACTIVATE_NOFOCUS = 1,
  SVUIA_ACTIVATE_FOCUS   = 2,
  SVUIA_INPLACEACTIVATE  = 3          /* new flag for IShellView2 */
} SVUIA_STATUS;

#define ICOM_INTERFACE IShellView
#define IShellView_METHODS \
	ICOM_METHOD1(HRESULT, TranslateAccelerator, LPMSG, lpmsg) \
	ICOM_METHOD1(HRESULT, EnableModeless, BOOL, fEnable) \
	ICOM_METHOD1(HRESULT, UIActivate, UINT, uState) \
	ICOM_METHOD(HRESULT, Refresh) \
	ICOM_METHOD5(HRESULT, CreateViewWindow, IShellView*, lpPrevView, LPCFOLDERSETTINGS, lpfs, IShellBrowser*, psb, RECT*, prcView, HWND*, phWnd) \
	ICOM_METHOD(HRESULT, DestroyViewWindow) \
	ICOM_METHOD1(HRESULT, GetCurrentInfo, LPFOLDERSETTINGS, lpfs) \
	ICOM_METHOD3(HRESULT, AddPropertySheetPages, DWORD, dwReserved, LPFNADDPROPSHEETPAGE, lpfn, LPARAM, lparam) \
	ICOM_METHOD (HRESULT, SaveViewState) \
	ICOM_METHOD2(HRESULT, SelectItem, LPCITEMIDLIST, pidlItem, UINT, uFlags) \
	ICOM_METHOD3(HRESULT, GetItemObject, UINT, uItem, REFIID, riid, LPVOID*, ppv)
#define IShellView_IMETHODS \
	IOleWindow_IMETHODS \
	IShellView_METHODS
ICOM_DEFINE(IShellView,IOleWindow)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
#define IShellView_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IShellView_AddRef(p)			ICOM_CALL(AddRef,p)
#define IShellView_Release(p)			ICOM_CALL(Release,p)
#define IShellView_GetWindow(p,a)		ICOM_CALL1(GetWindow,p,a)
#define IShellView_ContextSensitiveHelp(p,a)	ICOM_CALL1(ContextSensitiveHelp,p,a)
#define IShellView_TranslateAccelerator(p,a)	ICOM_CALL1(TranslateAccelerator,p,a)
#define IShellView_EnableModeless(p,a)		ICOM_CALL1(EnableModeless,p,a)
#define IShellView_UIActivate(p,a)		ICOM_CALL1(UIActivate,p,a)
#define IShellView_Refresh(p)			ICOM_CALL(Refresh,p)
#define IShellView_CreateViewWindow(p,a,b,c,d,e)	ICOM_CALL5(CreateViewWindow,p,a,b,c,d,e)
#define IShellView_DestroyViewWindow(p)		ICOM_CALL(DestroyViewWindow,p)
#define IShellView_GetCurrentInfo(p,a)		ICOM_CALL1(GetCurrentInfo,p,a)
#define IShellView_AddPropertySheetPages(p,a,b,c)	ICOM_CALL3(AddPropertySheetPages,p,a,b,c)
#define IShellView_SaveViewState(p)		ICOM_CALL(SaveViewState,p)
#define IShellView_SelectItem(p,a,b)		ICOM_CALL2(SelectItem,p,a,b)
#define IShellView_GetItemObject(p,a,b,c)	ICOM_CALL3(GetItemObject,p,a,b,c)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_ISHELLVIEW_H */
