#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

#include "wtypes.h"

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

#endif /* __WINE_OLE_COMPOBJ_H */
