/*
 * Defines the COM interfaces and APIs that allow an interface to
 * specify a custom marshaling for its objects.
 *
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

#ifndef __WINE_WINE_OBJ_MARSHAL_H
#define __WINE_WINE_OBJ_MARSHAL_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IMarshal,		0x00000003L, 0, 0);
typedef struct IMarshal IMarshal,*LPMARSHAL;

DEFINE_OLEGUID(IID_IStdMarshalInfo,	0x00000018L, 0, 0);
typedef struct IStdMarshalInfo IStdMarshalInfo,*LPSTDMARSHALINFO;

DEFINE_OLEGUID(CLSID_DfMarshal,		0x0000030BL, 0, 0);


/*****************************************************************************
 * IMarshal interface
 */
#define ICOM_INTERFACE IMarshal
#define IMarshal_METHODS \
    ICOM_METHOD6(HRESULT,GetUnmarshalClass,  REFIID,riid, void*,pv, DWORD,dwDestContext, void*,pvDestContext, DWORD,mshlflags, CLSID*,pCid) \
    ICOM_METHOD6(HRESULT,GetMarshalSizeMax,  REFIID,riid, void*,pv, DWORD,dwDestContext, void*,pvDestContext, DWORD,mshlflags, DWORD*,pSize) \
    ICOM_METHOD6(HRESULT,MarshalInterface,   IStream*,pStm, REFIID,riid, void*,pv, DWORD,dwDestContext, void*,pvDestContext, DWORD,mshlflags) \
    ICOM_METHOD3(HRESULT,UnmarshalInterface, IStream*,pStm, REFIID,riid, void**,ppv) \
    ICOM_METHOD1(HRESULT,ReleaseMarshalData, IStream*,pStm) \
    ICOM_METHOD1(HRESULT,DisconnectObject,   DWORD,dwReserved)
#define IMarshal_IMETHODS \
    IUnknown_IMETHODS \
    IMarshal_METHODS
ICOM_DEFINE(IMarshal,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IMarshal_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IMarshal_AddRef(p)             ICOM_CALL (AddRef,p)
#define IMarshal_Release(p)            ICOM_CALL (Release,p)
/*** IMarshal methods ***/
#define IMarshal_GetUnmarshalClass(p,a,b,c,d,e,f) ICOM_CALL6(GetUnmarshalClass,p,a,b,c,d,e,f)
#define IMarshal_GetMarshalSizeMax(p,a,b,c,d,e,f) ICOM_CALL6(GetMarshalSizeMax,p,a,b,c,d,e,f)
#define IMarshal_MarshalInterface(p,a,b,c,d,e,f)  ICOM_CALL6(MarshalInterface,p,a,b,c,d,e,f)
#define IMarshal_UnmarshalInterface(p,a,b,c)      ICOM_CALL3(UnmarshalInterface,p,a,b,c)
#define IMarshal_ReleaseMarshalData(p,a)          ICOM_CALL1(ReleaseMarshalData,p,a)
#define IMarshal_DisconnectObject(p,a)            ICOM_CALL1(DisconnectObject,p,a)


/*****************************************************************************
 * IStdMarshalInfo interface
 */
#define ICOM_INTERFACE IStdMarshalInfo
#define IStdMarshalInfo_METHODS \
    ICOM_METHOD3(HRESULT,GetClassForHandler,  DWORD,dwDestContext, void*,pvDestContext, CLSID*,pClsid)
#define IStdMarshalInfo_IMETHODS \
    IUnknown_IMETHODS \
    IStdMarshalInfo_METHODS
ICOM_DEFINE(IStdMarshalInfo,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IStdMarshalInfo_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IStdMarshalInfo_AddRef(p)             ICOM_CALL (AddRef,p)
#define IStdMarshalInfo_Release(p)            ICOM_CALL (Release,p)
/*** IStdMarshalInfo methods ***/
#define IStdMarshalInfo_GetClassForHandler(p,a,b,c) ICOM_CALL3(GetClassForHandler,p,a,b,c)


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_MARSHAL_H */
