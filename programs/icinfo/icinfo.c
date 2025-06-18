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
    va_list parms;
    DWORD               nOut;
    BOOL                res = FALSE;
    HANDLE              hout = GetStdHandle(STD_OUTPUT_HANDLE);

    va_start(parms, format);
    vswprintf(output_bufW, ARRAY_SIZE(output_bufW), format, parms);
    va_end(parms);

    res = WriteConsoleW(hout, output_bufW, lstrlenW(output_bufW), &nOut, NULL);
    if (!res)
    {
        DWORD   convertedChars;

        /* Convert to OEM, then output */
        convertedChars = WideCharToMultiByte(GetOEMCP(), 0, output_bufW, -1,
                                             output_bufA, sizeof(output_bufA),
                                             NULL, NULL);
        res = WriteFile(hout, output_bufA, convertedChars, &nOut, FALSE);
    }

    return res ? nOut : 0;
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    int i, n=0,doabout=0,doconfigure=0;

    for (i = 1; i < argc; i++) {
        if (!lstrcmpW(argv[i], L"-about"))
            doabout = 1;
        else if (!lstrcmpW(argv[i], L"-configure"))
            doconfigure = 1;
        else {
            mywprintf(L"Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    mywprintf(L"%s", L"Currently installed Video Compressors:\n");
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

        mywprintf(L"%c%c%c%c.%c%c%c%c: %s\n",
                  LOBYTE(ii.fccType),LOBYTE(ii.fccType>>8),LOBYTE(ii.fccType>>16),LOBYTE(ii.fccType>>24),
                  LOBYTE(ii.fccHandler),LOBYTE(ii.fccHandler>>8),LOBYTE(ii.fccHandler>>16),LOBYTE(ii.fccHandler>>24),
                  ii.szName);
        mywprintf(L"\tdwFlags: 0x%08x (",ii.dwFlags);

        if (ii.dwFlags & VIDCF_QUALITY) mywprintf(L"%s ", L"VIDCF_QUALITY");
        if (ii.dwFlags & VIDCF_CRUNCH) mywprintf(L"%s ", L"VIDCF_CRUNCH");
        if (ii.dwFlags & VIDCF_TEMPORAL) mywprintf(L"%s ", L"VIDCF_TEMPORAL");
        if (ii.dwFlags & VIDCF_COMPRESSFRAMES) mywprintf(L"%s ", L"VIDCF_COMPRESSFRAMES");
        if (ii.dwFlags & VIDCF_DRAW) mywprintf(L"%s ", L"VIDCF_DRAW");
        if (ii.dwFlags & VIDCF_FASTTEMPORALC) mywprintf(L"%s ", L"VIDCF_FASTTEMPORALC");
        if (ii.dwFlags & VIDCF_FASTTEMPORALD) mywprintf(L"%s ", L"VIDCF_FASTTEMPORALD");
        if (ii.dwFlags & VIDCF_QUALITYTIME) mywprintf(L"%s ", L"VIDCF_QUALITYTIME");

        mywprintf(L"%s", L")\n");
        mywprintf(L"\tdwVersion: 0x%08x\n", ii.dwVersion);
        mywprintf(L"\tdwVersionICM: 0x%08x\n", ii.dwVersionICM);
        mywprintf(L"\tszDescription: %s\n", ii.szDescription);
	if (doabout) ICAbout(hic,0);
	if (doconfigure && ICQueryConfigure(hic))
		ICConfigure(hic,0);
	ICClose(hic);
    }
    return 0;
}
