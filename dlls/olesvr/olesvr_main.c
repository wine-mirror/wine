/*
 *	OLESVR library
 *
 *	Copyright 1995	Martin von Loewis
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

/*	At the moment, these are only empty stubs.
 */

#include "config.h"

#include "ole.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static LONG OLE_current_handle;

/******************************************************************************
 *		OleRegisterServer	[OLESVR.2]
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
 *		OleBlockServer	[OLESVR.4]
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
 *		OleUnblockServer	[OLESVR.5]
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
 *		OleRegisterServerDoc	[OLESVR.6]
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
 *		OleRevokeServerDoc	[OLESVR.7]
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
	FIXME("(%s,%p,%p,%p,%d): stub!\n",svrname,olesvr,hRet,hinst,osu);
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
