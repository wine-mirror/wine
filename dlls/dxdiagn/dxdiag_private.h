/*
 * DXDiag private include file
 *
 * Copyright 2004 Raphael Junqueira
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

#ifndef __WINE_DXDIAG_PRIVATE_H
#define __WINE_DXDIAG_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "oleauto.h"

#include "dxdiag.h"

/* DXDiag Interfaces: */
typedef struct IDxDiagProviderImpl  IDxDiagProviderImpl;
typedef struct IDxDiagContainerImpl IDxDiagContainerImpl;

/* ---------------- */
/* IDxDiagProvider  */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDxDiagProvider) DirectPlay8Client_Vtbl;

/*****************************************************************************
 * IDxDiagProvider implementation structure
 */
struct IDxDiagProviderImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDxDiagProvider);
  DWORD       ref;
  /* IDxDiagProvider fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDxDiagProviderImpl_QueryInterface(PDXDIAGPROVIDER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI IDxDiagProviderImpl_AddRef(PDXDIAGPROVIDER iface);
extern ULONG WINAPI IDxDiagProviderImpl_Release(PDXDIAGPROVIDER iface);

/* IDxDiagProvider: */
extern HRESULT WINAPI IDxDiagProviderImpl_Initialize(PDXDIAGPROVIDER iface, DXDIAG_INIT_PARAMS* pParams);
extern HRESULT WINAPI IDxDiagProviderImpl_GetRootContainer(PDXDIAGPROVIDER iface, IDxDiagContainer** ppInstance);

/**
 * factories
 */
extern HRESULT DXDiag_CreateDXDiagProvider(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);



#endif
