static char RCSId[] = "$Id: resource.c,v 1.4 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "neexe.h"
#include "windows.h"
#include "gdi.h"

#define MIN(a,b)	((a) < (b) ? (a) : (b))

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

    /*
     * Move to beginning of resource table.
     */
    lseek(CurrentNEFile, (CurrentMZHeader->ne_offset +
			  CurrentNEHeader->resource_tab_offset), SEEK_SET);
    
    /*
     * Read block size.
     */
    if (read(CurrentNEFile, &size_shift, sizeof(size_shift)) != 
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
	if (read(CurrentNEFile, &typeinfo, sizeof(typeinfo)) !=
	    sizeof(typeinfo))
	{
	    return -1;
	}
	if (typeinfo.type_id != 0)
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
		if (read(CurrentNEFile, &nameinfo, sizeof(nameinfo)) != 
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

    /*
     * Move to beginning of resource table.
     */
    lseek(CurrentNEFile, (CurrentMZHeader->ne_offset +
			  CurrentNEHeader->resource_tab_offset), SEEK_SET);
    
    /*
     * Read block size.
     */
    if (read(CurrentNEFile, &size_shift, sizeof(size_shift)) != 
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
	if (read(CurrentNEFile, &typeinfo, sizeof(typeinfo)) !=
	    sizeof(typeinfo))
	{
	    return -1;
	}
	if (typeinfo.type_id == type_id || type_id == -1)
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
		if (read(CurrentNEFile, &nameinfo, sizeof(nameinfo)) != 
		    sizeof(nameinfo))
		{
		    return -1;
		}

		if (nameinfo.id & 0x8000)
		    continue;
		
		old_pos = lseek(CurrentNEFile, 0, SEEK_CUR);
		new_pos = (CurrentMZHeader->ne_offset +
			   CurrentNEHeader->resource_tab_offset +
			   nameinfo.id);
		lseek(CurrentNEFile, new_pos, SEEK_SET);
		read(CurrentNEFile, &nbytes, 1);
		read(CurrentNEFile, name, nbytes);
		lseek(CurrentNEFile, old_pos, SEEK_SET);
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
 *					LoadString
 */
int
LoadString(HANDLE instance, WORD resource_id, LPSTR buffer, int buflen)
{
    struct resource_nameinfo_s nameinfo;
    unsigned short target_id;
    unsigned char string_length;
    int size_shift;
    int string_num;
    int i;

#ifdef DEBUG_RESOURCE
    printf("LoadString: instance = %04x, id = %d, "
	   "buffer = %08x, length = %d\n",
	   instance, resource_id, buffer, buflen);
#endif
    
    /*
     * Find string entry.
     */
    target_id = (resource_id >> 4) + 0x8001;
    string_num = resource_id & 0x000f;

    size_shift = FindResourceByNumber(&nameinfo, NE_RSCTYPE_STRING, target_id);
    if (size_shift == -1)
	return 0;
    
    lseek(CurrentNEFile, (int) nameinfo.offset << size_shift, SEEK_SET);

    for (i = 0; i < string_num; i++)
    {
	read(CurrentNEFile, &string_length, 1);
	lseek(CurrentNEFile, string_length, SEEK_CUR);
    }
			
    read(CurrentNEFile, &string_length, 1);
    i = MIN(string_length, buflen - 1);
    read(CurrentNEFile, buffer, i);
    buffer[i] = '\0';
#ifdef DEBUG_RESOURCE
    printf("            '%s'\n", buffer);
#endif
    return i;
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
    /*
     * Get resource by ordinal
     */
    else if (((int) rsc_name & 0xffff0000) == 0)
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
    lseek(CurrentNEFile, ((int) nameinfo.offset << size_shift), SEEK_SET);

    image_size = nameinfo.length << size_shift;
    if (image_size_ret != NULL)
	*image_size_ret = image_size;
    
    hmem = GlobalAlloc(GMEM_MOVEABLE, image_size);
    image = GlobalLock(hmem);
    if (image == NULL || read(CurrentNEFile, image, image_size) != image_size)
    {
	GlobalFree(hmem);
	return 0;
    }

    GlobalUnlock(hmem);
    return hmem;
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
