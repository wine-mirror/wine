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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "mmsystem.h"
#include "vfw.h"


int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    int n=0,doabout=0,doconfigure=0;
    char	buf[128],type[5],handler[5];
    HMODULE	msvfw32 = LoadLibrary("msvfw32.dll");

BOOL  (VFWAPI  *fnICInfo)(DWORD fccType, DWORD fccHandler, ICINFO * lpicinfo);
LRESULT (VFWAPI *fnICClose)(HIC hic);
HIC	(VFWAPI	*fnICOpen)(DWORD fccType, DWORD fccHandler, UINT wMode);
LRESULT	(VFWAPI	*fnICGetInfo)(HIC hic,ICINFO *picinfo, DWORD cb);
LRESULT	(VFWAPI	*fnICSendMessage)(HIC hic, UINT msg, DWORD dw1, DWORD dw2);

#define XX(x) fn##x = (void*)GetProcAddress(msvfw32,#x);
	XX(ICInfo);
	XX(ICOpen);
	XX(ICClose);
	XX(ICGetInfo);
	XX(ICSendMessage);
#undef XX

    if (strstr(cmdline,"-about"))
    	doabout = 1;
    if (strstr(cmdline,"-configure"))
    	doconfigure = 1;

    printf("Currently installed Video Compressors:\n");
    while (1) {
    	ICINFO	ii;
	HIC	hic;

	ii.dwSize = sizeof(ii);
    	if (!fnICInfo(ICTYPE_VIDEO,n++,&ii))
	    break;
	if (!(hic=fnICOpen(ii.fccType,ii.fccHandler,ICMODE_QUERY)))
	    continue;
	if (!fnICGetInfo(hic,&ii,sizeof(ii))) {
	    fnICClose(hic);
	    continue;
	}
#define w2s(w,s) WideCharToMultiByte(0,0,w,-1,s,128,0,NULL)

	w2s(ii.szName,buf);
	memcpy(type,&(ii.fccType),4);type[4]='\0';
	memcpy(handler,&(ii.fccHandler),4);handler[4]='\0';
	printf("%s.%s: %s\n",type,handler,buf);
	printf("\tdwFlags: 0x%08lx (",ii.dwFlags);
#define XX(x) if (ii.dwFlags & x) printf(#x" ");
	XX(VIDCF_QUALITY);
	XX(VIDCF_CRUNCH);
	XX(VIDCF_TEMPORAL);
	XX(VIDCF_COMPRESSFRAMES);
	XX(VIDCF_DRAW);
	XX(VIDCF_FASTTEMPORALC);
	XX(VIDCF_FASTTEMPORALD);
	XX(VIDCF_QUALITYTIME);
#undef XX
	printf(")\n");
	printf("\tdwVersion: 0x%08lx\n",ii.dwVersion);
	printf("\tdwVersionICM: 0x%08lx\n",ii.dwVersionICM);
	w2s(ii.szDescription,buf);
	printf("\tszDescription: %s\n",buf);
	if (doabout) ICAbout(hic,0);
	if (doconfigure && ICQueryConfigure(hic))
		ICConfigure(hic,0);
	fnICClose(hic);
    }
    return 0;
}

