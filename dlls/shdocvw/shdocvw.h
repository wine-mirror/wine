/*
 * Header includes for shdocvw.dll
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_SHDOCVW_H
#define __WINE_SHDOCVW_H

#define COM_NO_WINDOWS_H
/* FIXME: Is there a better way to deal with all these includes? */
#include "wingdi.h"
#include "winbase.h"
#include "winuser.h"

#include "ole2.h"
#include "olectl.h"
#include "shlobj.h"
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
