/*
 *	OLECLI library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "winerror.h"
#include "ole.h"
#include "gdi.h"
#include "objidl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole)


extern LONG	OLE_current_handle;

/******************************************************************************
 *		OleRegisterClientDoc16	[OLECLI.41]
 */
OLESTATUS WINAPI OleRegisterClientDoc16(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    FIXME("(%s,%s,...): stub\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRegisterClientDoc32	[OLECLI32.41]
 */
OLESTATUS WINAPI OleRegisterClientDoc(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    FIXME("(%s,%s,...): stub\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameClientDoc16	[OLECLI.43]
 */
OLESTATUS WINAPI OleRenameClientDoc16(LHCLIENTDOC hDoc, LPCSTR newName)
{
    FIXME("(%ld,%s,...): stub\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameClientDoc32	[OLECLI32.43]
 */
OLESTATUS WINAPI OleRenameClientDoc(LHCLIENTDOC hDoc, LPCSTR newName)
{
    FIXME("(%ld,%s,...): stub\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeClientDoc16	[OLECLI.42]
 */
OLESTATUS WINAPI OleRevokeClientDoc16(LHCLIENTDOC hServerDoc)
{
    FIXME("(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeClientDoc32	[OLECLI32.42]
 */
OLESTATUS WINAPI OleRevokeClientDoc(LHCLIENTDOC hServerDoc)
{
    FIXME("(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevertClientDoc16	[OLECLI.44]
 */
OLESTATUS WINAPI OleRevertClientDoc16(LHCLIENTDOC hServerDoc)
{
    FIXME("(%ld): stub\n", hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleEnumObjects16	[OLECLI.47]
 */
OLESTATUS WINAPI OleEnumObjects16(LHCLIENTDOC hServerDoc, SEGPTR data)
{
    FIXME("(%ld, %04x:%04x): stub\n", hServerDoc, HIWORD(data),
	LOWORD(data));
    return OLE_OK;
}

/******************************************************************************
 *	     OleCreateLinkFromClip16	[OLECLI.11]
 */
OLESTATUS WINAPI OleCreateLinkFromClip16(
	LPCSTR name, LPOLECLIENT olecli, LHCLIENTDOC hclientdoc, LPCSTR xname,
	LPOLEOBJECT *lpoleob, UINT16 render, UINT16 clipformat
) {
	FIXME("(%s, %04x:%04x, %ld, %s, %04x:%04x, %d, %d): stub!\n",
	    (char *)PTR_SEG_TO_LIN(name), HIWORD(olecli), LOWORD(olecli),
	    hclientdoc, (char *)PTR_SEG_TO_LIN(xname), HIWORD(lpoleob),
	    LOWORD(lpoleob), render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleCreateLinkFromClip32	[OLECLI32.11]
 */
OLESTATUS WINAPI OleCreateLinkFromClip( 
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	LPOLEOBJECT *lpoleob,OLEOPT_RENDER render,OLECLIPFORMAT clipformat
) {
	FIXME("(%s,%p,%08lx,%s,%p,%d,%ld): stub!\n",
	      name,olecli,hclientdoc,xname,lpoleob,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryLinkFromClip16	[OLECLI.9]
 */
OLESTATUS WINAPI OleQueryLinkFromClip16(LPCSTR name, UINT16 render, UINT16 clipformat) {
	FIXME("(%s, %d, %d): stub!\n", (char*)(PTR_SEG_TO_LIN(name)),
		render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryLinkFromClip32	[OLECLI32.9]
 */
OLESTATUS WINAPI OleQueryLinkFromClip(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	FIXME("(%s,%d,%ld): stub!\n",name,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryCreateFromClip16	[OLECLI.10]
 */
OLESTATUS WINAPI OleQueryCreateFromClip16(LPCSTR name, UINT16 render, UINT16 clipformat) {
	FIXME("(%s, %d, %d): stub!\n", (char*)(PTR_SEG_TO_LIN(name)),
		render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryCreateFromClip32	[OLECLI32.10]
 */
OLESTATUS WINAPI OleQueryCreateFromClip(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	FIXME("(%s,%d,%ld): stub!\n",name,render,clipformat);
	return OLE_OK;
}

/******************************************************************************
 *		OleIsDcMeta16	[OLECLI.60]
 */
BOOL16 WINAPI OleIsDcMeta16(HDC16 hdc)
{
	TRACE("(%04x)\n",hdc);
	if (GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ) != 0) {
	    GDI_HEAP_UNLOCK( hdc );
	    return TRUE;
	}
	return FALSE;
}


/******************************************************************************
 *		OleIsDcMeta32	[OLECLI32.60]
 */
BOOL WINAPI OleIsDcMeta(HDC hdc)
{
        TRACE("(%04x)\n",hdc);
	if (GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ) != 0) {
	    GDI_HEAP_UNLOCK( hdc );
	    return TRUE;
	}
	return FALSE;
}


/******************************************************************************
 *		OleSetHostNames32	[OLECLI32.15]
 */
OLESTATUS WINAPI OleSetHostNames(LPOLEOBJECT oleob,LPCSTR name1,LPCSTR name2) {
	FIXME("(%p,%s,%s): stub\n",oleob,name1,name2);
	return OLE_OK;
}

/******************************************************************************
 *		OleQueryType16	[OLECLI.14]
 */
OLESTATUS WINAPI OleQueryType16(LPOLEOBJECT oleob,  SEGPTR xlong) {
	FIXME("(%p, %p): stub!\n",
		PTR_SEG_TO_LIN(oleob), PTR_SEG_TO_LIN(xlong));
	return OLE_OK;
}

/******************************************************************************
 *		OleQueryType32	[OLECLI32.14]
 */
OLESTATUS WINAPI OleQueryType(LPOLEOBJECT oleob,LONG*xlong) {
	FIXME("(%p,%p): stub!\n",oleob,xlong);
	if (!oleob)
		return 0x10;
	TRACE("Calling OLEOBJECT.QueryType (%p) (%p,%p)\n",
	      oleob->lpvtbl->QueryType,oleob,xlong);
	return oleob->lpvtbl->QueryType(oleob,xlong);
}

/******************************************************************************
 *		OleCreateFromClip16	[OLECLI.12]
 */
OLESTATUS WINAPI OleCreateFromClip16(
	LPCSTR name, LPOLECLIENT olecli, LHCLIENTDOC hclientdoc, LPCSTR xname,
	LPOLEOBJECT *lpoleob, UINT16 render, UINT16 clipformat
) {
	FIXME("(%s, %04x:%04x, %ld, %s, %04x:%04x, %d, %d): stub!\n",
	    (char *)PTR_SEG_TO_LIN(name), HIWORD(olecli), LOWORD(olecli),
	    hclientdoc, (char *)PTR_SEG_TO_LIN(xname), HIWORD(lpoleob),
	    LOWORD(lpoleob), render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *		OleCreateFromClip32	[OLECLI32.12]
 */
OLESTATUS WINAPI OleCreateFromClip(
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	LPOLEOBJECT *lpoleob,OLEOPT_RENDER render, OLECLIPFORMAT clipformat
) {
	FIXME("(%s,%p,%08lx,%s,%p,%d,%ld): stub!\n",
	      name,olecli,hclientdoc,xname,lpoleob,render,clipformat);
	/* clipb type, object kreieren entsprechend etc. */
	return OLE_OK;
}

/***********************************************************************
 *           OleIsCurrentClipboard32 [OLE32.110]
 */
HRESULT WINAPI OleIsCurrentClipboard (
  IDataObject *pDataObject)  /* ptr to the data obj previously copied or cut */
{
  FIXME("(DataObject %p): stub!\n", pDataObject);
  return S_FALSE;
}

