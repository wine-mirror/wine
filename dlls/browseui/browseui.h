/*
 * Internal header for browseui.dll
 *
 * Copyright 2007 Mikolaj Zalewski
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

#ifndef __WINE_BROWSEUI_H
#define __WINE_BROWSEUI_H

extern LONG BROWSEUI_refCount;
extern HINSTANCE BROWSEUI_hinstance;

extern HRESULT ACLMulti_Constructor(IUnknown *punkOuter, IUnknown **ppOut);
extern HRESULT ProgressDialog_Constructor(IUnknown *punkOuter, IUnknown **ppOut);
extern HRESULT CompCatCacheDaemon_Constructor(IUnknown *punkOuter, IUnknown **ppOut);
extern HRESULT ACLShellSource_Constructor(IUnknown *punkOuter, IUnknown **ppOut);

extern const GUID CLSID_CompCatCacheDaemon;

#endif /* __WINE_BROWSEUI_H */
