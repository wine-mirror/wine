/*
 *	OLECLI library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "winerror.h"
#include "ole.h"
#include "gdi.h"
#include "debug.h"
#include "ole2.h"
#include "objidl.h"


extern LONG	OLE_current_handle;

/******************************************************************************
 *		OleRegisterClientDoc16	[OLECLI.41]
 */
OLESTATUS WINAPI OleRegisterClientDoc16(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    FIXME(ole,"(%s,%s,...): stub\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRegisterClientDoc32	[OLECLI32.41]
 */
OLESTATUS WINAPI OleRegisterClientDoc32(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    FIXME(ole,"(%s,%s,...): stub\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameClientDoc16	[OLECLI.43]
 */
OLESTATUS WINAPI OleRenameClientDoc16(LHCLIENTDOC hDoc, LPCSTR newName)
{
    FIXME(ole,"(%ld,%s,...): stub\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameClientDoc32	[OLECLI32.43]
 */
OLESTATUS WINAPI OleRenameClientDoc32(LHCLIENTDOC hDoc, LPCSTR newName)
{
    FIXME(ole,"(%ld,%s,...): stub\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeClientDoc16	[OLECLI.42]
 */
OLESTATUS WINAPI OleRevokeClientDoc16(LHCLIENTDOC hServerDoc)
{
    FIXME(ole,"(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeClientDoc32	[OLECLI32.42]
 */
OLESTATUS WINAPI OleRevokeClientDoc32(LHCLIENTDOC hServerDoc)
{
    FIXME(ole,"(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *           OleCreateLinkFromClip32	[OLECLI32.11]
 */
OLESTATUS WINAPI OleCreateLinkFromClip32( 
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	LPOLEOBJECT *lpoleob,OLEOPT_RENDER render,OLECLIPFORMAT clipformat
) {
	FIXME(ole,"(%s,%p,%08lx,%s,%p,%d,%ld): stub!\n",
	      name,olecli,hclientdoc,xname,lpoleob,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryLinkFromClip32	[OLECLI32.9]
 */
OLESTATUS WINAPI OleQueryLinkFromClip32(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	FIXME(ole,"(%s,%d,%ld): stub!\n",name,render,clipformat);
	return OLE_OK;
}
/******************************************************************************
 *           OleQueryCreateFromClip32	[OLECLI32.10]
 */
OLESTATUS WINAPI OleQueryCreateFromClip32(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	FIXME(ole,"(%s,%d,%ld): stub!\n",name,render,clipformat);
	return OLE_OK;
}


/******************************************************************************
 *		OleIsDcMeta16	[OLECLI.60]
 */
BOOL16 WINAPI OleIsDcMeta16(HDC16 hdc)
{
	TRACE(ole,"(%04x)\n",hdc);
	if (GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ) != 0) {
	    GDI_HEAP_UNLOCK( hdc );
	    return TRUE;
	}
	return FALSE;
}


/******************************************************************************
 *		OleIsDcMeta32	[OLECLI32.60]
 */
BOOL32 WINAPI OleIsDcMeta32(HDC32 hdc)
{
        TRACE(ole,"(%04x)\n",hdc);
	if (GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ) != 0) {
	    GDI_HEAP_UNLOCK( hdc );
	    return TRUE;
	}
	return FALSE;
}


/******************************************************************************
 *		OleSetHostNames32	[OLECLI32.15]
 */
OLESTATUS WINAPI OleSetHostNames32(LPOLEOBJECT oleob,LPCSTR name1,LPCSTR name2) {
	FIXME(ole,"(%p,%s,%s): stub\n",oleob,name1,name2);
	return OLE_OK;
}

/******************************************************************************
 *		OleQueryType32	[OLECLI32.14]
 */
OLESTATUS WINAPI OleQueryType32(LPOLEOBJECT oleob,LONG*xlong) {
	FIXME(ole,"(%p,%p): stub!\n",oleob,xlong);
	if (!oleob)
		return 0x10;
	TRACE(ole,"Calling OLEOBJECT.QueryType (%p) (%p,%p)\n",
	      oleob->lpvtbl->QueryType,oleob,xlong);
	return oleob->lpvtbl->QueryType(oleob,xlong);
}

/******************************************************************************
 *		OleCreateFromClip32	[OLECLI32.12]
 */
OLESTATUS WINAPI OleCreateFromClip32(
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	LPOLEOBJECT *lpoleob,OLEOPT_RENDER render, OLECLIPFORMAT clipformat
) {
	FIXME(ole,"(%s,%p,%08lx,%s,%p,%d,%ld): stub!\n",
	      name,olecli,hclientdoc,xname,lpoleob,render,clipformat);
	/* clipb type, object kreieren entsprechend etc. */
	return OLE_OK;
}

/***********************************************************************
 *           OleIsCurrentClipboard32 [OLE32.110]
 */
HRESULT WINAPI OleIsCurrentClipboard32 (
  IDataObject *pDataObject)  /* ptr to the data obj previously copied or cut */
{
  FIXME(ole,"(DataObject %p): stub!\n", pDataObject);
  return S_FALSE;
}

