#ifndef __WINE_SERVPROV_H
#define __WINE_SERVPROV_H


#include "wine/obj_base.h"


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
