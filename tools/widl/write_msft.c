/*
 *      Typelib v2 (MSFT) generation
 *
 *	Copyright 2004  Alastair Bridgewater
 *                2004, 2005 Huw Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * --------------------------------------------------------------------------------------
 *  Known problems:
 *
 *    Badly incomplete.
 *
 *    Only works on little-endian systems.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/unicode.h"

#include "widltypes.h"
#include "typelib.h"
#include "typelib_struct.h"
#include "utils.h"
#include "hash.h"

enum MSFT_segment_index {
    MSFT_SEG_TYPEINFO = 0,  /* type information */
    MSFT_SEG_IMPORTINFO,    /* import information */
    MSFT_SEG_IMPORTFILES,   /* import filenames */
    MSFT_SEG_REFERENCES,    /* references (?) */
    MSFT_SEG_GUIDHASH,      /* hash table for guids? */
    MSFT_SEG_GUID,          /* guid storage */
    MSFT_SEG_NAMEHASH,      /* hash table for names */
    MSFT_SEG_NAME,          /* name storage */
    MSFT_SEG_STRING,        /* string storage */
    MSFT_SEG_TYPEDESC,      /* type descriptions */
    MSFT_SEG_ARRAYDESC,     /* array descriptions */
    MSFT_SEG_CUSTDATA,      /* custom data */
    MSFT_SEG_CUSTDATAGUID,  /* custom data guids */
    MSFT_SEG_UNKNOWN,       /* ??? */
    MSFT_SEG_UNKNOWN2,      /* ??? */
    MSFT_SEG_MAX            /* total number of segments */
};

typedef struct tagMSFT_ImpFile {
    int guid;
    LCID lcid;
    int version;
    char filename[0]; /* preceeded by two bytes of encoded (length << 2) + flags in the low two bits. */
} MSFT_ImpFile;

typedef struct _msft_typelib_t
{
    typelib_t *typelib;
    MSFT_Header typelib_header;
    MSFT_pSeg typelib_segdir[MSFT_SEG_MAX];
    char *typelib_segment_data[MSFT_SEG_MAX];
    int typelib_segment_block_length[MSFT_SEG_MAX];

    INT typelib_typeinfo_offsets[0x200]; /* Hope that's enough. */

    INT *typelib_namehash_segment;
    INT *typelib_guidhash_segment;

    INT help_string_dll_offset;

    struct _msft_typeinfo_t *typeinfos;
    struct _msft_typeinfo_t *last_typeinfo;
} msft_typelib_t;

typedef struct _msft_typeinfo_t
{
    msft_typelib_t *typelib;
    MSFT_TypeInfoBase *typeinfo;

    INT *typedata;
    int typedata_allocated;
    int typedata_length;

    int indices[42];
    int names[42];
    int offsets[42];

    int datawidth;

    struct _msft_typeinfo_t *next_typeinfo;
} msft_typeinfo_t;



/*================== Internal functions ===================================*/

/****************************************************************************
 *	ctl2_init_header
 *
 *  Initializes the type library header of a new typelib.
 */
static void ctl2_init_header(
	msft_typelib_t *typelib) /* [I] The typelib to initialize. */
{
    typelib->typelib_header.magic1 = 0x5446534d;
    typelib->typelib_header.magic2 = 0x00010002;
    typelib->typelib_header.posguid = -1;
    typelib->typelib_header.lcid = 0x0409; /* or do we use the current one? */
    typelib->typelib_header.lcid2 = 0x0;
    typelib->typelib_header.varflags = 0x40;
    typelib->typelib_header.version = 0;
    typelib->typelib_header.flags = 0;
    typelib->typelib_header.nrtypeinfos = 0;
    typelib->typelib_header.helpstring = -1;
    typelib->typelib_header.helpstringcontext = 0;
    typelib->typelib_header.helpcontext = 0;
    typelib->typelib_header.nametablecount = 0;
    typelib->typelib_header.nametablechars = 0;
    typelib->typelib_header.NameOffset = -1;
    typelib->typelib_header.helpfile = -1;
    typelib->typelib_header.CustomDataOffset = -1;
    typelib->typelib_header.res44 = 0x20;
    typelib->typelib_header.res48 = 0x80;
    typelib->typelib_header.dispatchpos = -1;
    typelib->typelib_header.res50 = 0;
}

/****************************************************************************
 *	ctl2_init_segdir
 *
 *  Initializes the segment directory of a new typelib.
 */
static void ctl2_init_segdir(
	msft_typelib_t *typelib) /* [I] The typelib to initialize. */
{
    int i;
    MSFT_pSeg *segdir;

    segdir = &typelib->typelib_segdir[MSFT_SEG_TYPEINFO];

    for (i = 0; i < 15; i++) {
	segdir[i].offset = -1;
	segdir[i].length = 0;
	segdir[i].res08 = -1;
	segdir[i].res0c = 0x0f;
    }
}

/****************************************************************************
 *	ctl2_hash_guid
 *
 *  Generates a hash key from a GUID.
 *
 * RETURNS
 *
 *  The hash key for the GUID.
 */
static int ctl2_hash_guid(
	REFGUID guid)                /* [I] The guid to hash. */
{
    int hash;
    int i;

    hash = 0;
    for (i = 0; i < 8; i ++) {
	hash ^= ((const short *)guid)[i];
    }

    return hash & 0x1f;
}

/****************************************************************************
 *	ctl2_find_guid
 *
 *  Locates a guid in a type library.
 *
 * RETURNS
 *
 *  The offset into the GUID segment of the guid, or -1 if not found.
 */
static int ctl2_find_guid(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against. */
	int hash_key,              /* [I] The hash key for the guid. */
	REFGUID guid)              /* [I] The guid to find. */
{
    int offset;
    MSFT_GuidEntry *guidentry;

    offset = typelib->typelib_guidhash_segment[hash_key];
    while (offset != -1) {
	guidentry = (MSFT_GuidEntry *)&typelib->typelib_segment_data[MSFT_SEG_GUID][offset];

	if (!memcmp(guidentry, guid, sizeof(GUID))) return offset;

	offset = guidentry->next_hash;
    }

    return offset;
}

/****************************************************************************
 *	ctl2_find_name
 *
 *  Locates a name in a type library.
 *
 * RETURNS
 *
 *  The offset into the NAME segment of the name, or -1 if not found.
 *
 * NOTES
 *
 *  The name must be encoded as with ctl2_encode_name().
 */
static int ctl2_find_name(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against. */
	char *name)                /* [I] The encoded name to find. */
{
    int offset;
    int *namestruct;

    offset = typelib->typelib_namehash_segment[name[2] & 0x7f];
    while (offset != -1) {
	namestruct = (int *)&typelib->typelib_segment_data[MSFT_SEG_NAME][offset];

	if (!((namestruct[2] ^ *((int *)name)) & 0xffff00ff)) {
	    /* hash codes and lengths match, final test */
	    if (!strncasecmp(name+4, (void *)(namestruct+3), name[0])) break;
	}

	/* move to next item in hash bucket */
	offset = namestruct[1];
    }

    return offset;
}

/****************************************************************************
 *	ctl2_encode_name
 *
 *  Encodes a name string to a form suitable for storing into a type library
 *  or comparing to a name stored in a type library.
 *
 * RETURNS
 *
 *  The length of the encoded name, including padding and length+hash fields.
 *
 * NOTES
 *
 *  Will throw an exception if name or result are NULL. Is not multithread
 *  safe in the slightest.
 */
static int ctl2_encode_name(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against (used for LCID only). */
	const char *name,          /* [I] The name string to encode. */
	char **result)             /* [O] A pointer to a pointer to receive the encoded name. */
{
    int length;
    static char converted_name[0x104];
    int offset;
    int value;

    length = strlen(name);
    memcpy(converted_name + 4, name, length);
    converted_name[0] = length & 0xff;

    converted_name[length + 4] = 0;

    converted_name[1] = 0x00;

    value = lhash_val_of_name_sys(typelib->typelib_header.varflags & 0x0f, typelib->typelib_header.lcid, converted_name + 4);

    converted_name[2] = value;
    converted_name[3] = value >> 8;

    for (offset = (4 - length) & 3; offset; offset--) converted_name[length + offset + 3] = 0x57;

    *result = converted_name;

    return (length + 7) & ~3;
}

/****************************************************************************
 *	ctl2_encode_string
 *
 *  Encodes a string to a form suitable for storing into a type library or
 *  comparing to a string stored in a type library.
 *
 * RETURNS
 *
 *  The length of the encoded string, including padding and length fields.
 *
 * NOTES
 *
 *  Will throw an exception if string or result are NULL. Is not multithread
 *  safe in the slightest.
 */
static int ctl2_encode_string(
	msft_typelib_t *typelib,   /* [I] The typelib to operate against (not used?). */
	const char *string,        /* [I] The string to encode. */
	char **result)             /* [O] A pointer to a pointer to receive the encoded string. */
{
    int length;
    static char converted_string[0x104];
    int offset;

    length = strlen(string);
    memcpy(converted_string + 2, string, length);
    converted_string[0] = length & 0xff;
    converted_string[1] = (length >> 8) & 0xff;

    if(length < 3) { /* strings of this length are padded with upto 8 bytes incl the 2 byte length */
        for(offset = 0; offset < 4; offset++)
            converted_string[length + offset + 2] = 0x57;
        length += 4;
    }
    for (offset = (4 - (length + 2)) & 3; offset; offset--) converted_string[length + offset + 1] = 0x57;

    *result = converted_string;

    return (length + 5) & ~3;
}

/****************************************************************************
 *	ctl2_alloc_segment
 *
 *  Allocates memory from a segment in a type library.
 *
 * RETURNS
 *
 *  Success: The offset within the segment of the new data area.
 *  Failure: -1 (this is invariably an out of memory condition).
 *
 * BUGS
 *
 *  Does not (yet) handle the case where the allocated segment memory needs to grow.
 */
static int ctl2_alloc_segment(
	msft_typelib_t *typelib,         /* [I] The type library in which to allocate. */
	enum MSFT_segment_index segment, /* [I] The segment in which to allocate. */
	int size,                        /* [I] The amount to allocate. */
	int block_size)                  /* [I] Initial allocation block size, or 0 for default. */
{
    int offset;

    if(!typelib->typelib_segment_data[segment]) {
	if (!block_size) block_size = 0x2000;

	typelib->typelib_segment_block_length[segment] = block_size;
	typelib->typelib_segment_data[segment] = xmalloc(block_size);
	if (!typelib->typelib_segment_data[segment]) return -1;
	memset(typelib->typelib_segment_data[segment], 0x57, block_size);
    }

    while ((typelib->typelib_segdir[segment].length + size) > typelib->typelib_segment_block_length[segment]) {
	char *block;

	block_size = typelib->typelib_segment_block_length[segment];
	block = realloc(typelib->typelib_segment_data[segment], block_size << 1);
	if (!block) return -1;

	if (segment == MSFT_SEG_TYPEINFO) {
	    /* TypeInfos have a direct pointer to their memory space, so we have to fix them up. */
	    msft_typeinfo_t *typeinfo;

	    for (typeinfo = typelib->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
		typeinfo->typeinfo = (void *)&block[((char *)typeinfo->typeinfo) - typelib->typelib_segment_data[segment]];
	    }
	}

	memset(block + block_size, 0x57, block_size);
	typelib->typelib_segment_block_length[segment] = block_size << 1;
	typelib->typelib_segment_data[segment] = block;
    }

    offset = typelib->typelib_segdir[segment].length;
    typelib->typelib_segdir[segment].length += size;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_typeinfo
 *
 *  Allocates and initializes a typeinfo structure in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new typeinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_typeinfo(
	msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
	int nameoffset)            /* [I] The offset of the name for this typeinfo. */
{
    int offset;
    MSFT_TypeInfoBase *typeinfo;

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEINFO, sizeof(MSFT_TypeInfoBase), 0);
    if (offset == -1) return -1;

    typelib->typelib_typeinfo_offsets[typelib->typelib_header.nrtypeinfos++] = offset;

    typeinfo = (void *)(typelib->typelib_segment_data[MSFT_SEG_TYPEINFO] + offset);

    typeinfo->typekind = (typelib->typelib_header.nrtypeinfos - 1) << 16;
    typeinfo->memoffset = -1; /* should be EOF if no elements */
    typeinfo->res2 = 0;
    typeinfo->res3 = -1;
    typeinfo->res4 = 3;
    typeinfo->res5 = 0;
    typeinfo->cElement = 0;
    typeinfo->res7 = 0;
    typeinfo->res8 = 0;
    typeinfo->res9 = 0;
    typeinfo->resA = 0;
    typeinfo->posguid = -1;
    typeinfo->flags = 0;
    typeinfo->NameOffset = nameoffset;
    typeinfo->version = 0;
    typeinfo->docstringoffs = -1;
    typeinfo->helpstringcontext = 0;
    typeinfo->helpcontext = 0;
    typeinfo->oCustData = -1;
    typeinfo->cbSizeVft = 0;
    typeinfo->cImplTypes = 0;
    typeinfo->size = 0;
    typeinfo->datatype1 = -1;
    typeinfo->datatype2 = 0;
    typeinfo->res18 = 0;
    typeinfo->res19 = -1;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_guid
 *
 *  Allocates and initializes a GUID structure in a type library. Also updates
 *  the GUID hash table as needed.
 *
 * RETURNS
 *
 *  Success: The offset of the new GUID.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_guid(
	msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
	MSFT_GuidEntry *guid)      /* [I] The GUID to store. */
{
    int offset;
    MSFT_GuidEntry *guid_space;
    int hash_key;

    hash_key = ctl2_hash_guid(&guid->guid);

    offset = ctl2_find_guid(typelib, hash_key, &guid->guid);
    if (offset != -1) return offset;

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_GUID, sizeof(MSFT_GuidEntry), 0);
    if (offset == -1) return -1;

    guid_space = (void *)(typelib->typelib_segment_data[MSFT_SEG_GUID] + offset);
    *guid_space = *guid;

    guid_space->next_hash = typelib->typelib_guidhash_segment[hash_key];
    typelib->typelib_guidhash_segment[hash_key] = offset;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_name
 *
 *  Allocates and initializes a name within a type library. Also updates the
 *  name hash table as needed.
 *
 * RETURNS
 *
 *  Success: The offset within the segment of the new name.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_name(
	msft_typelib_t *typelib,  /* [I] The type library to allocate in. */
	const char *name)         /* [I] The name to store. */
{
    int length;
    int offset;
    MSFT_NameIntro *name_space;
    char *encoded_name;

    length = ctl2_encode_name(typelib, name, &encoded_name);

    offset = ctl2_find_name(typelib, encoded_name);
    if (offset != -1) return offset;

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_NAME, length + 8, 0);
    if (offset == -1) return -1;

    name_space = (void *)(typelib->typelib_segment_data[MSFT_SEG_NAME] + offset);
    name_space->hreftype = -1;
    name_space->next_hash = -1;
    memcpy(&name_space->namelen, encoded_name, length);

    if (typelib->typelib_namehash_segment[encoded_name[2] & 0x7f] != -1)
	name_space->next_hash = typelib->typelib_namehash_segment[encoded_name[2] & 0x7f];

    typelib->typelib_namehash_segment[encoded_name[2] & 0x7f] = offset;

    typelib->typelib_header.nametablecount += 1;
    typelib->typelib_header.nametablechars += *encoded_name;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_string
 *
 *  Allocates and initializes a string in a type library.
 *
 * RETURNS
 *
 *  Success: The offset within the segment of the new string.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_string(
	msft_typelib_t *typelib,  /* [I] The type library to allocate in. */
	const char *string)       /* [I] The string to store. */
{
    int length;
    int offset;
    char *string_space;
    char *encoded_string;

    length = ctl2_encode_string(typelib, string, &encoded_string);

    for (offset = 0; offset < typelib->typelib_segdir[MSFT_SEG_STRING].length;
	 offset += ((((typelib->typelib_segment_data[MSFT_SEG_STRING][offset + 1] << 8) & 0xff)
	     | (typelib->typelib_segment_data[MSFT_SEG_STRING][offset + 0] & 0xff)) + 5) & ~3) {
	if (!memcmp(encoded_string, typelib->typelib_segment_data[MSFT_SEG_STRING] + offset, length)) return offset;
    }

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_STRING, length, 0);
    if (offset == -1) return -1;

    string_space = typelib->typelib_segment_data[MSFT_SEG_STRING] + offset;
    memcpy(string_space, encoded_string, length);

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_importinfo
 *
 *  Allocates and initializes an import information structure in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new importinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_importinfo(
        msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
	MSFT_ImpInfo *impinfo)     /* [I] The import information to store. */
{
    int offset;
    MSFT_ImpInfo *impinfo_space;

    for (offset = 0;
	 offset < typelib->typelib_segdir[MSFT_SEG_IMPORTINFO].length;
	 offset += sizeof(MSFT_ImpInfo)) {
	if (!memcmp(&(typelib->typelib_segment_data[MSFT_SEG_IMPORTINFO][offset]),
		    impinfo, sizeof(MSFT_ImpInfo))) {
	    return offset;
	}
    }

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_IMPORTINFO, sizeof(MSFT_ImpInfo), 0);
    if (offset == -1) return -1;

    impinfo_space = (void *)(typelib->typelib_segment_data[MSFT_SEG_IMPORTINFO] + offset);
    *impinfo_space = *impinfo;

    return offset;
}

/****************************************************************************
 *	ctl2_alloc_importfile
 *
 *  Allocates and initializes an import file definition in a type library.
 *
 * RETURNS
 *
 *  Success: The offset of the new importinfo.
 *  Failure: -1 (this is invariably an out of memory condition).
 */
static int ctl2_alloc_importfile(
	msft_typelib_t *typelib,   /* [I] The type library to allocate in. */
	int guidoffset,            /* [I] The offset to the GUID for the imported library. */
	int major_version,         /* [I] The major version number of the imported library. */
	int minor_version,         /* [I] The minor version number of the imported library. */
	const char *filename)      /* [I] The filename of the imported library. */
{
    int length;
    int offset;
    MSFT_ImpFile *importfile;
    char *encoded_string;

    length = ctl2_encode_string(typelib, filename, &encoded_string);

    encoded_string[0] <<= 2;
    encoded_string[0] |= 1;

    for (offset = 0; offset < typelib->typelib_segdir[MSFT_SEG_IMPORTFILES].length;
	 offset += ((((typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset + 0xd] << 8) & 0xff)
	     | (typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset + 0xc] & 0xff)) >> 2) + 0xc) {
	if (!memcmp(encoded_string, typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES] + offset + 0xc, length)) return offset;
    }

    offset = ctl2_alloc_segment(typelib, MSFT_SEG_IMPORTFILES, length + 0xc, 0);
    if (offset == -1) return -1;

    importfile = (MSFT_ImpFile *)&typelib->typelib_segment_data[MSFT_SEG_IMPORTFILES][offset];
    importfile->guid = guidoffset;
    importfile->lcid = typelib->typelib_header.lcid2;
    importfile->version = major_version | (minor_version << 16);
    memcpy(&importfile->filename, encoded_string, length);

    return offset;
}


/****************************************************************************
 *	encode_type
 *
 *  Encodes a type, storing information in the TYPEDESC and ARRAYDESC
 *  segments as needed.
 *
 * RETURNS
 *
 *  Success: 0.
 *  Failure: -1.
 */
static int encode_type(
	msft_typelib_t *typelib,   /* [I] The type library in which to encode the TYPEDESC. */
        int vt,                    /* [I] vt to encode */
	type_t *type,              /* [I] type */
	int *encoded_type,         /* [O] The encoded type description. */
	int *width,                /* [O] The width of the type, or NULL. */
	int *alignment,            /* [O] The alignment of the type, or NULL. */
	int *decoded_size)         /* [O] The total size of the unencoded TYPEDESCs, including nested descs. */
{
    int default_type;
    int scratch;
    int typeoffset;
    int *typedata;
    int target_type;
    int child_size;

    chat("encode_type vt %d type %p\n", vt, type);

    default_type = 0x80000000 | (vt << 16) | vt;
    if (!width) width = &scratch;
    if (!alignment) alignment = &scratch;
    if (!decoded_size) decoded_size = &scratch;


    switch (vt) {
    case VT_I1:
    case VT_UI1:
	*encoded_type = default_type;
	*width = 1;
	*alignment = 1;
	break;

    case VT_INT:
	*encoded_type = 0x80000000 | (VT_I4 << 16) | VT_INT;
	if ((typelib->typelib_header.varflags & 0x0f) == SYS_WIN16) {
	    *width = 2;
	    *alignment = 2;
	} else {
	    *width = 4;
	    *alignment = 4;
	}
	break;

    case VT_UINT:
	*encoded_type = 0x80000000 | (VT_UI4 << 16) | VT_UINT;
	if ((typelib->typelib_header.varflags & 0x0f) == SYS_WIN16) {
	    *width = 2;
	    *alignment = 2;
	} else {
	    *width = 4;
	    *alignment = 4;
	}
	break;

    case VT_UI2:
    case VT_I2:
    case VT_BOOL:
	*encoded_type = default_type;
	*width = 2;
	*alignment = 2;
	break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_BSTR:
    case VT_HRESULT:
	*encoded_type = default_type;
	*width = 4;
	*alignment = 4;
	break;

    case VT_CY:
	*encoded_type = default_type;
	*width = 8;
	*alignment = 4; /* guess? */
	break;

    case VT_VOID:
	*encoded_type = 0x80000000 | (VT_EMPTY << 16) | vt;
	*width = 0;
	*alignment = 1;
	break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        *encoded_type = default_type;
        *width = 4;
        *alignment = 4;
        break;

    case VT_VARIANT:
        *encoded_type = default_type;
        break;

    case VT_PTR:
      {
        int next_vt;
        while((next_vt = get_type_vt(type->ref)) == 0) {
            if(type->ref == NULL) {
                next_vt = VT_VOID;
                break;
            }
            type = type->ref;
        }

	encode_type(typelib, next_vt, type->ref, &target_type, NULL, NULL, &child_size);
        if(type->ref->type == RPC_FC_IP) {
            chat("encode_type: skipping ptr\n");
            *encoded_type = target_type;
            *width = 4;
            *alignment = 4;
            *decoded_size = child_size;
            break;
        }

	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_PTR) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & 0x3fff) | VT_BYREF;
	    } else {
		typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_PTR;
	    typedata[1] = target_type;
	}

	*encoded_type = typeoffset;

	*width = 4;
	*alignment = 4;
	*decoded_size = 8 /*sizeof(TYPEDESC)*/ + child_size;
        break;
    }
#if 0


    case VT_SAFEARRAY:
	/* FIXME: Make with the error checking. */
	FIXME("SAFEARRAY vartype, may not work correctly.\n");

	ctl2_encode_typedesc(typelib, tdesc->u.lptdesc, &target_type, NULL, NULL, &child_size);

	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_SAFEARRAY) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & VT_TYPEMASK) | VT_ARRAY;
	    } else {
		typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_SAFEARRAY;
	    typedata[1] = target_type;
	}

	*encoded_tdesc = typeoffset;

	*width = 4;
	*alignment = 4;
	*decoded_size = sizeof(TYPEDESC) + child_size;
	break;


#endif

    case VT_USERDEFINED:
      {
        int typeinfo_offset;
        chat("encode_type: VT_USERDEFINED - type %p name = %s idx %d\n", type, type->name, type->typelib_idx);

        if(type->typelib_idx == -1)
            error("encode_type: trying to ref not added type\n");

        typeinfo_offset = typelib->typelib_typeinfo_offsets[type->typelib_idx];
	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if ((typedata[0] == ((0x7fff << 16) | VT_USERDEFINED)) && (typedata[1] == typeinfo_offset)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (0x7fff << 16) | VT_USERDEFINED;
	    typedata[1] = typeinfo_offset;
	}

	*encoded_type = typeoffset;
	*width = 0;
	*alignment = 1;

        if(type->type == RPC_FC_IP) {
            typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (0x7fff << 16) | VT_PTR;
	    typedata[1] = *encoded_type;
            *encoded_type = typeoffset;
            *width = 4;
            *alignment = 4;
            *decoded_size += 8;
        }            
	break;
      }

    default:
	error("encode_type: unrecognized type %d.\n", vt);
	*encoded_type = default_type;
	*width = 0;
	*alignment = 1;
	break;
    }

    return 0;
}

static void dump_type(type_t *t)
{
    chat("dump_type: %p name %s type %d ref %p rname %s\n", t, t->name, t->type, t->ref, t->rname);
    if(t->ref) dump_type(t->ref);
}

static int encode_var(
	msft_typelib_t *typelib,   /* [I] The type library in which to encode the TYPEDESC. */
	var_t *var,                /* [I] The type description to encode. */
	int *encoded_type,         /* [O] The encoded type description. */
	int *width,                /* [O] The width of the type, or NULL. */
	int *alignment,            /* [O] The alignment of the type, or NULL. */
	int *decoded_size)         /* [O] The total size of the unencoded TYPEDESCs, including nested descs. */
{
    int typeoffset;
    int *typedata;
    int target_type;
    int child_size;
    int vt;
    int scratch;
    type_t *type;

    if (!width) width = &scratch;
    if (!alignment) alignment = &scratch;
    if (!decoded_size) decoded_size = &scratch;
    *decoded_size = 0;

    chat("encode_var: var %p var->tname %s var->type %p var->ptr_level %d var->type->ref %p\n", var, var->tname, var->type, var->ptr_level, var->type->ref);
    if(var->ptr_level) {
        int skip_ptr;
        var->ptr_level--;
	skip_ptr = encode_var(typelib, var, &target_type, NULL, NULL, &child_size);
        var->ptr_level++;

        if(skip_ptr == 2) {
            chat("encode_var: skipping ptr\n");
            *encoded_type = target_type;
            *decoded_size = child_size;
            *width = 4;
            *alignment = 4;
            return 0;
        }

	for (typeoffset = 0; typeoffset < typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length; typeoffset += 8) {
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];
	    if (((typedata[0] & 0xffff) == VT_PTR) && (typedata[1] == target_type)) break;
	}

	if (typeoffset == typelib->typelib_segdir[MSFT_SEG_TYPEDESC].length) {
	    int mix_field;
	    
	    if (target_type & 0x80000000) {
		mix_field = ((target_type >> 16) & 0x3fff) | VT_BYREF;
	    } else {
		typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][target_type];
		mix_field = ((typedata[0] >> 16) == 0x7fff)? 0x7fff: 0x7ffe;
	    }

	    typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	    typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	    typedata[0] = (mix_field << 16) | VT_PTR;
	    typedata[1] = target_type;
	}

	*encoded_type = typeoffset;

	*width = 4;
	*alignment = 4;
	*decoded_size = 8 /*sizeof(TYPEDESC)*/ + child_size;
        return 0;
    }

    if(var->array) {
        expr_t *dim = var->array;
        expr_t *array_save;
        int num_dims = 1, elements = 1, arrayoffset;
        int *arraydata;

        while(NEXT_LINK(dim)) {
            dim = NEXT_LINK(dim);
            num_dims++;
        }
        chat("array with %d dimensions\n", num_dims);
        array_save = var->array;
        var->array = NULL;
	encode_var(typelib, var, &target_type, width, alignment, NULL);
        var->array = array_save;
	arrayoffset = ctl2_alloc_segment(typelib, MSFT_SEG_ARRAYDESC, (2 + 2 * num_dims) * sizeof(long), 0);
	arraydata = (void *)&typelib->typelib_segment_data[MSFT_SEG_ARRAYDESC][arrayoffset];

	arraydata[0] = target_type;
        arraydata[1] = num_dims;
        arraydata[1] |= ((num_dims * 2 * sizeof(long)) << 16);

        arraydata += 2;
        while(dim) {
            arraydata[0] = dim->cval;
            arraydata[1] = 0;
            arraydata += 2;
            elements *= dim->cval;
            dim = PREV_LINK(dim);
        }

	typeoffset = ctl2_alloc_segment(typelib, MSFT_SEG_TYPEDESC, 8, 0);
	typedata = (void *)&typelib->typelib_segment_data[MSFT_SEG_TYPEDESC][typeoffset];

	typedata[0] = (0x7ffe << 16) | VT_CARRAY;
	typedata[1] = arrayoffset;

	*encoded_type = typeoffset;
	*width = *width * elements;
	*decoded_size = 20 /*sizeof(ARRAYDESC)*/ + (num_dims - 1) * 8 /*sizeof(SAFEARRAYBOUND)*/;
        return 0;
    }
    dump_type(var->type);

    vt = get_var_vt(var);
    type = var->type;
    while(!vt) {
        if(type->ref == NULL) {
            vt = VT_VOID;
            break;
        }
        type = type->ref;
        vt = get_type_vt(type);
    }
    encode_type(typelib, vt, type, encoded_type, width, alignment, decoded_size);
    if(type->type == RPC_FC_IP) return 2;
    return 0;
}

    
/****************************************************************************
 *	ctl2_find_nth_reference
 *
 *  Finds a reference by index into the linked list of reference records.
 *
 * RETURNS
 *
 *  Success: Offset of the desired reference record.
 *  Failure: -1.
 */
static int ctl2_find_nth_reference(
	msft_typelib_t *typelib,   /* [I] The type library in which to search. */
	int offset,                /* [I] The starting offset of the reference list. */
	int index)                 /* [I] The index of the reference to find. */
{
    MSFT_RefRecord *ref;

    for (; index && (offset != -1); index--) {
	ref = (MSFT_RefRecord *)&typelib->typelib_segment_data[MSFT_SEG_REFERENCES][offset];
	offset = ref->onext;
    }

    return offset;
}


static void write_value(msft_typelib_t* typelib, int *out, int vt, void *value)
{
    switch(vt) {
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_BOOL:
    case VT_I1:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
      {
        unsigned long *lv = value;
        if((*lv & 0x3ffffff) == *lv) {
            *out = 0x80000000;
            *out |= vt << 26;
            *out |= *lv;
        } else {
            int offset = ctl2_alloc_segment(typelib, MSFT_SEG_CUSTDATA, 8, 0);
            *((unsigned short *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset]) = vt;
            memcpy(&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+2], value, 4);
            *((unsigned short *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+6]) = 0x5757;
            *out = offset;
        }
        return;
      }
    case VT_BSTR:
      {
        char *s = (char *) value;
        int len = strlen(s), seg_len = (len + 6 + 3) & ~0x3;
        int offset = ctl2_alloc_segment(typelib, MSFT_SEG_CUSTDATA, seg_len, 0);
        *((unsigned short *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset]) = vt;
        *((unsigned int *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+2]) = len;        
        memcpy(&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+6], value, len);
        len += 6;
        while(len < seg_len) {
            *((char *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATA][offset+len]) = 0x57;
            len++;
        }
        *out = offset;
        return;
      }

    default:
        warning("can't write value of type %d yet\n", vt);
    }
    return;
}

static HRESULT set_custdata(msft_typelib_t *typelib, REFGUID guid,
                            int vt, void *value, int *offset)
{
    MSFT_GuidEntry guidentry;
    int guidoffset;
    int custoffset;
    int *custdata;
    int data_out;

    guidentry.guid = *guid;

    guidentry.hreftype = -1;
    guidentry.next_hash = -1;

    guidoffset = ctl2_alloc_guid(typelib, &guidentry);
    if (guidoffset == -1) return E_OUTOFMEMORY;
    write_value(typelib, &data_out, vt, value);

    custoffset = ctl2_alloc_segment(typelib, MSFT_SEG_CUSTDATAGUID, 12, 0);
    if (custoffset == -1) return E_OUTOFMEMORY;

    custdata = (int *)&typelib->typelib_segment_data[MSFT_SEG_CUSTDATAGUID][custoffset];
    custdata[0] = guidoffset;
    custdata[1] = data_out;
    custdata[2] = *offset;
    *offset = custoffset;

    return S_OK;
}

static HRESULT add_func_desc(msft_typeinfo_t* typeinfo, func_t *func, int index)
{
    int offset;
    int *typedata;
    int i, id;
    int decoded_size, extra_attr = 0;
    int num_params = 0, num_defaults = 0;
    var_t *arg, *last_arg = NULL;
    char *namedata;
    attr_t *attr;
    unsigned int funcflags = 0, callconv = 4 /* CC_STDCALL */;
    unsigned int funckind = 1 /* FUNC_PUREVIRTUAL */, invokekind = 1 /* INVOKE_FUNC */;
    int help_context = 0, help_string_context = 0, help_string_offset = -1;

    id = ((0x6000 | typeinfo->typeinfo->cImplTypes) << 16) | index;

    chat("add_func_desc(%p,%d)\n", typeinfo, index);

    for(attr = func->def->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_LOCAL) {
            chat("add_func_desc: skipping local function\n");
            return S_FALSE;
        }
    }

    if (!typeinfo->typedata) {
	typeinfo->typedata = xmalloc(0x2000);
	typeinfo->typedata[0] = 0;
    }

    for(arg = func->args; arg; arg = NEXT_LINK(arg)) {
        last_arg = arg;
        num_params++;
        for(attr = arg->attrs; attr; attr = NEXT_LINK(attr)) {
            if(attr->type == ATTR_DEFAULTVALUE_EXPR || attr->type == ATTR_DEFAULTVALUE_STRING) {
                num_defaults++;
                break;
            }
        }
    }

    chat("add_func_desc: num of params %d\n", num_params);

    for(attr = func->def->attrs; attr; attr = NEXT_LINK(attr)) {
        expr_t *expr = attr->u.pval; 
        switch(attr->type) {
        case ATTR_HELPCONTEXT:
            extra_attr = 1;
            help_context = expr->u.lval;
            break;
        case ATTR_HELPSTRING:
            extra_attr = 2;
            help_string_offset = ctl2_alloc_string(typeinfo->typelib, attr->u.pval);
            break;
        case ATTR_HELPSTRINGCONTEXT:
            extra_attr = 6;
            help_string_context = expr->u.lval;
            break;
        case ATTR_HIDDEN:
            funcflags |= 0x40; /* FUNCFLAG_FHIDDEN */
            break;
        case ATTR_ID:
            id = expr->u.lval;
            break;
        case ATTR_OUT:
            break;
        case ATTR_PROPGET:
            invokekind = 0x2; /* INVOKE_PROPERTYGET */
            break;
        case ATTR_PROPPUT:
            invokekind = 0x4; /* INVOKE_PROPERTYPUT */
            break;
        case ATTR_RESTRICTED:
            funcflags |= 0x1; /* FUNCFLAG_FRESTRICTED */
            break;
        default:
            warning("add_func_desc: ignoring attr %d\n", attr->type);
            break;
        }
    }
    /* allocate type data space for us */
    offset = typeinfo->typedata[0];
    typeinfo->typedata[0] += 0x18 + extra_attr * sizeof(int) + (num_params * (num_defaults ? 16 : 12));
    typedata = typeinfo->typedata + (offset >> 2) + 1;

    /* fill out the basic type information */
    typedata[0] = (0x18 + extra_attr * sizeof(int) + (num_params * (num_defaults ? 16 : 12))) | (index << 16);
    encode_var(typeinfo->typelib, func->def, &typedata[1], NULL, NULL, &decoded_size);
    typedata[2] = funcflags;
    typedata[3] = ((52 /*sizeof(FUNCDESC)*/ + decoded_size) << 16) | typeinfo->typeinfo->cbSizeVft;
    typedata[4] = (index << 16) | (callconv << 8) | (invokekind << 3) | funckind;
    if(num_defaults) typedata[4] |= 0x1000;
    typedata[5] = num_params;

    /* NOTE: High word of typedata[3] is total size of FUNCDESC + size of all ELEMDESCs for params + TYPEDESCs for pointer params and return types. */
    /* That is, total memory allocation required to reconstitute the FUNCDESC in its entirety. */
    typedata[3] += (16 /*sizeof(ELEMDESC)*/ * num_params) << 16;
    typedata[3] += (24 /*sizeof(PARAMDESCEX)*/ * num_defaults) << 16;

    switch(extra_attr) {
    case 6: typedata[11] = help_string_context;
    case 5: typedata[10] = -1;
    case 4: typedata[9] = -1;
    case 3: typedata[8] = -1;
    case 2: typedata[7] = help_string_offset;
    case 1: typedata[6] = help_context;
    case 0:
        break;
    default:
        warning("unknown number of optional attrs\n");
    }

    for (arg = last_arg, i = 0; arg; arg = PREV_LINK(arg), i++) {
        attr_t *attr;
        int paramflags = 0;
        int *paramdata = typedata + 6 + extra_attr + (num_defaults ? num_params : 0) + i * 3;
        int *defaultdata = num_defaults ? typedata + 6 + extra_attr + i : NULL;

        if(defaultdata) *defaultdata = -1;

	encode_var(typeinfo->typelib, arg, paramdata, NULL, NULL, &decoded_size);
        for(attr = arg->attrs; attr; attr = NEXT_LINK(attr)) {
            switch(attr->type) {
            case ATTR_DEFAULTVALUE_EXPR:
              {
                expr_t *expr = (expr_t *)attr->u.pval;
                paramflags |= 0x30; /* PARAMFLAG_FHASDEFAULT | PARAMFLAG_FOPT */
                chat("default value %ld\n", expr->cval);
                write_value(typeinfo->typelib, defaultdata, (*paramdata >> 16) & 0x1ff, &expr->cval);
                break;
              }
            case ATTR_DEFAULTVALUE_STRING:
              {
                char *s = (char *)attr->u.pval;
                paramflags |= 0x30; /* PARAMFLAG_FHASDEFAULT | PARAMFLAG_FOPT */
                chat("default value '%s'\n", s);
                write_value(typeinfo->typelib, defaultdata, (*paramdata >> 16) & 0x1ff, s);
                break;
              }
            case ATTR_IN:
                paramflags |= 0x01; /* PARAMFLAG_FIN */
                break;
            case ATTR_OPTIONAL:
                paramflags |= 0x10; /* PARAMFLAG_FOPT */
                break;
            case ATTR_OUT:
                paramflags |= 0x02; /* PARAMFLAG_FOUT */
                break;
            case ATTR_RETVAL:
                paramflags |= 0x08; /* PARAMFLAG_FRETVAL */
                break;
            default:
                chat("unhandled param attr %d\n", attr->type);
                break;
            }
        }
	paramdata[1] = -1;
	paramdata[2] = paramflags;
	typedata[3] += decoded_size << 16;
    }

    /* update the index data */
    typeinfo->indices[index] = id; 
    typeinfo->names[index] = -1;
    typeinfo->offsets[index] = offset;

    /* ??? */
    if (!typeinfo->typeinfo->res2) typeinfo->typeinfo->res2 = 0x20;
    typeinfo->typeinfo->res2 <<= 1;
    /* ??? */
    if (index < 2) typeinfo->typeinfo->res2 += num_params << 4;

    if (typeinfo->typeinfo->res3 == -1) typeinfo->typeinfo->res3 = 0;
    typeinfo->typeinfo->res3 += 0x38 + num_params * 0x10;
    if(num_defaults) typeinfo->typeinfo->res3 += num_params * 0x4;

    /* adjust size of VTBL */
    typeinfo->typeinfo->cbSizeVft += 4;

    /* Increment the number of function elements */
    typeinfo->typeinfo->cElement += 1;


    offset = ctl2_alloc_name(typeinfo->typelib, func->def->name);
    chat("name offset = %d index %d\n", offset, index);
    typeinfo->names[index] = offset;

    namedata = typeinfo->typelib->typelib_segment_data[MSFT_SEG_NAME] + offset;
    namedata[9] &= ~0x10;
    if (*((INT *)namedata) == -1) {
	*((INT *)namedata) = typeinfo->typelib->typelib_typeinfo_offsets[typeinfo->typeinfo->typekind >> 16];
    }

    for (arg = last_arg, i = 0; arg; arg = PREV_LINK(arg), i++) {
	/* FIXME: Almost certainly easy to break */
	int *paramdata = &typeinfo->typedata[typeinfo->offsets[index] >> 2];
        paramdata += 7 + extra_attr + (num_defaults ? num_params : 0) + i * 3;
	offset = ctl2_alloc_name(typeinfo->typelib, arg->name);
	paramdata[1] = offset;
        chat("param %d name %s offset %d\n", i, arg->name, offset);
    }
    return S_OK;
}


static void set_alignment(
        msft_typeinfo_t* typeinfo,
        WORD cbAlignment)
{

    if (!cbAlignment) return;
    if (cbAlignment > 16) return;

    typeinfo->typeinfo->typekind &= ~0xf800;

    /* FIXME: There's probably some way to simplify this. */
    switch (typeinfo->typeinfo->typekind & 15) {
    case TKIND_ALIAS:
    default:
	break;

    case TKIND_ENUM:
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
    case TKIND_COCLASS:
	if (cbAlignment > 4) cbAlignment = 4;
	break;

    case TKIND_RECORD:
    case TKIND_MODULE:
    case TKIND_UNION:
	cbAlignment = 1;
	break;
    }

    typeinfo->typeinfo->typekind |= cbAlignment << 11;

    return;
}

static HRESULT add_var_desc(msft_typeinfo_t *typeinfo, UINT index, var_t* var)
{
    int offset;
    INT *typedata;
    int var_datawidth;
    int var_alignment;
    int var_type_size;
    int alignment;
    int varflags = 0;
    attr_t *attr;
    char *namedata;

    chat("add_var_desc(%d,%s) array %p\n", index, var->name, var->array);

    if ((typeinfo->typeinfo->cElement >> 16) != index) {
	error("Out-of-order element.\n");
	return TYPE_E_ELEMENTNOTFOUND;
    }


    for(attr = var->attrs; attr; attr = NEXT_LINK(attr)) {
        switch(attr->type) {
        default:
            warning("AddVarDesc: unhandled attr type %d\n", attr->type);
            break;
        }
    }

    if (!typeinfo->typedata) {
	typeinfo->typedata = xmalloc(0x2000);
	typeinfo->typedata[0] = 0;
    }

    /* allocate type data space for us */
    offset = typeinfo->typedata[0];
    typeinfo->typedata[0] += 0x14;
    typedata = typeinfo->typedata + (offset >> 2) + 1;

    /* fill out the basic type information */
    typedata[0] = 0x14 | (index << 16);
    typedata[2] = varflags;
    typedata[3] = (36 /*sizeof(VARDESC)*/ << 16) | 0;

    /* update the index data */
    typeinfo->indices[index] = 0x40000000 + index;
    typeinfo->names[index] = -1;
    typeinfo->offsets[index] = offset;

    /* figure out type widths and whatnot */
    encode_var(typeinfo->typelib, var, &typedata[1], &var_datawidth,
               &var_alignment, &var_type_size);

    /* pad out starting position to data width */
    typeinfo->datawidth += var_alignment - 1;
    typeinfo->datawidth &= ~(var_alignment - 1);
    typedata[4] = typeinfo->datawidth;
    
    /* add the new variable to the total data width */
    typeinfo->datawidth += var_datawidth;

    /* add type description size to total required allocation */
    typedata[3] += var_type_size << 16;

    /* fix type alignment */
    alignment = (typeinfo->typeinfo->typekind >> 11) & 0x1f;
    if (alignment < var_alignment) {
	alignment = var_alignment;
	typeinfo->typeinfo->typekind &= ~0xf800;
	typeinfo->typeinfo->typekind |= alignment << 11;
    }

    /* ??? */
    if (!typeinfo->typeinfo->res2) typeinfo->typeinfo->res2 = 0x1a;
    if ((index == 0) || (index == 1) || (index == 2) || (index == 4) || (index == 9)) {
	typeinfo->typeinfo->res2 <<= 1;
    }

    /* ??? */
    if (typeinfo->typeinfo->res3 == -1) typeinfo->typeinfo->res3 = 0;
    typeinfo->typeinfo->res3 += 0x2c;

    /* increment the number of variable elements */
    typeinfo->typeinfo->cElement += 0x10000;

    /* pad data width to alignment */
    typeinfo->typeinfo->size = (typeinfo->datawidth + (alignment - 1)) & ~(alignment - 1);

    offset = ctl2_alloc_name(typeinfo->typelib, var->name);
    if (offset == -1) return E_OUTOFMEMORY;

    namedata = typeinfo->typelib->typelib_segment_data[MSFT_SEG_NAME] + offset;
    if (*((INT *)namedata) == -1) {
	*((INT *)namedata) = typeinfo->typelib->typelib_typeinfo_offsets[typeinfo->typeinfo->typekind >> 16];
	namedata[9] |= 0x10;
    }
    if ((typeinfo->typeinfo->typekind & 15) == TKIND_ENUM) {
	namedata[9] |= 0x20;
    }
    typeinfo->names[index] = offset;

    return S_OK;
}


static msft_typeinfo_t *create_msft_typeinfo(msft_typelib_t *typelib, typelib_entry_t *entry, int idx)
{
    msft_typeinfo_t *msft_typeinfo;
    int nameoffset;
    int typeinfo_offset;
    MSFT_TypeInfoBase *typeinfo;
    char *name;
    MSFT_GuidEntry guidentry;
    attr_t *attr;

    switch(entry->kind) {
    case TKIND_INTERFACE:
        name = entry->u.interface->name;
        attr = entry->u.interface->attrs;
        entry->u.interface->typelib_idx = idx;
        break;
    case TKIND_MODULE:
        name = entry->u.module->name;
        attr = entry->u.module->attrs;
        break;
    case TKIND_COCLASS:
        name = entry->u.class->name;
        attr = entry->u.class->attrs;
        break;
    case TKIND_RECORD:
        name = entry->u.structure->name;
        attr = entry->u.structure->attrs;
        entry->u.structure->typelib_idx = idx;
        chat("type = %p\n", entry->u.structure);
        break;
    default:
        error("create_msft_typeinfo: unhandled type %d\n", entry->kind);
        return NULL;
    }


    chat("Constructing msft_typeinfo for name %s kind %d\n", name, entry->kind);

    msft_typeinfo = xmalloc(sizeof(*msft_typeinfo));

    msft_typeinfo->typelib = typelib;

    nameoffset = ctl2_alloc_name(typelib, name);
    typeinfo_offset = ctl2_alloc_typeinfo(typelib, nameoffset);
    typeinfo = (MSFT_TypeInfoBase *)&typelib->typelib_segment_data[MSFT_SEG_TYPEINFO][typeinfo_offset];

    typelib->typelib_segment_data[MSFT_SEG_NAME][nameoffset + 9] = 0x38;
    *((int *)&typelib->typelib_segment_data[MSFT_SEG_NAME][nameoffset]) = typeinfo_offset;

    msft_typeinfo->typeinfo = typeinfo;

    typeinfo->typekind |= entry->kind | 0x220;
    set_alignment(msft_typeinfo, 4);

    switch (entry->kind) {
    case TKIND_ENUM:
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
    case TKIND_COCLASS:
	typeinfo->size = 4;
	break;

    case TKIND_RECORD:
    case TKIND_UNION:
	typeinfo->size = 0;
	break;

    case TKIND_MODULE:
	typeinfo->size = 2;
	break;

    case TKIND_ALIAS:
	typeinfo->size = -0x75;
	break;

    default:
	error("%s unrecognized typekind %d\n", name, entry->kind);
	typeinfo->size = 0xdeadbeef;
	break;
    }


    for( ; attr; attr = NEXT_LINK(attr)) {
        switch(attr->type) {
        case ATTR_HELPCONTEXT:
          {
            expr_t *expr = (expr_t*)attr->u.pval;
            typeinfo->helpcontext = expr->cval;
            break;
          }
        case ATTR_HELPSTRING:
          {
            int offset = ctl2_alloc_string(typelib, attr->u.pval);
            if (offset == -1) break;
            typeinfo->docstringoffs = offset;
            break;
          }
        case ATTR_HELPSTRINGCONTEXT:
          {
            expr_t *expr = (expr_t*)attr->u.pval;
            typeinfo->helpstringcontext = expr->cval;
            break;
          }
        case ATTR_HIDDEN:
            typeinfo->flags |= 0x10; /* TYPEFLAG_FHIDDEN */
            break;

        case ATTR_ODL:
            break;

        case ATTR_RESTRICTED:
            typeinfo->flags |= 0x200; /* TYPEFLAG_FRESTRICTED */
            break;

        case ATTR_UUID:
            guidentry.guid = *(GUID*)attr->u.pval;
            guidentry.hreftype = typelib->typelib_typeinfo_offsets[typeinfo->typekind >> 16];
            guidentry.next_hash = -1;
            typeinfo->posguid = ctl2_alloc_guid(typelib, &guidentry);
#if 0
            if (IsEqualIID(guid, &IID_IDispatch)) {
                typelib->typelib_header.dispatchpos = typelib->typelib_typeinfo_offsets[typeinfo->typekind >> 16];
            }
#endif
            break;

        case ATTR_VERSION:
            typeinfo->version = attr->u.ival;
            break;

        default:
            warning("create_msft_typeinfo: ignoring attr %d\n", attr->type);
            break;
        }
    }

    if (typelib->last_typeinfo) typelib->last_typeinfo->next_typeinfo = msft_typeinfo;
    typelib->last_typeinfo = msft_typeinfo;
    if (!typelib->typeinfos) typelib->typeinfos = msft_typeinfo;


    return msft_typeinfo;
}


static void set_name(msft_typelib_t *typelib)
{
    int offset;

    offset = ctl2_alloc_name(typelib, typelib->typelib->name);
    if (offset == -1) return;
    typelib->typelib_header.NameOffset = offset;
    return;
}

static void set_version(msft_typelib_t *typelib)
{
    long version = MAKELONG(0,0);
    attr_t *attr;

    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_VERSION) {
            version = attr->u.ival;
        }
    }
    typelib->typelib_header.version = version;
    return;
}

static void set_guid(msft_typelib_t *typelib)
{
    MSFT_GuidEntry guidentry;
    int offset;
    attr_t *attr;
    GUID guid = {0,0,0,{0,0,0,0,0,0}};

    guidentry.guid = guid;
    guidentry.hreftype = -2;
    guidentry.next_hash = -1;

    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_UUID) {
            guidentry.guid = *(GUID*)(attr->u.pval);
        }
    }

    offset = ctl2_alloc_guid(typelib, &guidentry);
    
    if (offset == -1) return;

    typelib->typelib_header.posguid = offset;

    return;
}

static void set_doc_string(msft_typelib_t *typelib)
{
    attr_t *attr;
    int offset;

    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_HELPSTRING) {
            offset = ctl2_alloc_string(typelib, attr->u.pval);
            if (offset == -1) return;
            typelib->typelib_header.helpstring = offset;
        }
    }
    return;
}

static void set_help_file_name(msft_typelib_t *typelib)
{
    int offset;
    attr_t *attr;
    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_HELPFILE) {
            offset = ctl2_alloc_string(typelib, attr->u.pval);
            if (offset == -1) return;
            typelib->typelib_header.helpfile = offset;
            typelib->typelib_header.varflags |= 0x10;
        }
    }
    return;
}

static void set_help_context(msft_typelib_t *typelib)
{
    attr_t *attr;
    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_HELPCONTEXT) {
            expr_t *expr = (expr_t *)attr->u.pval;
            typelib->typelib_header.helpcontext = expr->cval;
        }
    }
    return;
}

static void set_help_string_dll(msft_typelib_t *typelib)
{
    int offset;
    attr_t *attr;
    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_HELPSTRINGDLL) {
            offset = ctl2_alloc_string(typelib, attr->u.pval);
            if (offset == -1) return;
            typelib->help_string_dll_offset = offset;
            typelib->typelib_header.varflags |= 0x100;
        }
    }
    return;
}

static void set_help_string_context(msft_typelib_t *typelib)
{
    attr_t *attr;
    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        if(attr->type == ATTR_HELPSTRINGCONTEXT) {
            expr_t *expr = (expr_t *)attr->u.pval;
            typelib->typelib_header.helpstringcontext = expr->cval;
        }
    }
    return;
}

static void set_lcid(msft_typelib_t *typelib)
{
    typelib->typelib_header.lcid2 = 0x0;
    return;
}

static void set_lib_flags(msft_typelib_t *typelib)
{
    attr_t *attr;

    typelib->typelib_header.flags = 0;
    for(attr = typelib->typelib->attrs; attr; attr = NEXT_LINK(attr)) {
        switch(attr->type) {
        case ATTR_CONTROL:
            typelib->typelib_header.flags |= 0x02; /* LIBFLAG_FCONTROL */
            break;
        case ATTR_HIDDEN:
            typelib->typelib_header.flags |= 0x04; /* LIBFLAG_FHIDDEN */
            break;
        case ATTR_RESTRICTED:
            typelib->typelib_header.flags |= 0x01; /* LIBFLAG_FRESTRICTED */
            break;
        default:
            break;
        }
    }
    return;
}

static int ctl2_write_chunk(int fd, void *segment, int length)
{
    if (write(fd, segment, length) != length) {
        close(fd);
        return 0;
    }
    return -1;
}

static int ctl2_write_segment(msft_typelib_t *typelib, int fd, int segment)
{
    if (write(fd, typelib->typelib_segment_data[segment], typelib->typelib_segdir[segment].length)
        != typelib->typelib_segdir[segment].length) {
	close(fd);
	return 0;
    }

    return -1;
}

static void ctl2_finalize_typeinfos(msft_typelib_t *typelib, int filesize)
{
    msft_typeinfo_t *typeinfo;

    for (typeinfo = typelib->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
	typeinfo->typeinfo->memoffset = filesize;
	if (typeinfo->typedata) {
	    /*LayOut(typeinfo);*/
	    filesize += typeinfo->typedata[0] + ((typeinfo->typeinfo->cElement >> 16) * 12) + ((typeinfo->typeinfo->cElement & 0xffff) * 12) + 4;
	}
    }
}

static int ctl2_finalize_segment(msft_typelib_t *typelib, int filepos, int segment)
{
    if (typelib->typelib_segdir[segment].length) {
	typelib->typelib_segdir[segment].offset = filepos;
    } else {
	typelib->typelib_segdir[segment].offset = -1;
    }

    return typelib->typelib_segdir[segment].length;
}


static void ctl2_write_typeinfos(msft_typelib_t *typelib, int fd)
{
    msft_typeinfo_t *typeinfo;

    for (typeinfo = typelib->typeinfos; typeinfo; typeinfo = typeinfo->next_typeinfo) {
	if (!typeinfo->typedata) continue;

	ctl2_write_chunk(fd, typeinfo->typedata, typeinfo->typedata[0] + 4);
	ctl2_write_chunk(fd, typeinfo->indices, ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4);
        chat("writing name chunk len %d %08lx\n", ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4, *(DWORD*)typeinfo->names);
	ctl2_write_chunk(fd, typeinfo->names, ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4);
	ctl2_write_chunk(fd, typeinfo->offsets, ((typeinfo->typeinfo->cElement & 0xffff) + (typeinfo->typeinfo->cElement >> 16)) * 4);
    }
}

static int save_all_changes(msft_typelib_t *typelib)
{
    int retval;
    int filepos;
    int fd;

    chat("save_all_changes(%p)\n", typelib);

    retval = TYPE_E_IOERROR;

    fd = creat(typelib->typelib->filename, 0666);
    if (fd == -1) return retval;

    filepos = sizeof(MSFT_Header) + sizeof(MSFT_SegDir);
    if(typelib->typelib_header.varflags & 0x100) filepos += 4; /* helpstringdll */
    filepos += typelib->typelib_header.nrtypeinfos * 4;

    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_TYPEINFO);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_GUIDHASH);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_GUID);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_IMPORTINFO);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_IMPORTFILES);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_REFERENCES);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_NAMEHASH);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_NAME);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_STRING);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_TYPEDESC);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_ARRAYDESC);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_CUSTDATA);
    filepos += ctl2_finalize_segment(typelib, filepos, MSFT_SEG_CUSTDATAGUID);

    ctl2_finalize_typeinfos(typelib, filepos);

    if (!ctl2_write_chunk(fd, &typelib->typelib_header, sizeof(typelib->typelib_header))) return retval;
    if(typelib->typelib_header.varflags & 0x100)
        if (!ctl2_write_chunk(fd, &typelib->help_string_dll_offset, sizeof(typelib->help_string_dll_offset)))
            return retval;

    if (!ctl2_write_chunk(fd, typelib->typelib_typeinfo_offsets, typelib->typelib_header.nrtypeinfos * 4)) return retval;
    if (!ctl2_write_chunk(fd, &typelib->typelib_segdir, sizeof(typelib->typelib_segdir))) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_TYPEINFO    )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_GUIDHASH    )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_GUID        )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_IMPORTINFO  )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_IMPORTFILES )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_REFERENCES  )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_NAMEHASH    )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_NAME        )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_STRING      )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_TYPEDESC    )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_ARRAYDESC   )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_CUSTDATA    )) return retval;
    if (!ctl2_write_segment(typelib, fd, MSFT_SEG_CUSTDATAGUID)) return retval;

    ctl2_write_typeinfos(typelib, fd);

    if (close(fd) == -1) return retval;

    retval = S_OK;
    return retval;
}




int create_msft_typelib(typelib_t *typelib)
{
    msft_typelib_t *msft;
    int failed = 0, typelib_idx;
    typelib_entry_t *entry;
    time_t cur_time;
    unsigned int version = 5 << 24 | 1 << 16 | 164; /* 5.01.0164 */
    GUID midl_time_guid    = {0xde77ba63,0x517c,0x11d1,{0xa2,0xda,0x00,0x00,0xf8,0x77,0x3c,0xe9}}; 
    GUID midl_version_guid = {0xde77ba64,0x517c,0x11d1,{0xa2,0xda,0x00,0x00,0xf8,0x77,0x3c,0xe9}}; 

    msft = malloc(sizeof(*msft));
    if (!msft) return 0;
    memset(msft, 0, sizeof(*msft));
    msft->typelib = typelib;

    ctl2_init_header(msft);
    ctl2_init_segdir(msft);

    msft->typelib_header.varflags |= SYS_WIN32;

    /*
     * The following two calls return an offset or -1 if out of memory. We
     * specifically need an offset of 0, however, so...
     */
    if (ctl2_alloc_segment(msft, MSFT_SEG_GUIDHASH, 0x80, 0x80)) { failed = 1; }
    if (ctl2_alloc_segment(msft, MSFT_SEG_NAMEHASH, 0x200, 0x200)) { failed = 1; }

    if(failed) return 0;

    msft->typelib_guidhash_segment = (int *)msft->typelib_segment_data[MSFT_SEG_GUIDHASH];
    msft->typelib_namehash_segment = (int *)msft->typelib_segment_data[MSFT_SEG_NAMEHASH];

    memset(msft->typelib_guidhash_segment, 0xff, 0x80);
    memset(msft->typelib_namehash_segment, 0xff, 0x200);

    set_lib_flags(msft);
    set_lcid(msft);
    set_help_file_name(msft);
    set_doc_string(msft);
    set_guid(msft);
    set_version(msft);
    set_name(msft);
    set_help_context(msft);
    set_help_string_dll(msft);
    set_help_string_context(msft);
    
    /* midl adds two sets of custom data to the library: the current unix time
       and midl's version number */
    cur_time = time(NULL);
    set_custdata(msft, &midl_time_guid, VT_UI4, &cur_time, &msft->typelib_header.CustomDataOffset);
    set_custdata(msft, &midl_version_guid, VT_UI4, &version, &msft->typelib_header.CustomDataOffset);

    typelib_idx = 0;
    for(entry = typelib->entry; NEXT_LINK(entry); entry = NEXT_LINK(entry))
        ;
    for( ; entry; entry = PREV_LINK(entry)) {
        msft_typeinfo_t *msft_typeinfo = create_msft_typeinfo(msft, entry, typelib_idx);
        switch(entry->kind) {
        case TKIND_INTERFACE:
          {
            int idx = 0;
            func_t *cur = entry->u.interface->funcs;
            while(NEXT_LINK(cur)) cur = NEXT_LINK(cur);
            while(cur) {
                if(add_func_desc(msft_typeinfo, cur, idx) == S_OK)
                    idx++;
                cur = PREV_LINK(cur);
            }
            break;
          }
        case TKIND_RECORD:
          {
            int idx = 0;
            var_t *cur = entry->u.structure->fields;
            while(NEXT_LINK(cur)) cur = NEXT_LINK(cur);
            while(cur) {
                add_var_desc(msft_typeinfo, idx, cur);
                idx++;
                cur = PREV_LINK(cur);
            }
            break;
          }
                
        default:
            error("create_msft_typelib: unhandled type %d\n", entry->kind);
            break;
        }
        typelib_idx++;
    }
    save_all_changes(msft);
    return 1;
}
