/*
 * Defines the COM interfaces and APIs related to IServiceProvider
 *
 * Depends on 'obj_base.h'.
 *
 * Copyright (C) 2000 Juergen Schmied
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

#ifndef __WINE_WINE_OBJ_SERVICEPROVIDER_H
#define __WINE_WINE_OBJ_SERVICEPROVIDER_H

#include "objbase.h"
#include "winbase.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IServiceProvider, 0x6d5140c1, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
typedef struct IServiceProvider IServiceProvider, *LPSERVICEPROVIDER;


/*****************************************************************************
 * IServiceProvider interface
 */
#define INTERFACE IServiceProvider
#define IServiceProvider_METHODS \
    STDMETHOD(QueryService)(THIS_ REFGUID  guidService, REFIID  riid, void ** ppv) PURE;
#define IServiceProvider_IMETHODS \
    IUnknown_IMETHODS \
    IServiceProvider_METHODS
ICOM_DEFINE(IServiceProvider,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IServiceProvider_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IServiceProvider_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IServiceProvider_Release(p)                     (p)->lpVtbl->Release(p)
/*** IServiceProvider methods ***/
#define IServiceProvider_QueryService(p,a,b,c)          (p)->lpVtbl->QueryService(p,a,b,c)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SERVICEPROVIDER_H */
