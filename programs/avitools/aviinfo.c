#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include "windef.h"
#include "windows.h"
#include "mmsystem.h"
#include "vfw.h"


int PASCAL WinMain (HANDLE hInstance, HANDLE prev, LPSTR cmdline, int show)
{
    int			n;
    HRESULT		hres;
    HMODULE		avifil32 = LoadLibrary("avifil32.dll");
    PAVIFILE		avif;
    PAVISTREAM		vids,auds;
    AVIFILEINFO		afi;
    AVISTREAMINFO	asi;

void	(WINAPI *fnAVIFileInit)(void);
void	(WINAPI *fnAVIFileExit)(void);
ULONG	(WINAPI *fnAVIFileRelease)(PAVIFILE);
ULONG	(WINAPI *fnAVIStreamRelease)(PAVISTREAM);
HRESULT (WINAPI *fnAVIFileOpen)(PAVIFILE * ppfile,LPCTSTR szFile,UINT uMode,LPCLSID lpHandler);
HRESULT (WINAPI *fnAVIFileInfo)(PAVIFILE ppfile,AVIFILEINFO *afi,LONG size);
HRESULT (WINAPI *fnAVIFileGetStream)(PAVIFILE ppfile,PAVISTREAM *afi,DWORD fccType,LONG lParam);
HRESULT (WINAPI *fnAVIStreamInfo)(PAVISTREAM iface,AVISTREAMINFO *afi,LONG size);

#define XX(x) fn##x = (void*)GetProcAddress(avifil32,#x);assert(fn##x);
#ifdef UNICODE
# define XXT(x) fn##x = (void*)GetProcAddress(avifil32,#x##"W");assert(fn##x);
#else
# define XXT(x) fn##x = (void*)GetProcAddress(avifil32,#x##"A");assert(fn##x);
#endif
	/* Non character dependend routines: */
	XX (AVIFileInit);
	XX (AVIFileExit);
	XX (AVIFileRelease);
	XX (AVIStreamRelease);
	XX (AVIFileGetStream);
	/* A/W routines: */
	XXT(AVIFileOpen);
	XXT(AVIFileInfo);
	XXT(AVIStreamInfo);
#undef XX
#undef XXT
    
    fnAVIFileInit();
    if (-1==GetFileAttributes(cmdline)) {
    	fprintf(stderr,"Usage: aviinfo <avifilename>\n");
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
    fprintf(stderr,"AVI File Info:\n");
    fprintf(stderr,"\tdwMaxBytesPerSec: %ld\n",afi.dwMaxBytesPerSec);
#define FF(x) if (afi.dwFlags & AVIFILEINFO_##x) fprintf(stderr,#x##",");
    fprintf(stderr,"\tdwFlags: 0x%lx (",afi.dwFlags);
    FF(HASINDEX);FF(MUSTUSEINDEX);FF(ISINTERLEAVED);FF(WASCAPTUREFILE);
    FF(COPYRIGHTED);
    fprintf(stderr,")\n");
#undef FF
#define FF(x) if (afi.dwCaps & AVIFILECAPS_##x) fprintf(stderr,#x##",");
    fprintf(stderr,"\tdwCaps: 0x%lx (",afi.dwCaps);
    FF(CANREAD);FF(CANWRITE);FF(ALLKEYFRAMES);FF(NOCOMPRESSION);
    fprintf(stderr,")\n");
#undef FF
    fprintf(stderr,"\tdwStreams: %ld\n",afi.dwStreams);
    fprintf(stderr,"\tdwSuggestedBufferSize: %ld\n",afi.dwSuggestedBufferSize);
    fprintf(stderr,"\tdwWidth: %ld\n",afi.dwWidth);
    fprintf(stderr,"\tdwHeight: %ld\n",afi.dwHeight);
    fprintf(stderr,"\tdwScale: %ld\n",afi.dwScale);
    fprintf(stderr,"\tdwRate: %ld\n",afi.dwRate);
    fprintf(stderr,"\tdwLength: %ld\n",afi.dwLength);
    fprintf(stderr,"\tdwEditCount: %ld\n",afi.dwEditCount);
    fprintf(stderr,"\tszFileType: %s\n",afi.szFileType);

    for (n = 0;n<afi.dwStreams;n++) {
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
	    fprintf(stderr,"Stream %d:\n",n);
	    buf[4]='\0';memcpy(buf,&(asi.fccType),4);
	    fprintf(stderr,"\tfccType: %s\n",buf);
	    memcpy(buf,&(asi.fccHandler),4);
	    fprintf(stderr,"\tfccHandler: %s\n",buf);
	    fprintf(stderr,"\tdwFlags: 0x%08lx\n",asi.dwFlags);
	    fprintf(stderr,"\tdwCaps: 0x%08lx\n",asi.dwCaps);
	    fprintf(stderr,"\twPriority: %d\n",asi.wPriority);
	    fprintf(stderr,"\twLanguage: %d\n",asi.wLanguage);
	    fprintf(stderr,"\tdwScale: %ld\n",asi.dwScale);
	    fprintf(stderr,"\tdwRate: %ld\n",asi.dwRate);
	    fprintf(stderr,"\tdwStart: %ld\n",asi.dwStart);
	    fprintf(stderr,"\tdwLength: %ld\n",asi.dwLength);
	    fprintf(stderr,"\tdwInitialFrames: %ld\n",asi.dwInitialFrames);
	    fprintf(stderr,"\tdwSuggestedBufferSize: %ld\n",asi.dwSuggestedBufferSize);
	    fprintf(stderr,"\tdwQuality: %ld\n",asi.dwQuality);
	    fprintf(stderr,"\tdwSampleSize: %ld\n",asi.dwSampleSize);
	    fprintf(stderr,"\tdwEditCount: %ld\n",asi.dwEditCount);
	    fprintf(stderr,"\tdwFormatChangeCount: %ld\n",asi.dwFormatChangeCount);
	    fprintf(stderr,"\tszName: %s\n",asi.szName);
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
    fnAVIFileRelease(avif);
    fnAVIFileExit();
    return 0;    
}
