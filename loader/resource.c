/*
 * Resources
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "arch.h"
#include "windows.h"
#include "gdi.h"
#include "bitmap.h"
#include "global.h"
#include "neexe.h"
#include "icon.h"
#include "accel.h"
#include "dlls.h"
#include "module.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"

#define PrintId(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", (char *)PTR_SEG_TO_LIN(name)); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 


/**********************************************************************
 *	    LoadIconHandler    (USER.456)
 */
HICON LoadIconHandler( HANDLE hResource, BOOL bNew )
{
  return 0;
}


/**********************************************************************
 *	    FindResource    (KERNEL.60)
 */
HRSRC FindResource( HMODULE hModule, SEGPTR name, SEGPTR type )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "FindResource: module=%04x type=", hModule );
    PrintId( type );
    dprintf_resource( stddeb, " name=" );
    PrintId( name );
    dprintf_resource( stddeb, "\n" );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_FindResource( hModule, type, name );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}


/**********************************************************************
 *	    LoadResource    (KERNEL.61)
 */
HGLOBAL LoadResource( HMODULE hModule, HRSRC hRsrc )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "LoadResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_LoadResource( hModule, hRsrc );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}


/**********************************************************************
 *	    LockResource    (KERNEL.62)
 */
/* 16-bit version */
SEGPTR WIN16_LockResource( HGLOBAL handle )
{
    HMODULE hModule;
    WORD *pModule;

    dprintf_resource(stddeb, "LockResource: handle=%04x\n", handle );
    if (!handle) return (SEGPTR)0;
    hModule = GetExePtr( handle );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_LockResource( hModule, handle );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}

/* 32-bit version */
LPSTR LockResource( HGLOBAL handle )
{
    HMODULE hModule;
    WORD *pModule;

    dprintf_resource(stddeb, "LockResource: handle=%04x\n", handle );
    if (!handle) return NULL;
    hModule = GetExePtr( handle );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return (LPSTR)PTR_SEG_TO_LIN( NE_LockResource( hModule, handle ) );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}


/**********************************************************************
 *	    FreeResource    (KERNEL.63)
 */
BOOL FreeResource( HGLOBAL handle )
{
    HMODULE hModule;
    WORD *pModule;

    dprintf_resource(stddeb, "FreeResource: handle=%04x\n", handle );
    if (!handle) return FALSE;
    hModule = GetExePtr( handle );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_FreeResource( hModule, handle );
      case PE_SIGNATURE:
        return FALSE;
      default:
        return FALSE;
    }
}


/**********************************************************************
 *	    AccessResource    (KERNEL.64)
 */
int AccessResource( HMODULE hModule, HRSRC hRsrc )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AccessResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_AccessResource( hModule, hRsrc );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}


/**********************************************************************
 *	    SizeofResource    (KERNEL.65)
 */
DWORD SizeofResource( HMODULE hModule, HRSRC hRsrc )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "SizeofResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_SizeofResource( hModule, hRsrc );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}


/**********************************************************************
 *	    AllocResource    (KERNEL.66)
 */
HGLOBAL AllocResource( HMODULE hModule, HRSRC hRsrc, DWORD size )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AllocResource: module=%04x res=%04x size=%ld\n",
                     hModule, hRsrc, size );
    if (!hRsrc) return 0;
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_AllocResource( hModule, hRsrc, size );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
}

/**********************************************************************
 *      DirectResAlloc    (KERNEL.168)
 * Check Schulman, p. 232 for details
 */
HANDLE DirectResAlloc(HANDLE hInstance, WORD wType, WORD wSize)
{
	HANDLE hModule;
	dprintf_resource(stddeb,"DirectResAlloc(%x,%x,%x)\n",hInstance,wType,wSize);
	hModule = GetExePtr(hInstance);
	if(!hModule)return 0;
	if(wType != 0x10)	/* 0x10 is the only observed value, passed from
                           CreateCursorIndirect. */
		fprintf(stderr, "DirectResAlloc: wType = %x\n", wType);
	/* This hopefully does per-module allocation rather than per-instance */
	return GLOBAL_Alloc(GMEM_FIXED, wSize, hModule, FALSE, FALSE, FALSE);
}


/**********************************************************************
 *			ConvertCoreBitmap
 */
HBITMAP ConvertCoreBitmap( HDC hdc, BITMAPCOREHEADER * image )
{
    BITMAPINFO * bmpInfo;
    HBITMAP hbitmap;
    char * bits;
    int i, size, n_colors;
   
    n_colors = 1 << image->bcBitCount;
    if (image->bcBitCount < 24)
    {
	size = sizeof(BITMAPINFOHEADER) + n_colors * sizeof(RGBQUAD);	
	bits = (char *) (image + 1) + (n_colors * sizeof(RGBTRIPLE));
    }
    else
    {
	size = sizeof(BITMAPINFOHEADER);
	bits = (char *) (image + 1);
    }
    bmpInfo = (BITMAPINFO *) malloc( size );

    bmpInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmpInfo->bmiHeader.biWidth         = image->bcWidth;
    bmpInfo->bmiHeader.biHeight        = image->bcHeight;
    bmpInfo->bmiHeader.biPlanes        = image->bcPlanes;
    bmpInfo->bmiHeader.biBitCount      = image->bcBitCount;
    bmpInfo->bmiHeader.biCompression   = 0;
    bmpInfo->bmiHeader.biSizeImage     = 0;
    bmpInfo->bmiHeader.biXPelsPerMeter = 0;
    bmpInfo->bmiHeader.biYPelsPerMeter = 0;
    bmpInfo->bmiHeader.biClrUsed       = 0;
    bmpInfo->bmiHeader.biClrImportant  = 0;

    if (image->bcBitCount < 24)
    {
	RGBTRIPLE * oldMap = (RGBTRIPLE *)(image + 1);
	RGBQUAD * newMap = bmpInfo->bmiColors;
	for (i = 0; i < n_colors; i++, oldMap++, newMap++)
	{
	    newMap->rgbRed      = oldMap->rgbtRed;
	    newMap->rgbGreen    = oldMap->rgbtGreen;
	    newMap->rgbBlue     = oldMap->rgbtBlue;
	    newMap->rgbReserved = 0;
	}
    }

    hbitmap = CreateDIBitmap( hdc, &bmpInfo->bmiHeader, CBM_INIT,
			      bits, bmpInfo, DIB_RGB_COLORS );
    free( bmpInfo );
    return hbitmap;
}

/**********************************************************************
 *			ConvertInfoBitmap
 */
HBITMAP	ConvertInfoBitmap( HDC hdc, BITMAPINFO * image )
{
    char * bits = ((char *)image) + DIB_BitmapInfoSize(image, DIB_RGB_COLORS);
    return CreateDIBitmap( hdc, &image->bmiHeader, CBM_INIT,
			   bits, image, DIB_RGB_COLORS );
} 


/**********************************************************************
 *			LoadIcon [USER.174]
 */
HICON LoadIcon( HANDLE instance, SEGPTR icon_name )
{
    HICON 	hIcon;
    HRSRC       hRsrc;
    HANDLE 	rsc_mem;
    WORD 	*lp;
    ICONDESCRIP *lpicodesc;
    ICONALLOC	*lpico;
    int		width, height;
    BITMAPINFO 	*bmi;
    BITMAPINFOHEADER 	*bih;
    RGBQUAD	*rgbq;
    HDC 	hdc;
    BITMAPINFO *pInfo;
    char *bits;
    int size;

    if (HIWORD(icon_name))
        dprintf_resource( stddeb, "LoadIcon: %04x '%s'\n",
                          instance, (char *)PTR_SEG_TO_LIN( icon_name ) );
    else
        dprintf_resource( stddeb, "LoadIcon: %04x %04x\n",
                          instance, LOWORD(icon_name) );
    
    if (!instance)
    {
        if (HIWORD((int)icon_name)) return 0; /* FIXME: should handle '#xxx' */
        return OBM_LoadIcon( LOWORD((int)icon_name) );
    }

    if (!(hRsrc = FindResource( instance, icon_name, RT_GROUP_ICON))) return 0;
    rsc_mem = LoadResource( instance, hRsrc );
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %08x not Found !\n", (int) icon_name);
	return 0;
    }
    lp = (WORD *)LockResource(rsc_mem);
    lpicodesc = (ICONDESCRIP *)(lp + 3);
    hIcon = GlobalAlloc(GMEM_MOVEABLE, sizeof(ICONALLOC) + 1024);
    if (hIcon == (HICON)NULL) {
        FreeResource( rsc_mem );
	return 0;
    }
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    lpico->descriptor = *lpicodesc;
    width = lpicodesc->Width;
    height = lpicodesc->Height;
    FreeResource( rsc_mem );
    if (!(hRsrc = FindResource( instance,
                              MAKEINTRESOURCE(lpico->descriptor.icoDIBOffset), 
                              RT_ICON ))) return 0;
    if (!(rsc_mem = LoadResource( instance, hRsrc ))) return 0;

    bmi = (BITMAPINFO *)LockResource(rsc_mem);
    size = DIB_BitmapInfoSize( bmi, DIB_RGB_COLORS );
    pInfo = (BITMAPINFO *)malloc( size );
    memcpy( pInfo, bmi, size );
    bih = &pInfo->bmiHeader;
    bih->biHeight /= 2;

    if (!(hdc = GetDC( 0 ))) return 0;
    if (bih->biSize != sizeof(BITMAPINFOHEADER)) return 0;
    lpico->hBitmap = CreateDIBitmap( hdc, &pInfo->bmiHeader, CBM_INIT,
                                    (char*)bmi + size, pInfo, DIB_RGB_COLORS );

    if (bih->biCompression != BI_RGB)
    {
	fprintf(stderr,"Unknown size for compressed Icon bitmap.\n");
	FreeResource( rsc_mem );
	ReleaseDC( 0, hdc); 
	return 0;
    }
    bits = (char *)bmi + size +
      DIB_GetImageWidthBytes(bih->biWidth,bih->biBitCount) * bih->biHeight;
    bih->biBitCount = 1;
    bih->biClrUsed = bih->biClrImportant = 2;
    rgbq = &pInfo->bmiColors[0];
    rgbq[0].rgbBlue = rgbq[0].rgbGreen = rgbq[0].rgbRed = 0x00;
    rgbq[1].rgbBlue = rgbq[1].rgbGreen = rgbq[1].rgbRed = 0xff;
    rgbq[0].rgbReserved = rgbq[1].rgbReserved = 0;
    lpico->hBitMask = CreateDIBitmap(hdc, &pInfo->bmiHeader, CBM_INIT,
                                     bits, pInfo, DIB_RGB_COLORS );
    FreeResource( rsc_mem );
    ReleaseDC( 0, hdc);
    free( pInfo );
    GlobalUnlock(hIcon);
    dprintf_resource(stddeb,"LoadIcon Alloc hIcon=%X\n", hIcon);
    return hIcon;
}


/**********************************************************************
 *			CreateIcon [USER.407]
 */
HICON CreateIcon(HANDLE hInstance, int nWidth, int nHeight, 
                 BYTE nPlanes, BYTE nBitsPixel, LPSTR lpANDbits, 
                 LPSTR lpXORbits)
{
    HICON 	hIcon;
    ICONALLOC	*lpico;

    dprintf_resource(stddeb, "CreateIcon: hInstance = %04x, nWidth = %08x, nHeight = %08x \n",
	    hInstance, nWidth, nHeight);
    dprintf_resource(stddeb, "  nPlanes = %04x, nBitsPixel = %04x,",nPlanes, nBitsPixel);
    dprintf_resource(stddeb, " lpANDbits= %04x, lpXORbits = %04x, \n", (int)lpANDbits,
    		(int)lpXORbits);

    if (hInstance == (HANDLE)NULL) { 
        printf("CreateIcon / hInstance %04x not Found!\n",hInstance);
        return 0;
	}
    hIcon = GlobalAlloc(GMEM_MOVEABLE, sizeof(ICONALLOC) + 1024);
    if (hIcon == (HICON)NULL) {
	printf("Can't allocate memory for Icon in CreateIcon\n");
	return 0;
	}
    lpico= (ICONALLOC *)GlobalLock(hIcon);

    lpico->descriptor.Width=nWidth;
    lpico->descriptor.Height=nHeight;
    lpico->descriptor.ColorCount=16; /* Dummy Value */ 
    lpico->descriptor.Reserved1=0;
    lpico->descriptor.Reserved2=nPlanes;
    lpico->descriptor.Reserved3=nWidth*nHeight;

    /* either nPlanes and/or nBitCount is set to one */
    lpico->descriptor.icoDIBSize=nWidth*nHeight*nPlanes*nBitsPixel; 
    lpico->descriptor.icoDIBOffset=0; 

    if( !(lpico->hBitmap=CreateBitmap(nWidth, nHeight, nPlanes, nBitsPixel, 
                                 lpXORbits)) ) {
      printf("CreateIcon: couldn't create the XOR bitmap\n");
      return(0);
    }
    
    /* the AND BitMask is always monochrome */
    if( !(lpico->hBitMask=CreateBitmap(nWidth, nHeight, 1, 1, lpANDbits)) ) {
      printf("CreateIcon: couldn't create the AND bitmap\n");
      return(0);
    }

    GlobalUnlock(hIcon);
    dprintf_resource(stddeb, "CreateIcon Alloc hIcon=%X\n", hIcon);
    return hIcon;
}

/**********************************************************************
 *			DestroyIcon [USER.457]
 */
BOOL DestroyIcon(HICON hIcon)
{
    ICONALLOC	*lpico;
    
    if (hIcon == (HICON)NULL)
    	return FALSE;
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    if (lpico->hBitmap != (HBITMAP)NULL) 
    	DeleteObject(lpico->hBitmap);
    GlobalFree(hIcon);
    return TRUE;
}

/**********************************************************************
 *          DumpIcon [USER.459]
 */
DWORD DumpIcon(void* cursorIconInfo, WORD FAR *lpLen, LPSTR FAR *lpXorBits,
	LPSTR FAR *lpAndMask)
{
	dprintf_resource(stdnimp,"DumpIcon: Empty Stub!!!\n");
	return 0;
}


/**********************************************************************
 *			LoadAccelerators	[USER.177]
 */
HANDLE LoadAccelerators(HANDLE instance, SEGPTR lpTableName)
{
    HANDLE 	hAccel;
    HANDLE 	rsc_mem;
    HRSRC hRsrc;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, n;

    if (HIWORD(lpTableName))
        dprintf_accel( stddeb, "LoadAccelerators: %04x '%s'\n",
                      instance, (char *)PTR_SEG_TO_LIN( lpTableName ) );
    else
        dprintf_accel( stddeb, "LoadAccelerators: %04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource( instance, lpTableName, RT_ACCELERATOR )))
      return 0;
    if (!(rsc_mem = LoadResource( instance, hRsrc ))) return 0;

    lp = (BYTE *)LockResource(rsc_mem);
    n = SizeofResource( instance, hRsrc ) / sizeof(ACCELENTRY);
    hAccel = GlobalAlloc(GMEM_MOVEABLE, 
    	sizeof(ACCELHEADER) + (n + 1)*sizeof(ACCELENTRY));
    lpAccelTbl = (LPACCELHEADER)GlobalLock(hAccel);
    lpAccelTbl->wCount = 0;
    for (i = 0; i < n; i++) {
	lpAccelTbl->tbl[i].type = *(lp++);
	lpAccelTbl->tbl[i].wEvent = *((WORD *)lp);
	lp += 2;
	lpAccelTbl->tbl[i].wIDval = *((WORD *)lp);
	lp += 2;
    	if (lpAccelTbl->tbl[i].wEvent == 0) break;
	dprintf_accel(stddeb,
		"Accelerator #%u / event=%04X id=%04X type=%02X \n", 
		i, lpAccelTbl->tbl[i].wEvent, lpAccelTbl->tbl[i].wIDval, 
		lpAccelTbl->tbl[i].type);
	lpAccelTbl->wCount++;
 	}
    GlobalUnlock(hAccel);
    FreeResource( rsc_mem );
    return hAccel;
}

/**********************************************************************
 *			TranslateAccelerator 	[USER.178]
 */
int TranslateAccelerator(HWND hWnd, HANDLE hAccel, LPMSG msg)
{
    ACCELHEADER	*lpAccelTbl;
    int 	i;
    
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
    	msg->message != WM_CHAR) return 0;

    dprintf_accel(stddeb, "TranslateAccelerators hAccel=%04X !\n", hAccel);

    lpAccelTbl = (LPACCELHEADER)GlobalLock(hAccel);
    for (i = 0; i < lpAccelTbl->wCount; i++) {
	if (lpAccelTbl->tbl[i].type & VIRTKEY_ACCEL) {
	    if (msg->wParam == lpAccelTbl->tbl[i].wEvent &&
		msg->message == WM_KEYDOWN) {
		if ((lpAccelTbl->tbl[i].type & SHIFT_ACCEL) &&
		    !(GetKeyState(VK_SHIFT) & 0xf)) {
		    GlobalUnlock(hAccel);
		    return 0;
		}
		if ((lpAccelTbl->tbl[i].type & CONTROL_ACCEL) &&
		    !(GetKeyState(VK_CONTROL) & 0xf)) {
		    GlobalUnlock(hAccel);
		    return 0;
		}
		if ((lpAccelTbl->tbl[i].type & ALT_ACCEL) &&
		    !(GetKeyState(VK_MENU) & 0xf)) {
		    GlobalUnlock(hAccel);
		    return 0;
		}
		SendMessage(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval, 0x00010000L);
		GlobalUnlock(hAccel);
		return 1;
		}
	    if (msg->message == WM_KEYUP) return 1;
	    }
	else {
	    if (msg->wParam == lpAccelTbl->tbl[i].wEvent &&
		msg->message == WM_CHAR) {
		SendMessage(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval, 0x00010000L);
		GlobalUnlock(hAccel);
		return 1;
		}
	    }
	}
    GlobalUnlock(hAccel);
    return 0;
}

/**********************************************************************
 *					LoadString
 */
int
LoadString(HANDLE instance, WORD resource_id, LPSTR buffer, int buflen)
{
    HANDLE hmem, hrsrc;
    unsigned char *p;
    int string_num;
    int i;

    dprintf_resource(stddeb, "LoadString: instance = %04x, id = %d, buffer = %08x, "
	   "length = %d\n", instance, resource_id, (int) buffer, buflen);

    hrsrc = FindResource( instance, (SEGPTR)((resource_id>>4)+1), RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    dprintf_resource( stddeb, "strlen = %d\n", (int)*p );
    
    i = min(buflen - 1, *p);
    if (i > 0) {
	memcpy(buffer, p + 1, i);
	buffer[i] = '\0';
    } else {
	if (buflen > 1) {
	    buffer[0] = '\0';
	    return 0;
	}
	fprintf(stderr,"LoadString // I dont know why , but caller give buflen=%d *p=%d !\n", buflen, *p);
	fprintf(stderr,"LoadString // and try to obtain string '%s'\n", p + 1);
    }
    FreeResource( hmem );

    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
    return i;
}



