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
#include "debug.h"

extern LONG	OLE_current_handle;

/***********************************************************************
 *           OleRegisterClientDoc
 */
OLESTATUS WINAPI OleRegisterClientDoc16(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    fprintf(stdnimp,"OleRegisterClientDoc:%s %s\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/***********************************************************************
 *           OleRegisterClientDoc
 */
OLESTATUS WINAPI OleRegisterClientDoc32(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    fprintf(stdnimp,"OleRegisterClientDoc:%s %s\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/***********************************************************************
 *           OleRenameClientDoc
 */
OLESTATUS WINAPI OleRenameClientDoc16(LHCLIENTDOC hDoc, LPCSTR newName)
{
    fprintf(stdnimp,"OleRenameClientDoc: %ld %s\n",hDoc, newName);
    return OLE_OK;
}

/***********************************************************************
 *           OleRenameClientDoc
 */
OLESTATUS WINAPI OleRenameClientDoc32(LHCLIENTDOC hDoc, LPCSTR newName)
{
    fprintf(stdnimp,"OleRenameClientDoc: %ld %s\n",hDoc, newName);
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeClientDoc
 */
OLESTATUS WINAPI OleRevokeClientDoc16(LHCLIENTDOC hServerDoc)
{
    fprintf(stdnimp,"OleRevokeClientDoc:%ld\n",hServerDoc);
    return OLE_OK;
}

/***********************************************************************
 *           OleRevokeClientDoc
 */
OLESTATUS WINAPI OleRevokeClientDoc32(LHCLIENTDOC hServerDoc)
{
    fprintf(stdnimp,"OleRevokeClientDoc:%ld\n",hServerDoc);
    return OLE_OK;
}

/***********************************************************************
 *           OleCreateLinkFromClip32
 */
OLESTATUS WINAPI OleCreateLinkFromClip32( 
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	LPOLEOBJECT *lpoleob,OLEOPT_RENDER render,OLECLIPFORMAT clipformat
) {
	fprintf(stdnimp,"OleCreateLinkFromClip(%s,%p,%08lx,%s,%p,%d,%ld),stub!\n",
		name,olecli,hclientdoc,xname,lpoleob,render,clipformat
	);
	return OLE_OK;
}

/***********************************************************************
 *           OleQueryLinkFromClip32
 */
OLESTATUS WINAPI OleQueryLinkFromClip32(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	fprintf(stdnimp,"OleQueryLinkFromClip(%s,%d,%ld),stub!\n",
		name,render,clipformat
	);
	return OLE_OK;
}
/***********************************************************************
 *           OleQueryCreateFromClip32
 */
OLESTATUS WINAPI OleQueryCreateFromClip32(LPCSTR name,OLEOPT_RENDER render,OLECLIPFORMAT clipformat) {
	fprintf(stdnimp,"OleQueryCreateFromClip(%s,%d,%ld),stub!\n",
		name,render,clipformat
	);
	return OLE_OK;
}


/***********************************************************************
 *           OleIsDcMeta
 */
BOOL16 WINAPI OleIsDcMeta(HDC16 hdc)
{
	dprintf_info(ole,"OleIsDCMeta(%04x)\n",hdc);
	return GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ) != 0;
}

/***********************************************************************
 *           OleSetHostNames
 */
OLESTATUS WINAPI OleSetHostNames32(LPOLEOBJECT oleob,LPCSTR name1,LPCSTR name2) {
	fprintf(stdnimp,"OleSetHostNames(%p,%s,%s),stub\n",oleob,name1,name2);
	return OLE_OK;
}

/***********************************************************************
 *           OleQueryType32
 */
OLESTATUS WINAPI OleQueryType32(LPOLEOBJECT oleob,LONG*xlong) {
	fprintf(stdnimp,"OleQueryType(%p,%p),stub!\n",oleob,xlong);
	if (!oleob)
		return 0x10;
	fprintf(stddeb,"Calling OLEOBJECT.QueryType (%p) (%p,%p)\n",
		oleob->lpvtbl->QueryType,oleob,xlong
	);
	return oleob->lpvtbl->QueryType(oleob,xlong);
}

/***********************************************************************
 *           OleCreateFromClip
 */
OLESTATUS WINAPI OleCreateFromClip32(
	LPCSTR name,LPOLECLIENT olecli,LHCLIENTDOC hclientdoc,LPCSTR xname,
	LPOLEOBJECT *lpoleob,OLEOPT_RENDER render, OLECLIPFORMAT clipformat
) {
	fprintf(stdnimp,"OleCreateLinkFromClip(%s,%p,%08lx,%s,%p,%d,%ld),stub!\n",
		name,olecli,hclientdoc,xname,lpoleob,render,clipformat
	);
	/* clipb type, object kreieren entsprechend etc. */
	return OLE_OK;
}
