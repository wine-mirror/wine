#ifndef __WINE_SERVPROV_H
#define __WINE_SERVPROV_H


#include "unknwn.h"


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID   (IID_IServiceProvider,	0x6d5140c1L, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
typedef struct IServiceProvider IServiceProvider,*LPSERVICEPROVIDER;


/*****************************************************************************
 * IServiceProvider interface
 */
#define ICOM_INTERFACE IServiceProvider
ICOM_BEGIN(IServiceProvider,IUnknown)
    ICOM_METHOD3(HRESULT,QueryService, REFGUID,guidService, REFIID,riid, void**,ppvObject);
ICOM_END(IServiceProvider)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IServiceProvider_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IServiceProvider_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IServiceProvider_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IServiceProvider methods ***/
#define IServiceProvider_QueryService(p,a,b,c) ICOM_CALL3(QueryService,p,a,b,c)
#endif


#endif /* __WINE_SERVPROV_H */
