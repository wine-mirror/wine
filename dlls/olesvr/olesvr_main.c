/*
 *	OLESVR library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windef.h"
#include "wingdi.h"
#include "wtypes.h"
#include "ole.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole);

static LONG OLE_current_handle;

/******************************************************************************
 *		OleRegisterServer16	[OLESVR.2]
 */
OLESTATUS WINAPI OleRegisterServer16( LPCSTR name, LPOLESERVER serverStruct,
                                      LHSERVER *hRet, HINSTANCE16 hServer,
                                      OLE_SERVER_USE use )
{
    FIXME("(%s,...): stub\n",name);
    *hRet=++OLE_current_handle;
    /* return OLE_ERROR_MEMORY, OLE_ERROR_PROTECT_ONLY if you want it fail*/
    return OLE_OK;
}

/******************************************************************************
 *		OleBlockServer16	[OLESVR.4]
 */
OLESTATUS WINAPI OleBlockServer16(LHSERVER hServer)
{
    FIXME("(%ld): stub\n",hServer);
    return OLE_OK;
}

/******************************************************************************
 *		OleBlockServer	[OLESVR32.4]
 */
OLESTATUS WINAPI OleBlockServer(LHSERVER hServer)
{
    FIXME("(%ld): stub\n",hServer);
    return OLE_OK;
}

/******************************************************************************
 *		OleUnblockServer16	[OLESVR.5]
 */
OLESTATUS WINAPI OleUnblockServer16(LHSERVER hServer, BOOL16 *block)
{
    FIXME("(%ld): stub\n",hServer);
    /* no more blocked messages :) */
    *block=FALSE;
    return OLE_OK;
}

/******************************************************************************
 *		OleUnblockServer	[OLESVR32.5]
 */
OLESTATUS WINAPI OleUnblockServer(LHSERVER hServer, BOOL *block)
{
    FIXME("(%ld): stub\n",hServer);
    /* no more blocked messages :) */
    *block=FALSE;
    return OLE_OK;
}

/***********************************************************************
 *		OleRegisterServerDoc16	[OLESVR.6]
 */
OLESTATUS WINAPI OleRegisterServerDoc16( LHSERVER hServer, LPCSTR docname,
                                         LPOLESERVERDOC document,
                                         LHSERVERDOC *hRet)
{
    FIXME("(%ld,%s): stub\n",hServer, docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeServerDoc16	[OLESVR.7]
 */
OLESTATUS WINAPI OleRevokeServerDoc16(LHSERVERDOC hServerDoc)
{
    FIXME("%ld  - stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeServerDoc	[OLESVR32.7]
 */
OLESTATUS WINAPI OleRevokeServerDoc(LHSERVERDOC hServerDoc)
{
    FIXME("(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeServer	[OLESVR.3]
 */
OLESTATUS WINAPI OleRevokeServer16(LHSERVER hServer)
{
    FIXME("%ld - stub\n",hServer);
    return OLE_OK;
}

/******************************************************************************
 * OleRegisterServer [OLESVR32.2]
 */
OLESTATUS WINAPI OleRegisterServer(LPCSTR svrname,LPOLESERVER olesvr,LHSERVER* hRet,HINSTANCE hinst,OLE_SERVER_USE osu) {
	FIXME("(%s,%p,%p,%08x,%d): stub!\n",svrname,olesvr,hRet,hinst,osu);
    	*hRet=++OLE_current_handle;
	return OLE_OK;
}

/******************************************************************************
 * OleRegisterServerDoc [OLESVR32.6]
 */
OLESTATUS WINAPI OleRegisterServerDoc( LHSERVER hServer, LPCSTR docname,
                                         LPOLESERVERDOC document,
                                         LHSERVERDOC *hRet)
{
    FIXME("(%ld,%s): stub\n", hServer, docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameServerDoc	[OLESVR32.8]
 *
 */
OLESTATUS WINAPI OleRenameServerDoc(LHSERVERDOC hDoc, LPCSTR newName)
{
    FIXME("(%ld,%s): stub.\n",hDoc, newName);
    return OLE_OK;
}
