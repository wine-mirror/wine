/*
 * Header includes for shdocvw.dll
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#ifndef __WINE_SHDOCVW_H
#define __WINE_SHDOCVW_H

/* FIXME: Is there a better way to deal with all these includes? */
#include "wingdi.h"
#include "winbase.h"
#include "winuser.h"

#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_misc.h"
#include "wine/obj_moniker.h"
#include "wine/obj_inplace.h"
#include "wine/obj_dataobject.h"
#include "wine/obj_oleobj.h"
#include "wine/obj_oleaut.h"
#include "wine/obj_olefont.h"
#include "wine/obj_dragdrop.h"
#include "wine/obj_oleview.h"
#include "wine/obj_control.h"
#include "wine/obj_connection.h"
#include "wine/obj_property.h"
#include "wine/obj_oleundo.h"
#include "wine/obj_webbrowser.h"

/**********************************************************************
 * IClassFactory declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD ref;
} IClassFactoryImpl;

extern IClassFactoryImpl SHDOCVW_ClassFactory;


/**********************************************************************
 * IOleObject declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IOleObject);
    DWORD ref;
} IOleObjectImpl;

extern IOleObjectImpl SHDOCVW_OleObject;


/**********************************************************************
 * IOleInPlaceObject declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IOleInPlaceObject);
    DWORD ref;
} IOleInPlaceObjectImpl;

extern IOleInPlaceObjectImpl SHDOCVW_OleInPlaceObject;


/**********************************************************************
 * IOleControl declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IOleControl);
    DWORD ref;
} IOleControlImpl;

extern IOleControlImpl SHDOCVW_OleControl;


/**********************************************************************
 * IWebBrowser declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IWebBrowser);
    DWORD ref;
} IWebBrowserImpl;

extern IWebBrowserImpl SHDOCVW_WebBrowser;


/**********************************************************************
 * IProvideClassInfo declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IProvideClassInfo);
    DWORD ref;
} IProvideClassInfoImpl;

extern IProvideClassInfoImpl SHDOCVW_ProvideClassInfo;


/**********************************************************************
 * IProvideClassInfo2 declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IProvideClassInfo2);
    DWORD ref;
} IProvideClassInfo2Impl;

extern IProvideClassInfo2Impl SHDOCVW_ProvideClassInfo2;


/**********************************************************************
 * IPersistStorage declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IPersistStorage);
    DWORD ref;
} IPersistStorageImpl;

extern IPersistStorageImpl SHDOCVW_PersistStorage;


/**********************************************************************
 * IPersistStreamInit declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IPersistStreamInit);
    DWORD ref;
} IPersistStreamInitImpl;

extern IPersistStreamInitImpl SHDOCVW_PersistStreamInit;


/**********************************************************************
 * IQuickActivate declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IQuickActivate);
    DWORD ref;
} IQuickActivateImpl;

extern IQuickActivateImpl SHDOCVW_QuickActivate;


/**********************************************************************
 * IConnectionPointContainer declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IConnectionPointContainer);
    DWORD ref;
} IConnectionPointContainerImpl;

extern IConnectionPointContainerImpl SHDOCVW_ConnectionPointContainer;


/**********************************************************************
 * IConnectionPoint declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IConnectionPoint);
    DWORD ref;
} IConnectionPointImpl;

extern IConnectionPointImpl SHDOCVW_ConnectionPoint;


/* Other stuff.. */

DEFINE_GUID(IID_INotifyDBEvents,
0xdb526cc0, 0xd188, 0x11cd, 0xad, 0x48, 0x0, 0xaa, 0x0, 0x3c, 0x9c, 0xb6);

#endif /* __WINE_SHDOCVW_H */
