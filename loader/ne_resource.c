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
#include "debug.h"

/**********************************************************************
 *			NE_LoadNameTable
 */
static void NE_LoadNameTable(struct w_files *wpnt)
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short             size_shift;
    RESNAMTAB                 *top, *new;
    char                       read_buf[1024];
    char                      *p;
    int                        i;
    unsigned short             len;
    off_t		       saved_pos;
    
    top = NULL;
    /*
     * Move to beginning of resource table.
     */
    lseek(wpnt->fd, wpnt->mz_header->ne_offset + 
	    	wpnt->ne->ne_header->resource_tab_offset, SEEK_SET);

    /*
     * Read block size.
     */
    if (read(wpnt->fd, &size_shift, sizeof(size_shift)) != sizeof(size_shift))
	return;

    size_shift = CONV_SHORT(size_shift);

    /*
     * Find resource.
     */
    typeinfo.type_id = 0xffff;
    while (typeinfo.type_id != 0) 
    {
	if (read(wpnt->fd, &typeinfo, sizeof(typeinfo)) != sizeof(typeinfo))
	    break;

	if (typeinfo.type_id == 0) 
	    break;

	if (typeinfo.type_id == 0x800f) 
	{
	    for (i = 0; i < typeinfo.count; i++) 
	    {
		if (read(wpnt->fd, &nameinfo, sizeof(nameinfo)) != sizeof(nameinfo))
		    break;
		
		saved_pos = lseek(wpnt->fd, 0, SEEK_CUR);
		lseek(wpnt->fd, (long) nameinfo.offset << size_shift, 
		      SEEK_SET);
		read(wpnt->fd, &len, sizeof(len));
		while (len)
		{
		    new = (RESNAMTAB *) GlobalQuickAlloc(sizeof(*new));
		    new->next = top;
		    top = new;

		    read(wpnt->fd, &new->type_ord, 2);
		    read(wpnt->fd, &new->id_ord, 2);
		    read(wpnt->fd, read_buf, len - 6);
		    
		    p = read_buf + strlen(read_buf) + 1;
		    strncpy(new->id, p, MAX_NAME_LENGTH);
		    new->id[MAX_NAME_LENGTH - 1] = '\0';

		    read(wpnt->fd, &len, sizeof(len));
		}
		lseek(wpnt->fd, saved_pos, SEEK_SET);
	    }
	} else 
		lseek(wpnt->fd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
    }
    wpnt->ne->resnamtab = top;
}

static int type_match(int type_id1, int type_id2, int fd, off_t off)
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
	nbytes = CONV_CHAR_TO_LONG(c);

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
static int FindResourceByNumber(RESOURCE *r, int type_id, int resource_id)
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short size_shift;
    int i;
    off_t rtoff;

    dprintf_resource(stddeb, "FindResourceByNumber: type_id =%x,m res_id = %x\n",
		type_id, resource_id);

    /* Move to beginning of resource table */
    rtoff = (r->wpnt->mz_header->ne_offset +
	     r->wpnt->ne->ne_header->resource_tab_offset);
    lseek(r->wpnt->fd, rtoff, SEEK_SET);

    /* Read block size */
    if (read(r->wpnt->fd, &size_shift, sizeof(size_shift)) != sizeof(size_shift)) {
    	printf("FindResourceByNumber (%d) bad block size !\n",(int) resource_id);
	return -1;
    }
    size_shift = CONV_SHORT(size_shift);

    /* Find resource */
    for (;;) {
	if (read(r->wpnt->fd, &typeinfo, sizeof(typeinfo)) != sizeof(typeinfo)) {
	    printf("FindResourceByNumber (%X) bad typeinfo size !\n", resource_id);
	    return -1;
	}
	dprintf_resource(stddeb, "FindResourceByNumber type=%X count=%d ?=%ld searched=%08X\n", 
		typeinfo.type_id, typeinfo.count, typeinfo.reserved, type_id);
	if (typeinfo.type_id == 0)
		break;
	if (type_match(type_id, typeinfo.type_id, r->wpnt->fd, rtoff)) {

	    for (i = 0; i < typeinfo.count; i++) {
#ifndef WINELIB
		if (read(r->wpnt->fd, &nameinfo, sizeof(nameinfo)) != sizeof(nameinfo))
#else
		if (!load_nameinfo(r->wpnt->fd, &nameinfo))
#endif
		{
		    printf("FindResourceByNumber (%X) bad nameinfo size !\n", resource_id);
		    return -1;
		}
		dprintf_resource(stddeb, "FindResource: search type=%X id=%X // type=%X id=%X\n",
			type_id, resource_id, typeinfo.type_id, nameinfo.id);
		if (nameinfo.id == resource_id) {
			r->size = nameinfo.length << size_shift;
			r->offset = nameinfo.offset << size_shift;
			return size_shift;
		    }
	        }
	    }
	else
	    lseek(r->wpnt->fd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
        }
    return -1;
}

/**********************************************************************
 *					FindResourceByName
 */
static int FindResourceByName(RESOURCE *r, int type_id, char *resource_name)
{
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short size_shift;
    off_t old_pos, new_pos;
    unsigned char nbytes;
    char name[256];
    int i;
    off_t rtoff;

    /* Check for loaded name table */
    if (r->wpnt->ne->resnamtab != NULL) {
	RESNAMTAB *e;

	for (e = r->wpnt->ne->resnamtab; e != NULL; e = e->next)
	    if (e->type_ord == (type_id & 0x000f) && 
	    	strcasecmp(e->id, resource_name) == 0)
	    {
		return FindResourceByNumber(r, type_id, e->id_ord);
	    }
	    return -1;
    }

    /* Move to beginning of resource table */
    rtoff = (r->wpnt->mz_header->ne_offset +
	     r->wpnt->ne->ne_header->resource_tab_offset);
    lseek(r->wpnt->fd, rtoff, SEEK_SET);

    /* Read block size */
    if (read(r->wpnt->fd, &size_shift, sizeof(size_shift)) != sizeof(size_shift))
    {
    	printf("FindResourceByName (%s) bad block size !\n", resource_name);
	return -1;
    }
    size_shift = CONV_SHORT (size_shift);
    
    /* Find resource */
    for (;;)
    {
	if (read(r->wpnt->fd, &typeinfo, sizeof(typeinfo)) != sizeof(typeinfo)) {
	    printf("FindResourceByName (%s) bad typeinfo size !\n", resource_name);
	    return -1;
	}
	dprintf_resource(stddeb, "FindResourceByName typeinfo.type_id=%X count=%d type_id=%X\n",
			typeinfo.type_id, typeinfo.count, type_id);
	if (typeinfo.type_id == 0)
		break;
	if (type_match(type_id, typeinfo.type_id, r->wpnt->fd, rtoff))
	{
	    for (i = 0; i < typeinfo.count; i++)
	    {
#ifndef WINELIB
		if (read(r->wpnt->fd, &nameinfo, sizeof(nameinfo)) != sizeof(nameinfo))
#else
		if (!load_nameinfo (r->wpnt->fd, &nameinfo))
#endif
		{
		    printf("FindResourceByName (%s) bad nameinfo size !\n", resource_name);
		    return -1;
		}
/*
		if ((nameinfo.id & 0x8000) != 0) continue;
*/		
		dprintf_resource(stddeb, "FindResourceByName // nameinfo.id=%04X !\n", nameinfo.id);
		old_pos = lseek(r->wpnt->fd, 0, SEEK_CUR);
		new_pos = rtoff + nameinfo.id;
		lseek(r->wpnt->fd, new_pos, SEEK_SET);
		read(r->wpnt->fd, &nbytes, 1);
		dprintf_resource(stddeb, "FindResourceByName // namesize=%d !\n", nbytes);
 		nbytes = CONV_CHAR_TO_LONG (nbytes);
		read(r->wpnt->fd, name, nbytes);
		lseek(r->wpnt->fd, old_pos, SEEK_SET);
		name[nbytes] = '\0';
		dprintf_resource(stddeb, "FindResourceByName type_id=%X (%d of %d) name='%s' resource_name='%s'\n", 
			typeinfo.type_id, i + 1, typeinfo.count, 
			name, resource_name);
		if (strcasecmp(name, resource_name) == 0) {
			r->size = nameinfo.length << size_shift;
			r->offset = nameinfo.offset << size_shift;
			return size_shift;
		}
	    }
	}
	else
	    lseek(r->wpnt->fd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
    }
    return -1;
}


/**********************************************************************
 *					GetRsrcCount		[internal]
 */
int GetRsrcCount(HINSTANCE hInst, int type_id)
{
    struct w_files *wpnt;
    struct resource_typeinfo_s typeinfo;
    struct resource_nameinfo_s nameinfo;
    unsigned short size_shift;
    off_t rtoff;

    if (hInst == 0)
    	return 0;
    dprintf_resource(stddeb, "GetRsrcCount hInst=%04X typename=%08X\n", 
	hInst, type_id);

    if ((wpnt = GetFileInfo(hInst)) == NULL)
    	return 0;
    /*
     * Move to beginning of resource table.
     */
    rtoff = (wpnt->mz_header->ne_offset +
    		wpnt->ne->ne_header->resource_tab_offset);
    lseek(wpnt->fd, rtoff, SEEK_SET);
    /*
     * Read block size.
     */
    if (read(wpnt->fd, &size_shift, sizeof(size_shift)) != sizeof(size_shift)) {
		printf("GetRsrcCount // bad block size !\n");
		return -1;
    }
    size_shift = CONV_SHORT (size_shift);
    for (;;) {
	if (read(wpnt->fd, &typeinfo, sizeof(typeinfo)) != sizeof(typeinfo)) {
		printf("GetRsrcCount // bad typeinfo size !\n");
		return 0;
	}
	dprintf_resource(stddeb, "GetRsrcCount // typeinfo.type_id=%X count=%d type_id=%X\n",
				typeinfo.type_id, typeinfo.count, type_id);
	if (typeinfo.type_id == 0)
		break;
	if (type_match(type_id, typeinfo.type_id, wpnt->fd, rtoff))
		return typeinfo.count;
	else
		lseek(wpnt->fd, (typeinfo.count * sizeof(nameinfo)), SEEK_CUR);
    }
    return 0;
}

/**********************************************************************
 *			NE_FindResource	[KERNEL.60]
 */
int NE_FindResource(HANDLE instance, LPSTR resource_name, LPSTR type_name,
		RESOURCE *r)
{
    int type, x;

    dprintf_resource(stddeb, "NE_FindResource hInst=%04X typename=%p resname=%p\n", 
			instance, type_name, resource_name);

    r->size = r->offset = 0;

    /* nametable loaded ? */
    if (r->wpnt->ne->resnamtab == NULL)
	NE_LoadNameTable(r->wpnt);

    if (((int) type_name & 0xffff0000) == 0)
	type = (int) type_name;
    else {
    	if (type_name[0] == '\0')
		type = -1;
    	if (type_name[0] == '#')
		type = atoi(type_name + 1);
	    else
    		type = (int) type_name;
    }

    if (((int) resource_name & 0xffff0000) == 0)
	x = FindResourceByNumber(r, type, (int) resource_name | 0x8000);
    else {
	if (resource_name[0] == '\0')
		x = FindResourceByNumber(r, type, -1);
	if (resource_name[0] == '#')
		x = FindResourceByNumber(r, type, atoi(resource_name + 1));
	else
		x = FindResourceByName(r, type, resource_name);
    }
    if (x == -1) {
        printf("NE_FindResource hInst=%04X typename=%08X resname=%08X not found!\n", 
		instance, (int) type_name, (int) resource_name);
	return 0;
    }
    return 1;
}
