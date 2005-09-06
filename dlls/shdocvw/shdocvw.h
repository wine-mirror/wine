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
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "olectl.h"
#include "shlobj.h"
#include "exdisp.h"

/**********************************************************************
 * IClassFactory declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl *lpVtbl;
    LONG ref;
} IClassFactoryImpl;

extern IClassFactoryImpl SHDOCVW_ClassFactory;


/**********************************************************************
 * WebBrowser declaration for SHDOCVW.DLL
 */
typedef struct {
    const IWebBrowserVtbl   *lpWebBrowserVtbl;
    const IOleObjectVtbl    *lpOleObjectVtbl;

    LONG ref;
} WebBrowser;

#define WEBBROWSER(x)   ((IWebBrowser*)     &(x)->lpWebBrowserVtbl)
#define OLEOBJ(x)       ((IOleObject*)      &(x)->lpOleObjectVtbl)

void WebBrowser_OleObject_Init(WebBrowser*);

HRESULT WebBrowser_Create(IUnknown*,REFIID,void**);

/**********************************************************************
 * IProvideClassInfo declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IProvideClassInfoVtbl *lpVtbl;
    LONG ref;
} IProvideClassInfoImpl;

extern IProvideClassInfoImpl SHDOCVW_ProvideClassInfo;


/**********************************************************************
 * IProvideClassInfo2 declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IProvideClassInfo2Vtbl *lpVtbl;
    LONG ref;
} IProvideClassInfo2Impl;

extern IProvideClassInfo2Impl SHDOCVW_ProvideClassInfo2;


/**********************************************************************
 * IPersistStorage declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IPersistStorageVtbl *lpVtbl;
    LONG ref;
} IPersistStorageImpl;

extern IPersistStorageImpl SHDOCVW_PersistStorage;


/**********************************************************************
 * IPersistStreamInit declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IPersistStreamInitVtbl *lpVtbl;
    LONG ref;
} IPersistStreamInitImpl;

extern IPersistStreamInitImpl SHDOCVW_PersistStreamInit;


/**********************************************************************
 * IQuickActivate declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IQuickActivateVtbl *lpVtbl;
    LONG ref;
} IQuickActivateImpl;

extern IQuickActivateImpl SHDOCVW_QuickActivate;


/**********************************************************************
 * IConnectionPointContainer declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IConnectionPointContainerVtbl *lpVtbl;
    LONG ref;
} IConnectionPointContainerImpl;

extern IConnectionPointContainerImpl SHDOCVW_ConnectionPointContainer;


/**********************************************************************
 * IConnectionPoint declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IConnectionPointVtbl *lpVtbl;
    LONG ref;
} IConnectionPointImpl;

/**********************************************************************
 * IOleInPlaceObject declaration for SHDOCVW.DLL
 */
typedef struct
{
        /* IUnknown fields */
        const IOleInPlaceObjectVtbl *lpVtbl;
            DWORD ref;
} IOleInPlaceObjectImpl;

extern IOleInPlaceObjectImpl SHDOCVW_OleInPlaceObject;

/**********************************************************************
 * IOleControl declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IOleControlVtbl *lpVtbl;
    DWORD ref;
} IOleControlImpl;

extern IOleControlImpl SHDOCVW_OleControl;

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

/**********************************************************************
 * Dll lifetime tracking declaration for shdocvw.dll
 */
extern LONG SHDOCVW_refCount;
static inline void SHDOCVW_LockModule(void) { InterlockedIncrement( &SHDOCVW_refCount ); }
static inline void SHDOCVW_UnlockModule(void) { InterlockedDecrement( &SHDOCVW_refCount ); }

#endif /* __WINE_SHDOCVW_H */
