/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdlib.h>
#include "winerror.h"
#include "winnls.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32)


/******************************************************************************
 * GetACP [KERNEL32.276]  Gets current ANSI code-page identifier.
 *
 * RETURNS
 *    Current ANSI code-page identifier, default if no current defined
 */
UINT WINAPI GetACP(void)
{
    /* This introduces too many messages */
/*    FIXME(win32, "(void): stub\n"); */
    return 1252;    /* Windows 3.1 ISO Latin */
}


/***********************************************************************
 *           GetCPInfo            (KERNEL32.154)
 */
BOOL WINAPI GetCPInfo( UINT codepage, LPCPINFO cpinfo )
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
UINT WINAPI GetOEMCP(void)
{
    return 437;    /* MS-DOS United States */
}

/***********************************************************************
 *           IsValidCodePage   (KERNEL32.360)
 */
BOOL WINAPI IsValidCodePage(UINT CodePage)
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
 *              MultiByteToWideChar                (KERNEL32.534)
 *
 * PARAMS
 *   page [in]   Codepage character set to convert from
 *   flags [in]  Character mapping flags
 *   src [in]     Source string buffer
 *   srclen [in]  Length of source string buffer
 *   dst [in]     Destination buffer
 *   dstlen [in]  Length of destination buffer
 *
 * NOTES
 *   The returned length includes the null terminator character.
 *
 * RETURNS
 *   Success: If dstlen > 0, number of characters written to destination
 *            buffer.  If dstlen == 0, number of characters needed to do
 *            conversion.
 *   Failure: 0. Occurs if not enough space is available.
 *
 * ERRORS
 *   ERROR_INSUFFICIENT_BUFFER
 *   ERROR_INVALID_FLAGS (not yet implemented)
 *   ERROR_INVALID_PARAMETER (not yet implemented)
 *
 * BUGS
 *   Does not properly handle codepage conversions.
 *   Does not properly handle flags.
 *
 */
INT WINAPI MultiByteToWideChar(UINT page, DWORD flags,
			         LPCSTR src, INT srclen,
                                 LPWSTR dst, INT dstlen)
{
    int ret;

    if (srclen == -1)
	srclen = lstrlenA(src)+1;
    if (!dstlen || !dst)
	return srclen;

    ret = srclen;
    while (srclen && dstlen) {
	*dst = (WCHAR)(unsigned char)*src;
	dst++;    src++;
	dstlen--; srclen--;
    }
    if (!dstlen && srclen) {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return 0;
    }
    return ret;
}

/***********************************************************************
 *              WideCharToMultiByte                (KERNEL32.727)
 *
 * PARAMS
 *   page [in]    Codepage character set to convert to
 *   flags [in]   Character mapping flags
 *   src [in]     Source string buffer
 *   srclen [in]  Length of source string buffer
 *   dst [in]     Destination buffer
 *   dstlen [in]  Length of destination buffer
 *   defchar [in] Default character to use for conversion if no exact
 *		    conversion can be made
 *   used [out]   Set if default character was used in the conversion
 *
 * NOTES
 *   The returned length includes the null terminator character.
 *
 * RETURNS
 *   Success: If dstlen > 0, number of characters written to destination
 *            buffer.  If dstlen == 0, number of characters needed to do
 *            conversion.
 *   Failure: 0. Occurs if not enough space is available.
 *
 * ERRORS
 *   ERROR_INSUFFICIENT_BUFFER
 *   ERROR_INVALID_FLAGS (not yet implemented)
 *
 * BUGS
 *   Does not properly handle codepage conversions.
 *   Does not properly handle flags.
 *
 */
INT WINAPI WideCharToMultiByte(UINT page, DWORD flags, LPCWSTR src,
				 INT srclen,LPSTR dst, INT dstlen,
				 LPCSTR defchar, BOOL *used)
{
    int count = 0;
    int eos = 0;
    int care_for_eos=0;
    int dont_copy= (dstlen==0);

    if ((!src) || ((!dst) && (!dont_copy)) )
    {	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    
    if (page!=GetACP() && page!=CP_OEMCP && page!=CP_ACP)
	FIXME("Conversion in CP %d not supported\n",page);
#if 0
    if (flags)
	FIXME("flags %lx not supported\n",flags);
#endif
    if(used)
	*used=0;
    if (srclen == -1)
      {
	srclen = lstrlenW(src)+1;
	 care_for_eos=1;
      }
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
	if((!*src) && care_for_eos) {
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
BOOL WINAPI IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
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
 *           IsDBCSLeadByte   (KERNEL32.358)
 */
BOOL WINAPI IsDBCSLeadByte( BYTE testchar )
{
    return IsDBCSLeadByteEx(GetACP(), testchar);
}


/***********************************************************************
 *              EnumSystemCodePagesA                (KERNEL32.92)
 */
BOOL WINAPI EnumSystemCodePagesA(CODEPAGE_ENUMPROCA lpfnCodePageEnum,DWORD flags)
{
	TRACE("(%p,%08lx)\n",lpfnCodePageEnum,flags);
	lpfnCodePageEnum("437");
	return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePagesW                (KERNEL32.93)
 */
BOOL WINAPI EnumSystemCodePagesW( CODEPAGE_ENUMPROCW lpfnCodePageEnum,
                                      DWORD flags)
{
    WCHAR	*cp;
    TRACE("(%p,%08lx)\n",lpfnCodePageEnum,flags );

    cp = HEAP_strdupAtoW( GetProcessHeap(), 0, "437" );
    lpfnCodePageEnum(cp);
    HeapFree( GetProcessHeap(), 0, cp );
    return TRUE;
}
