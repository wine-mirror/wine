/*
 *	OLESVR library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "ole.h"
#include "stddebug.h"
#include "debug.h"

LONG	OLE_current_handle;

/***********************************************************************
 *           OleRegisterServer
 */
OLESTATUS OleRegisterServer( LPCSTR name, LPOLESERVER serverStruct,
                             LHSERVER *hRet, HINSTANCE16 hServer,
                             OLE_SERVER_USE use )
{
    dprintf_ole(stdnimp,"OleRegisterServer:%s\n",name);
    *hRet=++OLE_current_handle;
    /* return OLE_ERROR_MEMORY, OLE_ERROR_PROTECT_ONLY if you want it fail*/
    return OLE_OK;
}

/***********************************************************************
 *           OleBlockServer
 */
OLESTATUS OleBlockServer(LHSERVER hServer)
{
    fprintf(stdnimp,"OleBlockServer:%ld\n",hServer);
    return OLE_OK;
}

/***********************************************************************
 *           OleUnblockServer
 */
OLESTATUS OleUnblockServer(LHSERVER hServer, BOOL *block)
{
    fprintf(stdnimp,"OleUnblockServer:%ld\n",hServer);
    /* no more blocked messages :) */
    *block=FALSE;
    return OLE_OK;
}

/***********************************************************************
 *           OleRegisterServerDoc
 */
OLESTATUS OleRegisterServerDoc( LHSERVER hServer, LPCSTR docname,
                                LPOLESERVERDOC document, LHSERVERDOC *hRet)
{
    dprintf_ole(stdnimp,"OleRegisterServerDoc:%ld,%s\n", hServer, docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeServerDoc
 */
OLESTATUS OleRevokeServerDoc(LHSERVERDOC hServerDoc)
{
    dprintf_ole(stdnimp,"OleRevokeServerDoc:%ld\n",hServerDoc);
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeServer
 */
OLESTATUS OleRevokeServer(LHSERVER hServer)
{
    dprintf_ole(stdnimp,"OleRevokeServer:%ld\n",hServer);
    return OLE_OK;
}
