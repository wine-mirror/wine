#ifndef _WINE_OLEOBJ_H
#define _WINE_OLEOBJ_H

#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
#include "wine/obj_dataobject.h"

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

/* forward declaration of the objects*/
typedef struct tagOLEADVISEHOLDER	*LPOLEADVISEHOLDER,	IOleAdviseHolder;


/****************************************************************************
 *  OLE ID
 */

DEFINE_OLEGUID(IID_IOleAdviseHolder, 0x00000111L, 0, 0);


/*****************************************************************************
 * IOleAdviseHolder interface
 */
#define THIS LPOLEADVISEHOLDER this

typedef struct IOleAdviseHolder_VTable
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /*** IOleAdviseHolder methods ***/
    STDMETHOD(Advise)(THIS_ IAdviseSink *pAdvise, DWORD *pdwConnection) PURE;
    STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) PURE;
    STDMETHOD(Enum_Advise)(THIS_ IEnumSTATDATA**ppenumAdvise) PURE;
    STDMETHOD(SendOnRename)(THIS_ IMoniker *pmk) PURE;
    STDMETHOD(SendOnSave)(THIS) PURE;
    STDMETHOD(SendOnClose)(THIS) PURE;
} IOleAdviseHolder_VTable, *LPOLEADVISEHOLDER_VTABLE;

struct tagOLEADVISEHOLDER
{
    LPOLEADVISEHOLDER_VTABLE lpvtbl;
    DWORD                    ref;
};

#undef THIS



#undef PURE
#undef FAR
#undef THIS
#undef THIS_
#undef STDMETHOD
#undef STDMETHOD_
#endif /*_WINE_OLEOBJ_H*/

