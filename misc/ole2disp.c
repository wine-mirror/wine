/*
 *	OLE2DISP library
 *
 *	Copyright 1995	Martin von Loewis
 */

#include "windows.h"
#include "ole2.h"
#include "global.h"
#include "local.h"
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"

/* This implementation of the BSTR API is 16-bit only. It
   represents BSTR as a 16:16 far pointer, and the strings
   as ISO-8859 */

typedef DWORD	BSTR;
HGLOBAL	BSTRheapsel=0;
#define BSTR_HEAP_SIZE	65536

static BSTR BSTR_AllocBytes(int n)
{
	HLOCAL mem;
	if(!BSTRheapsel)
	{
		BSTRheapsel=GlobalAlloc(GMEM_FIXED,BSTR_HEAP_SIZE);
		LocalInit(BSTRheapsel,0,BSTR_HEAP_SIZE-1);
	}
	if(!BSTRheapsel)
		return 0;
	mem=LOCAL_Alloc(BSTRheapsel,LMEM_FIXED,n);
	return mem ? MAKELONG(mem,BSTRheapsel) : 0;
}

static void BSTR_Free(BSTR in)
{
	LOCAL_Free(BSTRheapsel, LOWORD(in));
}

static void* BSTR_GetAddr(BSTR in)
{
	return in ? PTR_SEG_TO_LIN(in) : 0;
}

/***********************************************************************
 *           SysAllocString         [OLE2DISP.2]
 */
BSTR SysAllocString(char *in)
{
	BSTR out=BSTR_AllocBytes(strlen(in)+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/***********************************************************************
 *           SysReAllocString       [OLE2DISP.3]
 */
int SysReAllocString(BSTR *old,char *in)
{
	BSTR new=SysAllocString(in);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysAllocStringLen      [OLE2DISP.4]
 */
BSTR SysAllocStringLen(char *in, int len)
{
	BSTR out=BSTR_AllocBytes(len+1);
	if(!out)return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
}

/***********************************************************************
 *           SysReAllocStringLen    [OLE2DISP.5]
 */
int SysReAllocStringLen(BSTR *old,char *in,int len)
{
	BSTR new=SysAllocStringLen(in,len);
	BSTR_Free(*old);
	*old=new;
	return 1;
}

/***********************************************************************
 *           SysFreeString          [OLE2DISP.6]
 */
void SysFreeString(BSTR in)
{
	BSTR_Free(in);
}

/***********************************************************************
 *           SysStringLen           [OLE2DISP.7]
 */
int SysStringLen(BSTR str)
{
	return strlen(BSTR_GetAddr(str));
}
