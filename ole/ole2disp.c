/*
 *	OLE2DISP library
 *
 *	Copyright 1995	Martin von Loewis
 */

#include "windows.h"
#include "ole.h"
#include "ole2.h"
#include "oleauto.h"
#include "interfaces.h"
#include "heap.h"
#include "ldt.h"
#include "debug.h"

/* This implementation of the BSTR API is 16-bit only. It
   represents BSTR as a 16:16 far pointer, and the strings
   as ISO-8859 */

static BSTR16 BSTR_AllocBytes(int n)
{
    void *ptr = SEGPTR_ALLOC(n);
    return (BSTR16)SEGPTR_GET(ptr);
}

static void BSTR_Free(BSTR16 in)
{
    SEGPTR_FREE( PTR_SEG_TO_LIN(in) );
}

static void* BSTR_GetAddr(BSTR16 in)
{
    return in ? PTR_SEG_TO_LIN(in) : 0;
}

/***********************************************************************
 *           SysAllocString         [OLE2DISP.2]
 */
BSTR16 WINAPI SysAllocString16(LPOLESTR16 in)
{
	BSTR16 out=BSTR_AllocBytes(strlen(in)+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/***********************************************************************
 *           SysAllocString         [OLEAUT32.2]
 */
BSTR32 WINAPI SysAllocString32(LPOLESTR32 in)
{
	return HEAP_strdupW(GetProcessHeap(),0,in);
}

/***********************************************************************
 *           SysReAllocString       [OLE2DISP.3]
 */
INT16 WINAPI SysReAllocString16(LPBSTR16 old,LPOLESTR16 in)
{
	BSTR16 new=SysAllocString16(in);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysReAllocString       [OLEAUT32.3]
 */
INT32 WINAPI SysReAllocString32(LPBSTR32 old,LPOLESTR32 in)
{
	BSTR32 new=SysAllocString32(in);
	HeapFree(GetProcessHeap(),0,*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysAllocStringLen      [OLE2DISP.4]
 */
BSTR16 WINAPI SysAllocStringLen16(char *in, int len)
{
	BSTR16 out=BSTR_AllocBytes(len+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/***********************************************************************
 *           SysReAllocStringLen    [OLE2DISP.5]
 */
int WINAPI SysReAllocStringLen16(BSTR16 *old,char *in,int len)
{
	BSTR16 new=SysAllocStringLen16(in,len);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysFreeString          [OLE2DISP.6]
 */
void WINAPI SysFreeString16(BSTR16 in)
{
	BSTR_Free(in);
}

/***********************************************************************
 *           SysFreeString          [OLEAUT32.6]
 */
void WINAPI SysFreeString32(BSTR32 in)
{
	HeapFree(GetProcessHeap(),0,in);
}

/***********************************************************************
 *           SysStringLen           [OLE2DISP.7]
 */
int WINAPI SysStringLen16(BSTR16 str)
{
	return strlen(BSTR_GetAddr(str));
}

OLESTATUS WINAPI CreateDispTypeInfo(INTERFACEDATA * pidata,LCID lcid,LPVOID/*ITypeInfo*/ * * pptinfo) {
	FIXME(ole,"(%p,%ld,%p),stub\n",pidata,lcid,pptinfo);
	return 0;
}

OLESTATUS WINAPI RegisterActiveObject(
	IUnknown * punk,REFCLSID rclsid,DWORD dwFlags, DWORD * pdwRegister
) {
	char	buf[80];
	WINE_StringFromCLSID(rclsid,buf);
	FIXME(ole,"RegisterActiveObject(%p,%s,0x%08lx,%p),stub\n",punk,buf,dwFlags,pdwRegister);
	return 0;
}
