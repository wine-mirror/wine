/*
 * Copyright (C) 1999 Francois Gouget
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

#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif

#ifndef __WINE_SERVPROV_H
#define __WINE_SERVPROV_H


#include "objbase.h"


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID   (IID_IServiceProvider,	0x6d5140c1L, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
typedef struct IServiceProvider IServiceProvider,*LPSERVICEPROVIDER;


/*****************************************************************************
 * IServiceProvider interface
 */
#define INTERFACE IServiceProvider
#define IServiceProvider_METHODS \
    STDMETHOD(QueryService)(THIS_ REFGUID guidService, REFIID riid, void **ppvObject) PURE;
#define IServiceProvider_IMETHODS \
    IUnknown_IMETHODS \
    IServiceProvider_METHODS
ICOM_DEFINE(IServiceProvider,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IServiceProvider_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IServiceProvider_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IServiceProvider_Release(p)            (p)->lpVtbl->Release(p)
/*** IServiceProvider methods ***/
#define IServiceProvider_QueryService(p,a,b,c) (p)->lpVtbl->QueryService(p,a,b,c)
#endif


#endif /* __WINE_SERVPROV_H */
