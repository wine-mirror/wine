/*
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
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

#include "wtypes.h"

extern void* StdGlobalInterfaceTable_Construct();
extern void  StdGlobalInterfaceTable_Destroy(void* self);

extern HRESULT WINE_StringFromCLSID(const CLSID *id,LPSTR idstr);
extern HRESULT create_marshalled_proxy(REFCLSID rclsid, REFIID iid, LPVOID *ppv);

inline static HRESULT
get_facbuf_for_iid(REFIID riid,IPSFactoryBuffer **facbuf) {
    HRESULT       hres;
    CLSID         pxclsid;

    if ((hres = CoGetPSClsid(riid,&pxclsid)))
	return hres;
    return CoGetClassObject(&pxclsid,CLSCTX_INPROC_SERVER,NULL,&IID_IPSFactoryBuffer,(LPVOID*)facbuf);
}

#define PIPEPREF "\\\\.\\pipe\\"
#define OLESTUBMGR PIPEPREF"WINE_OLE_StubMgr"
/* Standard Marshaling definitions */
typedef struct _wine_marshal_id {
    DWORD	processid;
    DWORD	objectid;	/* unique value corresp. IUnknown of object */
    IID		iid;
} wine_marshal_id;

inline static BOOL
MARSHAL_Compare_Mids(wine_marshal_id *mid1,wine_marshal_id *mid2) {
    return
	(mid1->processid == mid2->processid)	&&
	(mid1->objectid == mid2->objectid)	&&
	IsEqualIID(&(mid1->iid),&(mid2->iid))
    ;
}

/* compare without interface compare */
inline static BOOL
MARSHAL_Compare_Mids_NoInterface(wine_marshal_id *mid1, wine_marshal_id *mid2) {
    return
	(mid1->processid == mid2->processid)	&&
	(mid1->objectid == mid2->objectid)
    ;
}

HRESULT MARSHAL_Find_Stub_Buffer(wine_marshal_id *mid,IRpcStubBuffer **stub);
HRESULT MARSHAL_Find_Stub_Server(wine_marshal_id *mid,LPUNKNOWN *punk);
HRESULT MARSHAL_Register_Stub(wine_marshal_id *mid,LPUNKNOWN punk, IRpcStubBuffer *stub);

HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);

typedef struct _wine_marshal_data {
    DWORD	dwDestContext;
    DWORD	mshlflags;
} wine_marshal_data;


#define REQTYPE_REQUEST		0
typedef struct _wine_rpc_request_header {
    DWORD		reqid;
    wine_marshal_id	mid;
    DWORD		iMethod;
    DWORD		cbBuffer;
} wine_rpc_request_header;

#define REQTYPE_RESPONSE	1
typedef struct _wine_rpc_response_header {
    DWORD		reqid;
    DWORD		cbBuffer;
    DWORD		retval;
} wine_rpc_response_header;

#define REQSTATE_START			0
#define REQSTATE_REQ_QUEUED		1
#define REQSTATE_REQ_WAITING_FOR_REPLY	2
#define REQSTATE_REQ_GOT		3
#define REQSTATE_INVOKING		4
#define REQSTATE_RESP_QUEUED		5
#define REQSTATE_RESP_GOT		6
#define REQSTATE_DONE			6

void STUBMGR_Start();

extern HRESULT PIPE_GetNewPipeBuf(wine_marshal_id *mid, IRpcChannelBuffer **pipebuf);

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize();

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize();

/* This function decomposes a String path to a String Table containing all the elements ("\" or "subDirectory" or "Directory" or "FileName") of the path */
int WINAPI FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable);

HRESULT WINAPI __CLSIDFromStringA(LPCSTR idstr, CLSID *id);

#endif /* __WINE_OLE_COMPOBJ_H */
