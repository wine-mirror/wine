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
#define ICOM_INTERFACE IServiceProvider
#define IServiceProvider_METHODS \
    ICOM_METHOD3(HRESULT,QueryService, REFGUID,guidService, REFIID,riid, void**,ppvObject)
#define IServiceProvider_IMETHODS \
    IUnknown_IMETHODS \
    IServiceProvider_METHODS
ICOM_DEFINE(IServiceProvider,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IServiceProvider_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IServiceProvider_AddRef(p)             ICOM_CALL (AddRef,p)
#define IServiceProvider_Release(p)            ICOM_CALL (Release,p)
/*** IServiceProvider methods ***/
#define IServiceProvider_QueryService(p,a,b,c) ICOM_CALL3(QueryService,p,a,b,c)


#endif /* __WINE_SERVPROV_H */
