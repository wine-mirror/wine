/*
 * Very simple AVIPLAYER
 *
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
 *
 * Status:
 * 	- plays .avi streams, video only
 *	- requires Microsoft avifil32.dll and builtin msvfw32.dll.
 *
 * Todo:
 *	- audio support (including synchronization etc)
 *	- replace DirectDraw by a 'normal' window, including dithering, controls
 *	  etc.
 *
 * Bugs:
 *	- no time scheduling, video plays too fast using DirectDraw/XF86DGA
 *	- requires DirectDraw with all disadvantages.
 */

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windows.h"
#include "wingdi.h"
#include "mmsystem.h"
#include "ddraw.h"
#include "vfw.h"


int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    int			bytesline,i,n,pos;
    time_t		tstart,tend;
    LONG		cnt;
    BITMAPINFOHEADER	*bmi;
    HRESULT		hres;
    PAVIFILE		avif;
    PAVISTREAM		vids=NULL,auds=NULL;
    AVIFILEINFO		afi;
    AVISTREAMINFO	asi;
    PGETFRAME		vidgetframe=NULL;
    LPDIRECTDRAW	ddraw;
    DDSURFACEDESC	dsdesc;
    LPDIRECTDRAWSURFACE dsurf;
    LPDIRECTDRAWPALETTE dpal;
    PALETTEENTRY	palent[256];

    AVIFileInit();
    if (GetFileAttributes(cmdline) == INVALID_FILE_ATTRIBUTES) {
    	fprintf(stderr,"Usage: aviplay <avifilename>\n");
	exit(1);
    }
    hres = AVIFileOpen(&avif,cmdline,OF_READ,NULL);
    if (hres) {
    	fprintf(stderr,"AVIFileOpen: 0x%08lx\n",hres);
	exit(1);
    }
    hres = AVIFileInfo(avif,&afi,sizeof(afi));
    if (hres) {
    	fprintf(stderr,"AVIFileInfo: 0x%08lx\n",hres);
	exit(1);
    }
    for (n=0;n<afi.dwStreams;n++) {
    	    char buf[5];
	    PAVISTREAM	ast;

	    hres = AVIFileGetStream(avif,&ast,0,n);
	    if (hres) {
		fprintf(stderr,"AVIFileGetStream %d: 0x%08lx\n",n,hres);
		exit(1);
	    }
	    hres = AVIStreamInfo(ast,&asi,sizeof(asi));
	    if (hres) {
		fprintf(stderr,"AVIStreamInfo %d: 0x%08lx\n",n,hres);
		exit(1);
	    }
	    fprintf(stderr,"[Stream %d: ",n);
	    buf[4]='\0';memcpy(buf,&(asi.fccType),4);
	    fprintf(stderr,"%s.",buf);
	    buf[4]='\0';memcpy(buf,&(asi.fccHandler),4);
	    fprintf(stderr,"%s, %s]\n",buf,asi.szName);
	    switch (asi.fccType) {
	    case streamtypeVIDEO:
	    	vids = ast;
		break;
	    case streamtypeAUDIO:
	    	auds = ast;
		break;
	    default:  {
	    	char type[5];
		type[4]='\0';memcpy(type,&(asi.fccType),4);

	    	fprintf(stderr,"Unhandled streamtype %s\n",type);
	    	AVIStreamRelease(ast);
		break;
	    }
	    }
    }
/********************* begin video setup ***********************************/
    if (!vids) {
    	fprintf(stderr,"No video stream found. Good Bye.\n");
	exit(0);
    }
    cnt = sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD);
    bmi = HeapAlloc(GetProcessHeap(),0,cnt);
    hres = AVIStreamReadFormat(vids,0,bmi,&cnt);
    if (hres) {
    	fprintf(stderr,"AVIStreamReadFormat vids: 0x%08lx\n",hres);
	exit(1);
    }
    vidgetframe = NULL;
    bmi->biCompression = 0; /* we want it in raw form, uncompressed */
    /* recalculate the image size */
    bmi->biSizeImage = ((bmi->biWidth*bmi->biBitCount+31)&~0x1f)*bmi->biPlanes*bmi->biHeight/8;
    bytesline = ((bmi->biWidth*bmi->biBitCount+31)&~0x1f)*bmi->biPlanes/8;
    vidgetframe = AVIStreamGetFrameOpen(vids,bmi);
    if (!vidgetframe) {
    	fprintf(stderr,"AVIStreamGetFrameOpen: failed\n");
	exit(1);
    }
/********************* end video setup ***********************************/

/********************* begin display setup *******************************/
    hres = DirectDrawCreate(NULL,&ddraw,NULL);
    if (hres) {
    	fprintf(stderr,"DirectDrawCreate: 0x%08lx\n",hres);
	exit(1);
    }
    hres = IDirectDraw_SetDisplayMode(ddraw,bmi->biWidth,bmi->biHeight,bmi->biBitCount);
    if (hres) {
    	fprintf(stderr,"ddraw.SetDisplayMode: 0x%08lx (change resolution!)\n",hres);
	exit(1);
    }
    dsdesc.dwSize=sizeof(dsdesc);
    dsdesc.dwFlags = DDSD_CAPS;
    dsdesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hres = IDirectDraw_CreateSurface(ddraw,&dsdesc,&dsurf,NULL);
    if (hres) {
    	fprintf(stderr,"ddraw.CreateSurface: 0x%08lx\n",hres);
	exit(1);
    }
    if (bmi->biBitCount==8) {
    	RGBQUAD		*rgb = (RGBQUAD*)(bmi+1);
	int		i;

	hres = IDirectDraw_CreatePalette(ddraw,DDPCAPS_8BIT,NULL,&dpal,NULL);
	if (hres) {
	    fprintf(stderr,"ddraw.CreateSurface: 0x%08lx\n",hres);
	    exit(1);
	}
	IDirectDrawSurface_SetPalette(dsurf,dpal);
	for (i=0;i<bmi->biClrUsed;i++) {
	    palent[i].peRed = rgb[i].rgbRed;
	    palent[i].peBlue = rgb[i].rgbBlue;
	    palent[i].peGreen = rgb[i].rgbGreen;
	}
	IDirectDrawPalette_SetEntries(dpal,0,0,bmi->biClrUsed,palent);
    } else
	dpal = NULL;
/********************* end display setup *******************************/

    tstart = time(NULL);
    pos = 0;
    while (1) {
    	LPVOID		decodedframe;
	LPBITMAPINFOHEADER lpbmi;
	LPVOID		decodedbits;

/* video stuff */
	if (!(decodedframe=AVIStreamGetFrame(vidgetframe,pos++)))
	    break;
	lpbmi = (LPBITMAPINFOHEADER)decodedframe;
	decodedbits = (LPVOID)(((DWORD)decodedframe)+lpbmi->biSize);
	if (lpbmi->biBitCount == 8) {
	/* cant detect palette change that way I think */
	    RGBQUAD	*rgb = (RGBQUAD*)(lpbmi+1);
	    int		i,palchanged;

	    /* skip used colorentries. */
	    decodedbits=(char*)decodedbits+bmi->biClrUsed*sizeof(RGBQUAD);
	    palchanged = 0;
	    for (i=0;i<bmi->biClrUsed;i++) {
	    	if (	(palent[i].peRed != rgb[i].rgbRed) ||
	    		(palent[i].peBlue != rgb[i].rgbBlue) ||
	    		(palent[i].peGreen != rgb[i].rgbGreen)
		) {
			palchanged = 1;
			break;
		}
	    }
	    if (palchanged) {
		for (i=0;i<bmi->biClrUsed;i++) {
		    palent[i].peRed = rgb[i].rgbRed;
		    palent[i].peBlue = rgb[i].rgbBlue;
		    palent[i].peGreen = rgb[i].rgbGreen;
		}
		IDirectDrawPalette_SetEntries(dpal,0,0,bmi->biClrUsed,palent);
	    }
	}
	dsdesc.dwSize = sizeof(dsdesc);
	hres = IDirectDrawSurface_Lock(dsurf,NULL,&dsdesc,DDLOCK_WRITEONLY,0);
	if (hres) {
	    fprintf(stderr,"dsurf.Lock: 0x%08lx\n",hres);
	    exit(1);
	}
	/* Argh. AVIs are upside down. */
	for (i=0;i<dsdesc.dwHeight;i++) {
	    memcpy( (char *)dsdesc.lpSurface+(i*dsdesc.u1.lPitch),
		    (char *)decodedbits+bytesline*(dsdesc.dwHeight-i-1),
		    bytesline
	    );
	}
	IDirectDrawSurface_Unlock(dsurf,dsdesc.lpSurface);
    }
    tend = time(NULL);
    AVIStreamGetFrameClose(vidgetframe);

    IDirectDrawSurface_Release(dsurf);
    IDirectDraw_RestoreDisplayMode(ddraw);
    IDirectDraw_Release(ddraw);
    if (vids) AVIStreamRelease(vids);
    if (auds) AVIStreamRelease(auds);
    fprintf(stderr,"%d frames at %g frames/s\n",pos,pos*1.0/(tend-tstart));
    AVIFileRelease(avif);
    AVIFileExit();
    return 0;
}
