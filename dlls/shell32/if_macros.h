#ifndef _WINE_IFMACROS_
#define _WINE_IFMACROS_

#include "shlobj.h"

/*** IUnknown methods ***/
#define IShellBrowser_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define IShellBrowser_AddRef(p)                     ICOM_CALL (AddRef,p)
#define IShellBrowser_Release(p)                    ICOM_CALL (Release,p)
/*** IShellBrowser methods ***/
#define IShellBrowser_GetWindow(p,a)                ICOM_CALL1(GetWindow,p,a)
#define IShellBrowser_ContextSensitiveHelp(p,a)     ICOM_CALL1(ContextSensitiveHelp,p,a)
#define IShellBrowser_InsertMenusSB(p,a,b)          ICOM_CALL2(InsertMenusSB,p,a,b)
#define IShellBrowser_SetMenuSB(p,a,b,c)            ICOM_CALL3(SetMenuSB,p,a,b,c)
#define IShellBrowser_RemoveMenusSB(p,a)            ICOM_CALL1(RemoveMenusSB,p,a)
#define IShellBrowser_SetStatusTextSB(p,a)          ICOM_CALL1(SetStatusTextSB,p,a)
#define IShellBrowser_EnableModelessSB(p,a)         ICOM_CALL1(EnableModelessSB,p,a)
#define IShellBrowser_TranslateAcceleratorSB(p,a,b) ICOM_CALL2(TranslateAcceleratorSB,p,a,b)
#define IShellBrowser_BrowseObject(p,a,b)           ICOM_CALL2(BrowseObject,p,a,b)
#define IShellBrowser_GetViewStateStream(p,a,b)     ICOM_CALL2(GetViewStateStream,p,a,b)
#define IShellBrowser_GetControlWindow(p,a,b)       ICOM_CALL2(GetControlWindow,p,a,b)
#define IShellBrowser_SendControlMsg(p,a,b,c,d,e)   ICOM_CALL5(SendControlMsg,p,a,b,c,d,e)
#define IShellBrowser_QueryActiveShellView(p,a)     ICOM_CALL1(QueryActiveShellView,p,a)
#define IShellBrowser_OnViewWindowActive(p,a)       ICOM_CALL1(OnViewWindowActive,p,a)
#define IShellBrowser_SetToolbarItems(p,a,b,c)      ICOM_CALL3(SetToolbarItems,p,a,b,c)

#endif
