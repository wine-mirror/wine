static char RCSId[] = "$Id: resource.c,v 1.4 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "neexe.h"
#include "windows.h"

#define MIN(a,b)	((a) < (b) ? (a) : (b))

typedef struct resource_data_s
{
    int resource_type;
    void *resource_data;
} RSCD;

int ResourceSizes[16] =
{
    0, 0, sizeof(BITMAP), 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};

RSCD *Resources;
int ResourceArraySize;

/**********************************************************************
 *					ConvertCoreBitmap
 */
void *
ConvertCoreBitmap(BITMAPCOREHEADER *image, int image_size)
{
    BITMAP *new_image;
    char *old_p, *new_p;
    int old_line_length, new_line_length;
    unsigned int handle;
    int i;
    int n_colors;
    
    n_colors = 1 << image->bcBitCount;
    handle = GLOBAL_Alloc(GMEM_MOVEABLE, 
			  image_size + sizeof(*new_image) + n_colors);
    new_image = GLOBAL_Lock(handle);
    if (new_image == NULL)
	return NULL;

    new_image->bmType = 0;
    new_image->bmWidth = image->bcWidth;
    new_image->bmHeight = image->bcHeight;
    new_image->bmPlanes = image->bcPlanes;
    new_image->bmBitsPixel = image->bcBitCount;

    if (image->bcBitCount < 24)
    {
	RGBTRIPLE *old_color = (RGBTRIPLE *) (image + 1);
	RGBQUAD *new_color = (RGBQUAD *) (new_image + 1);
	for (i = 0; i < n_colors; i++)
	{
	    memcpy(new_color, old_color, sizeof(*old_color));
	    new_color++;
	    old_color++;
	}

	old_p = (char *) old_color;
	new_p = (char *) new_color;
	old_line_length = image->bcWidth / (8 / image->bcBitCount);
    }
    else
    {
	old_p = (char *) (image + 1);
	new_p = (char *) (new_image + 1);
	old_line_length = image->bcWidth * 3;
    }
    
    new_line_length = (old_line_length + 1) & ~1;
    old_line_length = (old_line_length + 3) & ~3;

    new_image->bmBits = (unsigned long) new_p;
    new_image->bmWidthBytes = new_line_length;

    for (i = 0; i < image->bcHeight; i++)
    {
	memcpy(new_p, old_p, new_line_length);
	new_p += new_line_length;
	old_p += old_line_length;
    }

    return new_image;
}

/**********************************************************************
 *					ConvertInfoBitmap
 */
void *
ConvertInfoBitmap(BITMAPINFOHEADER *image, int image_size)
{
}

/**********************************************************************
 *					AddResource
 */
int
AddResource(int type, void *data)
{
    RSCD *r;
    int i;
    int j;
    
    /*
     * Find free resource id.
     */
    r = Resources;
    for (i = 0; i < ResourceArraySize; i++, r++)
	if (r->resource_type == 0)
	    break;
    
    /*
     * Do we need to add more resource slots?
     */
    if (i == ResourceArraySize)
    {
	if (ResourceArraySize > 0)
	    r = realloc(Resources, (ResourceArraySize + 32) * sizeof(RSCD));
	else
	    r = malloc(32 * sizeof(RSCD));
	if (r == NULL)
	    return 0;

	for (j = ResourceArraySize; j < ResourceArraySize + 32; j++)
	    r[j].resource_type = 0;
	
	ResourceArraySize += 32;
	Resources = r;
	r = &Resources[i];
    }
    
    /*
     * Add new resource to list.
     */
    r->resource_type = type;
    r->resource_data = data;

    /*
     * Return a unique handle.
     */
    return i + 1;
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
 *					RSC_LoadString
 */
int
RSC_LoadString(int instance, int resource_id, char *buffer, int buflen)
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
 *					RSC_LoadResource
 */
int 
RSC_LoadResource(int instance, char *rsc_name, int type)
{
    struct resource_nameinfo_s nameinfo;
    void *image;
    void *rsc_image;
    long *lp;
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
	size_shift = -1;
    }
    if (size_shift == -1)
	return 0;

    /*
     * Read resource.
     */
    lseek(CurrentNEFile, ((int) nameinfo.offset << size_shift), SEEK_SET);

    image_size = nameinfo.length << size_shift;
    image = malloc(image_size);
    if (image == NULL || read(CurrentNEFile, image, image_size) != image_size)
    {
	free(image);
	return 0;
    }

    /*
     * Convert bitmap to internal format.
     */
    lp = (long *) image;
    if (*lp == sizeof(BITMAPCOREHEADER))
	rsc_image = ConvertCoreBitmap(image, image_size);
    else if (*lp == sizeof(BITMAPINFOHEADER))
	rsc_image = ConvertInfoBitmap(image, image_size);

    free(image);

    /*
     * Add to resource list.
     */
    if (rsc_image)
	return AddResource(type, rsc_image);
    else
	return 0;
}

/**********************************************************************
 *					RSC_LoadIcon
 */
int 
RSC_LoadIcon(int instance, char *icon_name)
{
#ifdef DEBUG_RESOURCE
    printf("LoadIcon: instance = %04x, name = %08x\n",
	   instance, icon_name);
#endif
    return RSC_LoadResource( instance, icon_name, NE_RSCTYPE_ICON);
}

/**********************************************************************
 *					RSC_LoadBitmap
 */
int 
RSC_LoadBitmap(int instance, char *bmp_name)
{
#ifdef DEBUG_RESOURCE
    printf("LoadBitmap: instance = %04x, name = %08x\n",
	   instance, bmp_name);
#endif
    return RSC_LoadResource( instance, bmp_name, NE_RSCTYPE_BITMAP);
}

/**********************************************************************
 *					RSC_LoadCursor
 */
int 
RSC_LoadCursor(int instance, char *cursor_name)
{
#ifdef DEBUG_RESOURCE
    printf("LoadCursor: instance = %04x, name = %08x\n",
	   instance, cursor_name);
#endif
    return RSC_LoadResource( instance, cursor_name, NE_RSCTYPE_CURSOR);
}

/**********************************************************************
 *					RSC_GetObject
 */
int
RSC_GetObject(int handle, int nbytes, void *buffer)
{
    if (handle > 0 && handle <= ResourceArraySize)
    {
	RSCD *r = &Resources[handle - 1];
	
	if (r->resource_type > 0)
	{
	    int n = MIN(nbytes, ResourceSizes[r->resource_type & 0xf]);
	    memcpy(buffer, r->resource_data, n);
	    return n;
	}
    }
    
    return 0;
}
