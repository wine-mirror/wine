/*
 * Defines the COM interfaces and APIs related to EnumGUID
 *
 * Depends on 'obj_base.h'.
 *
 * Copyright (C) 2002 John K. Hohm
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

#ifndef __WINE_WINE_OBJ_ENUMGUID_H
#define __WINE_WINE_OBJ_ENUMGUID_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumGUID, 0x0002E000L, 0, 0);
typedef struct IEnumGUID IEnumGUID, *LPENUMGUID;

/*****************************************************************************
 * IEnumGUID
 */
#define INTERFACE IEnumGUID
#define IEnumGUID_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Next)(THIS_ ULONG  celt, GUID * rgelt, ULONG * pceltFetched) PURE; \
    STDMETHOD(Skip)(THIS_ ULONG  celt) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_ IEnumGUID ** ppenum) PURE;
ICOM_DEFINE(IEnumGUID,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumGUID_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumGUID_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IEnumGUID_Release(p)            (p)->lpVtbl->Release(p)
/*** IEnumGUID methods ***/
#define IEnumGUID_Next(p,a,b,c)         (p)->lpVtbl->Next(p,a,b,c)
#define IEnumGUID_Skip(p,a)             (p)->lpVtbl->Skip(p,a)
#define IEnumGUID_Reset(p)              (p)->lpVtbl->Reset(p)
#define IEnumGUID_Clone(p,a)            (p)->lpVtbl->Clone(p,a)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_ENUMGUID_H */
