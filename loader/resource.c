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
#include "accel.h"

/* #define DEBUG_RESOURCE  */

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
extern HINSTANCE hSysRes;

HANDLE RSC_LoadResource(int instance, char *rsc_name, int type, 
			int *image_size_ret);
void RSC_LoadNameTable(void);

extern char *ProgramName;


/**********************************************************************
 *					RSC_LoadNameTable
 */
#ifndef WINELIB
void 
RSC_LoadNameTable()
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short             size_shift;
    RESNAMTAB                 *top, *new;
    char                       read_buf[1024];
    char                      *p;
    int                        i;
    unsigned short             len;
    off_t                      rtoff;
    off_t		       saved_pos;
    
    top = NULL;

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
	return;
    }
    size_shift = CONV_SHORT(size_shift);

    /*
     * Find resource.
     */
    typeinfo.type_id = 0xffff;
    while (typeinfo.type_id != 0) 
    {
	if (!load_typeinfo (ResourceFd, &typeinfo))
	    break;

	if (typeinfo.type_id == 0) 
	    break;
	if (typeinfo.type_id == 0x800f) 
	{
	    for (i = 0; i < typeinfo.count; i++) 
	    {
		if (read(ResourceFd, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
		{
		    break;
		}
		
		saved_pos = lseek(ResourceFd, 0, SEEK_CUR);
		lseek(ResourceFd, (long) nameinfo.offset << size_shift, 
		      SEEK_SET);
		read(ResourceFd, &len, sizeof(len));
		while (len)
		{
		    new = (RESNAMTAB *) GlobalQuickAlloc(sizeof(*new));
		    new->next = top;
		    top = new;

		    read(ResourceFd, &new->type_ord, 2);
		    read(ResourceFd, &new->id_ord, 2);
		    read(ResourceFd, read_buf, len - 6);
		    
		    p = read_buf + strlen(read_buf) + 1;
		    strncpy(new->id, p, MAX_NAME_LENGTH);
		    new->id[MAX_NAME_LENGTH - 1] = '\0';

		    read(ResourceFd, &len, sizeof(len));
		}

		lseek(ResourceFd, saved_pos, SEEK_SET);
	    }
	    break;
	}
	else 
	{
	    lseek(ResourceFd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
	}
    }

    ResourceFileInfo->resnamtab = top;
}
#endif /* WINELIB */

/**********************************************************************
 *					OpenResourceFile
 */
int
OpenResourceFile(HANDLE instance)
{
    struct w_files *w;
    char   *res_file;
    
    if (ResourceInst == instance)
	return ResourceFd;

    w = GetFileInfo(instance);
    if (w == NULL)
	return -1;
    ResourceFileInfo = w;
    res_file = w->filename;
    
    if (ResourceFd >= 0)
	close(ResourceFd);
    
    ResourceInst = instance;
    ResourceFd   = open (res_file, O_RDONLY);
#if 1
#ifndef WINELIB
    if (w->resnamtab == (RESNAMTAB *) -1)
    {
	RSC_LoadNameTable();
    }
#endif
#endif

#ifdef DEBUG_RESOURCE
    printf("OpenResourceFile(%04X) // file='%s' hFile=%04X !\n", 
		instance, w->filename, ResourceFd);
#endif
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

#ifndef WINELIB
load_typeinfo (int fd, struct resource_typeinfo_s *typeinfo)
{
    return read (fd, typeinfo, sizeof (*typeinfo)) == sizeof (*typeinfo);
}
#endif
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
    	printf("FindResourceByNumber (%s) bad block size !\n", resource_id);
	return -1;
    }
    size_shift = CONV_SHORT(size_shift);
    /*
     * Find resource.
     */
    typeinfo.type_id = 0xffff;
    while (typeinfo.type_id != 0) {
	if (!load_typeinfo (ResourceFd, &typeinfo)){
	    printf("FindResourceByNumber (%X) bad typeinfo size !\n", resource_id);
	    return -1;
	    }
#ifdef DEBUG_RESOURCE
	printf("FindResourceByNumber type=%X count=%d searched=%d \n", 
			typeinfo.type_id, typeinfo.count, type_id);
#endif
	if (typeinfo.type_id == 0) break;
	if (typeinfo.type_id == type_id || type_id == -1) {
	    for (i = 0; i < typeinfo.count; i++) {
#ifndef WINELIB
		if (read(ResourceFd, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
#else
		if (!load_nameinfo (ResourceFd, &nameinfo))
#endif
		{
		    printf("FindResourceByNumber (%X) bad nameinfo size !\n", resource_id);
		    return -1;
		    }
#ifdef DEBUG_RESOURCE
		printf("FindResource: search type=%X id=%X // type=%X id=%X\n",
			type_id, resource_id, typeinfo.type_id, nameinfo.id);
#endif
		if (nameinfo.id == resource_id) {
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
     * Check for loaded name table.
     */
    if (ResourceFileInfo->resnamtab != NULL)
    {
	RESNAMTAB *e;

	for (e = ResourceFileInfo->resnamtab; e != NULL; e = e->next)
	{
	    if (e->type_ord == (type_id & 0x000f) &&
		strcasecmp(e->id, resource_name) == 0)
	    {
		return FindResourceByNumber(result_p, type_id, e->id_ord);
	    }
	}

	return -1;
    }

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
    size_shift = CONV_SHORT (size_shift);
    
    /*
     * Find resource.
     */
    typeinfo.type_id = 0xffff;
    while (typeinfo.type_id != 0)
    {
	if (!load_typeinfo (ResourceFd, &typeinfo))
	{
	    printf("FindResourceByName (%s) bad typeinfo size !\n", resource_name);
	    return -1;
	}
#ifdef DEBUG_RESOURCE
	printf("FindResourceByName typeinfo.type_id=%X count=%d type_id=%X\n",
			typeinfo.type_id, typeinfo.count, type_id);
#endif
	if (typeinfo.type_id == 0) break;
	if (typeinfo.type_id == type_id || type_id == -1)
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
#ifndef WINELIB
		if (read(ResourceFd, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
#else
		if (!load_nameinfo (ResourceFd, &nameinfo))
#endif
		{
		    printf("FindResourceByName (%s) bad nameinfo size !\n", resource_name);
		    return -1;
		}
/*
		if ((nameinfo.id & 0x8000) != 0) continue;
*/		
#ifdef DEBUG_RESOURCE
		printf("FindResourceByName // nameinfo.id=%04X !\n", nameinfo.id);
#endif
		old_pos = lseek(ResourceFd, 0, SEEK_CUR);
		new_pos = rtoff + nameinfo.id;
		lseek(ResourceFd, new_pos, SEEK_SET);
		read(ResourceFd, &nbytes, 1);
#ifdef DEBUG_RESOURCE
		printf("FindResourceByName // namesize=%d !\n", nbytes);
#endif
 		nbytes = CONV_CHAR_TO_LONG (nbytes);
		read(ResourceFd, name, nbytes);
		lseek(ResourceFd, old_pos, SEEK_SET);
		name[nbytes] = '\0';
#ifdef DEBUG_RESOURCE
		printf("FindResourceByName type_id=%X (%d of %d) name='%s' resource_name='%s'\n", 
			typeinfo.type_id, i + 1, typeinfo.count, 
			name, resource_name);
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
    
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    rsc_mem = RSC_LoadResource(instance, icon_name, NE_RSCTYPE_GROUP_ICON, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %04X not Found !\n", icon_name);
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
    	NE_RSCTYPE_ICON, &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadIcon / Icon %04X Bitmaps not Found !\n", icon_name);
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
    ReleaseDC(GetDesktopWindow(), hdc);
    GlobalUnlock(hIcon);
#ifdef DEBUG_RESOURCE
    printf("LoadIcon Alloc hIcon=%X\n", hIcon);
#endif
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
    if (((LONG)lpTableName & 0xFFFF0000L) == 0L)
	printf("LoadAccelerators: instance = %04X, name = %08X\n",
			instance, lpTableName);
    else
	printf("LoadAccelerators: instance = %04X, name = '%s'\n",
			instance, lpTableName);
#endif
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    rsc_mem = RSC_LoadResource(instance, lpTableName, NE_RSCTYPE_ACCELERATOR, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadAccelerators / AccelTable %04X not Found !\n", lpTableName);
	return 0;
	}
    lp = (BYTE *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	return 0;
	}
#ifdef DEBUG_ACCEL
    printf("LoadAccelerators / image_size=%d\n", image_size);
#endif
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
#ifdef DEBUG_ACCEL
	printf("Accelerator #%u / event=%04X id=%04X type=%02X \n", 
		i, lpAccelTbl->tbl[i].wEvent, lpAccelTbl->tbl[i].wIDval, 
		lpAccelTbl->tbl[i].type);
#endif
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
    int 	i, image_size;
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
    	msg->message != WM_CHAR) return 0;
#ifdef DEBUG_ACCEL
    printf("TranslateAccelerators hAccel=%04X !\n", hAccel);
#endif
    lpAccelTbl = (LPACCELHEADER)GlobalLock(hAccel);
    for (i = 0; i < lpAccelTbl->wCount; i++) {
/*	if (lpAccelTbl->tbl[i].type & SHIFT_ACCEL) { */
/*	if (lpAccelTbl->tbl[i].type & CONTROL_ACCEL) { */
	if (lpAccelTbl->tbl[i].type & VIRTKEY_ACCEL) {
	    if (msg->wParam == lpAccelTbl->tbl[i].wEvent &&
		msg->message == WM_KEYDOWN) {
		SendMessage(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval, 0x00010000L);
		return 1;
		}
	    if (msg->message == WM_KEYUP) return 1;
	    }
	else {
	    if (msg->wParam == lpAccelTbl->tbl[i].wEvent &&
		msg->message == WM_CHAR) {
		SendMessage(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval, 0x00010000L);
		return 1;
		}
	    }
	}
    GlobalUnlock(hAccel);
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
    
#ifdef DEBUG_RESOURCE
    printf("FindResource hInst=%04X typename=%08X resname=%08X\n", 
		instance, type_name, resource_name);
#endif
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
	r->size_shift = FindResourceByNumber(&r->nameinfo, (int)type_name,
					     (int) resource_name | 0x8000);
    }
    else
    {
	r->size_shift = FindResourceByName(&r->nameinfo, (int)type_name, 
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

    lseek(ResourceFd, ((int) r->nameinfo.offset << r->size_shift), SEEK_SET);

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
    	if ((LONG)rsc_name >= 0x00010000L)
	    printf("RSC_LoadResource / Resource '%s' not Found !\n", rsc_name);
	else
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
	if (i > 0) {
		memcpy(buffer, p + 1, i);
		buffer[i] = '\0';
		}
	else {
		if (buflen > 1) {
			buffer[0] = '\0';
			return 0;
			}
		printf("LoadString // I dont know why , but caller give buflen=%d *p=%d !\n", buflen, *p);
		printf("LoadString // and try to obtain string '%s'\n", p + 1);
		}
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
#ifdef DEBUG_RESOURCE
    printf("RSC_LoadMenu: instance = %04x, name = '%s'\n",
	   instance, menu_name);
#endif
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
    int size;
    
#ifdef DEBUG_RESOURCE
    printf("LoadBitmap: instance = %04x, name = %08x\n",
	   instance, bmp_name);
#endif
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;

    rsc_mem = RSC_LoadResource(instance, bmp_name, NE_RSCTYPE_BITMAP, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadBitmap / BitMap %04X not Found !\n", bmp_name);
	return 0;
	}
    lp = (long *) GlobalLock(rsc_mem);
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

/**********************************************************************
 *			CreateIcon [USER.407]
 */
HICON CreateIcon(HANDLE hInstance, int nWidth, int nHeight, 
                 BYTE nPlanes, BYTE nBitsPixel, LPSTR lpANDbits, 
                 LPSTR lpXORbits)
{
    HICON 	hIcon;
    ICONALLOC	*lpico;

#ifdef DEBUG_RESOURCE
    printf("CreateIcon: hInstance = %04x, nWidth = %08x, nHeight = %08x \n",
	    hInstance, nWidth, nHeight);
    printf("  nPlanes = %04x, nBitsPixel = %04x,",nPlanes, nBitsPixel);
    printf(" lpANDbits= %04x, lpXORbits = %04x, \n",lpANDbits, lpXORbits);
#endif

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
#ifdef DEBUG_RESOURCE
    printf("CreateIcon Alloc hIcon=%X\n", hIcon);
#endif
    return hIcon;
}
