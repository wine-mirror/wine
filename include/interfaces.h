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


DEFINE_OLEGUID(IID_IDispatch,       0x00020400,0,0);
DEFINE_OLEGUID(IID_ITypeInfo,       0x00020401,0,0);
DEFINE_OLEGUID(IID_ITypeLib,        0x00020402,0,0);
DEFINE_OLEGUID(IID_ITypeComp,       0x00020403,0,0);
DEFINE_OLEGUID(IID_IEnumVariant,    0x00020404,0,0);
DEFINE_OLEGUID(IID_ICreateTypeInfo, 0x00020405,0,0);
DEFINE_OLEGUID(IID_ICreateTypeLib,  0x00020406,0,0);
DEFINE_OLEGUID(IID_ICreateTypeInfo2,0x0002040E,0,0);
DEFINE_OLEGUID(IID_ICreateTypeLib2, 0x0002040F,0,0);
DEFINE_OLEGUID(IID_ITypeChangeEvents,0x00020410,0,0);
DEFINE_OLEGUID(IID_ITypeLib2,       0x00020411,0,0);
DEFINE_OLEGUID(IID_ITypeInfo2,      0x00020412,0,0);
DEFINE_GUID(IID_IErrorInfo,         0x1CF2B120,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
DEFINE_GUID(IID_ICreateErrorInfo,   0x22F03340,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
DEFINE_GUID(IID_ISupportErrorInfo,  0xDF0B3D60,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);

#include "objbase.h"

#define THIS LPCLASSFACTORY this
typedef struct IClassFactory *LPCLASSFACTORY,IClassFactory;
typedef struct {
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;
	STDMETHOD(CreateInstance) (THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObject) PURE;
	STDMETHOD(LockServer) (THIS_ BOOL32) PURE;
} *LPCLASSFACTORY_VTABLE,IClassFactory_VTable;

struct IClassFactory {
	LPCLASSFACTORY_VTABLE	lpvtbl;
	DWORD			ref;
};
#undef THIS

#define THIS LPMALLOC32 this
typedef struct IMalloc32 *LPMALLOC32,IMalloc32;
typedef struct {
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD_(LPVOID,Alloc) ( THIS_ DWORD cb);
	STDMETHOD_(LPVOID,Realloc) ( THIS_ LPVOID pv,DWORD cb);
	STDMETHOD_(VOID,Free) ( THIS_ LPVOID pv);
	STDMETHOD_(DWORD,GetSize) ( THIS_ LPVOID pv);
	STDMETHOD_(INT32,DidAlloc) ( THIS_ LPVOID pv);
	STDMETHOD_(LPVOID,HeapMinimize) ( THIS );
} *LPMALLOC32_VTABLE,IMalloc32_VTable;

struct IMalloc32 {
	LPMALLOC32_VTABLE	lpvtbl;
	DWORD			ref;
};
#undef THIS

#define THIS LPMALLOC16 this
typedef struct IMalloc16 *LPMALLOC16,IMalloc16;
typedef struct {
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD_(LPVOID,Alloc) ( THIS_ DWORD cb);
	STDMETHOD_(LPVOID,Realloc) ( THIS_ LPVOID pv,DWORD cb);
	STDMETHOD_(VOID,Free) ( THIS_ LPVOID pv);
	STDMETHOD_(DWORD,GetSize) ( THIS_ LPVOID pv);
	STDMETHOD_(INT16,DidAlloc) ( THIS_ LPVOID pv);
	STDMETHOD_(LPVOID,HeapMinimize) ( THIS );
} *LPMALLOC16_VTABLE,IMalloc16_VTable;

struct IMalloc16 {
	LPMALLOC16_VTABLE	lpvtbl;
	DWORD			ref;
	/* Gmm, I think one is not enough, we should probably manage a list of
	 * heaps
	 */
	HGLOBAL16		heap;
};
#undef THIS

/* private prototypes for the constructors */
#ifdef __WINE__
LPUNKNOWN	IUnknown_Constructor(void);
LPMALLOC16	IMalloc16_Constructor(void);
LPMALLOC32	IMalloc32_Constructor(void);
#endif

HRESULT WINAPI CoGetMalloc32(DWORD, LPMALLOC32*);

#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef FAR
#undef THIS_
#endif /*_WINE_INTERFACES_H*/
