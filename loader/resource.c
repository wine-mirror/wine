static char RCSId[] = "$Id: resource.c,v 1.4 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "arch.h"
#include "prototypes.h"
#include "windows.h"
#include "gdi.h"
#include "wine.h"
#include "icon.h"
#include "menu.h"
#include "accel.h"
#include "dlls.h"
#include "resource.h"
#include "stddebug.h"
/* #define DEBUG_RESOURCE */
/* #undef  DEBUG_RESOURCE */
/* #define DEBUG_ACCEL    */
/* #undef  DEBUG_ACCEL    */
#include "debug.h"

#define MIN(a,b)	((a) < (b) ? (a) : (b))

RESOURCE *Top = NULL;
extern HINSTANCE hSysRes;

extern int NE_FindResource(HANDLE, LPSTR, LPSTR, RESOURCE *);
extern int PE_FindResource(HANDLE, LPSTR, LPSTR, RESOURCE *);
extern HBITMAP BITMAP_LoadOEMBitmap( WORD id );  /* objects/bitmap.c */

#define PrintId(name) \
	if (HIWORD((DWORD)name)) \
		printf(", %s", name); \
	else \
		printf(", #%d", (int) name); 

/**********************************************************************
 *			FindResource	[KERNEL.60]
 */
HANDLE FindResource(HANDLE instance, LPSTR name, LPSTR type)
{
	int status;
	RESOURCE *r;
	HANDLE rh;

#ifdef DEBUG_RESOURCE 
	printf("FindResource(%04X", instance);
	PrintId(name);
	PrintId(type);
	printf(")\n");
#endif

	if (instance == (HANDLE)NULL)
		instance = hSysRes;

	/* FIXME: did we already find this one ? */

	if ((rh = GlobalAlloc(GMEM_MOVEABLE, sizeof(RESOURCE))) == 0)
		return 0;

	r = (RESOURCE *)GlobalLock(rh);
	r->next = Top;
	Top = r;
	r->info_mem = rh;
	r->rsc_mem = 0;
	r->count = 0;
	if (HIWORD((DWORD)name))
		r->name = strdup(name);
	else
		r->name = name;

	if (HIWORD((DWORD)type))
		r->type = strdup(type);
	else
		r->type = type;

	r->wpnt = GetFileInfo(instance);
	r->fd = dup(r->wpnt->fd);
	if (r->wpnt->ne)
		status = NE_FindResource(instance, name, type, r);
	else
		status = PE_FindResource(instance, name, type, r);

	if (!status) {
		if (HIWORD((DWORD)r->name))
			free(r->name);

		if (HIWORD((DWORD)r->type))
			free(r->type);
		close(r->fd);

		Top = r->next;
		GlobalUnlock(rh);
		return 0;
	} else
		return rh;
}

/**********************************************************************
 *			AllocResource	[KERNEL.66]
 */
HANDLE AllocResource(HANDLE instance, HANDLE hResInfo, DWORD dwSize)
{
	RESOURCE *r;
	int image_size;

	dprintf_resource(stddeb, "AllocResource(%04X, %04X, %08X);\n", 
		instance, hResInfo, (int) dwSize);

	if (instance == (HANDLE)NULL)
		instance = hSysRes;

	if ((r = (RESOURCE *)GlobalLock(hResInfo)) == NULL)
		return 0;
    
	image_size = r->size;

	if (dwSize == 0)
		r->rsc_mem = GlobalAlloc(GMEM_MOVEABLE, image_size);
	else
		r->rsc_mem = GlobalAlloc(GMEM_MOVEABLE, dwSize);

	GlobalUnlock(hResInfo);

	return r->rsc_mem;
}

/**********************************************************************
 *				AccessResource	[KERNEL.64]
 */
int AccessResource(HANDLE instance, HANDLE hResInfo)
{
	int fd;
	RESOURCE *r;

	dprintf_resource(stddeb, "AccessResource(%04X, %04X);\n", 
		instance, hResInfo);

	if (instance == (HANDLE)NULL)
		instance = hSysRes;

	if ((r = (RESOURCE *)GlobalLock(hResInfo)) == NULL)
		return -1;

	fd = r->fd;
	lseek(fd, r->offset, SEEK_SET);
	GlobalUnlock(hResInfo);

	return fd;
}

/**********************************************************************
 *				SizeofResource	[KERNEL.65]
 */
WORD SizeofResource(HANDLE instance, HANDLE hResInfo)
{
	RESOURCE *r;
	int size;
	
	dprintf_resource(stddeb, "SizeofResource(%04X, %04X);\n", 
		instance, hResInfo);

	if (instance == (HANDLE)NULL)
		instance = hSysRes;

	if ((r = (RESOURCE *)GlobalLock(hResInfo)) == NULL)
		return 0;

	size = r->size;
	GlobalUnlock(hResInfo);

	return size;
}

/**********************************************************************
 *			LoadResource	[KERNEL.61]
 */
HANDLE LoadResource(HANDLE instance, HANDLE hResInfo)
{
    RESOURCE *r;
    int image_size, fd;
    void *image;
    HANDLE h;

    dprintf_resource(stddeb, "LoadResource(%04X, %04X);\n", instance, hResInfo);

    if (instance == (HANDLE)NULL)
	instance = hSysRes;

    if ((r = (RESOURCE *)GlobalLock(hResInfo)) == NULL)
	return 0;
    
    h = r->rsc_mem = AllocResource(instance, hResInfo, 0);
    image = GlobalLinearLock(h);
    image_size = r->size;
    fd = AccessResource(instance, hResInfo);

    if (image == NULL || read(fd, image, image_size) != image_size) {
	GlobalFree(h);
	GlobalUnlock(hResInfo);
	return 0;
    }
    r->count++;
    close(fd);
    GlobalLinearUnlock(h);
    GlobalUnlock(hResInfo);
    return h;
}

/**********************************************************************
 *				LockResource	[KERNEL.62]
 */
LPSTR LockResource(HANDLE hResData)
{
    return GlobalLock(hResData);
}

/**********************************************************************
 *				FreeResource	[KERNEL.63]
 */
HANDLE FreeResource(HANDLE hResData)
{
    RESOURCE *r, *rp;

    dprintf_resource(stddeb, "FreeResource: handle %04x\n", hResData);

    for (r = rp = Top; r ; r = r->next) {
	if (r->rsc_mem == hResData) {
	    if (r->count == 0) {
		    if (rp != r)
			rp->next = r->next;
		    else
			Top = r->next;

		    if (HIWORD((DWORD)r->name))
		    	free(r->name);
                    if (HIWORD((DWORD)r->type))
			free(r->type);
		    GlobalFree(r->rsc_mem);
		    GlobalFree(r->info_mem);
		    return 0;
	    } else
	    	r->count--;
	}
	rp = r;
    }
    return hResData;
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
 *			RSC_LoadResource
 */
HANDLE
RSC_LoadResource(int instance, LPSTR rsc_name, LPSTR type, int *image_size_ret)
{
	HANDLE hResInfo;
	RESOURCE *r;

	if (instance == (HANDLE)NULL)
		instance = hSysRes;

	dprintf_resource(stddeb, "RSC_LoadResource: instance = %04x, name = %08x, type = %08x\n",
	   instance, (int) rsc_name, (int) type);

	if ((hResInfo = FindResource(instance, rsc_name, (LPSTR) type)) == (HANDLE) NULL) {
		return (HANDLE)NULL;
	}
	r = (RESOURCE *)GlobalLock(hResInfo);
	if (image_size_ret) 
		*image_size_ret = r->size;
	r->count++;
	GlobalUnlock(hResInfo);
	return LoadResource(instance, hResInfo);
}

/**********************************************************************
 *			LoadIcon [USER.174]
 */
HICON LoadIcon(HANDLE instance, LPSTR icon_name)
{
    HICON 	hIcon;
    HANDLE 	rsc_mem;
    WORD 	*lp;
    ICONDESCRIP *lpicodesc;
    ICONALLOC	*lpico;
    int		width, height;
    BITMAPINFO 	*bmi;
    BITMAPINFOHEADER 	*bih;
    RGBQUAD	*rgbq;
    HDC 	hMemDC;
    HDC 	hMemDC2;
    HDC 	hdc;
    int 	image_size;
    HBITMAP     hbmpOld1, hbmpOld2;

#ifdef DEBUG_RESOURCE
	printf("LoadIcon(%04X", instance);
	PrintId(icon_name);
	printf(")\n");
#endif
    
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    rsc_mem = RSC_LoadResource(instance, icon_name, (LPSTR) NE_RSCTYPE_GROUP_ICON, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %04X not Found !\n", (int) icon_name);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lpicodesc = (ICONDESCRIP *)(lp + 3);
    hIcon = GlobalAlloc(GMEM_MOVEABLE, sizeof(ICONALLOC) + 1024);
    if (hIcon == (HICON)NULL) {
	GlobalFree(rsc_mem);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    lpico->descriptor = *lpicodesc;
    width = lpicodesc->Width;
    height = lpicodesc->Height;
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    rsc_mem = RSC_LoadResource(instance, 
    	MAKEINTRESOURCE(lpicodesc->icoDIBOffset), 
    	(LPSTR) NE_RSCTYPE_ICON, &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %04X Bitmaps not Found !\n", (int) icon_name);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
 	}
    bmi = (BITMAPINFO *)lp;
    bih = (BITMAPINFOHEADER *)lp;
    rgbq = &bmi->bmiColors[0];
    bih->biHeight = bih->biHeight / 2;
/*
    printf("LoadIcon / image_size=%d width=%d height=%d bih->biBitCount=%d bih->biSizeImage=%ld\n", 
    	image_size, width, height, bih->biBitCount, bih->biSizeImage);
*/
    if (bih->biSize == sizeof(BITMAPINFOHEADER))
	lpico->hBitmap = ConvertInfoBitmap(hdc, (BITMAPINFO *)bih);
    else
        lpico->hBitmap = 0;
    bih->biBitCount = 1;
    bih->biClrUsed = bih->biClrImportant  = 2;
    rgbq[0].rgbBlue 	= 0xFF;
    rgbq[0].rgbGreen 	= 0xFF;
    rgbq[0].rgbRed 	= 0xFF;
    rgbq[0].rgbReserved = 0x00;
    rgbq[1].rgbBlue 	= 0x00;
    rgbq[1].rgbGreen 	= 0x00;
    rgbq[1].rgbRed 	= 0x00;
    rgbq[1].rgbReserved = 0x00;
    if (bih->biSizeImage == 0) {
	if (bih->biCompression != BI_RGB) {
	    fprintf(stderr,"Unknown size for compressed Icon bitmap.\n");
	    GlobalFree(rsc_mem);
	    ReleaseDC(GetDesktopWindow(), hdc); 
	    return 0;
	    }
	bih->biSizeImage = (bih->biWidth * bih->biHeight * bih->biBitCount
			    + 7) / 8;
	}
    lpico->hBitMask = CreateDIBitmap(hdc, bih, CBM_INIT,
    	(LPSTR)lp + bih->biSizeImage - sizeof(BITMAPINFOHEADER) / 2 - 4,
	(BITMAPINFO *)bih, DIB_RGB_COLORS );
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    hMemDC = CreateCompatibleDC(hdc);
    hMemDC2 = CreateCompatibleDC(hdc);
    hbmpOld1 = SelectObject(hMemDC, lpico->hBitmap);
    hbmpOld2 = SelectObject(hMemDC2, lpico->hBitMask);
    BitBlt(hMemDC, 0, 0, bih->biWidth, bih->biHeight, hMemDC2, 0, 0,SRCINVERT);
    SelectObject( hMemDC, hbmpOld1 );
    SelectObject( hMemDC2, hbmpOld2 );
    DeleteDC(hMemDC);
    DeleteDC(hMemDC2);
    ReleaseDC(GetDesktopWindow(), hdc);
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
 *			LoadAccelerators	[USER.177]
 */
HANDLE LoadAccelerators(HANDLE instance, LPSTR lpTableName)
{
    HANDLE 	hAccel;
    HANDLE 	rsc_mem;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, image_size, n;

#ifdef DEBUG_ACCEL
	printf("LoadAccelerators(%04X", instance);
	PrintId(lpTableName);
	printf(")\n");
#endif

    rsc_mem = RSC_LoadResource(instance, lpTableName, (LPSTR) NE_RSCTYPE_ACCELERATOR, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadAccelerators(%04X", instance);
	PrintId(lpTableName);
	printf(") not found !\n");
	return 0;
	}
    lp = (BYTE *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	return 0;
	}
    dprintf_accel(stddeb,"LoadAccelerators / image_size=%d\n", image_size);
    n = image_size/5;
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
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
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
    HANDLE hmem;
    int rsc_size;
    unsigned char *p;
    int string_num;
    int i;

    dprintf_resource(stddeb, "LoadString: instance = %04x, id = %d, buffer = %08x, "
	   "length = %d\n", instance, resource_id, (int) buffer, buflen);

    hmem = RSC_LoadResource(instance, (char *) ((resource_id >> 4) + 1),
			    (LPSTR) NE_RSCTYPE_STRING, &rsc_size);
    if (hmem == 0)
	return 0;
    
    p = GlobalLock(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    i = MIN(buflen - 1, *p);
	if (i > 0) {
		memcpy(buffer, p + 1, i);
		buffer[i] = '\0';
		}
	else {
		if (buflen > 1) {
			buffer[0] = '\0';
			return 0;
			}
		fprintf(stderr,"LoadString // I dont know why , but caller give buflen=%d *p=%d !\n", buflen, *p);
		fprintf(stderr,"LoadString // and try to obtain string '%s'\n", p + 1);
		}
    GlobalFree(hmem);

    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *			LoadMenu		[USER.150]
 */
HMENU LoadMenu(HINSTANCE instance, char *menu_name)
{
	HMENU     		hMenu;
	HANDLE		hMenu_desc;
	MENU_HEADER 	*menu_desc;

#ifdef DEBUG_MENU
	printf("LoadMenu(%04X", instance);
	PrintId(menu_name);
	printf(")\n");
#endif
	if (menu_name == NULL)
		return 0;

	if ((hMenu_desc = RSC_LoadResource(instance, menu_name, (LPSTR) NE_RSCTYPE_MENU, NULL)) == (HANDLE) NULL)
		return 0;
	
	menu_desc = (MENU_HEADER *) GlobalLock(hMenu_desc);
	hMenu = LoadMenuIndirect((LPSTR)menu_desc);
	return hMenu;
}

/**********************************************************************
 *					LoadBitmap
 */
HBITMAP 
LoadBitmap(HANDLE instance, LPSTR bmp_name)
{
    HBITMAP hbitmap;
    HANDLE rsc_mem;
    HDC hdc;
    long *lp;
    int image_size;
    int size;
    
#ifdef DEBUG_RESOURCE
	printf("LoadBitmap(%04X", instance);
	PrintId(bmp_name);
	printf(")\n");
#endif

    if (!instance) {
	hbitmap = BITMAP_LoadOEMBitmap(((int) bmp_name) & 0xffff);
	if (hbitmap)
		return hbitmap;
    }

    rsc_mem = RSC_LoadResource(instance, bmp_name, (LPSTR) NE_RSCTYPE_BITMAP, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadBitmap(%04X", instance);
	PrintId(bmp_name);
	printf(") NOT found!\n");
	return 0;
	}
    lp = (long *) GlobalLinearLock(rsc_mem);
    if (!(hdc = GetDC(0))) lp = NULL;
    if (lp == NULL)
    {
	GlobalFree(rsc_mem);
	return 0;
    }
    size = CONV_LONG (*lp);
    if (size == sizeof(BITMAPCOREHEADER)){
	CONV_BITMAPCOREHEADER (lp);
	hbitmap = ConvertCoreBitmap( hdc, (BITMAPCOREHEADER *) lp );
    } else if (size == sizeof(BITMAPINFOHEADER)){
	CONV_BITMAPINFO (lp);
	hbitmap = ConvertInfoBitmap( hdc, (BITMAPINFO *) lp );
    } else hbitmap = 0;
    GlobalFree(rsc_mem);
    ReleaseDC( 0, hdc );
    return hbitmap;
}
