/*
 *	OLESVR library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "ole.h"
#include "debug.h"

LONG	OLE_current_handle;

/***********************************************************************
 *           OleRegisterServer
 */
OLESTATUS WINAPI OleRegisterServer16( LPCSTR name, LPOLESERVER serverStruct,
                                      LHSERVER *hRet, HINSTANCE16 hServer,
                                      OLE_SERVER_USE use )
{
    FIXME(ole,"(%s,...): stub\n",name);
    *hRet=++OLE_current_handle;
    /* return OLE_ERROR_MEMORY, OLE_ERROR_PROTECT_ONLY if you want it fail*/
    return OLE_OK;
}

/***********************************************************************
 *           OleBlockServer
 */
OLESTATUS WINAPI OleBlockServer16(LHSERVER hServer)
{
    FIXME(ole,"(%ld): stub\n",hServer);
    return OLE_OK;
}

/***********************************************************************
 *           OleBlockServer
 */
OLESTATUS WINAPI OleBlockServer32(LHSERVER hServer)
{
    FIXME(ole,"(%ld): stub\n",hServer);
    return OLE_OK;
}

/***********************************************************************
 *           OleUnblockServer
 */
OLESTATUS WINAPI OleUnblockServer16(LHSERVER hServer, BOOL16 *block)
{
    FIXME(ole,"(%ld): stub\n",hServer);
    /* no more blocked messages :) */
    *block=FALSE;
    return OLE_OK;
}

/***********************************************************************
 *           OleUnblockServer
 */
OLESTATUS WINAPI OleUnblockServer32(LHSERVER hServer, BOOL32 *block)
{
    FIXME(ole,"(%ld): stub\n",hServer);
    /* no more blocked messages :) */
    *block=FALSE;
    return OLE_OK;
}

/***********************************************************************
 *           OleRegisterServerDoc
 */
OLESTATUS WINAPI OleRegisterServerDoc16( LHSERVER hServer, LPCSTR docname,
                                         LPOLESERVERDOC document,
                                         LHSERVERDOC *hRet)
{
    FIXME(ole,"(%ld,%s): stub\n",hServer, docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeServerDoc
 */
OLESTATUS WINAPI OleRevokeServerDoc16(LHSERVERDOC hServerDoc)
{
    FIXME(ole,"%ld  - stub\n",hServerDoc);
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeServerDoc
 */
OLESTATUS WINAPI OleRevokeServerDoc32(LHSERVERDOC hServerDoc)
{
    FIXME(ole,"(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeServer
 */
OLESTATUS WINAPI OleRevokeServer(LHSERVER hServer)
{
    FIXME(ole,"%ld - stub\n",hServer);
    return OLE_OK;
}

OLESTATUS WINAPI OleRegisterServer32(LPCSTR svrname,LPOLESERVER olesvr,LHSERVER* hRet,HINSTANCE32 hinst,OLE_SERVER_USE osu) {
	FIXME(ole,"(%s,%p,%p,%08x,%d): stub!\n",svrname,olesvr,hRet,hinst,osu);
    	*hRet=++OLE_current_handle;
	return OLE_OK;
}

OLESTATUS WINAPI OleRegisterServerDoc32( LHSERVER hServer, LPCSTR docname,
                                         LPOLESERVERDOC document,
                                         LHSERVERDOC *hRet)
{
    FIXME(ole,"(%ld,%s): stub\n", hServer, docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/***********************************************************************
 *           OleRenameServerDoc32
 *
 */
OLESTATUS WINAPI OleRenameServerDoc32(LHSERVERDOC hDoc, LPCSTR newName)
{
    FIXME(ole,"(%ld,%s): stub.\n",hDoc, newName);
    return OLE_OK;
}
