static char RCSId[] = "$Id: resource.c,v 1.4 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <X11/Intrinsic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "wine.h"
#include "icon.h"
#include "cursor.h"

#define MIN(a,b)	((a) < (b) ? (a) : (b))

typedef struct resource_s
{
    struct resource_s *next;
    HANDLE info_mem;
    int size_shift;
    struct resource_nameinfo_s nameinfo;
    HANDLE rsc_mem;
} RESOURCE;

static int ResourceFd = -1;
static HANDLE ResourceInst = 0;
static struct w_files *ResourceFileInfo = NULL;
static RESOURCE *Top = NULL;
static HCURSOR hActiveCursor;
extern HINSTANCE hSysRes;

HANDLE RSC_LoadResource(int instance, char *rsc_name, int type, int *image_size_ret);


/**********************************************************************
 *					OpenResourceFile
 */
int
OpenResourceFile(HANDLE instance)
{
    struct w_files *w;
    
    if (ResourceInst == instance)
	return ResourceFd;
    
    w = GetFileInfo(instance);
    if (w == NULL)
	return -1;
    
    if (ResourceFd >= 0)
	close(ResourceFd);
    
    ResourceInst = instance;
    ResourceFileInfo = w;
    ResourceFd = open(w->filename, O_RDONLY);
    
    return ResourceFd;
}

/**********************************************************************
 *					ConvertCoreBitmap
 */
HBITMAP
ConvertCoreBitmap( HDC hdc, BITMAPCOREHEADER * image )
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
 *					ConvertInfoBitmap
 */
HBITMAP
ConvertInfoBitmap( HDC hdc, BITMAPINFO * image )
{
    char * bits = ((char *)image) + DIB_BitmapInfoSize(image, DIB_RGB_COLORS);
    return CreateDIBitmap( hdc, &image->bmiHeader, CBM_INIT,
			   bits, image, DIB_RGB_COLORS );
} 

/**********************************************************************
 *					FindResourceByNumber
 */
int
FindResourceByNumber(struct resource_nameinfo_s *result_p,
		     int type_id, int resource_id)
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short size_shift;
    int i;
    off_t rtoff;

    /*
     * Move to beginning of resource table.
     */
    rtoff = (ResourceFileInfo->mz_header->ne_offset +
	     ResourceFileInfo->ne_header->resource_tab_offset);
    lseek(ResourceFd, rtoff, SEEK_SET);
    
    /*
     * Read block size.
     */
    if (read(ResourceFd, &size_shift, sizeof(size_shift)) != 
	sizeof(size_shift))
    {
	return -1;
    }
    
    /*
     * Find resource.
     */
    typeinfo.type_id = 0xffff;
    while (typeinfo.type_id != 0)
    {
	if (read(ResourceFd, &typeinfo, sizeof(typeinfo)) !=
	    sizeof(typeinfo))
	{
	    return -1;
	}
	if (typeinfo.type_id != 0)
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
		if (read(ResourceFd, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
		{
		    return -1;
		}

#if defined(DEBUG_RESOURCE) && defined(VERBOSE_DEBUG)
		if (type_id == typeinfo.type_id)
		{
		    printf("FindResource: type id = %d, resource id = %x\n",
			   type_id, nameinfo.id);
		}
#endif
		if ((type_id == -1 || typeinfo.type_id == type_id) &&
		    nameinfo.id == resource_id)
		{
		    memcpy(result_p, &nameinfo, sizeof(nameinfo));
		    return size_shift;
		}
	    }
	}
    }
    
    return -1;
}

/**********************************************************************
 *					FindResourceByName
 */
int
FindResourceByName(struct resource_nameinfo_s *result_p,
		     int type_id, char *resource_name)
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short size_shift;
    off_t old_pos, new_pos;
    unsigned char nbytes;
    char name[256];
    int i;
    off_t rtoff;

    /*
     * Move to beginning of resource table.
     */
    rtoff = (ResourceFileInfo->mz_header->ne_offset +
	     ResourceFileInfo->ne_header->resource_tab_offset);
    lseek(ResourceFd, rtoff, SEEK_SET);
    
    /*
     * Read block size.
     */
    if (read(ResourceFd, &size_shift, sizeof(size_shift)) != 
	sizeof(size_shift))
    {
    	printf("FindResourceByName (%s) bad block size !\n", resource_name);
	return -1;
    }
    
    /*
     * Find resource.
     */
    typeinfo.type_id = 0xffff;
    while (typeinfo.type_id != 0)
    {
	if (read(ResourceFd, &typeinfo, sizeof(typeinfo)) !=
	    sizeof(typeinfo))
	{
	    printf("FindResourceByName (%s) bad typeinfo size !\n", resource_name);
	    return -1;
	}
#ifdef DEBUG_RESOURCE
	printf("FindResourceByName typeinfo.type_id=%d type_id=%d\n",
			typeinfo.type_id, type_id);
#endif
	if (typeinfo.type_id == 0) break;
	if (typeinfo.type_id == type_id || type_id == -1)
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
		if (read(ResourceFd, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
		{
		    printf("FindResourceByName (%s) bad nameinfo size !\n", resource_name);
		    return -1;
		}

		if (nameinfo.id & 0x8000)
		    continue;
		
		old_pos = lseek(ResourceFd, 0, SEEK_CUR);
		new_pos = rtoff + nameinfo.id;
		lseek(ResourceFd, new_pos, SEEK_SET);
		read(ResourceFd, &nbytes, 1);
		read(ResourceFd, name, nbytes);
		lseek(ResourceFd, old_pos, SEEK_SET);
		name[nbytes] = '\0';
#ifdef DEBUG_RESOURCE
		printf("FindResourceByName type_id=%d name='%s' resource_name='%s'\n", 
				typeinfo.type_id, name, resource_name);
#endif
		if (strcasecmp(name, resource_name) == 0)
		{
		    memcpy(result_p, &nameinfo, sizeof(nameinfo));
		    return size_shift;
		}
	    }
	}
	else {
	    lseek(ResourceFd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
	}
    }
    return -1;
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
    HBITMAP 	hBitMap;
    HDC 	hMemDC;
    HDC 	hMemDC2;
    HDC 	hdc;
    int 	i, j, image_size;
#ifdef DEBUG_RESOURCE
    printf("LoadIcon: instance = %04x, name = %08x\n",
	   instance, icon_name);
#endif
    
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    rsc_mem = RSC_LoadResource(instance, icon_name, NE_RSCTYPE_GROUP_ICON, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %04X not Found !\n", icon_name);
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	return 0;
	}
    lpicodesc = (ICONDESCRIP *)(lp + 3);
    hIcon = GlobalAlloc(GMEM_MOVEABLE, sizeof(ICONALLOC) + 1024);
    if (hIcon == (HICON)NULL) {
	GlobalFree(rsc_mem);
	return 0;
	}
    printf("LoadIcon Alloc hIcon=%X\n", hIcon);
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    lpico->descriptor = *lpicodesc;
    width = lpicodesc->Width;
    height = lpicodesc->Height;
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    rsc_mem = RSC_LoadResource(instance, 
    	MAKEINTRESOURCE(lpicodesc->icoDIBOffset), 
    	NE_RSCTYPE_ICON, &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %04X Bitmaps not Found !\n", icon_name);
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
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
    lpico->hBitMask = CreateDIBitmap(hdc, bih, CBM_INIT,
    	(LPSTR)lp + bih->biSizeImage - sizeof(BITMAPINFOHEADER) / 2 - 4,
	(BITMAPINFO *)bih, DIB_RGB_COLORS );
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    
    hMemDC = CreateCompatibleDC(hdc);
    hMemDC2 = CreateCompatibleDC(hdc);
    SelectObject(hMemDC, lpico->hBitmap);
    SelectObject(hMemDC2, lpico->hBitMask);
    BitBlt(hMemDC, 0, 0, bih->biWidth, bih->biHeight, hMemDC2, 0, 0, SRCINVERT);
    DeleteDC(hMemDC);
    DeleteDC(hMemDC2);

    ReleaseDC(0, hdc);
    return hIcon;
}


/**********************************************************************
 *			DestroyIcon [USER.457]
 */
BOOL DestroyIcon(HICON hIcon)
{
    ICONALLOC	*lpico;
    if (hIcon == (HICON)NULL) return FALSE;
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    if (lpico->hBitmap != (HBITMAP)NULL) DeleteObject(lpico->hBitmap);
    GlobalFree(hIcon);
    return TRUE;
}


/**********************************************************************
 *			LoadCursor [USER.173]
 */
HCURSOR LoadCursor(HANDLE instance, LPSTR cursor_name)
{
    XColor	bkcolor;
    XColor	fgcolor;
    HCURSOR 	hCursor;
    HANDLE 	rsc_mem;
    WORD 	*lp;
    CURSORDESCRIP *lpcurdesc;
    CURSORALLOC	  *lpcur;
    BITMAP 	BitMap;
    HBITMAP 	hBitMap;
    HDC 	hMemDC;
    HDC 	hdc;
    int i, j, image_size;
#ifdef DEBUG_RESOURCE
    printf("LoadCursor: instance = %04x, name = %08x\n",
	   instance, cursor_name);
#endif    
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    rsc_mem = RSC_LoadResource(instance, cursor_name, NE_RSCTYPE_GROUP_CURSOR, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadCursor / Cursor %08X not Found !\n", cursor_name);
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	return 0;
	}
    lpcurdesc = (CURSORDESCRIP *)(lp + 3);
#ifdef DEBUG_CURSOR
    printf("LoadCursor / image_size=%d\n", image_size);
    printf("LoadCursor / curReserved=%X\n", *lp);
    printf("LoadCursor / curResourceType=%X\n", *(lp + 1));
    printf("LoadCursor / curResourceCount=%X\n", *(lp + 2));
    printf("LoadCursor / cursor Width=%d\n", (int)lpcurdesc->Width);
    printf("LoadCursor / cursor Height=%d\n", (int)lpcurdesc->Height);
    printf("LoadCursor / cursor curXHotspot=%d\n", (int)lpcurdesc->curXHotspot);
    printf("LoadCursor / cursor curYHotspot=%d\n", (int)lpcurdesc->curYHotspot);
    printf("LoadCursor / cursor curDIBSize=%lX\n", (DWORD)lpcurdesc->curDIBSize);
    printf("LoadCursor / cursor curDIBOffset=%lX\n", (DWORD)lpcurdesc->curDIBOffset);
#endif
    hCursor = GlobalAlloc(GMEM_MOVEABLE, sizeof(CURSORALLOC) + 1024L); 
    if (hCursor == (HCURSOR)NULL) {
	GlobalFree(rsc_mem);
	return 0;
	}
    printf("LoadCursor Alloc hCursor=%X\n", hCursor);
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    lpcur->descriptor = *lpcurdesc;
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    rsc_mem = RSC_LoadResource(instance, 
    	MAKEINTRESOURCE(lpcurdesc->curDIBOffset), 
    	NE_RSCTYPE_CURSOR, &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadCursor / Cursor %08X Bitmap not Found !\n", cursor_name);
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	return 0;
 	}
    lp += 2;
    for (j = 0; j < 16; j++)
    	printf("%04X ", *(lp + j));
    if (*lp == sizeof(BITMAPINFOHEADER))
	lpcur->hBitmap = ConvertInfoBitmap(hdc, (BITMAPINFO *)lp);
    else
        lpcur->hBitmap = 0;
    lp += sizeof(BITMAP);
    for (i = 0; i < 81; i++) {
	char temp = *((char *)lp + 162 + i);
	*((char *)lp + 162 + i) = *((char *)lp + 324 - i);
	*((char *)lp + 324 - i) = temp;
	}
printf("LoadCursor / before XCreatePixmapFromBitmapData !\n");
    lpcur->pixshape = XCreatePixmapFromBitmapData(
    	XT_display, DefaultRootWindow(XT_display), 
        ((char *)lp + 211), 32, 32,
/*
        lpcurdesc->Width / 2, lpcurdesc->Height / 4, 
*/
        WhitePixel(XT_display, DefaultScreen(XT_display)), 
        BlackPixel(XT_display, DefaultScreen(XT_display)), 1);
    lpcur->pixmask = lpcur->pixshape;
    bkcolor.pixel = WhitePixel(XT_display, DefaultScreen(XT_display)); 
    fgcolor.pixel = BlackPixel(XT_display, DefaultScreen(XT_display));
printf("LoadCursor / before XCreatePixmapCursor !\n");
    lpcur->xcursor = XCreatePixmapCursor(XT_display,
 	lpcur->pixshape, lpcur->pixmask, 
 	&fgcolor, &bkcolor, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot);

    ReleaseDC(0, hdc); 
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    return hCursor;
}



/**********************************************************************
 *			DestroyCursor [USER.458]
 */
BOOL DestroyCursor(HCURSOR hCursor)
{
    CURSORALLOC	*lpcur;
    if (hCursor == (HCURSOR)NULL) return FALSE;
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    if (lpcur->hBitmap != (HBITMAP)NULL) DeleteObject(lpcur->hBitmap);
    GlobalUnlock(hCursor);
    GlobalFree(hCursor);
    return TRUE;
}


/**********************************************************************
 *			SetCursor [USER.69]
 */
HCURSOR SetCursor(HCURSOR hCursor)
{
    HDC		hDC;
    HDC		hMemDC;
    BITMAP	bm;
    CURSORALLOC	*lpcur;
    HCURSOR	hOldCursor;
#ifdef DEBUG_CURSOR
    printf("SetCursor / hCursor=%04X !\n", hCursor);
#endif
    if (hCursor == (HCURSOR)NULL) return FALSE;
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    hOldCursor = hActiveCursor;
    
printf("SetCursor / before XDefineCursor !\n");
    XDefineCursor(XT_display, DefaultRootWindow(XT_display), lpcur->xcursor);
    GlobalUnlock(hCursor);
    hActiveCursor = hCursor;
    return hOldCursor;
}


/**********************************************************************
 *			ShowCursor [USER.71]
 */
int ShowCursor(BOOL bShow)
{
    if (bShow) {
    	}
    return 0;
}


/**********************************************************************
 *					LoadAccelerators
 */
HANDLE
LoadAccelerators(HANDLE instance, LPSTR lpTableName)
{
  fprintf(stderr,"LoadAccelerators: (%d),%d\n",instance,lpTableName);
    return 0;
}

/**********************************************************************
 *				FindResource	[KERNEL.60]
 */
HANDLE FindResource(HANDLE instance, LPSTR resource_name, LPSTR type_name)
{
    RESOURCE *r;
    HANDLE rh;

    if (instance == 0)
	return 0;
    
    if (OpenResourceFile(instance) < 0)
	return 0;
    
    rh = GlobalAlloc(GMEM_MOVEABLE, sizeof(*r));
    if (rh == 0)
	return 0;
    r = (RESOURCE *)GlobalLock(rh);

    r->next = Top;
    Top = r;
    r->info_mem = rh;
    r->rsc_mem = 0;

    if (((int) resource_name & 0xffff0000) == 0)
    {
	r->size_shift = FindResourceByNumber(&r->nameinfo, type_name,
					     (int) resource_name | 0x8000);
    }
    else
    {
	r->size_shift = FindResourceByName(&r->nameinfo, type_name, 
					   resource_name);
    }
    
    if (r->size_shift == -1)
    {
	GlobalFree(rh);
	return 0;
    }

    return rh;
}

/**********************************************************************
 *				LoadResource	[KERNEL.61]
 */
HANDLE LoadResource(HANDLE instance, HANDLE hResInfo)
{
    RESOURCE *r;
    int image_size;
    void *image;
    HANDLE h;

    if (instance == 0)
	return 0;
    
    if (OpenResourceFile(instance) < 0)
	return 0;

    r = (RESOURCE *)GlobalLock(hResInfo);
    if (r == NULL)
	return 0;
    
    image_size = r->nameinfo.length << r->size_shift;

    h = r->rsc_mem = GlobalAlloc(GMEM_MOVEABLE, image_size);
    image = GlobalLock(h);

    if (image == NULL || read(ResourceFd, image, image_size) != image_size)
    {
	GlobalFree(h);
	GlobalUnlock(hResInfo);
	return 0;
    }

    GlobalUnlock(h);
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
    
    for (r = rp = Top; r != NULL; r = r->next)
    {
	if (r->info_mem == hResData)
	{
	    if (rp != NULL)
		rp->next = r->next;
	    else
		Top = r->next;
	    
	    GlobalFree(r->rsc_mem);
	    GlobalFree(r->info_mem);
	    return 0;
	}
    }
    
    return hResData;
}

/**********************************************************************
 *				AccessResource	[KERNEL.64]
 */
int AccessResource(HANDLE instance, HANDLE hResInfo)
{
    int	resfile;
#ifdef DEBUG_RESOURCE
    printf("AccessResource(%04X, %04X);\n", instance, hResInfo);
#endif
/*
    resfile = OpenResourceFile(instance);
    return resfile;
*/
    return - 1;
}



/**********************************************************************
 *					RSC_LoadResource
 */
HANDLE
RSC_LoadResource(int instance, char *rsc_name, int type, int *image_size_ret)
{
    struct resource_nameinfo_s nameinfo;
    HANDLE hmem;
    void *image;
    int image_size;
    int size_shift;
    
    /*
     * Built-in resources
     */
    if (instance == 0)
    {
	return 0;
    }
    else if (OpenResourceFile(instance) < 0)
	return 0;
    
    /*
     * Get resource by ordinal
     */
    if (((int) rsc_name & 0xffff0000) == 0)
    {
	size_shift = FindResourceByNumber(&nameinfo, type,
					  (int) rsc_name | 0x8000);
    }
    /*
     * Get resource by name
     */
    else
    {
	size_shift = FindResourceByName(&nameinfo, type, rsc_name);
    }
    if (size_shift == -1) {
    	printf("RSC_LoadResource / Resource '%X' not Found !\n", rsc_name);
	return 0;
	}
    /*
     * Read resource.
     */
    lseek(ResourceFd, ((int) nameinfo.offset << size_shift), SEEK_SET);

    image_size = nameinfo.length << size_shift;
    if (image_size_ret != NULL)
	*image_size_ret = image_size;
    hmem = GlobalAlloc(GMEM_MOVEABLE, image_size);
    image = GlobalLock(hmem);
    if (image == NULL || read(ResourceFd, image, image_size) != image_size)
    {
	GlobalFree(hmem);
	return 0;
    }

    GlobalUnlock(hmem);
    return hmem;
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

#ifdef DEBUG_RESOURCE
    printf("LoadString: instance = %04x, id = %d, "
	   "buffer = %08x, length = %d\n",
	   instance, resource_id, buffer, buflen);
#endif

    hmem = RSC_LoadResource(instance, (char *) ((resource_id >> 4) + 1),
			    NE_RSCTYPE_STRING, &rsc_size);
    if (hmem == 0)
	return 0;
    
    p = GlobalLock(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    i = MIN(buflen - 1, *p);
    memcpy(buffer, p + 1, i);
    buffer[i] = '\0';

    GlobalFree(hmem);

#ifdef DEBUG_RESOURCE
    printf("            '%s'\n", buffer);
#endif
    return i;
}

/**********************************************************************
 *					RSC_LoadMenu
 */
HANDLE
RSC_LoadMenu(HANDLE instance, LPSTR menu_name)
{
    return RSC_LoadResource(instance, menu_name, NE_RSCTYPE_MENU, NULL);
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

#ifdef DEBUG_RESOURCE
    printf("LoadBitmap: instance = %04x, name = %08x\n",
	   instance, bmp_name);
#endif
    printf("LoadBitmap: instance = %04x, name = %08x\n",
	   instance, bmp_name);
    
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;

printf("before RSC_Load\n");
    rsc_mem = RSC_LoadResource(instance, bmp_name, NE_RSCTYPE_BITMAP, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadBitmap / BitMap %04X not Found !\n", bmp_name);
	return 0;
	}
printf("before GlobalLock\n");
    lp = (long *) GlobalLock(rsc_mem);
    if (lp == NULL)
    {
	GlobalFree(rsc_mem);
	return 0;
    }
    if (*lp == sizeof(BITMAPCOREHEADER))
	hbitmap = ConvertCoreBitmap( hdc, (BITMAPCOREHEADER *) lp );
    else if (*lp == sizeof(BITMAPINFOHEADER))
	hbitmap = ConvertInfoBitmap( hdc, (BITMAPINFO *) lp );
    else hbitmap = 0;
printf("LoadBitmap %04X\n", hbitmap);
    GlobalFree(rsc_mem);
    ReleaseDC( 0, hdc );
    return hbitmap;
}


