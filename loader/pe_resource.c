/*
 *	(c) 1994	Erik Bos	<erik@xs4all.nl>
 *
 *	based on Eric Youndale's pe-test and:
 *
 *	ftp.microsoft.com:/pub/developer/MSDN/CD8/PEFILE.ZIP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "windows.h"
#include "ldt.h"
#include "neexe.h"
#include "peexe.h"
#include "dlls.h"
#include "pe_image.h"
#include "resource.h"
#include "stddebug.h"
/* #define DEBUG_RESOURCE */
#include "debug.h"

#if 0

static int
find_lang(char *root, struct PE_Resource_Directory *resource, RESOURCE *r)
{
	struct PE_Directory_Entry *type_dir;
	struct PE_Resource_Leaf_Entry *leaf;

	type_dir = (struct PE_Directory_Entry *)(resource + 1);
	type_dir += resource->NumberOfNamedEntries;

	/* grab the 1st resource available */
	leaf = (struct PE_Resource_Leaf_Entry *) (root + type_dir->OffsetToData);
		dprintf_resource(stddeb, "\t\tPE_findlang: id %8x\n", (int) type_dir->Name);
		dprintf_resource(stddeb, "\t\taddress %ld, size %ld, language id %ld\n", leaf->OffsetToData, leaf->Size, leaf->CodePage);
	r->offset = leaf->OffsetToData - r->wpnt->pe->resource_offset;
	r->size = leaf->Size;
	printf("\t\toffset %d, size %d\n", r->offset, r->size);
	return 1;

/*	for(i=0; i< resource->NumberOfIdEntries; i++) {
		leaf = (root + (type_dir->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY));
		dprintf_resource(stddeb, "\t\tPE_findlang: id %8x\n", 
			(int) type_dir->Name);
		dprintf_resource(stddeb, "\t\t%x %x %x\n", leaf->OffsetToData, 
			leaf->Size, leaf->CodePage);
		type_dir++;
	} */
}

static int 
find_resource(char *root, struct PE_Resource_Directory *resource, 
		LPSTR resource_name, RESOURCE *r)
{
	int i;
	char res_name[256];
	struct PE_Directory_Entry *type_dir;
	struct PE_Directory_Name_String_U *name;
	
	type_dir = (struct PE_Directory_Entry *)(resource + 1);

	if (HIWORD((DWORD)resource_name)) {
		for(i=0; i< resource->NumberOfNamedEntries; i++) {
			name = (struct PE_Directory_Name_String_U *)(root + (type_dir->Name & ~IMAGE_RESOURCE_NAME_IS_STRING));
			memset(res_name, 0, sizeof(res_name));
			my_wcstombs(res_name, name->NameString, name->Length);
			dprintf_resource(stddeb, "\tPE_findresource: name %s\n", res_name);
			if (strcasecmp(res_name, resource_name) == 0) 
				return find_lang(root, (struct PE_Resource_Directory *) (root + (type_dir->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY)), r);
			type_dir++;
		}
	} else {
		type_dir += resource->NumberOfNamedEntries;
		for(i=0; i< resource->NumberOfIdEntries; i++) {
			dprintf_resource(stddeb, "\tPE_findresource: name %8x\n", (int) type_dir->Name);
			if (type_dir->Name == ((int) resource_name & 0xff))
				return find_lang(root, (struct PE_Resource_Directory *) (root + (type_dir->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY)), r);
			type_dir++;
		}
	}
	return 0;
}

static int 
find_type(struct PE_Resource_Directory *resource, LPSTR resource_name,
		LPSTR type_name)
{
	int i;
	char *root, res_name[256];
	struct PE_Directory_Entry *type_dir;
	struct PE_Directory_Name_String_U *name;
	
	root = (char *) resource;
	type_dir = (struct PE_Directory_Entry *)(resource + 1);

	if (HIWORD((DWORD)type_name)) {
		for(i=0; i< resource->NumberOfNamedEntries; i++) {
			name = (struct PE_Directory_Name_String_U *)(root + (type_dir->Name & ~IMAGE_RESOURCE_NAME_IS_STRING));
			memset(res_name, 0, sizeof(res_name));
			my_wcstombs(res_name, name->NameString, name->Length);
			dprintf_resource(stddeb, "PE_findtype: type %s\n", 
				res_name);
			if (strcasecmp(res_name, type_name) == 0) 
				return find_resource(root, (struct PE_Resource_Directory *) (root + (type_dir->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY)), resource_name, r);
			type_dir++;
		}
	} else {
		type_dir += resource->NumberOfNamedEntries;
		for(i=0; i< resource->NumberOfIdEntries; i++) {
			dprintf_resource(stddeb, "PE_findtype: type %8x\n", (int) type_dir->Name);
			if (type_dir->Name == ((int) type_name & 0xff))
				return find_resource(root, (struct PE_Resource_Directory *) (root + (type_dir->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY)), resource_name, r);
			type_dir++;
		}
	}
	return 0;
}

/**********************************************************************
 *			PE_FindResource	[KERNEL.60]
 */
int
PE_FindResource(HANDLE instance, SEGPTR resource_name, SEGPTR type_name,
		RESOURCE *r)
{
	dprintf_resource(stddeb, "PE_FindResource hInst=%04X typename=%08X resname=%08X\n", 
		instance, (int) type_name, (int) resource_name);
	if (HIWORD(resource_name))
        {
                char *resource_name_ptr = PTR_SEG_TO_LIN( resource_name );
		if (resource_name_ptr[0] == '#')
			resource_name = (SEGPTR) atoi(resource_name_ptr + 1);
                else
                    resource_name = (SEGPTR)resource_name_ptr;
        }
	if (HIWORD(type_name)) 
        {
                char *type_name_ptr = PTR_SEG_TO_LIN( type_name );
		if (type_name_ptr[0] == '#')
			type_name = (SEGPTR) atoi(type_name_ptr + 1);
                else
                        type_name = (SEGPTR) type_name_ptr;
        }
	return find_type(r->wpnt->pe->pe_resource, resource_name, type_name);
}
#endif
