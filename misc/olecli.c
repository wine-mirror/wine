/*
 *	OLECLI library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "ole.h"
#include "gdi.h"
#include "stddebug.h"
#include "debug.h"

extern LONG	OLE_current_handle;

/***********************************************************************
 *           OleRegisterClientDoc
 */
OLESTATUS OleRegisterClientDoc(	LPCSTR classname, LPCSTR docname,
                                LONG reserved, LHCLIENTDOC *hRet )
{
    dprintf_ole(stdnimp,"OleRegisterClientDoc:%s %s\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/***********************************************************************
 *           OleRenameClientDoc
 */
OLESTATUS OleRenameClientDoc(LHCLIENTDOC hDoc, LPCSTR newName)
{
    dprintf_ole(stdnimp,"OleRenameClientDoc: %ld %s\n",hDoc, newName);
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeClientDoc
 */
OLESTATUS OleRevokeClientDoc(LHCLIENTDOC hServerDoc)
{
    dprintf_ole(stdnimp,"OleRevokeClientDoc:%ld\n",hServerDoc);
    return OLE_OK;
}

/***********************************************************************
 *           OleIsDcMeta
 */
BOOL OleIsDcMeta(HDC hdc)
{
	dprintf_ole(stddeb,"OleIsDCMeta(%04x)\n",hdc);
	return GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ) != 0;
}
