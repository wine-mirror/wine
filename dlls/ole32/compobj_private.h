/*
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
 * Copyright 2003 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Mike Hearn, CodeWeavers Inc
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

#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

#include <stdarg.h>

#include "wine/list.h"

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dcom.h"
#include "winreg.h"
#include "winternl.h"

struct apartment;


/* exported interface */
typedef struct tagXIF {
  struct tagXIF *next;
  LPVOID iface;            /* interface pointer */
  IID iid;                 /* interface ID */
  IPID ipid;               /* exported interface ID */
  LPRPCSTUBBUFFER stub;    /* interface stub */
  DWORD refs;              /* external reference count */
  HRESULT hres;            /* result of stub creation attempt */
} XIF;

/* exported object */
typedef struct tagXOBJECT {
  IRpcStubBufferVtbl *lpVtbl;
  struct apartment *parent;
  struct tagXOBJECT *next;
  LPUNKNOWN obj;           /* object identity (IUnknown) */
  OID oid;                 /* object ID */
  DWORD ifc;               /* interface ID counter */
  XIF *ifaces;             /* exported interfaces */
  DWORD refs;              /* external reference count */
} XOBJECT;

/* imported interface proxy */
struct ifproxy
{
  struct list entry;
  LPVOID iface;            /* interface pointer */
  IID iid;                 /* interface ID */
  IPID ipid;               /* imported interface ID */
  LPRPCPROXYBUFFER proxy;  /* interface proxy */
  DWORD refs;              /* imported (public) references */
};

/* imported object / proxy manager */
struct proxy_manager
{
  const IInternalUnknownVtbl *lpVtbl;
  struct apartment *parent;
  struct list entry;
  LPRPCCHANNELBUFFER chan; /* channel to object */
  OXID oxid;               /* object exported ID */
  OID oid;                 /* object ID */
  struct list interfaces;  /* imported interfaces */
  DWORD refs;              /* proxy reference count */
  CRITICAL_SECTION cs;     /* thread safety for this object and children */
};

/* this needs to become a COM object that implements IRemUnknown */
struct apartment
{
  struct list entry;       

  DWORD refs;              /* refcount of the apartment */
  DWORD model;             /* threading model */
  DWORD tid;               /* thread id */
  HANDLE thread;           /* thread handle */
  OXID oxid;               /* object exporter ID */
  OID oidc;                /* object ID counter, starts at 1, zero is invalid OID */
  HWND win;                /* message window */
  CRITICAL_SECTION cs;     /* thread safety */
  LPMESSAGEFILTER filter;  /* message filter */
  XOBJECT *objs;           /* exported objects */
  struct list proxies;     /* imported objects */
  DWORD listenertid;       /* id of apartment_listener_thread */
  struct list stubmgrs;    /* stub managers for exported objects */
};

typedef struct apartment APARTMENT;

extern void* StdGlobalInterfaceTable_Construct(void);
extern void  StdGlobalInterfaceTable_Destroy(void* self);
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv);

extern HRESULT WINE_StringFromCLSID(const CLSID *id,LPSTR idstr);
extern HRESULT create_marshalled_proxy(REFCLSID rclsid, REFIID iid, LPVOID *ppv);

extern void* StdGlobalInterfaceTableInstance;

/* Standard Marshalling definitions */
typedef struct _wine_marshal_id {
    OXID    oxid;       /* id of apartment */
    OID     oid;        /* id of stub manager */
    IID     iid;        /* id of interface (NOT ifptr) */
} wine_marshal_id;

inline static BOOL
MARSHAL_Compare_Mids(wine_marshal_id *mid1,wine_marshal_id *mid2) {
    return
	(mid1->oxid == mid2->oxid)	&&
	(mid1->oid == mid2->oid)	&&
	IsEqualIID(&(mid1->iid),&(mid2->iid))
    ;
}

HRESULT MARSHAL_Disconnect_Proxies(APARTMENT *apt);
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);

/* an interface stub */
struct ifstub   
{
    struct list       entry;
    IRpcStubBuffer   *stubbuffer;
    IID               iid;         /* fixme: this should be an IPID not an IID */
    IUnknown         *iface;

    BOOL              table;
};


/* stub managers hold refs on the object and each interface stub */
struct stub_manager
{
    struct list       entry;
    struct list       ifstubs;
    CRITICAL_SECTION  lock;
    APARTMENT        *apt;        /* owning apt */

    DWORD             refcount;   /* count of 'external' references */
    OID               oid;        /* apartment-scoped unique identifier */
    IUnknown         *object;     /* the object we are managing the stub for */
    DWORD             next_ipid;  /* currently unused */
};

struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object);
int stub_manager_ref(struct stub_manager *m, int refs);
int stub_manager_unref(struct stub_manager *m, int refs);
IRpcStubBuffer *stub_manager_iid_to_stubbuffer(struct stub_manager *m, IID *iid);
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, IID *iid, BOOL tablemarshal);
struct stub_manager *get_stub_manager(OXID oxid, OID oid);
struct stub_manager *get_stub_manager_from_object(OXID oxid, void *object);
void stub_manager_delete_ifstub(struct stub_manager *m, IID *iid);   /* fixme: should be ipid */

IRpcStubBuffer *mid_to_stubbuffer(wine_marshal_id *mid);

void start_apartment_listener_thread(void);

extern HRESULT PIPE_GetNewPipeBuf(wine_marshal_id *mid, IRpcChannelBuffer **pipebuf);
void RPC_StartLocalServer(REFCLSID clsid, IStream *stream);

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void);

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void);

/* This function decomposes a String path to a String Table containing all the elements ("\" or "subDirectory" or "Directory" or "FileName") of the path */
int WINAPI FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable);

HRESULT WINAPI __CLSIDFromStringA(LPCSTR idstr, CLSID *id);

/* compobj.c */
APARTMENT *COM_CreateApartment(DWORD model);
APARTMENT *COM_ApartmentFromOXID(OXID oxid, BOOL ref);
DWORD COM_ApartmentAddRef(struct apartment *apt);
DWORD COM_ApartmentRelease(struct apartment *apt);

extern CRITICAL_SECTION csApartment;

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    IUnknown         *state;       /* see CoSetState */
};

/*
 * Per-thread values are stored in the TEB on offset 0xF80,
 * see http://www.microsoft.com/msj/1099/bugslayer/bugslayer1099.htm
 */

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    if (!NtCurrentTeb()->ReservedForOle)     
        NtCurrentTeb()->ReservedForOle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct oletls));
    
    return NtCurrentTeb()->ReservedForOle;
}

static inline APARTMENT* COM_CurrentApt(void)
{  
    return COM_CurrentInfo()->apt;
}

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

#endif /* __WINE_OLE_COMPOBJ_H */
