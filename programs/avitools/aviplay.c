/*
 * Very simple AVIPLAYER
 * 
 * Copyright 1999 Marcus Meissner
 * 
 * Status:
 * 	- plays .avi streams, video only
 *	- requires MicroSoft avifil32.dll and builtin msvfw32.dll.
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
#include <strings.h>
#include "wintypes.h"
#include "windows.h"
#include "wingdi.h"
#include "mmsystem.h"
#include "ddraw.h"
#include "vfw.h"


int PASCAL WinMain (HANDLE hInstance, HANDLE prev, LPSTR cmdline, int show)
{
    int			i,n,pos;
    time_t		tstart,tend;
    LONG		cnt;
    BITMAPINFOHEADER	*bmi;
    HRESULT		hres;
    HMODULE		avifil32 = LoadLibrary("avifil32.dll");
    PAVIFILE		avif;
    PAVISTREAM		vids=NULL,auds=NULL;
    AVIFILEINFO		afi;
    AVISTREAMINFO	asi;
    PGETFRAME		vidgetframe=NULL;
    LPDIRECTDRAW	ddraw;
    DDSURFACEDESC	dsdesc;
    LPDIRECTDRAWSURFACE dsurf;

void	(WINAPI *fnAVIFileInit)(void);
void	(WINAPI *fnAVIFileExit)(void);
ULONG	(WINAPI *fnAVIFileRelease)(PAVIFILE);
ULONG	(WINAPI *fnAVIStreamRelease)(PAVISTREAM);
HRESULT (WINAPI *fnAVIFileOpen)(PAVIFILE * ppfile,LPCTSTR szFile,UINT uMode,LPCLSID lpHandler);
HRESULT (WINAPI *fnAVIFileInfo)(PAVIFILE ppfile,AVIFILEINFO *afi,LONG size);
HRESULT (WINAPI *fnAVIFileGetStream)(PAVIFILE ppfile,PAVISTREAM *afi,DWORD fccType,LONG lParam);
HRESULT (WINAPI *fnAVIStreamInfo)(PAVISTREAM iface,AVISTREAMINFO *afi,LONG size);
HRESULT (WINAPI *fnAVIStreamReadFormat)(PAVISTREAM iface,LONG pos,LPVOID format,LPLONG size);
PGETFRAME (WINAPI *fnAVIStreamGetFrameOpen)(PAVISTREAM iface,LPBITMAPINFOHEADER wanted);
LPVOID (WINAPI *fnAVIStreamGetFrame)(PGETFRAME pg,LONG pos);
HRESULT (WINAPI *fnAVIStreamGetFrameClose)(PGETFRAME pg);

#define XX(x) fn##x = (void*)GetProcAddress(avifil32,#x);assert(fn##x);
#ifdef UNICODE
# define XXT(x) fn##x = (void*)GetProcAddress(avifil32,#x##"W");assert(fn##x);
#else
# define XXT(x) fn##x = (void*)GetProcAddress(avifil32,#x##"A");assert(fn##x);
#endif
	/* non character dependend routines: */
	XX (AVIFileInit);
	XX (AVIFileExit);
	XX (AVIFileRelease);
	XX (AVIFileGetStream);
	XX (AVIStreamRelease);
	XX (AVIStreamReadFormat);
	XX (AVIStreamGetFrameOpen);
	XX (AVIStreamGetFrame);
	XX (AVIStreamGetFrameClose);
	/* A/W routines: */
	XXT(AVIFileOpen);
	XXT(AVIFileInfo);
	XXT(AVIStreamInfo);
#undef XX
#undef XXT
    
    fnAVIFileInit();
    if (-1==GetFileAttributes(cmdline)) {
    	fprintf(stderr,"Usage: aviplay <avifilename>\n");
	exit(1);
    }
    hres = fnAVIFileOpen(&avif,cmdline,OF_READ,NULL);
    if (hres) {
    	fprintf(stderr,"AVIFileOpen: 0x%08lx\n",hres);
	exit(1);
    }
    hres = fnAVIFileInfo(avif,&afi,sizeof(afi));
    if (hres) {
    	fprintf(stderr,"AVIFileInfo: 0x%08lx\n",hres);
	exit(1);
    }
    for (n=0;n<afi.dwStreams;n++) {
    	    char buf[5];
	    PAVISTREAM	ast;

	    hres = fnAVIFileGetStream(avif,&ast,0,n);
	    if (hres) {
		fprintf(stderr,"AVIFileGetStream %d: 0x%08lx\n",n,hres);
		exit(1);
	    }
	    hres = fnAVIStreamInfo(ast,&asi,sizeof(asi));
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
	    	fnAVIStreamRelease(ast);
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
    hres = fnAVIStreamReadFormat(vids,0,bmi,&cnt);
    if (hres) {
    	fprintf(stderr,"AVIStreamReadFormat vids: 0x%08lx\n",hres);
	exit(1);
    }
    bmi->biCompression = 0; /* we want it in raw form, uncompressed */
    vidgetframe = fnAVIStreamGetFrameOpen(vids,bmi);
    if (!vidgetframe) {
    	fprintf(stderr,"AVIStreamGetFrameOpen: failed.\n");
	exit(1);
    }
/********************* end video setup ***********************************/

/********************* begin display setup *******************************/
    hres = DirectDrawCreate(NULL,&ddraw,NULL);
    if (hres) {
    	fprintf(stderr,"DirectDrawCreate: 0x%08lx\n",hres);
	exit(1);
    }
    hres = ddraw->lpvtbl->fnSetDisplayMode(ddraw,bmi->biWidth,bmi->biHeight,bmi->biBitCount);
    if (hres) {
    	fprintf(stderr,"ddraw.SetDisplayMode: 0x%08lx (change resolution!)\n",hres);
	exit(1);
    }
    dsdesc.dwSize=sizeof(dsdesc);
    dsdesc.dwFlags = DDSD_CAPS;
    dsdesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hres = ddraw->lpvtbl->fnCreateSurface(ddraw,&dsdesc,&dsurf,NULL);
    if (hres) {
    	fprintf(stderr,"ddraw.CreateSurface: 0x%08lx\n",hres);
	exit(1);
    }
/********************* end display setup *******************************/

    tstart = time(NULL);
    pos = 0;
    while (1) {
    	LPVOID		decodedframe;
	LPBITMAPINFOHEADER lpbmi;
	LPVOID		decodedbits;
	int		bytesline;

/* video stuff */
	if (!(decodedframe=fnAVIStreamGetFrame(vidgetframe,pos++)))
	    break;
	lpbmi = (LPBITMAPINFOHEADER)decodedframe;
	decodedbits = (LPVOID)(((DWORD)decodedframe)+lpbmi->biSize);
	dsdesc.dwSize = sizeof(dsdesc);
	hres = dsurf->lpvtbl->fnLock(dsurf,NULL,&dsdesc,DDLOCK_WRITEONLY,0);
	if (hres) {
	    fprintf(stderr,"dsurf.Lock: 0x%08lx\n",hres);
	    exit(1);
	}
	bytesline = dsdesc.dwWidth * dsdesc.ddpfPixelFormat.x.dwRGBBitCount/8;
	/* Argh. AVIs are upside down. */
	for (i=0;i<dsdesc.dwHeight;i++) {
	    memcpy( dsdesc.y.lpSurface+(i*dsdesc.lPitch),
		    decodedbits+bytesline*(dsdesc.dwHeight-i),
		    bytesline
	    );
	}
	dsurf->lpvtbl->fnUnlock(dsurf,dsdesc.y.lpSurface);
    }
    tend = time(NULL);
    fnAVIStreamGetFrameClose(vidgetframe);

    ((IUnknown*)dsurf)->lpvtbl->fnRelease((IUnknown*)dsurf);
    ddraw->lpvtbl->fnRestoreDisplayMode(ddraw);
    ((IUnknown*)ddraw)->lpvtbl->fnRelease((IUnknown*)ddraw);
    if (vids) fnAVIStreamRelease(vids);
    if (auds) fnAVIStreamRelease(auds);
    fprintf(stderr,"%d frames at %g frames/s\n",pos,pos*1.0/(tend-tstart));
    fnAVIFileRelease(avif);
    fnAVIFileExit();
    return 0;    
}
