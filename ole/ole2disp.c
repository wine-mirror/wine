/*
 *	OLE2DISP library
 *
 *	Copyright 1995	Martin von Loewis
 */

#include "windows.h"
#include "ole2.h"
#include "heap.h"
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"

/* This implementation of the BSTR API is 16-bit only. It
   represents BSTR as a 16:16 far pointer, and the strings
   as ISO-8859 */

typedef DWORD	BSTR;

static BSTR BSTR_AllocBytes(int n)
{
    void *ptr = SEGPTR_ALLOC(n);
    return SEGPTR_GET(ptr);
}

static void BSTR_Free(BSTR in)
{
    SEGPTR_FREE( PTR_SEG_TO_LIN(in) );
}

static void* BSTR_GetAddr(BSTR in)
{
    return in ? PTR_SEG_TO_LIN(in) : 0;
}

/***********************************************************************
 *           SysAllocString         [OLE2DISP.2]
 */
BSTR WINAPI SysAllocString(char *in)
{
	BSTR out=BSTR_AllocBytes(strlen(in)+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/***********************************************************************
 *           SysReAllocString       [OLE2DISP.3]
 */
int WINAPI SysReAllocString(BSTR *old,char *in)
{
	BSTR new=SysAllocString(in);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysAllocStringLen      [OLE2DISP.4]
 */
BSTR WINAPI SysAllocStringLen(char *in, int len)
{
	BSTR out=BSTR_AllocBytes(len+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/***********************************************************************
 *           SysReAllocStringLen    [OLE2DISP.5]
 */
int WINAPI SysReAllocStringLen(BSTR *old,char *in,int len)
{
	BSTR new=SysAllocStringLen(in,len);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysFreeString          [OLE2DISP.6]
 */
void WINAPI SysFreeString(BSTR in)
{
	BSTR_Free(in);
}

/***********************************************************************
 *           SysStringLen           [OLE2DISP.7]
 */
int WINAPI SysStringLen(BSTR str)
{
	return strlen(BSTR_GetAddr(str));
}
