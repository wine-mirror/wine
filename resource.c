static char RCSId[] = "$Id: resource.c,v 1.3 1993/06/30 14:24:33 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "neexe.h"

#define MIN(a,b)	((a) < (b) ? (a) : (b))

typedef struct resource_data_s
{
    int resource_type;
    void *resource_data;
} RSCD;

RSCD *Resources;
int ResourceArraySize;

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
 *					RSC_LoadBitmap
 */
int 
RSC_LoadBitmap(int instance, char *bmp_name)
{
    struct resource_nameinfo_s nameinfo;
    void *image;
    int image_size;
    int size_shift;
    
#ifdef DEBUG_RESOURCE
    printf("LoadBitmap: instance = %04x, name = %08x\n",
	   instance, bmp_name);
#endif
    /*
     * Built-in bitmaps
     */
    if (instance == 0)
    {
	return 0;
    }
    /*
     * Get bitmap by ordinal
     */
    else if (((int) bmp_name & 0xffff0000) == 0)
    {
	size_shift = FindResourceByNumber(&nameinfo, NE_RSCTYPE_BITMAP,
					  (int) bmp_name | 0x8000);
    }
    /*
     * Get bitmap by name
     */
    else
    {
	size_shift = -1;
    }
    if (size_shift == -1)
	return 0;

    /*
     * Read bitmap.
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
     * Add to resource list.
     */
    return AddResource(NE_RSCTYPE_BITMAP, image);
}
