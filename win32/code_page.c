/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <malloc.h>
#include "windows.h"
#include "winerror.h"
#include "winnls.h"
#include "stddebug.h"
#include "debug.h"
#include "string32.h"


/***********************************************************************
 *           GetACP               (KERNEL32.148)
 */
UINT GetACP(void)
{
    return 1252;    /* Windows 3.1 ISO Latin */
}

/***********************************************************************
 *           GetCPInfo            (KERNEL32.154)
 */
BOOL GetCPInfo(UINT codepage, LPCPINFO cpinfo)
{
    cpinfo->MaxCharSize = 1;
    cpinfo->DefaultChar[0] = '?';

    return 1;
}

/***********************************************************************
 *              GetOEMCP                (KERNEL32.248)
 */
UINT GetOEMCP(void)
{
    return 437;    /* MS-DOS United States */
}

/***********************************************************************
 *              MultiByteToWideChar                (KERNEL32.392)
 */
int MultiByteToWideChar(UINT page, DWORD flags, char *src, int srclen,
                        WCHAR *dst, int dstlen)
{
    return (srclen==-1) ? strlen(src) * 2: srclen*2; 
}

int WideCharToMultiByte(UINT page, DWORD flags, WCHAR *src, int srclen,
						char *dst, int dstlen, char* defchar, BOOL *used)
{
	int count = 0;
	int dont_copy= (dstlen==0);
	if(page!=GetACP() && page!=CP_OEMCP && page!=CP_ACP)
		fprintf(stdnimp,"Conversion in CP %d not supported\n",page);
	if(flags)
		fprintf(stdnimp,"WideCharToMultiByte flags %lx not supported\n",flags);
	if(used)
		*used=0;
	while(srclen && (dont_copy || dstlen))
	{
		if(!dont_copy){
			if(*src<256)
				*dst = *src;
			else
			{
				/* FIXME: Is this correct ?*/
				if(flags & WC_DEFAULTCHAR){
					*dst = defchar ? *defchar : '?';
					if(used)*used=1;
				}
			}
			dstlen--;
			dst++;
		}
		count++;
		if(!*src)
			break;
		if(srclen!=-1)srclen--;
		src++;
	}
	return count;
}

/***********************************************************************
 *              EnumSystemCodePages32A                (KERNEL32.92)
 */
BOOL32
EnumSystemCodePages32A(CODEPAGE_ENUMPROC32A lpfnCodePageEnum,DWORD flags) {
	dprintf_win32(stddeb,"EnumSystemCodePages32A(%p,%08lx)\n",
		lpfnCodePageEnum,flags
	);
	lpfnCodePageEnum("437");
	return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePages32W                (KERNEL32.93)
 */
BOOL32
EnumSystemCodePages32W(CODEPAGE_ENUMPROC32W lpfnCodePageEnum,DWORD flags) {
	WCHAR	*cp;
	dprintf_win32(stddeb,"EnumSystemCodePages32W(%p,%08lx)\n",
		lpfnCodePageEnum,flags
	);

	cp=STRING32_DupAnsiToUni("437");
	lpfnCodePageEnum(cp);
	free(cp);
	return TRUE;
}
