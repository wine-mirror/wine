/*
 * Copyright 1999 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "mmsystem.h"
#include "vfw.h"

static int WINAPIV mywprintf(const WCHAR *format, ...)
{
    static char output_bufA[65536];
    static WCHAR output_bufW[sizeof(output_bufA)];
    __ms_va_list parms;
    DWORD               nOut;
    BOOL                res = FALSE;
    HANDLE              hout = GetStdHandle(STD_OUTPUT_HANDLE);

    __ms_va_start(parms, format);
    vswprintf(output_bufW, ARRAY_SIZE(output_bufW), format, parms);
    __ms_va_end(parms);

    /* Try to write as unicode whenever we think it's a console */
    if (((DWORD_PTR)hout & 3) == 3)
    {
        res = WriteConsoleW(hout, output_bufW, lstrlenW(output_bufW), &nOut, NULL);
    }
    else
    {
        DWORD   convertedChars;

        /* Convert to OEM, then output */
        convertedChars = WideCharToMultiByte(GetConsoleOutputCP(), 0, output_bufW, -1,
                                             output_bufA, sizeof(output_bufA),
                                             NULL, NULL);
        res = WriteFile(hout, output_bufA, convertedChars, &nOut, FALSE);
    }

    return res ? nOut : 0;
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    int i, n=0,doabout=0,doconfigure=0;

    static const WCHAR header[] = {'C','u','r','r','e','n','t','l','y',' ','i','n','s','t','a','l','l','e','d',' ',
                                   'V','i','d','e','o',' ','C','o','m','p','r','e','s','s','o','r','s',':','\n',0};
    static const WCHAR close_flags[] = {')','\n',0};
    static const WCHAR s_fmt[] = {'%','s',0};
    static const WCHAR sspc_fmt[] = {'%','s',' ',0};
    static const WCHAR fcc_fmt[] = {'%','c','%','c','%','c','%','c','.','%','c','%','c','%','c','%','c',':',' ','%','s','\n',0};
    static const WCHAR desc_fmt[] = {'\t','s','z','D','e','s','c','r','i','p','t','i','o','n',':',' ','%','s','\n',0};
    static const WCHAR flags_fmt[] = {'\t','d','w','F','l','a','g','s',':',' ','0','x','%','0','8','x',' ','(',0};
    static const WCHAR version_fmt[] = {'\t','d','w','V','e','r','s','i','o','n',':',' ','0','x','%','0','8','x','\n',0};
    static const WCHAR versicm_fmt[] = {'\t','d','w','V','e','r','s','i','o','n','I','C','M',':',' ','0','x','%','0','8','x','\n',0};
    static const WCHAR VIDCF_QUALITY_W[] = {'V','I','D','C','F','_','Q','U','A','L','I','T','Y',0};
    static const WCHAR VIDCF_CRUNCH_W[] = {'V','I','D','C','F','_','C','R','U','N','C','H',0};
    static const WCHAR VIDCF_TEMPORAL_W[] = {'V','I','D','C','F','_','T','E','M','P','O','R','A','L',0};
    static const WCHAR VIDCF_COMPRESSFRAMES_W[] = {'V','I','D','C','F','_','C','O','M','P','R','E','S','S','F','R','A','M','E','S',0};
    static const WCHAR VIDCF_DRAW_W[] = {'V','I','D','C','F','_','D','R','A','W',0};
    static const WCHAR VIDCF_FASTTEMPORALC_W[] = {'V','I','D','C','F','_','F','A','S','T','T','E','M','P','O','R','A','L','C',0};
    static const WCHAR VIDCF_FASTTEMPORALD_W[] = {'V','I','D','C','F','_','F','A','S','T','T','E','M','P','O','R','A','L','D',0};
    static const WCHAR VIDCF_QUALITYTIME_W[] = {'V','I','D','C','F','_','Q','U','A','L','I','T','Y','T','I','M','E',0};
    static const WCHAR about[] = {'-','a','b','o','u','t','\0'};
    static const WCHAR configure[] = {'-','c','o','n','f','i','g','u','r','e','\0'};
    static const WCHAR unk_opt_fmt[] = {'U','n','k','n','o','w','n',' ','o','p','t','i','o','n',':',' ','%','s','\n',0};

    for (i = 1; i < argc; i++) {
        if (!lstrcmpW(argv[i], about))
            doabout = 1;
        else if (!lstrcmpW(argv[i], configure))
            doconfigure = 1;
        else {
            mywprintf(unk_opt_fmt, argv[i]);
            return -1;
        }
    }

    mywprintf(s_fmt, header);
    while (1) {
    	ICINFO	ii;
	HIC	hic;

	ii.dwSize = sizeof(ii);
    	if (!ICInfo(ICTYPE_VIDEO,n++,&ii))
	    break;
	if (!(hic=ICOpen(ii.fccType,ii.fccHandler,ICMODE_QUERY)))
	    continue;
	if (!ICGetInfo(hic,&ii,sizeof(ii))) {
	    ICClose(hic);
	    continue;
	}

	mywprintf(fcc_fmt,
                  LOBYTE(ii.fccType),LOBYTE(ii.fccType>>8),LOBYTE(ii.fccType>>16),LOBYTE(ii.fccType>>24),
                  LOBYTE(ii.fccHandler),LOBYTE(ii.fccHandler>>8),LOBYTE(ii.fccHandler>>16),LOBYTE(ii.fccHandler>>24),
                  ii.szName);
	mywprintf(flags_fmt,ii.dwFlags);

	if (ii.dwFlags & VIDCF_QUALITY) mywprintf(sspc_fmt, VIDCF_QUALITY_W);
	if (ii.dwFlags & VIDCF_CRUNCH) mywprintf(sspc_fmt, VIDCF_CRUNCH_W);
	if (ii.dwFlags & VIDCF_TEMPORAL) mywprintf(sspc_fmt, VIDCF_TEMPORAL_W);
	if (ii.dwFlags & VIDCF_COMPRESSFRAMES) mywprintf(sspc_fmt, VIDCF_COMPRESSFRAMES_W);
	if (ii.dwFlags & VIDCF_DRAW) mywprintf(sspc_fmt, VIDCF_DRAW_W);
	if (ii.dwFlags & VIDCF_FASTTEMPORALC) mywprintf(sspc_fmt, VIDCF_FASTTEMPORALC_W);
	if (ii.dwFlags & VIDCF_FASTTEMPORALD) mywprintf(sspc_fmt, VIDCF_FASTTEMPORALD_W);
	if (ii.dwFlags & VIDCF_QUALITYTIME) mywprintf(sspc_fmt, VIDCF_QUALITYTIME_W);

	mywprintf(s_fmt, close_flags);
	mywprintf(version_fmt,ii.dwVersion);
	mywprintf(versicm_fmt,ii.dwVersionICM);
	mywprintf(desc_fmt,ii.szDescription);
	if (doabout) ICAbout(hic,0);
	if (doconfigure && ICQueryConfigure(hic))
		ICConfigure(hic,0);
	ICClose(hic);
    }
    return 0;
}
