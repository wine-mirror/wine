/*
 * Defines the COM interfaces and APIs related to EnumIDList
 *
 * Depends on 'obj_base.h'.
 *
 * Copyright (C) 1999 Juergen Schmied
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

#ifndef __WINE_WINE_OBJ_ENUMIDLIST_H
#define __WINE_WINE_OBJ_ENUMIDLIST_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IEnumIDList IEnumIDList, *LPENUMIDLIST;

#define INTERFACE IEnumIDList
#define IEnumIDList_METHODS \
    STDMETHOD(Next)(THIS_ ULONG  celt, LPITEMIDLIST * rgelt, ULONG * pceltFetched) PURE; \
    STDMETHOD(Skip)(THIS_ ULONG  celt) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_ IEnumIDList ** ppenum) PURE;
#define IEnumIDList_IMETHODS \
    IUnknown_IMETHODS \
    IEnumIDList_METHODS
ICOM_DEFINE(IEnumIDList,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IEnumIDList_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumIDList_AddRef(p)			ICOM_CALL (AddRef,p)
#define IEnumIDList_Release(p)			ICOM_CALL (Release,p)
/*** IEnumIDList methods ***/
#define IEnumIDList_Next(p,a,b,c)		ICOM_CALL3(Next,p,a,b,c)
#define IEnumIDList_Skip(p,a)			ICOM_CALL1(Skip,p,a)
#define IEnumIDList_Reset(p)			ICOM_CALL(Reset,p)
#define IEnumIDList_Clone(p,a)			ICOM_CALL1(Clone,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_ENUMIDLIST_H */
