#ifndef _WINE_INTERFACES_H
#define _WINE_INTERFACES_H

#include "ole.h"
#include "ole2.h"
#include "compobj.h"

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

typedef struct tagUNKNOWN *LPUNKNOWN,IUnknown;
typedef struct {
	HRESULT	(CALLBACK *fnQueryInterface)(LPUNKNOWN this,REFIID refiid,LPVOID *obj);
	HRESULT	(CALLBACK *fnAddRef)(LPUNKNOWN this);
	HRESULT	(CALLBACK *fnRelease)(LPUNKNOWN this);
} *LPUNKNOWN_VTABLE;

struct tagUNKNOWN {
	LPUNKNOWN_VTABLE	lpvtbl;
	/* internal stuff. Not needed until we actually implement IUnknown */
};

typedef struct tagCLASSFACTORY *LPCLASSFACTORY,IClassFactory;
typedef struct {
	HRESULT	(CALLBACK *fnQueryInterface)(LPCLASSFACTORY this,REFIID refiid,LPVOID *obj);
	HRESULT	(CALLBACK *fnAddRef)(LPCLASSFACTORY this);
	HRESULT	(CALLBACK *fnRelease)(LPCLASSFACTORY this);
	HRESULT (CALLBACK *fnCreateInstance)(LPCLASSFACTORY this,LPUNKNOWN pUnkOuter,REFIID riid,LPVOID * ppvObject);
} *LPCLASSFACTORY_VTABLE;

struct tagCLASSFACTORY {
	LPCLASSFACTORY_VTABLE lpvtbl;
	/*internal stuff. Not needed until we actually implement IClassFactory*/
};
#endif /*_WINE_INTERFACES_H*/
