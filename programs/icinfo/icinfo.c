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


int main(int argc, char* argv[])
{
    int i, n=0,doabout=0,doconfigure=0;
    char	buf[128],type[5],handler[5];

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-about"))
            doabout = 1;
        else if (!strcmp(argv[i], "-configure"))
            doconfigure = 1;
        else {
            printf("Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    printf("Currently installed Video Compressors:\n");
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
#define w2s(w,s) WideCharToMultiByte(0,0,w,-1,s,128,0,NULL)

	w2s(ii.szName,buf);
	memcpy(type,&(ii.fccType),4);type[4]='\0';
	memcpy(handler,&(ii.fccHandler),4);handler[4]='\0';
	printf("%s.%s: %s\n",type,handler,buf);
	printf("\tdwFlags: 0x%08x (",ii.dwFlags);
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
	printf("\tdwVersion: 0x%08x\n",ii.dwVersion);
	printf("\tdwVersionICM: 0x%08x\n",ii.dwVersionICM);
	w2s(ii.szDescription,buf);
	printf("\tszDescription: %s\n",buf);
	if (doabout) ICAbout(hic,0);
	if (doconfigure && ICQueryConfigure(hic))
		ICConfigure(hic,0);
	ICClose(hic);
    }
    return 0;
}
