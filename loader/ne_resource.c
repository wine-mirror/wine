static char RCSId[] = "$Id: ne_resource.c,v 1.4 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "windows.h"
#include "neexe.h"
#include "peexe.h"
#include "arch.h"
#include "dlls.h"
#include "resource.h"
#include "stddebug.h"
/* #define DEBUG_RESOURCE */
/* #undef  DEBUG_RESOURCE */
#include "debug.h"


static int ResourceFd = -1;
static HANDLE ResourceInst = 0;
static struct w_files *ResourceFileInfo;

/**********************************************************************
 *			RSC_LoadNameTable
 */
void RSC_LoadNameTable(void)
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
	     ResourceFileInfo->ne->ne_header->resource_tab_offset);
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
    ResourceFileInfo->ne->resnamtab = top;
}

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
    if (w->ne->resnamtab == (RESNAMTAB *) -1)
    {
	RSC_LoadNameTable();
    }
#endif
#endif

    dprintf_resource(stddeb, "OpenResourceFile(%04X) // file='%s' hFile=%04X !\n", 
		instance, w->filename, ResourceFd);
    return ResourceFd;
}

int load_typeinfo (int fd, struct resource_typeinfo_s *typeinfo)
{
    return read (fd, typeinfo, sizeof (*typeinfo)) == sizeof (*typeinfo);
}

int type_match(int type_id1, int type_id2, int fd, off_t off)
{
	off_t old_pos;
	unsigned char c;
	size_t nbytes;
	char name[256];

	if (type_id1 == -1)
		return 1;
	if ((type_id1 & 0xffff0000) == 0) {
		if ((type_id2 & 0x8000) == 0)
			return 0;
		return (type_id1 & 0x000f) == (type_id2 & 0x000f);
	}
	if ((type_id2 & 0x8000) != 0)
		return 0;
	dprintf_resource(stddeb, "type_compare: type_id2=%04X !\n", type_id2);
	old_pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, off + type_id2, SEEK_SET);
	read(fd, &c, 1);
	nbytes = CONV_CHAR_TO_LONG (c);
	dprintf_resource(stddeb, "type_compare: namesize=%d\n", nbytes);
	read(fd, name, nbytes);
	lseek(fd, old_pos, SEEK_SET);
	name[nbytes] = '\0';
	dprintf_resource(stddeb, "type_compare: name=`%s'\n", name);
	return strcasecmp((char *) type_id1, name) == 0;
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
	     ResourceFileInfo->ne->ne_header->resource_tab_offset);
    lseek(ResourceFd, rtoff, SEEK_SET);
    
    /*
     * Read block size.
     */
    if (read(ResourceFd, &size_shift, sizeof(size_shift)) != 
	sizeof(size_shift))
    {
    	printf("FindResourceByNumber (%d) bad block size !\n",(int) resource_id);
	return -1;
    }
    size_shift = CONV_SHORT(size_shift);
    /*
     * Find resource.
     */
    for (;;) {
	if (!load_typeinfo (ResourceFd, &typeinfo)){
	    printf("FindResourceByNumber (%X) bad typeinfo size !\n", resource_id);
	    return -1;
	    }
	dprintf_resource(stddeb, "FindResourceByNumber type=%X count=%d ?=%ld searched=%08X\n", 
		typeinfo.type_id, typeinfo.count, typeinfo.reserved, type_id);
	if (typeinfo.type_id == 0) break;
	if (type_match(type_id, typeinfo.type_id, ResourceFd, rtoff)) {

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
		dprintf_resource(stddeb, "FindResource: search type=%X id=%X // type=%X id=%X\n",
			type_id, resource_id, typeinfo.type_id, nameinfo.id);
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
    if (ResourceFileInfo->ne->resnamtab != NULL)
    {
	RESNAMTAB *e;

	for (e = ResourceFileInfo->ne->resnamtab; e != NULL; e = e->next)
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
	     ResourceFileInfo->ne->ne_header->resource_tab_offset);
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
    for (;;)
    {
	if (!load_typeinfo (ResourceFd, &typeinfo))
	{
	    printf("FindResourceByName (%s) bad typeinfo size !\n", resource_name);
	    return -1;
	}
	dprintf_resource(stddeb, "FindResourceByName typeinfo.type_id=%X count=%d type_id=%X\n",
			typeinfo.type_id, typeinfo.count, type_id);
	if (typeinfo.type_id == 0) break;
	if (type_match(type_id, typeinfo.type_id, ResourceFd, rtoff))
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
		dprintf_resource(stddeb, "FindResourceByName // nameinfo.id=%04X !\n", nameinfo.id);
		old_pos = lseek(ResourceFd, 0, SEEK_CUR);
		new_pos = rtoff + nameinfo.id;
		lseek(ResourceFd, new_pos, SEEK_SET);
		read(ResourceFd, &nbytes, 1);
		dprintf_resource(stddeb, "FindResourceByName // namesize=%d !\n", nbytes);
 		nbytes = CONV_CHAR_TO_LONG (nbytes);
		read(ResourceFd, name, nbytes);
		lseek(ResourceFd, old_pos, SEEK_SET);
		name[nbytes] = '\0';
		dprintf_resource(stddeb, "FindResourceByName type_id=%X (%d of %d) name='%s' resource_name='%s'\n", 
			typeinfo.type_id, i + 1, typeinfo.count, 
			name, resource_name);
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
 *					GetRsrcCount		[internal]
 */
int GetRsrcCount(HINSTANCE hInst, int type_id)
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short size_shift;
    off_t rtoff;

    if (hInst == 0) return 0;
    dprintf_resource(stddeb, "GetRsrcCount hInst=%04X typename=%08X\n", 
	hInst, type_id);
    if (OpenResourceFile(hInst) < 0)	return 0;

    /*
     * Move to beginning of resource table.
     */
    rtoff = (ResourceFileInfo->mz_header->ne_offset +
	     ResourceFileInfo->ne->ne_header->resource_tab_offset);
    lseek(ResourceFd, rtoff, SEEK_SET);
    /*
     * Read block size.
     */
    if (read(ResourceFd, &size_shift, sizeof(size_shift)) != sizeof(size_shift)) {
		printf("GetRsrcCount // bad block size !\n");
		return -1;
		}
    size_shift = CONV_SHORT (size_shift);
    for (;;) {
		if (!load_typeinfo (ResourceFd, &typeinfo))	{
			printf("GetRsrcCount // bad typeinfo size !\n");
			return 0;
			}
		dprintf_resource(stddeb, "GetRsrcCount // typeinfo.type_id=%X count=%d type_id=%X\n",
				typeinfo.type_id, typeinfo.count, type_id);
		if (typeinfo.type_id == 0) break;
		if (type_match(type_id, typeinfo.type_id, ResourceFd, rtoff)) {
			return typeinfo.count;
			}
		else {
			lseek(ResourceFd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
			}
		}
    return 0;
}

/**********************************************************************
 *			NE_FindResource	[KERNEL.60]
 */
int
NE_FindResource(HANDLE instance, LPSTR resource_name, LPSTR type_name,
		RESOURCE *r)
{
    int type;

    dprintf_resource(stddeb, "NE_FindResource hInst=%04X typename=%p resname=%p\n", 
			instance, type_name, resource_name);

    ResourceFd = r->fd;
    ResourceFileInfo = r->wpnt;

    /* nametable loaded ? */
    if (r->wpnt->ne->resnamtab == NULL)
	RSC_LoadNameTable();

    if (((int) type_name & 0xffff0000) == 0)
    {
	type = (int) type_name;
    }
    else if (type_name[0] == '\0')
    {
	type = -1;
    }
    else if (type_name[0] == '#')
    {
	type = atoi(type_name + 1);
    }
    else
    {
	type = (int) type_name;
    }
    if (((int) resource_name & 0xffff0000) == 0)
    {
	r->size_shift = FindResourceByNumber(&r->nameinfo, type,
					     (int) resource_name | 0x8000);
    }
    else if (resource_name[0] == '\0')
    {
	r->size_shift = FindResourceByNumber(&r->nameinfo, type, -1);
    }
    else if (resource_name[0] == '#')
    {
	r->size_shift = FindResourceByNumber(&r->nameinfo, type,
					     atoi(resource_name + 1));
    }
    else
    {
	r->size_shift = FindResourceByName(&r->nameinfo, type, resource_name);
    }

    if (r->size_shift == -1)
    {
        printf("NE_FindResource hInst=%04X typename=%08X resname=%08X not found!\n", 
		instance, (int) type_name, (int) resource_name);
	return 0;
    }
    r->size = r->nameinfo.length << r->size_shift;
    r->offset = r->nameinfo.offset << r->size_shift;
    return 1;
}
