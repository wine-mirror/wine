/*
 * Copyright (C) 2001 Hidenori Takeshima
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

#ifndef WINE_WINE_OBJ_KSPROPERTY_H
#define WINE_WINE_OBJ_KSPROPERTY_H

/* NOTE: IKsPropertySet is declared in both dsound.h and strmif.h */

DEFINE_GUID(IID_IKsPropertySet,		0x31EFAC30,0x515C,0x11D0,0xA9,0xAA,0x00,0xAA,0x00,0x61,0xBE,0x93);
typedef struct IKsPropertySet IKsPropertySet,*LPKSPROPERTYSET;

/*****************************************************************************
 * IKsPropertySet interface
 */
#define KSPROPERTY_SUPPORT_GET	1
#define KSPROPERTY_SUPPORT_SET	2

#define ICOM_INTERFACE IKsPropertySet
#define IKsPropertySet_METHODS \
    ICOM_METHOD7(HRESULT,Get,REFGUID,rgid,ULONG,x1,LPVOID,p1,ULONG,x2,LPVOID,p2,ULONG,x3,ULONG*,px4);\
    ICOM_METHOD6(HRESULT,Set,REFGUID,rgid,ULONG,x1,LPVOID,p1,ULONG,x2,LPVOID,p2,ULONG,x3);\
    ICOM_METHOD3(HRESULT,QuerySupport,REFGUID,rgid,ULONG,x1,ULONG*,px2);
#define IKsPropertySet_IMETHODS \
        IUnknown_IMETHODS \
	IKsPropertySet_METHODS
ICOM_DEFINE(IKsPropertySet,IUnknown)
#undef ICOM_INTERFACE

#define IKsPropertySet_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IKsPropertySet_AddRef(p)		ICOM_CALL (AddRef,p)
#define IKsPropertySet_Release(p)		ICOM_CALL (Release,p)
#define IKsPropertySet_Get(p,a,b,c,d,e,f,g)	ICOM_CALL7(Get,p,a,b,c,d,e,f,g)
#define IKsPropertySet_Set(p,a,b,c,d,e,f)	ICOM_CALL6(Set,p,a,b,c,d,e,f)
#define IKsPropertySet_QuerySupport(p,a,b,c)	ICOM_CALL3(QuerySupport,p,a,b,c)
/* The real windows header declares 'QuerySupport' in dsound.h */
/* but 'QuerySupported' in strmif.h ! */
#define IKsPropertySet_QuerySupported(p,a,b,c)	ICOM_CALL3(QuerySupport,p,a,b,c)


#endif /* WINE_WINE_OBJ_KSPROPERTY_H */
