#ifndef _WINE_IFMACROS_
#define _WINE_IFMACROS_

#include "shlobj.h"

#define IShellBrowser_QueryInterface(p,a,b)	(p)->lpvtbl->fnQueryInterface(p,a,b)
#define IShellBrowser_AddRef(p)			(p)->lpvtbl->fnAddRef(p)
#define IShellBrowser_Release(p)		(p)->lpvtbl->fnRelease(p)
#define IShellBrowser_GetWindow(p,a)		(p)->lpvtbl->fnGetWindow(p,a)
#define IShellBrowser_ContextSensitiveHelp(p,a)	(p)->lpvtbl->fnContextSensitiveHelp(p,a)
#define IShellBrowser_InsertMenusSB(p,a,b)	(p)->lpvtbl->fnInsertMenusSB(p,a,b)
#define IShellBrowser_SetMenuSB(p,a,b,c)	(p)->lpvtbl->fnSetMenuSB(p,a,b,c)
#define IShellBrowser_RemoveMenusSB(p,a)	(p)->lpvtbl->fnRemoveMenusSB(p,a)
#define IShellBrowser_SetStatusTextSB(p,a)	(p)->lpvtbl->fnSetStatusTextSB(p,a)
#define IShellBrowser_EnableModelessSB(p,a)	(p)->lpvtbl->fnEnableModelessSB(p,a)
#define IShellBrowser_TranslateAcceleratorSB(p,a,b)	(p)->lpvtbl->fnTranslateAcceleratorSB(p,a,b)
#define IShellBrowser_BrowseObject(p,a,b)	(p)->lpvtbl->fnBrowseObject(p,a,b)
#define IShellBrowser_GetViewStateStream(p,a,b)	(p)->lpvtbl->fnGetViewStateStream(p,a,b)
#define IShellBrowser_GetControlWindow(p,a,b)	(p)->lpvtbl->fnGetControlWindow(p,a,b)
#define IShellBrowser_SendControlMsg(p,a,b,c,d,e)	(p)->lpvtbl->fnSendControlMsg(p,a,b,c,d,e)
#define IShellBrowser_QueryActiveShellView(p,a)	(p)->lpvtbl->fnQueryActiveShellView(p,a)
#define IShellBrowser_OnViewWindowActive(p,a)	(p)->lpvtbl->fnOnViewWindowActive(p,a)
#define IShellBrowser_SetToolbarItems(p,a,b,c)	(p)->lpvtbl->fnSetToolbarItems(p,a,b,c)

#endif
