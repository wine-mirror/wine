#ifndef _WINE_OLEOBJ_H
#define _WINE_OLEOBJ_H

#include "ole.h"
#include "ole2.h"
#include "compobj.h"
/* #include "interfaces.h" */

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

/* forward declaration of the objects*/
typedef struct tagOLEADVISEHOLDER	*LPOLEADVISEHOLDER,	IOleAdviseHolder;
typedef struct tagADVISESINK		*LPADVISESINK,		IAdviseSink;
typedef struct tagENUMSTATDATA		*LPENUMSTATDATA,	IEnumSTATDATA;


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

