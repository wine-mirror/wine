/*
 *	OLE2DISP library
 *
 *	Copyright 1995	Martin von Loewis
 */

#include <string.h>
#include "windows.h"
#include "ole.h"
#include "ole2.h"
#include "oleauto.h"
#include "wtypes.h"
#include "wine/obj_base.h"
#include "objbase.h"
#include "heap.h"
#include "ldt.h"
#include "debug.h"

/* This implementation of the BSTR API is 16-bit only. It
   represents BSTR as a 16:16 far pointer, and the strings
   as ISO-8859 */

/******************************************************************************
 *		BSTR_AllocBytes	[Internal]
 */
static BSTR16 BSTR_AllocBytes(int n)
{
    void *ptr = SEGPTR_ALLOC(n);
    return (BSTR16)SEGPTR_GET(ptr);
}

/******************************************************************************
 * BSTR_Free [INTERNAL]
 */
static void BSTR_Free(BSTR16 in)
{
    SEGPTR_FREE( PTR_SEG_TO_LIN(in) );
}

/******************************************************************************
 * BSTR_GetAddr [INTERNAL]
 */
static void* BSTR_GetAddr(BSTR16 in)
{
    return in ? PTR_SEG_TO_LIN(in) : 0;
}

/******************************************************************************
 *		SysAllocString16	[OLE2DISP.2]
 */
BSTR16 WINAPI SysAllocString16(LPOLESTR16 in)
{
	BSTR16 out=BSTR_AllocBytes(strlen(in)+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/******************************************************************************
 *		SysAllocString32	[OLEAUT32.2]
 */
BSTR32 WINAPI SysAllocString32(LPOLESTR32 in)
{
    /* Delegate this to the SysAllocStringLen32 method. */
    return SysAllocStringLen32(in, lstrlen32W(in));
}

/******************************************************************************
 *		SysReAllocString16	[OLE2DISP.3]
 */
INT16 WINAPI SysReAllocString16(LPBSTR16 old,LPOLESTR16 in)
{
	BSTR16 new=SysAllocString16(in);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/******************************************************************************
 *		SysReAllocString32	[OLEAUT32.3]
 */
INT32 WINAPI SysReAllocString32(LPBSTR32 old,LPOLESTR32 in)
{
    /*
     * Sanity check
     */
    if (old==NULL) 
      return 0;

    /*
     * Make sure we free the old string.
     */
    if (*old!=NULL)      
      SysFreeString32(*old);

    /*
     * Allocate the new string
     */
    *old = SysAllocString32(in);

     return 1;
}

/******************************************************************************
 *		SysAllocStringLen16	[OLE2DISP.4]
 */
BSTR16 WINAPI SysAllocStringLen16(char *in, int len)
{
	BSTR16 out=BSTR_AllocBytes(len+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/******************************************************************************
 *             SysAllocStringLen32     [OLEAUT32.4]
 *
 * In "Inside OLE, second edition" by Kraig Brockshmidt. In the Automation
 * section, he describes the DWORD value placed before the BSTR data type.
 * he describes it as a "DWORD count of characters". By experimenting with
 * a windows application, this count seems to be a DWORD count of bytes in
 * the string. Meaning that the count is double the number of wide 
 * characters in the string.
 */
BSTR32 WINAPI SysAllocStringLen32(WCHAR *in, int len)
{
    DWORD  bufferSize;
    DWORD* newBuffer;
    WCHAR* stringBuffer;

    /*
     * Find the lenth of the buffer passed-in in bytes.
     */
    bufferSize = len * sizeof (WCHAR);

    /*
     * Allocate a new buffer to hold the string.
     * dont't forget to keep an empty spot at the begining of the
     * buffer for the character count and an extra character at the
     * end for the NULL.
     */
    newBuffer = (DWORD*)HeapAlloc(GetProcessHeap(),
                                 0,
                                 bufferSize + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = bufferSize;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    /*
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (in != 0)
      memcpy(newBuffer, in, bufferSize);
    else
      memset(newBuffer, 0, bufferSize);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (WCHAR*)newBuffer;
    stringBuffer[len] = L'\0';

    return (LPWSTR)stringBuffer;
}

/******************************************************************************
 *		SysReAllocStringLen16	[OLE2DISP.5]
 */
int WINAPI SysReAllocStringLen16(BSTR16 *old,char *in,int len)
{
	BSTR16 new=SysAllocStringLen16(in,len);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

 
/******************************************************************************
 *             SysReAllocStringLen32   [OLEAUT32.5]
 */
int WINAPI SysReAllocStringLen32(BSTR32* old, WCHAR* in, int len)
{
    /*
     * Sanity check
     */
    if (old==NULL) 
      return 0;

    /*
     * Make sure we free the old string.
     */
    if (*old!=NULL)      
      SysFreeString32(*old);

    /*
     * Allocate the new string
     */
    *old = SysAllocStringLen32(in, len);

    return 1;
}

/******************************************************************************
 *		SysFreeString16	[OLE2DISP.6]
 */
void WINAPI SysFreeString16(BSTR16 in)
{
	BSTR_Free(in);
}

/******************************************************************************
 *		SysFreeString32	[OLEAUT32.6]
 */
void WINAPI SysFreeString32(BSTR32 in)
{
    DWORD* bufferPointer;

    /*
     * We have to be careful when we free a BSTR pointer, it points to
     * the beginning of the string but it skips the byte count contained
     * before the string.
     */
    bufferPointer = (DWORD*)in;

    bufferPointer--;

    /*
     * Free the memory from it's "real" origin.
     */
    HeapFree(GetProcessHeap(), 0, bufferPointer);
}

/******************************************************************************
 *		SysStringLen16	[OLE2DISP.7]
 */
int WINAPI SysStringLen16(BSTR16 str)
{
	return strlen(BSTR_GetAddr(str));
}

/******************************************************************************
 *             SysStringLen32  [OLEAUT32.7]
 *
 * The Windows documentation states that the length returned by this function
 * is not necessarely the same as the length returned by the _lstrlenW method.
 * It is the same number that was passed in as the "len" parameter if the
 * string was allocated with a SysAllocStringLen method call.
 */
int WINAPI SysStringLen32(BSTR32 str)
{
    DWORD* bufferPointer;

    /*
     * The length of the string (in bytes) is contained in a DWORD placed 
     * just before the BSTR pointer
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    return (int)(*bufferPointer/sizeof(WCHAR));
}

/******************************************************************************
 * CreateDispTypeInfo [OLE2DISP.31]
 */
OLESTATUS WINAPI CreateDispTypeInfo(
	INTERFACEDATA *pidata,
	LCID lcid,
	LPVOID **pptinfo /*ITypeInfo*/ 
) {
	FIXME(ole,"(%p,%ld,%p),stub\n",pidata,lcid,pptinfo);
	return 0;
}

/******************************************************************************
 * RegisterActiveObject [OLE2DISP.35]
 */
OLESTATUS WINAPI RegisterActiveObject(
	IUnknown * punk,REFCLSID rclsid,DWORD dwFlags, DWORD * pdwRegister
) {
	char	buf[80];
	WINE_StringFromCLSID(rclsid,buf);
	FIXME(ole,"(%p,%s,0x%08lx,%p):stub\n",punk,buf,dwFlags,pdwRegister);
	return 0;
}
