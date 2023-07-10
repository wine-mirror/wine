/*
 * ExplorerFrame main include
 *
 * Copyright 2010 David Hedberg
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_EXPLORERFRAME_H
#define __WINE_EXPLORERFRAME_H

#define COBJMACROS

#include "shlobj.h"

extern HINSTANCE explorerframe_hinstance;

extern LONG EFRAME_refCount;
static inline void EFRAME_LockModule(void) { InterlockedIncrement( &EFRAME_refCount ); }
static inline void EFRAME_UnlockModule(void) { InterlockedDecrement( &EFRAME_refCount ); }

HRESULT NamespaceTreeControl_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv);
HRESULT TaskbarList_Constructor(IUnknown*,REFIID,void**);

#endif /* __WINE_EXPLORERFRAME_H */
