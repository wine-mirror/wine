/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "winerror.h"
#include "winnls.h"
#include "heap.h"
#include "debug.h"


/***********************************************************************
 *           GetACP               (KERNEL32.148)
 */
UINT32 WINAPI GetACP(void)
{
    return 1252;    /* Windows 3.1 ISO Latin */
}

/***********************************************************************
 *           GetCPInfo            (KERNEL32.154)
 */
BOOL32 WINAPI GetCPInfo( UINT32 codepage, LPCPINFO cpinfo )
{
    cpinfo->DefaultChar[0] = '?';
    switch (codepage)
    {
    case 932 : /* Shift JIS (japan) */
        cpinfo->MaxCharSize = 2;
        cpinfo->LeadByte[0]= 0x81; cpinfo->LeadByte[1] = 0x9F;
        cpinfo->LeadByte[2]= 0xE0; cpinfo->LeadByte[3] = 0xFC;
        cpinfo->LeadByte[4]= 0x00; cpinfo->LeadByte[5] = 0x00;
        break;
    case 936 : /* GB2312 (Chinese) */
    case 949 : /* KSC5601-1987 (Korean) */
    case 950 : /* BIG5 (Chinese) */
        cpinfo->MaxCharSize = 2;
        cpinfo->LeadByte[0]= 0x81; cpinfo->LeadByte[1] = 0xFE;
        cpinfo->LeadByte[2]= 0x00; cpinfo->LeadByte[3] = 0x00;
        break;
    case 1361 : /* Johab (Korean) */
        cpinfo->MaxCharSize = 2;
        cpinfo->LeadByte[0]= 0x84; cpinfo->LeadByte[1] = 0xD3;
        cpinfo->LeadByte[2]= 0xD8; cpinfo->LeadByte[3] = 0xDE;
        cpinfo->LeadByte[4]= 0xE0; cpinfo->LeadByte[5] = 0xF9;
        cpinfo->LeadByte[6]= 0x00; cpinfo->LeadByte[7] = 0x00;
        break;
    default :
    	cpinfo->MaxCharSize = 1;
        cpinfo->LeadByte[0]= 0x00; cpinfo->LeadByte[1] = 0x00;
    	break;
    }
    return 1;
}

/***********************************************************************
 *              GetOEMCP                (KERNEL32.248)
 */
UINT32 WINAPI GetOEMCP(void)
{
    return 437;    /* MS-DOS United States */
}

/***********************************************************************
 *           IsValidCodePage   (KERNEL32.360)
 */
BOOL32 WINAPI IsValidCodePage(UINT32 CodePage)
{
    switch ( CodePage )
    {
    case 1252 :
    case 437 :
        return TRUE;
    default :
        return FALSE;
    }
}


/***********************************************************************
 *              MultiByteToWideChar                (KERNEL32.392)
 */
INT32 WINAPI MultiByteToWideChar(UINT32 page, DWORD flags,
			         LPCSTR src, INT32 srclen,
                                 LPWSTR dst, INT32 dstlen)
{
    int ret;

    if (srclen == -1)
	srclen = lstrlen32A(src)+1;
    if (!dstlen || !dst)
	return srclen;

    ret = srclen;
    while (srclen > 0 && dstlen > 0) {
	*dst = (WCHAR)(unsigned char)*src;
	if (!*src)
	    return ret;
	dst++;    src++;
	dstlen--; srclen--;
    }
    if (dstlen == 0) {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return 0;
    }
    return ret;
}

INT32 WINAPI WideCharToMultiByte(UINT32 page, DWORD flags, LPCWSTR src,
				 INT32 srclen,LPSTR dst, INT32 dstlen,
				 LPCSTR defchar, BOOL32 *used)
{
    int count = 0;
    int eos = 0;
    int dont_copy= (dstlen==0);
    if (page!=GetACP() && page!=CP_OEMCP && page!=CP_ACP)
	fprintf(stdnimp,"Conversion in CP %d not supported\n",page);
#if 0
    if (flags)
	fprintf(stdnimp,"WideCharToMultiByte flags %lx not supported\n",flags);
#endif
    if(used)
	*used=0;
    if (srclen == -1)
	srclen = lstrlen32W(src)+1;
    while(srclen && (dont_copy || dstlen))
    {
	if(!dont_copy){
	    if(*src<256)
		*dst = *src;
	    else
	    {
		/* ??? The WC_DEFAULTCHAR flag only gets used in
		 * combination with the WC_COMPOSITECHECK flag or at
		 * least this is what it seems from using the function
		 * on NT4.0 in combination with reading the documentation.
		 */
		*dst = defchar ? *defchar : '?';
		if(used)*used=1;
	    }
	    dstlen--;
	    dst++;
	}
	count++;
	srclen--;
	if(!*src) {
	    eos = 1;
	    break;
	}
	src++;
    }
    if (dont_copy)
	return count;

    if (!eos && srclen > 0) {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return 0;
    }
    return count;
}


/***********************************************************************
 *           IsDBCSLeadByteEx   (KERNEL32.359)
 */
BOOL32 WINAPI IsDBCSLeadByteEx( UINT32 codepage, BYTE testchar )
{
    CPINFO cpinfo;
    int i;

    GetCPInfo(codepage, &cpinfo);
    for (i = 0 ; i < sizeof(cpinfo.LeadByte)/sizeof(cpinfo.LeadByte[0]); i+=2)
    {
	if (cpinfo.LeadByte[i] == 0)
            return FALSE;
	if (cpinfo.LeadByte[i] <= testchar && testchar <= cpinfo.LeadByte[i+1])
            return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           IsDBCSLeadByte16   (KERNEL.207)
 */
BOOL16 WINAPI IsDBCSLeadByte16( BYTE testchar )
{
    return IsDBCSLeadByteEx(GetACP(), testchar);
}


/***********************************************************************
 *           IsDBCSLeadByte32   (KERNEL32.358)
 */
BOOL32 WINAPI IsDBCSLeadByte32( BYTE testchar )
{
    return IsDBCSLeadByteEx(GetACP(), testchar);
}


/***********************************************************************
 *              EnumSystemCodePages32A                (KERNEL32.92)
 */
BOOL32 WINAPI EnumSystemCodePages32A(CODEPAGE_ENUMPROC32A lpfnCodePageEnum,DWORD flags)
{
	dprintf_info(win32,"EnumSystemCodePages32A(%p,%08lx)\n",
		lpfnCodePageEnum,flags
	);
	lpfnCodePageEnum("437");
	return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePages32W                (KERNEL32.93)
 */
BOOL32 WINAPI EnumSystemCodePages32W( CODEPAGE_ENUMPROC32W lpfnCodePageEnum,
                                      DWORD flags)
{
    WCHAR	*cp;
    dprintf_info(win32,"EnumSystemCodePages32W(%p,%08lx)\n",
                  lpfnCodePageEnum,flags );

    cp = HEAP_strdupAtoW( GetProcessHeap(), 0, "437" );
    lpfnCodePageEnum(cp);
    HeapFree( GetProcessHeap(), 0, cp );
    return TRUE;
}
