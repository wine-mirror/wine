#ifndef _WINE_INTERFACES_H
#define _WINE_INTERFACES_H

#include "ole.h"
#include "ole2.h"
#include "compobj.h"

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define	THIS_ THIS,


DEFINE_OLEGUID(IID_IUnknown,0,0,0);
DEFINE_OLEGUID(IID_IClassFactory,1,0,0);
DEFINE_OLEGUID(IID_IMalloc,2,0,0);
DEFINE_OLEGUID(IID_IMarshal,3,0,0);
DEFINE_OLEGUID(IID_IStorage,0xb,0,0);
DEFINE_OLEGUID(IID_IStream,0xc,0,0);
DEFINE_OLEGUID(IID_IBindCtx,0xe,0,0);
DEFINE_OLEGUID(IID_IMoniker,0xf,0,0);
DEFINE_OLEGUID(IID_IRunningObject,0x10,0,0);
DEFINE_OLEGUID(IID_IRootStorage,0x12,0,0);
DEFINE_OLEGUID(IID_IMessageFilter,0x16,0,0);
DEFINE_OLEGUID(IID_IStdMarshalInfo,0x18,0,0);

#define THIS LPUNKNOWN this
typedef struct IUnknown *LPUNKNOWN,IUnknown;
typedef struct {
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;
} *LPUNKNOWN_VTABLE,IUnknown_VTable;

struct IUnknown {
	LPUNKNOWN_VTABLE	lpvtbl;
	DWORD			ref;
};
#undef THIS

#define THIS LPCLASSFACTORY this
typedef struct IClassFactory *LPCLASSFACTORY,IClassFactory;
typedef struct {
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;
	STDMETHOD(CreateInstance) (THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObject) PURE;
} *LPCLASSFACTORY_VTABLE,IClassFactory_VTable;

struct IClassFactory {
	LPCLASSFACTORY_VTABLE	lpvtbl;
	DWORD			ref;
};
#undef THIS

#define THIS LPMALLOC this
typedef struct IMalloc *LPMALLOC,IMalloc;
typedef struct {
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD_(LPVOID,Alloc) ( THIS_ DWORD cb);
	STDMETHOD_(LPVOID,Realloc) ( THIS_ LPVOID pv,DWORD cb);
	STDMETHOD_(VOID,Free) ( THIS_ LPVOID pv);
	STDMETHOD_(DWORD,GetSize) ( THIS_ LPVOID pv);
	STDMETHOD_(LPINT32,DidAlloc) ( THIS_ LPVOID pv);
	STDMETHOD_(LPVOID,HeapMinimize) ( THIS );
} *LPMALLOC_VTABLE,IMalloc_VTable;

struct IMalloc {
	LPMALLOC_VTABLE lpvtbl;
	DWORD		ref;
};
#undef THIS

/* private prototypes for the constructors */
#ifdef __WINE__
LPUNKNOWN	IUnknown_Constructor();
LPMALLOC	IMalloc_Constructor();
#endif

#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef FAR
#undef THIS_
#endif /*_WINE_INTERFACES_H*/
