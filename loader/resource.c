static char RCSId[] = "$Id: resource.c,v 1.4 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "neexe.h"
#include "windows.h"
#include "gdi.h"
#include "wine.h"

#define MIN(a,b)	((a) < (b) ? (a) : (b))

static int ResourceFd = -1;
static HANDLE ResourceInst = 0;
static struct w_files *ResourceFileInfo = NULL;


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
	if (typeinfo.type_id == type_id || type_id == -1)
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
		if (read(ResourceFd, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
		{
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

		if (strcasecmp(name, resource_name) == 0)
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
 *					LoadIcon
 */
HICON
LoadIcon(HANDLE instance, LPSTR icon_name)
{
  fprintf(stderr,"LoadIcon: (%d),%d\n",instance,icon_name);
    return 0;
}

/**********************************************************************
 *					LoadCursor
 */
HCURSOR
LoadCursor(HANDLE instance, LPSTR cursor_name)
{
  fprintf(stderr,"LoadCursor: (%d),%d\n",instance,cursor_name);
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
    fprintf(stderr,"FindResource: (%d),%d\n",instance, resource_name, type_name);
    return 0;
}

/**********************************************************************
 *				LoadResource	[KERNEL.61]
 */
HANDLE LoadResource(HANDLE instance, HANDLE hResInfo)
{
    fprintf(stderr,"LoadResource: (%d),%d\n",instance, hResInfo);
    return ;
}

/**********************************************************************
 *				LockResource	[KERNEL.62]
 */
LPSTR LockResource(HANDLE hResData)
{
    fprintf(stderr,"LockResource: %d\n", hResData);
    return ;
}

/**********************************************************************
 *				FreeResource	[KERNEL.63]
 */
BOOL FreeResource(HANDLE hResData)
{
    fprintf(stderr,"FreeResource: %d\n", hResData);
    return ;
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
    if (size_shift == -1)
	return 0;

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

    hmem = RSC_LoadResource(instance, (char *) (resource_id >> 4),
			    NE_RSCTYPE_STRING, &rsc_size);
    if (hmem == 0)
	return 0;
    
    p = GlobalLock(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < resource_id; i++)
	p += *p;
    
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
    
    if (!(hdc = GetDC( 0 ))) return 0;

    rsc_mem = RSC_LoadResource(instance, bmp_name, NE_RSCTYPE_BITMAP, 
			       &image_size);
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

    GlobalFree(rsc_mem);
    ReleaseDC( 0, hdc );
    return hbitmap;
}


