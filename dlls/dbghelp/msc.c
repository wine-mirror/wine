/*
 * File msc.c - read VC++ debug information from COFF and eventually
 * from PDB files.
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004-2009, Eric Pouech.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Note - this handles reading debug information for 32 bit applications
 * that run under Windows-NT for example.  I doubt that this would work well
 * for 16 bit applications, but I don't think it really matters since the
 * file format is different, and we should never get in here in such cases.
 *
 * TODO:
 *	Get 16 bit CV stuff working.
 *	Add symbol size to internal symbol table.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "wine/exception.h"
#include "wine/debug.h"
#include "dbghelp_private.h"
#include "wine/mscvpdb.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_msc);

static const GUID null_guid;

struct pdb_stream_name
{
    const char* name;
    unsigned    index;
};

enum pdb_kind {PDB_JG, PDB_DS};

struct pdb_file_info
{
    enum pdb_kind               kind;
    struct pdb_reader          *pdb_reader; /* new pdb reader */
    const char*                 image;
    struct pdb_stream_name*     stream_dict;
    unsigned                    fpoext_stream;
    union
    {
        struct
        {
            struct PDB_JG_TOC*  toc;
        } jg;
        struct
        {
            struct PDB_DS_TOC*  toc;
        } ds;
    } u;
};

/* FIXME: don't make it static */
#define CV_MAX_MODULES          32
struct pdb_module_info
{
    unsigned                    used_subfiles;
    struct pdb_file_info        pdb_files[CV_MAX_MODULES];
};

#define loc_cv_local_range (loc_user + 0) /* loc.offset contain the copy of all defrange* Codeview records following S_LOCAL */

struct cv_module_snarf
{
    const struct codeview_type_parse*           ipi_ctp;
    const struct CV_DebugSSubsectionHeader_t*   dbgsubsect;
    unsigned                                    dbgsubsect_size;
    const PDB_STRING_TABLE*                     strimage;
};

BOOL pdb_hack_get_main_info(struct module_format *modfmt, struct pdb_reader **pdb, unsigned *fpoext_stream)
{
    struct pdb_module_info*     pdb_info;

    if (!modfmt) return FALSE;
    pdb_info = modfmt->u.pdb_info;
    *pdb = pdb_info->pdb_files[0].pdb_reader;
    if (fpoext_stream)
        *fpoext_stream = pdb_info->pdb_files[0].fpoext_stream;
    return TRUE;
}

/*========================================================================
 * Debug file access helper routines
 */

static void dump(const void* ptr, unsigned len)
{
    unsigned int i, j;
    char        msg[128];
    const char* hexof = "0123456789abcdef";
    const BYTE* x = ptr;

    for (i = 0; i < len; i += 16)
    {
        sprintf(msg, "%08x: ", i);
        memset(msg + 10, ' ', 3 * 16 + 1 + 16);
        for (j = 0; j < min(16, len - i); j++)
        {
            msg[10 + 3 * j + 0] = hexof[x[i + j] >> 4];
            msg[10 + 3 * j + 1] = hexof[x[i + j] & 15];
            msg[10 + 3 * j + 2] = ' ';
            msg[10 + 3 * 16 + 1 + j] = (x[i + j] >= 0x20 && x[i + j] < 0x7f) ?
                x[i + j] : '.';
        }
        msg[10 + 3 * 16] = ' ';
        msg[10 + 3 * 16 + 1 + 16] = '\0';
        FIXME("%s\n", msg);
    }
}

/*========================================================================
 * Process CodeView type information.
 */

static struct symt*     cv_basic_types[T_MAXPREDEFINEDTYPE];

struct cv_defined_module
{
    BOOL                allowed;
    unsigned int        first_type_index;
    unsigned int        last_type_index;
    struct symt**       defined_types;
};
/* FIXME: don't make it static */
#define CV_MAX_MODULES          32
static struct cv_defined_module cv_zmodules[CV_MAX_MODULES];
static struct cv_defined_module*cv_current_module;

static void codeview_init_basic_types(struct module* module)
{
    unsigned ptrsz = module->cpu->word_size;

    /*
     * These are the common builtin types that are used by VC++.
     */
    cv_basic_types[T_NOTYPE] = NULL;
    cv_basic_types[T_ABS]    = NULL;
    cv_basic_types[T_VOID]   = &symt_get_basic(btVoid,   0)->symt; /* void */
    cv_basic_types[T_CHAR]   = &symt_get_basic(btInt,    1)->symt; /* signed char (and char in C) */
    cv_basic_types[T_SHORT]  = &symt_get_basic(btInt,    2)->symt; /* short int */
    cv_basic_types[T_LONG]   = &symt_get_basic(btLong,   4)->symt; /* long int */
    cv_basic_types[T_QUAD]   = &symt_get_basic(btInt,    8)->symt; /* long long int */
    cv_basic_types[T_UCHAR]  = &symt_get_basic(btUInt,   1)->symt; /* unsigned char */
    cv_basic_types[T_USHORT] = &symt_get_basic(btUInt,   2)->symt; /* unsigned short */
    cv_basic_types[T_ULONG]  = &symt_get_basic(btULong,  4)->symt; /* unsigned long */
    cv_basic_types[T_UQUAD]  = &symt_get_basic(btUInt,   8)->symt; /* unsigned long long */
    cv_basic_types[T_BOOL08] = &symt_get_basic(btBool,   1)->symt; /* BOOL08 */
    cv_basic_types[T_BOOL16] = &symt_get_basic(btBool,   2)->symt; /* BOOL16 */
    cv_basic_types[T_BOOL32] = &symt_get_basic(btBool,   4)->symt; /* BOOL32 */
    cv_basic_types[T_BOOL64] = &symt_get_basic(btBool,   8)->symt; /* BOOL64 */
    cv_basic_types[T_REAL32] = &symt_get_basic(btFloat,  4)->symt; /* float */
    cv_basic_types[T_REAL64] = &symt_get_basic(btFloat,  8)->symt; /* double */
    cv_basic_types[T_REAL80] = &symt_get_basic(btFloat, 10)->symt; /* long double */
    cv_basic_types[T_RCHAR]  = &symt_get_basic(btChar,   1)->symt; /* "real" char (char in C++) */
    cv_basic_types[T_WCHAR]  = &symt_get_basic(btWChar,  2)->symt; /* char8_t */
    cv_basic_types[T_CHAR16] = &symt_get_basic(btChar16, 2)->symt; /* char16_t */
    cv_basic_types[T_CHAR32] = &symt_get_basic(btChar32, 4)->symt; /* char32_t */
    cv_basic_types[T_CHAR8]  = &symt_get_basic(btChar8,  1)->symt; /* char8_t */
    cv_basic_types[T_INT2]   = &symt_get_basic(btInt,    2)->symt; /* INT2 */
    cv_basic_types[T_UINT2]  = &symt_get_basic(btUInt,   2)->symt; /* UINT2 */
    cv_basic_types[T_INT4]   = &symt_get_basic(btInt,    4)->symt; /* INT4 */
    cv_basic_types[T_UINT4]  = &symt_get_basic(btUInt,   4)->symt; /* UINT4 */
    cv_basic_types[T_INT8]   = &symt_get_basic(btInt,    8)->symt; /* INT8 */
    cv_basic_types[T_UINT8]  = &symt_get_basic(btUInt,   8)->symt; /* UINT8 */
    cv_basic_types[T_OCT]    = &symt_get_basic(btInt,    8)->symt; /* INT8 */
    cv_basic_types[T_UOCT]   = &symt_get_basic(btUInt,   8)->symt; /* UINT8 */

    cv_basic_types[T_HRESULT]= &symt_get_basic(btUInt,   4)->symt; /* HRESULT */

    cv_basic_types[T_32PVOID]   = &symt_new_pointer(module, cv_basic_types[T_VOID], 4)->symt;
    cv_basic_types[T_32PCHAR]   = &symt_new_pointer(module, cv_basic_types[T_CHAR], 4)->symt;
    cv_basic_types[T_32PSHORT]  = &symt_new_pointer(module, cv_basic_types[T_SHORT], 4)->symt;
    cv_basic_types[T_32PLONG]   = &symt_new_pointer(module, cv_basic_types[T_LONG], 4)->symt;
    cv_basic_types[T_32PQUAD]   = &symt_new_pointer(module, cv_basic_types[T_QUAD], 4)->symt;
    cv_basic_types[T_32POCT]    = &symt_new_pointer(module, cv_basic_types[T_OCT], 4)->symt;
    cv_basic_types[T_32PUCHAR]  = &symt_new_pointer(module, cv_basic_types[T_UCHAR], 4)->symt;
    cv_basic_types[T_32PUSHORT] = &symt_new_pointer(module, cv_basic_types[T_USHORT], 4)->symt;
    cv_basic_types[T_32PULONG]  = &symt_new_pointer(module, cv_basic_types[T_ULONG], 4)->symt;
    cv_basic_types[T_32PUQUAD]  = &symt_new_pointer(module, cv_basic_types[T_UQUAD], 4)->symt;
    cv_basic_types[T_32PUOCT]   = &symt_new_pointer(module, cv_basic_types[T_UOCT], 4)->symt;
    cv_basic_types[T_32PBOOL08] = &symt_new_pointer(module, cv_basic_types[T_BOOL08], 4)->symt;
    cv_basic_types[T_32PBOOL16] = &symt_new_pointer(module, cv_basic_types[T_BOOL16], 4)->symt;
    cv_basic_types[T_32PBOOL32] = &symt_new_pointer(module, cv_basic_types[T_BOOL32], 4)->symt;
    cv_basic_types[T_32PBOOL64] = &symt_new_pointer(module, cv_basic_types[T_BOOL64], 4)->symt;
    cv_basic_types[T_32PREAL32] = &symt_new_pointer(module, cv_basic_types[T_REAL32], 4)->symt;
    cv_basic_types[T_32PREAL64] = &symt_new_pointer(module, cv_basic_types[T_REAL64], 4)->symt;
    cv_basic_types[T_32PREAL80] = &symt_new_pointer(module, cv_basic_types[T_REAL80], 4)->symt;
    cv_basic_types[T_32PRCHAR]  = &symt_new_pointer(module, cv_basic_types[T_RCHAR], 4)->symt;
    cv_basic_types[T_32PWCHAR]  = &symt_new_pointer(module, cv_basic_types[T_WCHAR], 4)->symt;
    cv_basic_types[T_32PCHAR16] = &symt_new_pointer(module, cv_basic_types[T_CHAR16], 4)->symt;
    cv_basic_types[T_32PCHAR32] = &symt_new_pointer(module, cv_basic_types[T_CHAR32], 4)->symt;
    cv_basic_types[T_32PCHAR8]  = &symt_new_pointer(module, cv_basic_types[T_CHAR8], 4)->symt;
    cv_basic_types[T_32PINT2]   = &symt_new_pointer(module, cv_basic_types[T_INT2], 4)->symt;
    cv_basic_types[T_32PUINT2]  = &symt_new_pointer(module, cv_basic_types[T_UINT2], 4)->symt;
    cv_basic_types[T_32PINT4]   = &symt_new_pointer(module, cv_basic_types[T_INT4], 4)->symt;
    cv_basic_types[T_32PUINT4]  = &symt_new_pointer(module, cv_basic_types[T_UINT4], 4)->symt;
    cv_basic_types[T_32PINT8]   = &symt_new_pointer(module, cv_basic_types[T_INT8], 4)->symt;
    cv_basic_types[T_32PUINT8]  = &symt_new_pointer(module, cv_basic_types[T_UINT8], 4)->symt;
    cv_basic_types[T_32PHRESULT]= &symt_new_pointer(module, cv_basic_types[T_HRESULT], 4)->symt;

    cv_basic_types[T_64PVOID]   = &symt_new_pointer(module, cv_basic_types[T_VOID], 8)->symt;
    cv_basic_types[T_64PCHAR]   = &symt_new_pointer(module, cv_basic_types[T_CHAR], 8)->symt;
    cv_basic_types[T_64PSHORT]  = &symt_new_pointer(module, cv_basic_types[T_SHORT], 8)->symt;
    cv_basic_types[T_64PLONG]   = &symt_new_pointer(module, cv_basic_types[T_LONG], 8)->symt;
    cv_basic_types[T_64PQUAD]   = &symt_new_pointer(module, cv_basic_types[T_QUAD], 8)->symt;
    cv_basic_types[T_64POCT]    = &symt_new_pointer(module, cv_basic_types[T_OCT], 8)->symt;
    cv_basic_types[T_64PUCHAR]  = &symt_new_pointer(module, cv_basic_types[T_UCHAR], 8)->symt;
    cv_basic_types[T_64PUSHORT] = &symt_new_pointer(module, cv_basic_types[T_USHORT], 8)->symt;
    cv_basic_types[T_64PULONG]  = &symt_new_pointer(module, cv_basic_types[T_ULONG], 8)->symt;
    cv_basic_types[T_64PUQUAD]  = &symt_new_pointer(module, cv_basic_types[T_UQUAD], 8)->symt;
    cv_basic_types[T_64PUOCT]   = &symt_new_pointer(module, cv_basic_types[T_UOCT], 8)->symt;
    cv_basic_types[T_64PBOOL08] = &symt_new_pointer(module, cv_basic_types[T_BOOL08], 8)->symt;
    cv_basic_types[T_64PBOOL16] = &symt_new_pointer(module, cv_basic_types[T_BOOL16], 8)->symt;
    cv_basic_types[T_64PBOOL32] = &symt_new_pointer(module, cv_basic_types[T_BOOL32], 8)->symt;
    cv_basic_types[T_64PBOOL64] = &symt_new_pointer(module, cv_basic_types[T_BOOL64], 8)->symt;
    cv_basic_types[T_64PREAL32] = &symt_new_pointer(module, cv_basic_types[T_REAL32], 8)->symt;
    cv_basic_types[T_64PREAL64] = &symt_new_pointer(module, cv_basic_types[T_REAL64], 8)->symt;
    cv_basic_types[T_64PREAL80] = &symt_new_pointer(module, cv_basic_types[T_REAL80], 8)->symt;
    cv_basic_types[T_64PRCHAR]  = &symt_new_pointer(module, cv_basic_types[T_RCHAR], 8)->symt;
    cv_basic_types[T_64PWCHAR]  = &symt_new_pointer(module, cv_basic_types[T_WCHAR], 8)->symt;
    cv_basic_types[T_64PCHAR16] = &symt_new_pointer(module, cv_basic_types[T_CHAR16], 8)->symt;
    cv_basic_types[T_64PCHAR32] = &symt_new_pointer(module, cv_basic_types[T_CHAR32], 8)->symt;
    cv_basic_types[T_64PCHAR8]  = &symt_new_pointer(module, cv_basic_types[T_CHAR8], 8)->symt;
    cv_basic_types[T_64PINT2]   = &symt_new_pointer(module, cv_basic_types[T_INT2], 8)->symt;
    cv_basic_types[T_64PUINT2]  = &symt_new_pointer(module, cv_basic_types[T_UINT2], 8)->symt;
    cv_basic_types[T_64PINT4]   = &symt_new_pointer(module, cv_basic_types[T_INT4], 8)->symt;
    cv_basic_types[T_64PUINT4]  = &symt_new_pointer(module, cv_basic_types[T_UINT4], 8)->symt;
    cv_basic_types[T_64PINT8]   = &symt_new_pointer(module, cv_basic_types[T_INT8], 8)->symt;
    cv_basic_types[T_64PUINT8]  = &symt_new_pointer(module, cv_basic_types[T_UINT8], 8)->symt;
    cv_basic_types[T_64PHRESULT]= &symt_new_pointer(module, cv_basic_types[T_HRESULT], 8)->symt;

    cv_basic_types[T_PVOID]   = &symt_new_pointer(module, cv_basic_types[T_VOID],   ptrsz)->symt;
    cv_basic_types[T_PCHAR]   = &symt_new_pointer(module, cv_basic_types[T_CHAR],   ptrsz)->symt;
    cv_basic_types[T_PSHORT]  = &symt_new_pointer(module, cv_basic_types[T_SHORT],  ptrsz)->symt;
    cv_basic_types[T_PLONG]   = &symt_new_pointer(module, cv_basic_types[T_LONG],   ptrsz)->symt;
    cv_basic_types[T_PQUAD]   = &symt_new_pointer(module, cv_basic_types[T_QUAD],   ptrsz)->symt;
    cv_basic_types[T_POCT]    = &symt_new_pointer(module, cv_basic_types[T_OCT],  ptrsz)->symt;
    cv_basic_types[T_PUCHAR]  = &symt_new_pointer(module, cv_basic_types[T_UCHAR],  ptrsz)->symt;
    cv_basic_types[T_PUSHORT] = &symt_new_pointer(module, cv_basic_types[T_USHORT], ptrsz)->symt;
    cv_basic_types[T_PULONG]  = &symt_new_pointer(module, cv_basic_types[T_ULONG],  ptrsz)->symt;
    cv_basic_types[T_PUQUAD]  = &symt_new_pointer(module, cv_basic_types[T_UQUAD],  ptrsz)->symt;
    cv_basic_types[T_PUOCT]   = &symt_new_pointer(module, cv_basic_types[T_UOCT],  ptrsz)->symt;
    cv_basic_types[T_PBOOL08] = &symt_new_pointer(module, cv_basic_types[T_BOOL08], ptrsz)->symt;
    cv_basic_types[T_PBOOL16] = &symt_new_pointer(module, cv_basic_types[T_BOOL16], ptrsz)->symt;
    cv_basic_types[T_PBOOL32] = &symt_new_pointer(module, cv_basic_types[T_BOOL32], ptrsz)->symt;
    cv_basic_types[T_PBOOL64] = &symt_new_pointer(module, cv_basic_types[T_BOOL64], ptrsz)->symt;
    cv_basic_types[T_PREAL32] = &symt_new_pointer(module, cv_basic_types[T_REAL32], ptrsz)->symt;
    cv_basic_types[T_PREAL64] = &symt_new_pointer(module, cv_basic_types[T_REAL64], ptrsz)->symt;
    cv_basic_types[T_PREAL80] = &symt_new_pointer(module, cv_basic_types[T_REAL80], ptrsz)->symt;
    cv_basic_types[T_PRCHAR]  = &symt_new_pointer(module, cv_basic_types[T_RCHAR],  ptrsz)->symt;
    cv_basic_types[T_PWCHAR]  = &symt_new_pointer(module, cv_basic_types[T_WCHAR],  ptrsz)->symt;
    cv_basic_types[T_PCHAR16] = &symt_new_pointer(module, cv_basic_types[T_CHAR16], ptrsz)->symt;
    cv_basic_types[T_PCHAR32] = &symt_new_pointer(module, cv_basic_types[T_CHAR32], ptrsz)->symt;
    cv_basic_types[T_PCHAR8]  = &symt_new_pointer(module, cv_basic_types[T_CHAR8],  ptrsz)->symt;
    cv_basic_types[T_PINT2]   = &symt_new_pointer(module, cv_basic_types[T_INT2],   ptrsz)->symt;
    cv_basic_types[T_PUINT2]  = &symt_new_pointer(module, cv_basic_types[T_UINT2],  ptrsz)->symt;
    cv_basic_types[T_PINT4]   = &symt_new_pointer(module, cv_basic_types[T_INT4],   ptrsz)->symt;
    cv_basic_types[T_PUINT4]  = &symt_new_pointer(module, cv_basic_types[T_UINT4],  ptrsz)->symt;
    cv_basic_types[T_PINT8]   = &symt_new_pointer(module, cv_basic_types[T_INT8],   ptrsz)->symt;
    cv_basic_types[T_PUINT8]  = &symt_new_pointer(module, cv_basic_types[T_UINT8],  ptrsz)->symt;
}

static int leaf_as_variant(VARIANT *v, const unsigned char *leaf)
{
    unsigned short int type = *(const unsigned short *)leaf;
    int length = 2;

    if (type < LF_NUMERIC)
    {
        V_VT(v) = VT_I2;
        V_I2(v) = type;
    }
    else
    {
        leaf += length;
        switch (type)
        {
        case LF_CHAR:
            length += 1;
            V_VT(v) = VT_I1;
            V_I1(v) = *(const char*)leaf;
            break;

        case LF_SHORT:
            length += 2;
            V_VT(v) = VT_I2;
            V_I2(v) = *(const short*)leaf;
            break;

        case LF_USHORT:
            length += 2;
            V_VT(v) = VT_UI2;
            V_UI2(v) = *(const unsigned short*)leaf;
            break;

        case LF_LONG:
            length += 4;
            V_VT(v) = VT_I4;
            V_I4(v) = *(const int*)leaf;
            break;

        case LF_ULONG:
            length += 4;
            V_VT(v) = VT_UI4;
            V_UI4(v) = *(const unsigned int*)leaf;
            break;

        case LF_QUADWORD:
            length += 8;
            V_VT(v) = VT_I8;
            V_I8(v) = *(const long long int*)leaf;
            break;

        case LF_UQUADWORD:
            length += 8;
            V_VT(v) = VT_UI8;
            V_UI8(v) = *(const long long unsigned int*)leaf;
            break;

        case LF_REAL32:
            length += 4;
            V_VT(v) = VT_R4;
            V_R4(v) = *(const float*)leaf;
            break;

        case LF_REAL48:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 6;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        case LF_REAL64:
            length += 8;
            V_VT(v) = VT_R8;
            V_R8(v) = *(const double*)leaf;
            break;

        case LF_REAL80:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 10;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        case LF_REAL128:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 16;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        case LF_COMPLEX32:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 4;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        case LF_COMPLEX64:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 8;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        case LF_COMPLEX80:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 10;
            break;

        case LF_COMPLEX128:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 16;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        case LF_VARSTRING:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 2 + *leaf;
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;

        default:
	    FIXME("Unknown numeric leaf type %04x\n", type);
            V_VT(v) = VT_EMPTY;     /* FIXME */
            break;
        }
    }

    return length;
}

#define numeric_leaf(v,l) _numeric_leaf(__LINE__,v,l)
static int _numeric_leaf(unsigned line, int *value, const unsigned char *leaf)
{
    unsigned short int type = *(const unsigned short int *)leaf;
    int length = 2;
    leaf += length;

    if (type < LF_NUMERIC)
    {
        *value = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:
            length += 1;
            *value = *(const char*)leaf;
            break;

        case LF_SHORT:
            length += 2;
            *value = *(const short*)leaf;
            break;

        case LF_USHORT:
            length += 2;
            *value = *(const unsigned short*)leaf;
            break;

        case LF_LONG:
            length += 4;
            *value = *(const int*)leaf;
            break;

        case LF_ULONG:
            length += 4;
            *value = *(const unsigned int*)leaf;
            break;

        case LF_QUADWORD:
        case LF_UQUADWORD:
	    FIXME("Unsupported numeric leaf type %04x from %u (%I64x)\n", type, line, *(ULONG64*)leaf);
            length += 8;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL32:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 4;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL48:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 6;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL64:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 8;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL80:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 10;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL128:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 16;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX32:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 4;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX64:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 8;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX80:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 10;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX128:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 16;
            *value = 0;    /* FIXME */
            break;

        case LF_VARSTRING:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 2 + *leaf;
            *value = 0;    /* FIXME */
            break;

        default:
	    FIXME("Unknown numeric leaf type %04x\n", type);
            *value = 0;
            break;
        }
    }

    return length;
}

/* convert a pascal string (as stored in debug information) into
 * a C string (null terminated).
 */
static const char* terminate_string(const struct p_string* p_name)
{
    static char symname[256];

    memcpy(symname, p_name->name, p_name->namelen);
    symname[p_name->namelen] = '\0';

    return (!*symname || strcmp(symname, "__unnamed") == 0) ? NULL : symname;
}

static struct symt*  codeview_get_type(unsigned int typeno, BOOL quiet)
{
    struct symt*        symt = NULL;

    /*
     * Convert Codeview type numbers into something we can grok internally.
     * Numbers < T_MAXPREDEFINEDTYPE all fixed builtin types.
     * Numbers from T_FIRSTDEFINABLETYPE and up are all user defined (structs, etc).
     */
    if (typeno < T_MAXPREDEFINEDTYPE)
        symt = cv_basic_types[typeno];
    else if (typeno >= T_FIRSTDEFINABLETYPE)
    {
        unsigned        mod_index = typeno >> 24;
        unsigned        mod_typeno = typeno & 0x00FFFFFF;
        struct cv_defined_module*       mod;

        mod = (mod_index == 0) ? cv_current_module : &cv_zmodules[mod_index];

        if (mod_index >= CV_MAX_MODULES || !mod->allowed)
            FIXME("Module of index %d isn't loaded yet (%x)\n", mod_index, typeno);
        else
        {
            if (mod_typeno >= mod->first_type_index && mod_typeno < mod->last_type_index)
                symt = mod->defined_types[mod_typeno - mod->first_type_index];
        }
    }
    if (!quiet && !symt && typeno) FIXME("Returning NULL symt for type-id %x\n", typeno);
    return symt;
}

struct hash_link
{
    unsigned            id;
    struct hash_link*   next;
};

struct codeview_type_parse
{
    struct module*      module;
    PDB_TYPES           header;
    const BYTE*         table;
    const DWORD*        offset;      /* typeindex => offset in 'table' */
    BYTE*               hash_stream; /* content of stream header.hash_file */
    struct hash_link**  hash;        /* hash_table (deserialized from PDB hash table) */
    struct hash_link*   alloc_hash;  /* allocated hash_link (id => hash_link) */
};

static inline const void* codeview_jump_to_type(const struct codeview_type_parse* ctp, DWORD idx)
{
    return (idx >= ctp->header.first_index && idx < ctp->header.last_index) ?
        ctp->table + ctp->offset[idx - ctp->header.first_index] : NULL;
}

static int codeview_add_type(unsigned int typeno, struct symt* dt)
{
    unsigned idx;
    if (!cv_current_module)
    {
        FIXME("Adding %x to non allowed module\n", typeno);
        return FALSE;
    }
    if ((typeno >> 24) != 0)
        FIXME("No module index while inserting type-id assumption is wrong %x\n",
              typeno);
    if (typeno < cv_current_module->first_type_index || typeno >= cv_current_module->last_type_index)
    {
        FIXME("Type number %x out of bounds [%x, %x)\n",
              typeno, cv_current_module->first_type_index, typeno >= cv_current_module->last_type_index);
        return FALSE;
    }
    idx = typeno - cv_current_module->first_type_index;
    if (cv_current_module->defined_types[idx])
    {
        if (cv_current_module->defined_types[idx] != dt)
            FIXME("Overwriting at %x\n", typeno);
    }
    cv_current_module->defined_types[idx] = dt;
    return TRUE;
}

static void codeview_clear_type_table(void)
{
    int i;

    for (i = 0; i < CV_MAX_MODULES; i++)
    {
        if (cv_zmodules[i].allowed)
            free(cv_zmodules[i].defined_types);
        cv_zmodules[i].allowed = FALSE;
        cv_zmodules[i].defined_types = NULL;
        cv_zmodules[i].first_type_index = 0;
        cv_zmodules[i].last_type_index = 0;
    }
    cv_current_module = NULL;
}

static struct symt* codeview_parse_one_type(struct codeview_type_parse* ctp,
                                            unsigned curr_type,
                                            const union codeview_type* type);

static struct symt* codeview_fetch_type(struct codeview_type_parse* ctp,
                                        unsigned typeno)
{
    struct symt*                symt;
    const union codeview_type*  p;

    if (!typeno) return NULL;
    if ((symt = codeview_get_type(typeno, TRUE))) return symt;

    if ((p = codeview_jump_to_type(ctp, typeno)))
        symt = codeview_parse_one_type(ctp, typeno, p);
    if (!symt) FIXME("Couldn't load type %x\n", typeno);
    return symt;
}

static UINT32 codeview_compute_hash(const char* ptr, unsigned len)
{
    const char* last = ptr + len;
    UINT32 ret = 0;

    while (ptr + sizeof(UINT32) <= last)
    {
        ret ^= *(UINT32*)ptr;
        ptr += sizeof(UINT32);
    }
    if (ptr + sizeof(UINT16) <= last)
    {
        ret ^= *(UINT16*)ptr;
        ptr += sizeof(UINT16);
    }
    if (ptr + sizeof(BYTE) <= last)
        ret ^= *(BYTE*)ptr;
    ret |= 0x20202020;
    ret ^= (ret >> 11);
    return ret ^ (ret >> 16);
}

/* We call 'forwardable' a type which can have a forward declaration, and we need to merge
 * (when they both exist) the type record of the forward declaration and the type record
 * of the full definition into a single symt.
 */
static BOOL codeview_is_forwardable_type(const union codeview_type* type)
{
    switch (type->generic.id)
    {
    case LF_CLASS_V1:
    case LF_CLASS_V2:
    case LF_CLASS_V3:
    case LF_ENUM_V1:
    case LF_ENUM_V2:
    case LF_ENUM_V3:
    case LF_STRUCTURE_V1:
    case LF_STRUCTURE_V2:
    case LF_STRUCTURE_V3:
    case LF_UNION_V1:
    case LF_UNION_V2:
    case LF_UNION_V3:
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOL codeview_type_is_forward(const union codeview_type* cvtype)
{
    cv_property_t property;

    switch (cvtype->generic.id)
    {
    case LF_STRUCTURE_V1:
    case LF_CLASS_V1:     property = cvtype->struct_v1.property;       break;
    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:     property = cvtype->struct_v2.property;       break;
    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:     property = cvtype->struct_v3.property;       break;
    case LF_UNION_V1:     property = cvtype->union_v1.property;        break;
    case LF_UNION_V2:     property = cvtype->union_v2.property;        break;
    case LF_UNION_V3:     property = cvtype->union_v3.property;        break;
    case LF_ENUM_V1:      property = cvtype->enumeration_v1.property;  break;
    case LF_ENUM_V2:      property = cvtype->enumeration_v2.property;  break;
    case LF_ENUM_V3:      property = cvtype->enumeration_v3.property;  break;
    default:
        return FALSE;
    }
    return property.is_forward_defn;
}

static BOOL codeview_type_extract_name(const union codeview_type* cvtype,
                                       const char** name, unsigned* len, const char** decorated_name)
{
    int value, leaf_len;
    const struct p_string* p_name = NULL;
    const char* c_name = NULL;
    BOOL decorated = FALSE;

    switch (cvtype->generic.id)
    {
    case LF_STRUCTURE_V1:
    case LF_CLASS_V1:
        leaf_len = numeric_leaf(&value, cvtype->struct_v1.data);
        p_name = (const struct p_string*)&cvtype->struct_v1.data[leaf_len];
        break;
    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:
        leaf_len = numeric_leaf(&value, cvtype->struct_v2.data);
        p_name = (const struct p_string*)&cvtype->struct_v2.data[leaf_len];
        break;
    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        leaf_len = numeric_leaf(&value, cvtype->struct_v3.data);
        c_name = (const char*)&cvtype->struct_v3.data[leaf_len];
        decorated = cvtype->struct_v3.property.has_decorated_name;
        break;
    case LF_UNION_V1:
        leaf_len = numeric_leaf(&value, cvtype->union_v1.data);
        p_name = (const struct p_string*)&cvtype->union_v1.data[leaf_len];
        break;
    case LF_UNION_V2:
        leaf_len = numeric_leaf(&value, cvtype->union_v2.data);
        p_name = (const struct p_string*)&cvtype->union_v2.data[leaf_len];
        break;
    case LF_UNION_V3:
        leaf_len = numeric_leaf(&value, cvtype->union_v3.data);
        c_name = (const char*)&cvtype->union_v3.data[leaf_len];
        decorated = cvtype->union_v3.property.has_decorated_name;
        break;
    case LF_ENUM_V1:
        p_name = &cvtype->enumeration_v1.p_name;
        break;
    case LF_ENUM_V2:
        p_name = &cvtype->enumeration_v2.p_name;
        break;
    case LF_ENUM_V3:
        c_name = cvtype->enumeration_v3.name;
        decorated = cvtype->enumeration_v3.property.has_decorated_name;
        break;
    default:
        return FALSE;
    }
    if (p_name)
    {
        *name = p_name->name;
        *len = p_name->namelen;
        *decorated_name = NULL;
        return TRUE;
    }
    if (c_name)
    {
        *name = c_name;
        *len = strlen(c_name);
        *decorated_name = decorated ? (c_name + *len + 1) : NULL;
        return TRUE;
    }
    return FALSE;
}

static unsigned pdb_read_hash_value(const struct codeview_type_parse* ctp, unsigned idx)
{
    const void* where = ctp->hash_stream + ctp->header.hash_offset + (idx - ctp->header.first_index) * ctp->header.hash_value_size;
    switch (ctp->header.hash_value_size)
    {
    case 2: return *(unsigned short*)where;
    case 4: return *(unsigned*)where;
    }
    return 0;
}

static BOOL codeview_resolve_forward_type(struct codeview_type_parse* ctp, const union codeview_type* cvref,
                                          unsigned reftype, unsigned *impl_type)
{
    const union codeview_type* cvdecl;
    struct hash_link* hl;
    const char* nameref;
    const char* decoratedref;
    unsigned lenref;
    unsigned hash;

    /* Unfortunately, hash of forward defs are computed on the whole type record, not its name
     * (unlike hash of UDT & enum implementations which is based primarily on the name... sigh)
     * So compute the hash of the expected implementation.
     */
    if (!codeview_type_extract_name(cvref,  &nameref,  &lenref,  &decoratedref)) return FALSE;
    hash = codeview_compute_hash(nameref, lenref) % ctp->header.hash_num_buckets;

    for (hl = ctp->hash[hash]; hl; hl = hl->next)
    {
        if (hl->id == reftype) continue;
        cvdecl = codeview_jump_to_type(ctp, hl->id);
        if (cvdecl && !codeview_type_is_forward(cvdecl) && cvref->generic.id == cvdecl->generic.id)
        {
            const char* namedecl;
            const char* decorateddecl;
            unsigned lendecl;

            if (codeview_type_extract_name(cvdecl, &namedecl, &lendecl, &decorateddecl) &&
                ((decoratedref && decorateddecl && !strcmp(decoratedref, decorateddecl)) ||
                 (!decoratedref && !decorateddecl && lenref == lendecl && !memcmp(nameref, namedecl, lenref))))
            {
                TRACE("mapping forward type %s (%s) %x into %x\n", debugstr_an(nameref, lenref), debugstr_a(decoratedref), reftype, hl->id);
                *impl_type = hl->id;
                return TRUE;
            }
        }
    }
    WARN("Couldn't resolve forward declaration for %s\n", debugstr_an(nameref, lenref));
    return FALSE;
}

static struct symt* codeview_add_type_pointer(struct codeview_type_parse* ctp,
                                              unsigned int pointee_type)
{
    struct symt* pointee;

    pointee = codeview_fetch_type(ctp, pointee_type);
    if (!pointee) return NULL;
    return &symt_new_pointer(ctp->module, pointee, ctp->module->cpu->word_size)->symt;
}

static struct symt* codeview_add_type_array(struct codeview_type_parse* ctp, 
                                            const char* name,
                                            unsigned int elemtype,
                                            unsigned int indextype,
                                            unsigned int arr_len)
{
    struct symt*        elem = codeview_fetch_type(ctp, elemtype);
    struct symt*        index = codeview_fetch_type(ctp, indextype);
    DWORD64             elem_size;
    DWORD               count = 0;

    if (symt_get_info(ctp->module, elem, TI_GET_LENGTH, &elem_size) && elem_size)
    {
        if (arr_len % (DWORD)elem_size)
            FIXME("array size should be a multiple of element size %u %I64u\n", arr_len, elem_size);
        count = arr_len / (unsigned)elem_size;
    }
    return &symt_new_array(ctp->module, 0, count, elem, index)->symt;
}

static BOOL codeview_add_type_enum_field_list(struct codeview_type_parse* ctp,
                                              struct symt_enum* symt,
                                              unsigned typeno)
{
    const union codeview_reftype*       ref_type;
    const unsigned char*                ptr;
    const unsigned char*                last;
    const union codeview_fieldtype*     type;

    if (!typeno) return TRUE;
    if (!(ref_type = codeview_jump_to_type(ctp, typeno))) return FALSE;
    ptr = ref_type->fieldlist.list;
    last = (const BYTE*)ref_type + ref_type->generic.len + 2;

    while (ptr < last)
    {
        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr += *ptr & 0x0f;
            continue;
        }

        type = (const union codeview_fieldtype*)ptr;

        switch (type->generic.id)
        {
        case LF_ENUMERATE_V1:
        {
            VARIANT v;
            int vlen = leaf_as_variant(&v, type->enumerate_v1.data);
            const struct p_string* p_name = (const struct p_string*)&type->enumerate_v1.data[vlen];

            symt_add_enum_element(ctp->module, symt, terminate_string(p_name), &v);
            ptr += 2 + 2 + vlen + (1 + p_name->namelen);
            break;
        }
        case LF_ENUMERATE_V3:
        {
            VARIANT v;
            int vlen = leaf_as_variant(&v, type->enumerate_v3.data);
            const char* name = (const char*)&type->enumerate_v3.data[vlen];

            symt_add_enum_element(ctp->module, symt, name, &v);
            ptr += 2 + 2 + vlen + (1 + strlen(name));
            break;
        }

        case LF_INDEX_V1:
            if (!codeview_add_type_enum_field_list(ctp, symt, type->index_v1.ref))
                return FALSE;
            ptr += 2 + 2;
            break;

        case LF_INDEX_V2:
            if (!codeview_add_type_enum_field_list(ctp, symt, type->index_v2.ref))
                return FALSE;
            ptr += 2 + 2 + 4;
            break;

        default:
            FIXME("Unsupported type %04x in ENUM field list\n", type->generic.id);
            return FALSE;
        }
    }
    return TRUE;
}

static void codeview_add_udt_element(struct codeview_type_parse* ctp,
                                     struct symt_udt* symt, const char* name,
                                     int value, unsigned type)
{
    struct symt*                subtype;
    const union codeview_reftype*cv_type;

    if ((cv_type = codeview_jump_to_type(ctp, type)))
    {
        switch (cv_type->generic.id)
        {
        case LF_BITFIELD_V1:
            symt_add_udt_element(ctp->module, symt, name,
                                 symt_ptr_to_symref(codeview_fetch_type(ctp, cv_type->bitfield_v1.type)),
                                 value, cv_type->bitfield_v1.bitoff,
                                 cv_type->bitfield_v1.nbits);
            return;
        case LF_BITFIELD_V2:
            symt_add_udt_element(ctp->module, symt, name,
                                 symt_ptr_to_symref(codeview_fetch_type(ctp, cv_type->bitfield_v2.type)),
                                 value, cv_type->bitfield_v2.bitoff,
                                 cv_type->bitfield_v2.nbits);
            return;
        }
    }
    subtype = codeview_fetch_type(ctp, type);

    if (subtype)
    {
        DWORD64 elem_size = 0;
        symt_get_info(ctp->module, subtype, TI_GET_LENGTH, &elem_size);
        symt_add_udt_element(ctp->module, symt, name, symt_ptr_to_symref(subtype),
                             value, 0, 0);
    }
}

static int codeview_add_type_struct_field_list(struct codeview_type_parse* ctp,
                                               struct symt_udt* symt,
                                               unsigned fieldlistno)
{
    const unsigned char*        ptr;
    const unsigned char*        last;
    int                         value, leaf_len;
    const struct p_string*      p_name;
    const char*                 c_name;
    const union codeview_reftype*type_ref;
    const union codeview_fieldtype* type;

    if (!fieldlistno) return TRUE;
    type_ref = codeview_jump_to_type(ctp, fieldlistno);
    ptr = type_ref->fieldlist.list;
    last = (const BYTE*)type_ref + type_ref->generic.len + 2;

    while (ptr < last)
    {
        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr += *ptr & 0x0f;
            continue;
        }

        type = (const union codeview_fieldtype*)ptr;

        switch (type->generic.id)
        {
        case LF_BCLASS_V1:
            leaf_len = numeric_leaf(&value, type->bclass_v1.data);

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 2 + leaf_len;
            break;

        case LF_BCLASS_V2:
            leaf_len = numeric_leaf(&value, type->bclass_v2.data);

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 4 + leaf_len;
            break;

        case LF_VBCLASS_V1:
        case LF_IVBCLASS_V1:
            {
                const unsigned char* p_vboff;
                int vpoff, vplen;
                leaf_len = numeric_leaf(&value, type->vbclass_v1.data);
                p_vboff = &type->vbclass_v1.data[leaf_len];
                vplen = numeric_leaf(&vpoff, p_vboff);

                /* FIXME: ignored for now */

                ptr += 2 + 2 + 2 + 2 + leaf_len + vplen;
            }
            break;

        case LF_VBCLASS_V2:
        case LF_IVBCLASS_V2:
            {
                const unsigned char* p_vboff;
                int vpoff, vplen;
                leaf_len = numeric_leaf(&value, type->vbclass_v2.data);
                p_vboff = &type->vbclass_v2.data[leaf_len];
                vplen = numeric_leaf(&vpoff, p_vboff);

                /* FIXME: ignored for now */

                ptr += 2 + 2 + 4 + 4 + leaf_len + vplen;
            }
            break;

        case LF_MEMBER_V1:
            leaf_len = numeric_leaf(&value, type->member_v1.data);
            p_name = (const struct p_string*)&type->member_v1.data[leaf_len];

            codeview_add_udt_element(ctp, symt, terminate_string(p_name), value,
                                     type->member_v1.type);

            ptr += 2 + 2 + 2 + leaf_len + (1 + p_name->namelen);
            break;

        case LF_MEMBER_V2:
            leaf_len = numeric_leaf(&value, type->member_v2.data);
            p_name = (const struct p_string*)&type->member_v2.data[leaf_len];

            codeview_add_udt_element(ctp, symt, terminate_string(p_name), value,
                                     type->member_v2.type);

            ptr += 2 + 2 + 4 + leaf_len + (1 + p_name->namelen);
            break;

        case LF_MEMBER_V3:
            leaf_len = numeric_leaf(&value, type->member_v3.data);
            c_name = (const char*)&type->member_v3.data[leaf_len];

            codeview_add_udt_element(ctp, symt, c_name, value, type->member_v3.type);

            ptr += 2 + 2 + 4 + leaf_len + (strlen(c_name) + 1);
            break;

        case LF_STMEMBER_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 2 + (1 + type->stmember_v1.p_name.namelen);
            break;

        case LF_STMEMBER_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 4 + 2 + (1 + type->stmember_v2.p_name.namelen);
            break;

        case LF_STMEMBER_V3:
            /* FIXME: ignored for now */
            ptr += 2 + 4 + 2 + (strlen(type->stmember_v3.name) + 1);
            break;

        case LF_METHOD_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 2 + (1 + type->method_v1.p_name.namelen);
            break;

        case LF_METHOD_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (1 + type->method_v2.p_name.namelen);
            break;

        case LF_METHOD_V3:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (strlen(type->method_v3.name) + 1);
            break;

        case LF_NESTTYPE_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + (1 + type->nesttype_v1.p_name.namelen);
            break;

        case LF_NESTTYPE_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (1 + type->nesttype_v2.p_name.namelen);
            break;

        case LF_NESTTYPE_V3:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (strlen(type->nesttype_v3.name) + 1);
            break;

        case LF_VFUNCTAB_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2;
            break;

        case LF_VFUNCTAB_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4;
            break;

        case LF_ONEMETHOD_V1:
            /* FIXME: ignored for now */
            switch ((type->onemethod_v1.attribute >> 2) & 7)
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 2 + 4 + (1 + type->onemethod_virt_v1.p_name.namelen);
                break;

            default:
                ptr += 2 + 2 + 2 + (1 + type->onemethod_v1.p_name.namelen);
                break;
            }
            break;

        case LF_ONEMETHOD_V2:
            /* FIXME: ignored for now */
            switch ((type->onemethod_v2.attribute >> 2) & 7)
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 4 + 4 + (1 + type->onemethod_virt_v2.p_name.namelen);
                break;

            default:
                ptr += 2 + 2 + 4 + (1 + type->onemethod_v2.p_name.namelen);
                break;
            }
            break;

        case LF_ONEMETHOD_V3:
            /* FIXME: ignored for now */
            switch ((type->onemethod_v3.attribute >> 2) & 7)
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 4 + 4 + (strlen(type->onemethod_virt_v3.name) + 1);
                break;

            default:
                ptr += 2 + 2 + 4 + (strlen(type->onemethod_v3.name) + 1);
                break;
            }
            break;

        case LF_INDEX_V1:
            if (!codeview_add_type_struct_field_list(ctp, symt, type->index_v1.ref))
                return FALSE;
            ptr += 2 + 2;
            break;

        case LF_INDEX_V2:
            if (!codeview_add_type_struct_field_list(ctp, symt, type->index_v2.ref))
                return FALSE;
            ptr += 2 + 2 + 4;
            break;

        default:
            FIXME("Unsupported type %04x in STRUCT field list\n", type->generic.id);
            return FALSE;
        }
    }

    return TRUE;
}

static struct symt* codeview_new_func_signature(struct codeview_type_parse* ctp,
                                                enum CV_call_e call_conv)
{
    struct symt_function_signature*     sym;

    sym = symt_new_function_signature(ctp->module, NULL, call_conv);
    return &sym->symt;
}

static void codeview_add_func_signature_args(struct codeview_type_parse* ctp,
                                             struct symt_function_signature* sym,
                                             unsigned ret_type,
                                             unsigned args_list)
{
    const union codeview_reftype*       reftype;

    sym->rettype = codeview_fetch_type(ctp, ret_type);
    if (args_list && (reftype = codeview_jump_to_type(ctp, args_list)))
    {
        unsigned int i;
        switch (reftype->generic.id)
        {
        case LF_ARGLIST_V1:
            for (i = 0; i < reftype->arglist_v1.num; i++)
                symt_add_function_signature_parameter(ctp->module, sym,
                                                      codeview_fetch_type(ctp, reftype->arglist_v1.args[i]));
            break;
        case LF_ARGLIST_V2:
            for (i = 0; i < reftype->arglist_v2.num; i++)
                symt_add_function_signature_parameter(ctp->module, sym,
                                                      codeview_fetch_type(ctp, reftype->arglist_v2.args[i]));
            break;
        default:
            FIXME("Unexpected leaf %x for signature's pmt\n", reftype->generic.id);
        }
    }
}

static struct symt* codeview_parse_one_type(struct codeview_type_parse* ctp,
                                            unsigned curr_type,
                                            const union codeview_type* type)
{
    struct symt*                symt = NULL;
    int                         value, leaf_len;
    const struct p_string*      p_name;
    const char*                 c_name;

    switch (type->generic.id)
    {
    case LF_MODIFIER_V1:
        /* FIXME: we don't handle modifiers,
         * but read previous type on the curr_type
         */
        WARN("Modifier on %x: %s%s%s%s\n",
             type->modifier_v1.type,
             type->modifier_v1.attribute & 0x01 ? "const " : "",
             type->modifier_v1.attribute & 0x02 ? "volatile " : "",
             type->modifier_v1.attribute & 0x04 ? "unaligned " : "",
             type->modifier_v1.attribute & ~0x07 ? "unknown " : "");
        symt = codeview_fetch_type(ctp, type->modifier_v1.type);
        break;
    case LF_MODIFIER_V2:
        /* FIXME: we don't handle modifiers, but readd previous type on the curr_type */
        WARN("Modifier on %x: %s%s%s%s\n",
             type->modifier_v2.type,
             type->modifier_v2.attribute & 0x01 ? "const " : "",
             type->modifier_v2.attribute & 0x02 ? "volatile " : "",
             type->modifier_v2.attribute & 0x04 ? "unaligned " : "",
             type->modifier_v2.attribute & ~0x07 ? "unknown " : "");
        symt = codeview_fetch_type(ctp, type->modifier_v2.type);
        break;

    case LF_POINTER_V1:
        symt = codeview_add_type_pointer(ctp, type->pointer_v1.datatype);
        break;
    case LF_POINTER_V2:
        symt = codeview_add_type_pointer(ctp, type->pointer_v2.datatype);
        break;

    case LF_ARRAY_V1:
        leaf_len = numeric_leaf(&value, type->array_v1.data);
        p_name = (const struct p_string*)&type->array_v1.data[leaf_len];
        symt = codeview_add_type_array(ctp, terminate_string(p_name),
                                       type->array_v1.elemtype,
                                       type->array_v1.idxtype, value);
        break;
    case LF_ARRAY_V2:
        leaf_len = numeric_leaf(&value, type->array_v2.data);
        p_name = (const struct p_string*)&type->array_v2.data[leaf_len];

        symt = codeview_add_type_array(ctp, terminate_string(p_name),
                                       type->array_v2.elemtype,
                                       type->array_v2.idxtype, value);
        break;
    case LF_ARRAY_V3:
        leaf_len = numeric_leaf(&value, type->array_v3.data);
        c_name = (const char*)&type->array_v3.data[leaf_len];

        symt = codeview_add_type_array(ctp, c_name,
                                       type->array_v3.elemtype,
                                       type->array_v3.idxtype, value);
        break;

    case LF_STRUCTURE_V1:
    case LF_CLASS_V1:
        if (!type->struct_v1.property.is_forward_defn)
            codeview_add_type_struct_field_list(ctp, (struct symt_udt*)codeview_get_type(curr_type, TRUE),
                                                type->struct_v1.fieldlist);
        break;

    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:
        if (!type->struct_v2.property.is_forward_defn)
            codeview_add_type_struct_field_list(ctp, (struct symt_udt*)codeview_get_type(curr_type, TRUE),
                                                type->struct_v2.fieldlist);
        break;

    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        if (!type->struct_v3.property.is_forward_defn)
            codeview_add_type_struct_field_list(ctp, (struct symt_udt*)codeview_get_type(curr_type, TRUE),
                                                type->struct_v3.fieldlist);
        break;

    case LF_UNION_V1:
        if (!type->union_v1.property.is_forward_defn)
            codeview_add_type_struct_field_list(ctp, (struct symt_udt*)codeview_get_type(curr_type, TRUE),
                                                type->union_v1.fieldlist);
        break;

    case LF_UNION_V2:
        if (!type->union_v2.property.is_forward_defn)
            codeview_add_type_struct_field_list(ctp, (struct symt_udt*)codeview_get_type(curr_type, TRUE),
                                                type->union_v2.fieldlist);
        break;

    case LF_UNION_V3:
        if (!type->union_v3.property.is_forward_defn)
            codeview_add_type_struct_field_list(ctp, (struct symt_udt*)codeview_get_type(curr_type, TRUE),
                                                type->union_v3.fieldlist);
        break;

    case LF_ENUM_V1:
        {
            struct symt_enum* senum = (struct symt_enum*)codeview_get_type(curr_type, TRUE);
            senum->base_type = codeview_fetch_type(ctp, type->enumeration_v1.type);
            codeview_add_type_enum_field_list(ctp, senum, type->enumeration_v1.fieldlist);
        }
        break;

    case LF_ENUM_V2:
        {
            struct symt_enum* senum = (struct symt_enum*)codeview_get_type(curr_type, TRUE);
            senum->base_type = codeview_fetch_type(ctp, type->enumeration_v2.type);
            codeview_add_type_enum_field_list(ctp, senum, type->enumeration_v2.fieldlist);
        }
        break;

    case LF_ENUM_V3:
        {
            struct symt_enum* senum = (struct symt_enum*)codeview_get_type(curr_type, TRUE);
            senum->base_type = codeview_fetch_type(ctp, type->enumeration_v3.type);
            codeview_add_type_enum_field_list(ctp, senum, type->enumeration_v3.fieldlist);
        }
        break;

    case LF_PROCEDURE_V1:
        symt = codeview_new_func_signature(ctp, type->procedure_v1.callconv);
        codeview_add_func_signature_args(ctp,
                                         (struct symt_function_signature*)symt,
                                         type->procedure_v1.rvtype,
                                         type->procedure_v1.arglist);
        break;
    case LF_PROCEDURE_V2:
        symt = codeview_new_func_signature(ctp,type->procedure_v2.callconv);
        codeview_add_func_signature_args(ctp,
                                         (struct symt_function_signature*)symt,
                                         type->procedure_v2.rvtype,
                                         type->procedure_v2.arglist);
        break;

    case LF_MFUNCTION_V1:
        /* FIXME: for C++, this is plain wrong, but as we don't use arg types
         * nor class information, this would just do for now
         */
        symt = codeview_new_func_signature(ctp, type->mfunction_v1.callconv);
        codeview_add_func_signature_args(ctp,
                                         (struct symt_function_signature*)symt,
                                         type->mfunction_v1.rvtype,
                                         type->mfunction_v1.arglist);
        break;
    case LF_MFUNCTION_V2:
        /* FIXME: for C++, this is plain wrong, but as we don't use arg types
         * nor class information, this would just do for now
         */
        symt = codeview_new_func_signature(ctp, type->mfunction_v2.callconv);
        codeview_add_func_signature_args(ctp,
                                         (struct symt_function_signature*)symt,
                                         type->mfunction_v2.rvtype,
                                         type->mfunction_v2.arglist);
        break;

    case LF_VTSHAPE_V1:
        /* this is an ugly hack... FIXME when we have C++ support */
        {
            char    buf[128];
            snprintf(buf, sizeof(buf), "__internal_vt_shape_%x\n", curr_type);
            symt = &symt_new_udt(ctp->module, buf, 0, UdtStruct)->symt;
        }
        break;
    /* types we can simply silence for now */
    case LF_LABEL_V1:
    case LF_VFTABLE_V3:
        break;
    default:
        FIXME("Unsupported type-id leaf %x\n", type->generic.id);
        dump(type, 2 + type->generic.len);
        return NULL;
    }
    return symt && codeview_add_type(curr_type, symt) ? symt : NULL;
}

static struct symt* codeview_load_forwardable_type(struct codeview_type_parse* ctp,
                                                   const union codeview_type* type)
{
    struct symt*                symt;
    int                         value, leaf_len;
    const struct p_string*      p_name;
    const char*                 c_name;

    switch (type->generic.id)
    {
    case LF_STRUCTURE_V1:
    case LF_CLASS_V1:
        leaf_len = numeric_leaf(&value, type->struct_v1.data);
        p_name = (const struct p_string*)&type->struct_v1.data[leaf_len];
        symt = &symt_new_udt(ctp->module, terminate_string(p_name), value,
                             type->generic.id == LF_CLASS_V1 ? UdtClass : UdtStruct)->symt;
        break;

    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:
        leaf_len = numeric_leaf(&value, type->struct_v2.data);
        p_name = (const struct p_string*)&type->struct_v2.data[leaf_len];
        symt = &symt_new_udt(ctp->module, terminate_string(p_name), value,
                             type->generic.id == LF_CLASS_V2 ? UdtClass : UdtStruct)->symt;
        break;

    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        leaf_len = numeric_leaf(&value, type->struct_v3.data);
        c_name = (const char*)&type->struct_v3.data[leaf_len];
        symt = &symt_new_udt(ctp->module, c_name, value,
                             type->generic.id == LF_CLASS_V3 ? UdtClass : UdtStruct)->symt;
        break;

    case LF_UNION_V1:
        leaf_len = numeric_leaf(&value, type->union_v1.data);
        p_name = (const struct p_string*)&type->union_v1.data[leaf_len];
        symt = &symt_new_udt(ctp->module, terminate_string(p_name), value, UdtUnion)->symt;
        break;

    case LF_UNION_V2:
        leaf_len = numeric_leaf(&value, type->union_v2.data);
        p_name = (const struct p_string*)&type->union_v2.data[leaf_len];
        symt = &symt_new_udt(ctp->module, terminate_string(p_name), value, UdtUnion)->symt;
        break;

    case LF_UNION_V3:
        leaf_len = numeric_leaf(&value, type->union_v3.data);
        c_name = (const char*)&type->union_v3.data[leaf_len];
        symt = &symt_new_udt(ctp->module, c_name, value, UdtUnion)->symt;
        break;

    case LF_ENUM_V1:
        symt = &symt_new_enum(ctp->module, terminate_string(&type->enumeration_v1.p_name), NULL)->symt;
        break;

    case LF_ENUM_V2:
        symt = &symt_new_enum(ctp->module, terminate_string(&type->enumeration_v2.p_name), NULL)->symt;
        break;

    case LF_ENUM_V3:
        symt = &symt_new_enum(ctp->module, type->enumeration_v3.name, NULL)->symt;
        break;

    default: symt = NULL;
    }
    return symt;
}

static inline BOOL codeview_is_top_level_type(const union codeview_type* type)
{
    /* type records we're interested in are the ones referenced by symbols
     * The known ranges are (X mark the ones we want):
     *   X  0000-0016       for V1 types
     *      0200-020c       for V1 types referenced by other types
     *      0400-040f       for V1 types (complex lists & sets)
     *   X  1000-100f       for V2 types
     *      1200-120c       for V2 types referenced by other types
     *      1400-140f       for V1 types (complex lists & sets)
     *   X  1500-150d       for V3 types
     *      8000-8010       for numeric leafes
     */
    return !(type->generic.id & 0x8600) || (type->generic.id & 0x0100);
}

static BOOL codeview_parse_type_table(struct codeview_type_parse* ctp)
{
    unsigned int                i, curr_type;
    const union codeview_type*  type;

    cv_current_module->first_type_index = ctp->header.first_index;
    cv_current_module->last_type_index = ctp->header.last_index;
    cv_current_module->defined_types = calloc(ctp->header.last_index - ctp->header.first_index,
                                              sizeof(*cv_current_module->defined_types));

    /* pass I: + load implementation of forwardable types, but without their content
     *         + merge forward declarations with their implementations (when the later exists)
     *         + do it in the order generated from PDB hash table to preserve that order
     *           (several versions coming from different compilations can exist in the PDB file,
     *           and using PDB order ensures that we use the relevant one.
     *           (needed for forward resolution and type lookup by name)
     *           (dbghelp hash table inserts new elements at the end of bucket's list)
     * Note: for a given type, we must handle:
     * - only an implementation type record
     * - only a forward type record (eg using struct foo* without struct foo being defined)
     * - a forward type record and on an implementation type record: this is the most common, but
     *   depending on hash values, we cannot tell which on will show up first
     */
    for (i = 0; i < ctp->header.hash_num_buckets; i++)
    {
        struct hash_link* hl;
        for (hl = ctp->hash[i]; hl; hl = hl->next)
        {
            struct symt* symt;
            type = codeview_jump_to_type(ctp, hl->id);
            if (!codeview_is_top_level_type(type)) continue;
            if (codeview_type_is_forward(type))
            {
                unsigned impl_type;
                /* make the forward declaration point to the implementation (if any) */
                if (codeview_resolve_forward_type(ctp, type, hl->id, &impl_type))
                {
                    /* impl already loaded? */
                    if (!(symt = codeview_get_type(impl_type, TRUE)))
                    {
                        /* no load it */
                        if ((symt = codeview_load_forwardable_type(ctp, codeview_jump_to_type(ctp, impl_type))))
                            codeview_add_type(impl_type, symt);
                        else
                            FIXME("forward def of %x => %x, unable to load impl\n", hl->id, impl_type);
                    }
                }
                else
                    /* forward type definition without implementation, create empty type */
                    symt = codeview_load_forwardable_type(ctp, type);
            }
            else
            {
                /* if not already loaded (from previous forward declaration), load it */
                if (!(symt = codeview_get_type(hl->id, TRUE)))
                    symt = codeview_load_forwardable_type(ctp, type);
            }
            codeview_add_type(hl->id, symt);
        }
    }
    /* pass II: + non forwardable types: load them, but since they can be indirectly
     *            loaded (from another type), but don't load them twice
     *          + forwardable types: load their content
     */
    for (curr_type = ctp->header.first_index; curr_type < ctp->header.last_index; curr_type++)
    {
        type = codeview_jump_to_type(ctp, curr_type);
        if (codeview_is_top_level_type(type) &&
            (!codeview_get_type(curr_type, TRUE) || codeview_is_forwardable_type(type)))
            codeview_parse_one_type(ctp, curr_type, type);
    }

    return TRUE;
}

/*========================================================================
 * Process CodeView line number information.
 */
static ULONG_PTR codeview_get_address(const struct msc_debug_info* msc_dbg,
                                          unsigned seg, unsigned offset);

static BOOL codeview_snarf_linetab(const struct msc_debug_info* msc_dbg, const BYTE* linetab,
                                   int size, BOOL pascal_str)
{
    const BYTE*                 ptr = linetab;
    int				nfile, nseg;
    int                         i, j;
    unsigned int                k;
    const unsigned int*         filetab;
    const unsigned int*         lt_ptr;
    const unsigned int*         offsets;
    const unsigned short*       linenos;
    const struct startend*      start;
    unsigned                    source;
    ULONG_PTR                   addr, func_addr0;
    struct symt_function*       func;
    const struct codeview_linetab_block* ltb;

    nfile = *(const short*)linetab;
    if (!nfile) return FALSE;
    filetab = (const unsigned int*)(linetab + 2 * sizeof(short));

    for (i = 0; i < nfile; i++)
    {
        ptr = linetab + filetab[i];
        nseg = *(const short*)ptr;
        lt_ptr = (const unsigned int*)(ptr + 2 * sizeof(short));
        start = (const struct startend*)(lt_ptr + nseg);

        /*
         * Now snarf the filename for all of the segments for this file.
         */
        if (pascal_str)
            source = source_new(msc_dbg->module, NULL, terminate_string((const struct p_string*)(start + nseg)));
        else
            source = source_new(msc_dbg->module, NULL, (const char*)(start + nseg));

        for (j = 0; j < nseg; j++)
	{
            ltb = (const struct codeview_linetab_block*)(linetab + *lt_ptr++);
            offsets = (const unsigned int*)&ltb->data;
            linenos = (const unsigned short*)&offsets[ltb->num_lines];
            func_addr0 = codeview_get_address(msc_dbg, ltb->seg, start[j].start);
            if (!func_addr0) continue;
            for (func = NULL, k = 0; k < ltb->num_lines; k++)
            {
                /* now locate function (if any) */
                addr = func_addr0 + offsets[k] - start[j].start;
                /* unfortunately, we can have several functions in the same block, if there's no
                 * gap between them... find the new function if needed
                 */
                if (!func || addr >= func->ranges[0].high)
                {
                    func = (struct symt_function*)symt_find_symbol_at(msc_dbg->module, addr);
                    /* FIXME: at least labels support line numbers */
                    if (!symt_check_tag(&func->symt, SymTagFunction) && !symt_check_tag(&func->symt, SymTagInlineSite))
                    {
                        WARN("--not a func at %04x:%08x %Ix tag=%d\n",
                             ltb->seg, offsets[k], addr, func ? func->symt.tag : -1);
                        func = NULL;
                        break;
                    }
                }
                symt_add_func_line(msc_dbg->module, func, source, linenos[k], addr);
            }
	}
    }
    return TRUE;
}

static const char* pdb_get_string_table_entry(const PDB_STRING_TABLE* table, unsigned offset);

static BOOL codeview_snarf_linetab2(const struct msc_debug_info* msc_dbg, const struct cv_module_snarf* cvmod)
{
    unsigned    i;
    const void* hdr_last = (const char*)cvmod->dbgsubsect + cvmod->dbgsubsect_size;
    const struct CV_DebugSSubsectionHeader_t*     hdr;
    const struct CV_DebugSSubsectionHeader_t*     hdr_next;
    const struct CV_DebugSSubsectionHeader_t*     hdr_files = NULL;
    const struct CV_DebugSLinesHeader_t*          lines_hdr;
    const struct CV_DebugSLinesFileBlockHeader_t* files_hdr;
    const struct CV_Line_t*                       lines;
    const struct CV_Checksum_t*                   chksms;
    unsigned    source;
    struct symt_function* func;

    /* locate DEBUG_S_FILECHKSMS (if any) */
    for (hdr = cvmod->dbgsubsect; CV_IS_INSIDE(hdr, hdr_last); hdr = CV_RECORD_GAP(hdr, hdr->cbLen))
    {
        if (hdr->type == DEBUG_S_FILECHKSMS)
        {
            hdr_files = hdr;
            break;
        }
    }
    if (!hdr_files)
    {
        TRACE("No DEBUG_S_FILECHKSMS found\n");
        return FALSE;
    }

    for (hdr = cvmod->dbgsubsect; CV_IS_INSIDE(hdr, hdr_last); hdr = hdr_next)
    {
        hdr_next = CV_RECORD_GAP(hdr, hdr->cbLen);
        if (!(hdr->type & DEBUG_S_IGNORE))
        {
            ULONG_PTR lineblk_base;
            /* FIXME: should also check that whole lines_blk fits in linetab + size */
            switch (hdr->type)
            {
            case DEBUG_S_LINES:
                lines_hdr = CV_RECORD_AFTER(hdr);
                files_hdr = CV_RECORD_AFTER(lines_hdr);
                /* Skip blocks that are too small - Intel C Compiler generates these. */
                if (!CV_IS_INSIDE(files_hdr, hdr_next)) break;
                TRACE("block from %04x:%08x #%x\n",
                      lines_hdr->segCon, lines_hdr->offCon, lines_hdr->cbCon);
                chksms = CV_RECORD_GAP(hdr_files, files_hdr->offFile);
                if (!CV_IS_INSIDE(chksms, CV_RECORD_GAP(hdr_files, hdr_files->cbLen)))
                {
                    WARN("Corrupt PDB file: offset in CHKSMS subsection is invalid\n");
                    break;
                }
                source = source_new(msc_dbg->module, NULL, pdb_get_string_table_entry(cvmod->strimage, chksms->strOffset));
                lineblk_base = codeview_get_address(msc_dbg, lines_hdr->segCon, lines_hdr->offCon);
                lines = CV_RECORD_AFTER(files_hdr);
                for (i = 0; i < files_hdr->nLines; i++)
                {
                    func = (struct symt_function*)symt_find_symbol_at(msc_dbg->module, lineblk_base + lines[i].offset);
                    /* FIXME: at least labels support line numbers */
                    if (!symt_check_tag(&func->symt, SymTagFunction) && !symt_check_tag(&func->symt, SymTagInlineSite))
                    {
                        WARN("--not a func at %04x:%08x %Ix tag=%d\n",
                             lines_hdr->segCon, lines_hdr->offCon + lines[i].offset, lineblk_base + lines[i].offset, func ? func->symt.tag : -1);
                        continue;
                    }
                    symt_add_func_line(msc_dbg->module, func, source,
                                       lines[i].linenumStart, lineblk_base + lines[i].offset);
                }
                break;
            case DEBUG_S_FILECHKSMS: /* skip */
                break;
            default:
                break;
            }
        }
        hdr = hdr_next;
    }
    return TRUE;
}

/*========================================================================
 * Process CodeView symbol information.
 */

static unsigned int codeview_map_offset(const struct msc_debug_info* msc_dbg,
                                        unsigned int offset)
{
    int                 nomap = msc_dbg->nomap;
    const OMAP*         omapp = msc_dbg->omapp;
    int                 i;

    if (!nomap || !omapp) return offset;

    /* FIXME: use binary search */
    for (i = 0; i < nomap - 1; i++)
        if (omapp[i].rva <= offset && omapp[i+1].rva > offset)
            return !omapp[i].rvaTo ? 0 : omapp[i].rvaTo + (offset - omapp[i].rva);

    return 0;
}

static ULONG_PTR codeview_get_address(const struct msc_debug_info* msc_dbg,
                                          unsigned seg, unsigned offset)
{
    int			        nsect = msc_dbg->nsect;
    const IMAGE_SECTION_HEADER* sectp = msc_dbg->sectp;

    if (!seg || seg > nsect) return 0;
    return msc_dbg->module->module.BaseOfImage +
        codeview_map_offset(msc_dbg, sectp[seg-1].VirtualAddress + offset);
}

static BOOL func_has_local(struct symt_function* func, const char* name)
{
    int i;

    for (i = 0; i < func->vchildren.num_elts; ++i)
    {
        struct symt* p = *(struct symt**)vector_at(&func->vchildren, i);
        if (symt_check_tag(p, SymTagData) && !strcmp(((struct symt_data*)p)->hash_elt.name, name))
            return TRUE;
    }
    return FALSE;
}

static const union codeview_symbol* get_next_sym(const union codeview_symbol* sym)
{
    return (const union codeview_symbol*)((const char*)sym + sym->generic.len + 2);
}

static inline void codeview_add_variable(const struct msc_debug_info* msc_dbg,
                                         struct symt_compiland* compiland,
                                         struct symt_function* func,
                                         struct symt_block* block,
                                         const char* name,
                                         unsigned segment, unsigned offset,
                                         unsigned symtype, BOOL is_local, BOOL in_tls, BOOL dontcheck)
{
    if (name && *name)
    {
        struct location loc;

        loc.kind = in_tls ? loc_tlsrel : loc_absolute;
        loc.reg = 0;
        loc.offset = in_tls ? offset : codeview_get_address(msc_dbg, segment, offset);
        if (func)
        {
            if (!is_local || in_tls) WARN("Unsupported construct\n");
            symt_add_func_local(msc_dbg->module, func, DataIsStaticLocal, &loc, block,
                                symt_ptr_to_symref(codeview_get_type(symtype, FALSE)), name);
            return;
        }
        if (!dontcheck && !in_tls)
        {
            /* Check that we don't add twice the same variable */
            struct hash_table_iter      hti;
            void*                       ptr;
            struct symt_ht*             sym;

            hash_table_iter_init(&msc_dbg->module->ht_symbols, &hti, name);
            while ((ptr = hash_table_iter_up(&hti)))
            {
                sym = CONTAINING_RECORD(ptr, struct symt_ht, hash_elt);
                if (symt_check_tag(&sym->symt, SymTagData) && !strcmp(sym->hash_elt.name, name))
                {
                    struct symt_data* symdata = (struct symt_data*)&sym->symt;
                    if (symdata->kind == (is_local ? DataIsFileStatic : DataIsGlobal) &&
                        symdata->u.var.kind == loc.kind &&
                        symdata->u.var.offset == loc.offset &&
                        symdata->container == &compiland->symt)
                    {
                        /* We don't compare types yet... Unfortunately, they are not
                         * always the same typeid... it'd require full type equivalence
                         * (eg: we've seen 'int* foo' <> 'int[4] foo')
                         */
                        return;
                    }
                }
            }
        }
        if (is_local ^ (compiland != NULL)) FIXME("Unsupported construct\n");
        symt_new_global_variable(msc_dbg->module, compiland, name, is_local, loc, 0,
                                 symt_ptr_to_symref(codeview_get_type(symtype, FALSE)));
     }
}

struct cv_local_info
{
    unsigned short      kind; /* the S_DEFRANGE* */
    unsigned short      ngaps; /* number of gaps */
    unsigned short      reg;
    unsigned short      rangelen; /* after start */
    short               offset;
    DWORD_PTR           start;
    struct cv_addr_gap  gaps[0];
};

static const struct cv_addr_gap* codeview_get_gaps(const union codeview_symbol* symrange)
{
    const struct cv_addr_gap* gap;
    switch (symrange->generic.id)
    {
    case S_DEFRANGE: gap = symrange->defrange_v3.gaps; break;
    case S_DEFRANGE_SUBFIELD: gap = symrange->defrange_subfield_v3.gaps; break;
    case S_DEFRANGE_REGISTER: gap = symrange->defrange_register_v3.gaps; break;
    case S_DEFRANGE_FRAMEPOINTER_REL: gap = symrange->defrange_frameptrrel_v3.gaps; break;
    case S_DEFRANGE_SUBFIELD_REGISTER: gap = symrange->defrange_subfield_register_v3.gaps; break;
    case S_DEFRANGE_REGISTER_REL: gap = symrange->defrange_registerrel_v3.gaps; break;
        /* no gaps for that one */
    case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
    default: return NULL;
    }
    return gap != (const struct cv_addr_gap*)get_next_sym(symrange) ? gap : NULL;
}

static void codeview_xform_range(const struct msc_debug_info* msc_dbg,
                                 struct cv_local_info* locinfo,
                                 const struct cv_addr_range* adrange)
{
    locinfo->start = codeview_get_address(msc_dbg, adrange->isectStart, adrange->offStart);
    locinfo->rangelen = adrange->cbRange;
}

static unsigned codeview_defrange_length(const union codeview_symbol* sym)
{
    const union codeview_symbol* first_symrange = get_next_sym(sym);
    const union codeview_symbol* symrange;

    for (symrange = first_symrange;
         symrange->generic.id >= S_DEFRANGE && symrange->generic.id <= S_DEFRANGE_REGISTER_REL;
         symrange = get_next_sym(symrange)) {}
    return (const char*)symrange - (const char*)first_symrange;
}

static unsigned codeview_transform_defrange(const struct msc_debug_info* msc_dbg,
                                            struct symt_function* curr_func,
                                            const union codeview_symbol* sym,
                                            struct location* loc)
{
    const union codeview_symbol* first_symrange = get_next_sym(sym);
    const union codeview_symbol* symrange;
    const struct cv_addr_gap* gap;
    unsigned len, alloc = sizeof(DWORD); /* for terminating kind = 0 */
    unsigned char* ptr;

    /* we need to transform the cv_addr_range into cv_local_info */
    for (symrange = first_symrange;
         symrange->generic.id >= S_DEFRANGE && symrange->generic.id <= S_DEFRANGE_REGISTER_REL;
         symrange = get_next_sym(symrange))
    {
        gap = codeview_get_gaps(symrange);
        alloc += sizeof(struct cv_local_info) +
            (gap ? (const char*)get_next_sym(symrange) - (const char*)gap : 0);
    }
    /* total length of all S_DEFRANGE* records (in bytes) following S_LOCAL */
    len = (const char*)symrange - (const char*)first_symrange;

    ptr = pool_alloc(&msc_dbg->module->pool, alloc);
    if (ptr)
    {
        struct cv_local_info* locinfo = (struct cv_local_info*)ptr;

        loc->kind = loc_cv_local_range;
        loc->offset = (DWORD_PTR)ptr;
        /* transform the cv_addr_range into cv_local_info */
        for (symrange = first_symrange;
             symrange->generic.id >= S_DEFRANGE && symrange->generic.id <= S_DEFRANGE_REGISTER_REL;
             symrange = get_next_sym(symrange))
        {
            locinfo->kind = symrange->generic.id;
            switch (symrange->generic.id)
            {
            case S_DEFRANGE:
                codeview_xform_range(msc_dbg, locinfo, &symrange->defrange_v3.range);
                /* FIXME: transformation unsupported; let loc_compute bark if actually needed */
                break;
            case S_DEFRANGE_SUBFIELD:
                codeview_xform_range(msc_dbg, locinfo, &symrange->defrange_subfield_v3.range);
                /* FIXME: transformation unsupported; let loc_compute bark if actually needed */
                break;
            case S_DEFRANGE_REGISTER:
                locinfo->reg = symrange->defrange_register_v3.reg;
                codeview_xform_range(msc_dbg, locinfo, &symrange->defrange_register_v3.range);
                break;
            case S_DEFRANGE_FRAMEPOINTER_REL:
                locinfo->offset = symrange->defrange_frameptrrel_v3.offFramePointer;
                codeview_xform_range(msc_dbg, locinfo, &symrange->defrange_frameptrrel_v3.range);
                break;
            case S_DEFRANGE_SUBFIELD_REGISTER:
                locinfo->reg = symrange->defrange_subfield_register_v3.reg;
                locinfo->offset = symrange->defrange_subfield_register_v3.offParent;
                codeview_xform_range(msc_dbg, locinfo, &symrange->defrange_subfield_register_v3.range);
                break;
            case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
                locinfo->offset = symrange->defrange_frameptr_relfullscope_v3.offFramePointer;
                locinfo->start = curr_func->ranges[0].low;
                locinfo->rangelen = addr_range_size(&curr_func->ranges[0]);
                break;
            case S_DEFRANGE_REGISTER_REL:
                locinfo->reg = symrange->defrange_registerrel_v3.baseReg;
                locinfo->offset = symrange->defrange_registerrel_v3.offBasePointer;
                codeview_xform_range(msc_dbg, locinfo, &symrange->defrange_registerrel_v3.range);
                break;
            default:
                assert(0);
            }
            gap = codeview_get_gaps(symrange);
            if (gap)
            {
                unsigned gaplen = (const char*)get_next_sym(symrange) - (const char*)gap;
                locinfo->ngaps = gaplen / sizeof(*gap);
                memcpy(locinfo->gaps, gap, gaplen);
                locinfo = (struct cv_local_info*)((char*)(locinfo + 1) + gaplen);
            }
            else
            {
                locinfo->ngaps = 0;
                locinfo++;
            }
        }
        *(DWORD*)locinfo = 0; /* store terminating kind == 0 */
    }
    else
    {
        loc->kind = loc_error;
        loc->reg = loc_err_internal;
    }
    return len;
}

static unsigned codeview_binannot_uncompress(const unsigned char** pptr)
{
    unsigned res = (unsigned)(-1);
    const unsigned char* ptr = *pptr;

    if ((*ptr & 0x80) == 0x00)
        res = (unsigned)(*ptr++);
    else if ((*ptr & 0xC0) == 0x80)
    {
        res = (unsigned)((*ptr++ & 0x3f) << 8);
        res |= *ptr++;
    }
    else if ((*ptr & 0xE0) == 0xC0)
    {
        res = (*ptr++ & 0x1f) << 24;
        res |= *ptr++ << 16;
        res |= *ptr++ << 8;
        res |= *ptr++;
    }
    else res = (unsigned)(-1);
    *pptr = ptr;
    return res;
}

struct cv_binannot
{
    const unsigned char* annot; /* current pointer */
    const unsigned char* last_annot; /* end of binary annotation stream (first byte after) */
    unsigned opcode; /* last decoded opcode */
    unsigned arg1, arg2;
};

static BOOL codeview_advance_binannot(struct cv_binannot* cvba)
{
    if (cvba->annot >= cvba->last_annot) return FALSE;
    cvba->opcode = codeview_binannot_uncompress(&cvba->annot);
    if (cvba->opcode <= BA_OP_Invalid || cvba->opcode > BA_OP_ChangeColumnEnd) return FALSE;
    if (cvba->annot >= cvba->last_annot) return FALSE;
    cvba->arg1 = codeview_binannot_uncompress(&cvba->annot);
    if (cvba->opcode == BA_OP_ChangeCodeOffsetAndLineOffset)
    {
        cvba->arg2 = cvba->arg1 >> 4;
        cvba->arg1 &= 0x0F;
    }
    else if (cvba->opcode == BA_OP_ChangeCodeLengthAndCodeOffset)
    {
        if (cvba->annot >= cvba->last_annot) return FALSE;
        cvba->arg2 = codeview_binannot_uncompress(&cvba->annot);
    }
    else cvba->arg2 = 0;
    return TRUE;
}

static inline int binannot_getsigned(unsigned i)
{
    return (i & 1) ? -(int)(i >> 1) : (int)(i >> 1);
}

static BOOL cv_dbgsubsect_find_inlinee(const struct msc_debug_info* msc_dbg,
                                       unsigned inlineeid,
                                       const struct cv_module_snarf* cvmod,
                                       const struct CV_DebugSSubsectionHeader_t* hdr_files,
                                       unsigned* srcfile, unsigned* srcline)
{
    const struct CV_DebugSSubsectionHeader_t*     hdr;
    const struct CV_DebugSSubsectionHeader_t*     next_hdr;
    const struct CV_InlineeSourceLine_t*          inlsrc;
    const struct CV_InlineeSourceLineEx_t*        inlsrcex;
    const struct CV_Checksum_t*                   chksms;

    for (hdr = cvmod->dbgsubsect; CV_IS_INSIDE(hdr, cvmod->dbgsubsect + cvmod->dbgsubsect_size); hdr = next_hdr)
    {
        next_hdr = CV_RECORD_GAP(hdr, hdr->cbLen);
        if (hdr->type != DEBUG_S_INLINEELINES) continue;
        /* subsection starts with a DWORD signature */
        switch (*(DWORD*)CV_RECORD_AFTER(hdr))
        {
        case CV_INLINEE_SOURCE_LINE_SIGNATURE:
            inlsrc = CV_RECORD_GAP(hdr, sizeof(DWORD));
            while (CV_IS_INSIDE(inlsrc, next_hdr))
            {
                if (inlsrc->inlinee == inlineeid)
                {
                    chksms = CV_RECORD_GAP(hdr_files, inlsrc->fileId);
                    if (!CV_IS_INSIDE(chksms, CV_RECORD_GAP(hdr_files, hdr_files->cbLen))) return FALSE;
                    *srcfile = source_new(msc_dbg->module, NULL, pdb_get_string_table_entry(cvmod->strimage, chksms->strOffset));
                    *srcline = inlsrc->sourceLineNum;
                    return TRUE;
                }
                ++inlsrc;
            }
            break;
        case CV_INLINEE_SOURCE_LINE_SIGNATURE_EX:
            inlsrcex = CV_RECORD_GAP(hdr, sizeof(DWORD));
            while (CV_IS_INSIDE(inlsrcex, next_hdr))
            {
                if (inlsrcex->inlinee == inlineeid)
                {
                    chksms = CV_RECORD_GAP(hdr_files, inlsrcex->fileId);
                    if (!CV_IS_INSIDE(chksms, CV_RECORD_GAP(hdr_files, hdr_files->cbLen))) return FALSE;
                    *srcfile = source_new(msc_dbg->module, NULL, pdb_get_string_table_entry(cvmod->strimage, chksms->strOffset));
                    *srcline = inlsrcex->sourceLineNum;
                    return TRUE;
                }
                inlsrcex = CV_RECORD_GAP(inlsrcex, inlsrcex->countOfExtraFiles * sizeof(inlsrcex->extraFileId[0]));
            }
            break;
        default:
            FIXME("Unknown signature %lx in INLINEELINES subsection\n", *(DWORD*)CV_RECORD_AFTER(hdr));
            break;
        }
    }
    return FALSE;
}

static inline void inline_site_update_last_range(struct symt_function* inlined, unsigned index, ULONG_PTR hi)
{
    if (index && index <= inlined->num_ranges)
    {
        struct addr_range* range = &inlined->ranges[index - 1];
        /* only change range if it has no span (code start without code end) */
        if (range->low == range->high)
            range->high = hi;
    }
}

static unsigned inline_site_get_num_ranges(const unsigned char* annot,
                                           const unsigned char* last_annot)
{
    struct cv_binannot cvba;
    unsigned num_ranges = 0;

    cvba.annot = annot;
    cvba.last_annot = last_annot;

    while (codeview_advance_binannot(&cvba))
    {
        switch (cvba.opcode)
        {
        case BA_OP_CodeOffset:
        case BA_OP_ChangeCodeLength:
        case BA_OP_ChangeFile:
        case BA_OP_ChangeLineOffset:
            break;
        case BA_OP_ChangeCodeOffset:
        case BA_OP_ChangeCodeOffsetAndLineOffset:
        case BA_OP_ChangeCodeLengthAndCodeOffset:
            num_ranges++;
            break;
        default:
            WARN("Unsupported op %d\n", cvba.opcode);
            break;
        }
    }
    return num_ranges;
}

static struct symt_function* codeview_create_inline_site(const struct msc_debug_info* msc_dbg,
                                                         const struct cv_module_snarf* cvmod,
                                                         struct symt_function* top_func,
                                                         struct symt* container,
                                                         cv_itemid_t inlinee,
                                                         const unsigned char* annot,
                                                         const unsigned char* last_annot)
{
    const struct CV_DebugSSubsectionHeader_t* hdr_files = NULL;
    const union codeview_type* cvt;
    struct symt_function* inlined;
    struct cv_binannot cvba;
    BOOL srcok;
    unsigned num_ranges;
    unsigned offset, index, line, srcfile;
    const struct CV_Checksum_t* chksms;

    if (!cvmod->ipi_ctp || !(cvt = codeview_jump_to_type(cvmod->ipi_ctp, inlinee)))
    {
        FIXME("Couldn't find type %x in IPI stream\n", inlinee);
        return NULL;
    }
    num_ranges = inline_site_get_num_ranges(annot, last_annot);
    if (!num_ranges) return NULL;

    switch (cvt->generic.id)
    {
    case LF_FUNC_ID:
        inlined = symt_new_inlinesite(msc_dbg->module, top_func, container,
                                      cvt->func_id_v3.name,
                                      symt_ptr_to_symref(codeview_get_type(cvt->func_id_v3.type, FALSE)),
                                      num_ranges);
        break;
    case LF_MFUNC_ID:
        /* FIXME we just declare a function, not a method */
        inlined = symt_new_inlinesite(msc_dbg->module, top_func, container,
                                      cvt->mfunc_id_v3.name,
                                      symt_ptr_to_symref(codeview_get_type(cvt->mfunc_id_v3.type, FALSE)),
                                      num_ranges);
        break;
    default:
        FIXME("unsupported inlinee kind %x\n", cvt->generic.id);
        return NULL;
    }

    for (hdr_files = cvmod->dbgsubsect;
         CV_IS_INSIDE(hdr_files, cvmod->dbgsubsect + cvmod->dbgsubsect_size);
         hdr_files = CV_RECORD_GAP(hdr_files, hdr_files->cbLen))
    {
        if (hdr_files->type == DEBUG_S_FILECHKSMS)
            break;
    }
    if (!hdr_files) return FALSE;
    srcok = cv_dbgsubsect_find_inlinee(msc_dbg, inlinee, cvmod, hdr_files, &srcfile, &line);

    if (!srcok)
        srcfile = line = 0;

    /* rescan all annotations and store ranges & line information */
    offset = 0;
    index = 0;
    cvba.annot = annot;
    cvba.last_annot = last_annot;

    while (codeview_advance_binannot(&cvba))
    {
        switch (cvba.opcode)
        {
        case BA_OP_CodeOffset:
            offset = cvba.arg1;
            break;
        case BA_OP_ChangeCodeOffset:
            offset += cvba.arg1;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            if (srcok)
                symt_add_func_line(msc_dbg->module, inlined, srcfile, line, top_func->ranges[0].low + offset);
            inlined->ranges[index  ].low = top_func->ranges[0].low + offset;
            inlined->ranges[index++].high = top_func->ranges[0].low + offset;
            break;
        case BA_OP_ChangeCodeLength:
            /* this op isn't widely used by MSVC, but clang uses it a lot... */
            offset += cvba.arg1;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            break;
        case BA_OP_ChangeFile:
            chksms = CV_RECORD_GAP(hdr_files, cvba.arg1);
            if (CV_IS_INSIDE(chksms, CV_RECORD_GAP(hdr_files, hdr_files->cbLen)))
                srcfile = source_new(msc_dbg->module, NULL, pdb_get_string_table_entry(cvmod->strimage, chksms->strOffset));
            break;
        case BA_OP_ChangeLineOffset:
            line += binannot_getsigned(cvba.arg1);
            break;
        case BA_OP_ChangeCodeOffsetAndLineOffset:
            line += binannot_getsigned(cvba.arg2);
            offset += cvba.arg1;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            if (srcok)
                symt_add_func_line(msc_dbg->module, inlined, srcfile, line, top_func->ranges[0].low + offset);
            inlined->ranges[index  ].low = top_func->ranges[0].low + offset;
            inlined->ranges[index++].high = top_func->ranges[0].low + offset;
            break;
        case BA_OP_ChangeCodeLengthAndCodeOffset:
            offset += cvba.arg2;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            if (srcok)
                symt_add_func_line(msc_dbg->module, inlined, srcfile, line, top_func->ranges[0].low + offset);
            inlined->ranges[index  ].low = top_func->ranges[0].low + offset;
            inlined->ranges[index++].high = top_func->ranges[0].low + offset + cvba.arg1;
            break;
        default:
            WARN("Unsupported op %d\n", cvba.opcode);
            break;
        }
    }
    if (index != num_ranges) /* sanity check */
        FIXME("Internal logic error\n");
    if (inlined->num_ranges)
    {
        struct addr_range* range = &inlined->ranges[inlined->num_ranges - 1];
        if (range->low == range->high) WARN("pending empty range at end of %s inside %s\n",
                                            debugstr_a(inlined->hash_elt.name),
                                            debugstr_a(top_func->hash_elt.name));
    }
    return inlined;
}

static struct symt_compiland* codeview_new_compiland(const struct msc_debug_info* msc_dbg, const char* objname)
{
    unsigned int    src_idx = source_new(msc_dbg->module, NULL, objname);
    unsigned int    i;

    /* In some cases MSVC generates several compiland entries with same pathname in PDB file.
     * (for example: for an import library, one compiland entry per imported function is generated).
     * But native dbghelp (in this case) merges in a single compiland instance.
     */
    for (i = 0; i < msc_dbg->module->top->vchildren.num_elts; i++)
    {
        struct symt_compiland** p = vector_at(&msc_dbg->module->top->vchildren, i);
        if (symt_check_tag(&(*p)->symt, SymTagCompiland) && (*p)->source == src_idx)
            return *p;
    }
    return symt_new_compiland(msc_dbg->module, src_idx);
}

static BOOL codeview_snarf(const struct msc_debug_info* msc_dbg,
                           unsigned stream_id,
                           const BYTE* root, unsigned offset, unsigned size,
                           const struct cv_module_snarf* cvmod,
                           const char* objname)
{
    struct symt_function*               top_func = NULL;
    struct symt_function*               curr_func = NULL;
    struct symt*                        func_signature;
    int                                 i, length;
    struct symt_block*                  block = NULL;
    struct symt*                        symt;
    struct symt_compiland*              compiland = NULL;
    struct location                     loc;
    unsigned                            top_frame_size = -1;

    /* overwrite compiland name from outer context (if any) */
    if (objname)
        compiland = codeview_new_compiland(msc_dbg, objname);
    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    for (i = offset; i < size; i += length)
    {
        const union codeview_symbol* sym = (const union codeview_symbol*)(root + i);
        length = sym->generic.len + 2;
        if (i + length > size) break;
        if (!sym->generic.id || length < 4) break;
        if (length & 3) FIXME("unpadded len %u\n", length);

        switch (sym->generic.id)
        {
        /*
         * Global and local data symbols.  We don't associate these
         * with any given source file.
         */
	case S_GDATA32_16t:
	case S_LDATA32_16t:
            codeview_add_variable(msc_dbg, compiland, curr_func, block, terminate_string(&sym->data_v1.p_name),
                                  sym->data_v1.segment, sym->data_v1.offset, sym->data_v1.symtype,
                                  sym->generic.id == S_LDATA32_16t, FALSE, TRUE);
	    break;
	case S_GDATA32_ST:
	case S_LDATA32_ST:
            codeview_add_variable(msc_dbg, compiland, curr_func, block, terminate_string(&sym->data_v2.p_name),
                                  sym->data_v2.segment, sym->data_v2.offset, sym->data_v2.symtype,
                                  sym->generic.id == S_LDATA32_ST, FALSE, TRUE);
	    break;
	case S_GDATA32:
	case S_LDATA32:
            codeview_add_variable(msc_dbg, compiland, curr_func, block, sym->data_v3.name,
                                  sym->data_v3.segment, sym->data_v3.offset, sym->data_v3.symtype,
                                  sym->generic.id == S_LDATA32, FALSE, TRUE);
	    break;

        /* variables with thread storage */
	case S_GTHREAD32_16t:
	case S_LTHREAD32_16t:
            codeview_add_variable(msc_dbg, compiland, curr_func, block, terminate_string(&sym->thread_v1.p_name),
                                  sym->thread_v1.segment, sym->thread_v1.offset, sym->thread_v1.symtype,
                                  sym->generic.id == S_LTHREAD32_16t, TRUE, TRUE);
	    break;
	case S_GTHREAD32_ST:
	case S_LTHREAD32_ST:
            codeview_add_variable(msc_dbg, compiland, curr_func, block, terminate_string(&sym->thread_v2.p_name),
                                  sym->thread_v2.segment, sym->thread_v2.offset, sym->thread_v2.symtype,
                                  sym->generic.id == S_LTHREAD32_ST, TRUE, TRUE);
	    break;
	case S_GTHREAD32:
	case S_LTHREAD32:
            codeview_add_variable(msc_dbg, compiland, curr_func, block, sym->thread_v3.name,
                                  sym->thread_v3.segment, sym->thread_v3.offset, sym->thread_v3.symtype,
                                  sym->generic.id == S_LTHREAD32, TRUE, TRUE);
	    break;

        /* Public symbols */
	case S_PUB32_16t:
	case S_PUB32_ST:
        case S_PUB32:
        case S_PROCREF:
        case S_LPROCREF:
            /* will be handled later on in codeview_snarf_public */
            break;

        /*
         * Sort of like a global function, but it just points
         * to a thunk, which is a stupid name for what amounts to
         * a PLT slot in the normal jargon that everyone else uses.
         */
	case S_THUNK32_ST:
            symt_new_thunk(msc_dbg->module, compiland,
                           terminate_string(&sym->thunk_v1.p_name), sym->thunk_v1.thtype,
                           codeview_get_address(msc_dbg, sym->thunk_v1.segment, sym->thunk_v1.offset),
                           sym->thunk_v1.thunk_len);
	    break;
	case S_THUNK32:
            symt_new_thunk(msc_dbg->module, compiland,
                           sym->thunk_v3.name, sym->thunk_v3.thtype,
                           codeview_get_address(msc_dbg, sym->thunk_v3.segment, sym->thunk_v3.offset),
                           sym->thunk_v3.thunk_len);
	    break;

        /*
         * Global and static functions.
         */
	case S_GPROC32_16t:
	case S_LPROC32_16t:
            if (top_func) FIXME("nested function\n");
            func_signature = codeview_get_type(sym->proc_v1.proctype, FALSE);
            if (!func_signature || func_signature->tag == SymTagFunctionType)
            {
                top_func = symt_new_function(msc_dbg->module, compiland,
                                             terminate_string(&sym->proc_v1.p_name),
                                             codeview_get_address(msc_dbg, sym->proc_v1.segment, sym->proc_v1.offset),
                                             sym->proc_v1.proc_len,
                                             symt_ptr_to_symref(func_signature));
                curr_func = top_func;
                loc.kind = loc_absolute;
                loc.offset = sym->proc_v1.debug_start;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, &loc, NULL);
                loc.offset = sym->proc_v1.debug_end;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, &loc, NULL);
            }
	    break;
	case S_GPROC32_ST:
	case S_LPROC32_ST:
            if (top_func) FIXME("nested function\n");
            func_signature = codeview_get_type(sym->proc_v2.proctype, FALSE);
            if (!func_signature || func_signature->tag == SymTagFunctionType)
            {
                top_func = symt_new_function(msc_dbg->module, compiland,
                                             terminate_string(&sym->proc_v2.p_name),
                                             codeview_get_address(msc_dbg, sym->proc_v2.segment, sym->proc_v2.offset),
                                             sym->proc_v2.proc_len,
                                             symt_ptr_to_symref(func_signature));
                curr_func = top_func;
                loc.kind = loc_absolute;
                loc.offset = sym->proc_v2.debug_start;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, &loc, NULL);
                loc.offset = sym->proc_v2.debug_end;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, &loc, NULL);
            }
	    break;
	case S_GPROC32:
	case S_LPROC32:
            if (top_func) FIXME("nested function\n");
            func_signature = codeview_get_type(sym->proc_v3.proctype, FALSE);
            if (!func_signature || func_signature->tag == SymTagFunctionType)
            {
                top_func = symt_new_function(msc_dbg->module, compiland,
                                             sym->proc_v3.name,
                                             codeview_get_address(msc_dbg, sym->proc_v3.segment, sym->proc_v3.offset),
                                             sym->proc_v3.proc_len,
                                             symt_ptr_to_symref(func_signature));
                curr_func = top_func;
                loc.kind = loc_absolute;
                loc.offset = sym->proc_v3.debug_start;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, &loc, NULL);
                loc.offset = sym->proc_v3.debug_end;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, &loc, NULL);
            }
	    break;
        /*
         * Function parameters and stack variables.
         */
	case S_BPREL32_16t:
            loc.kind = loc_regrel;
            /* Yes, it's i386 dependent, but that's the symbol purpose. S_REGREL is used on other CPUs */
            loc.reg = CV_REG_EBP;
            loc.offset = sym->stack_v1.offset;
            symt_add_func_local(msc_dbg->module, curr_func,
                                sym->stack_v1.offset > 0 ? DataIsParam : DataIsLocal,
                                &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->stack_v1.symtype, FALSE)),
                                terminate_string(&sym->stack_v1.p_name));
            break;
	case S_BPREL32_ST:
            loc.kind = loc_regrel;
            /* Yes, it's i386 dependent, but that's the symbol purpose. S_REGREL is used on other CPUs */
            loc.reg = CV_REG_EBP;
            loc.offset = sym->stack_v2.offset;
            symt_add_func_local(msc_dbg->module, curr_func,
                                sym->stack_v2.offset > 0 ? DataIsParam : DataIsLocal,
                                &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->stack_v2.symtype, FALSE)),
                                terminate_string(&sym->stack_v2.p_name));
            break;
	case S_BPREL32:
            /* S_BPREL32 can be present after S_LOCAL; prefer S_LOCAL when present */
            if (func_has_local(curr_func, sym->stack_v3.name)) break;
            loc.kind = loc_regrel;
            /* Yes, it's i386 dependent, but that's the symbol purpose. S_REGREL is used on other CPUs */
            loc.reg = CV_REG_EBP;
            loc.offset = sym->stack_v3.offset;
            symt_add_func_local(msc_dbg->module, curr_func,
                                sym->stack_v3.offset > 0 ? DataIsParam : DataIsLocal,
                                &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->stack_v3.symtype, FALSE)),
                                sym->stack_v3.name);
            break;
	case S_REGREL32:
            /* S_REGREL32 can be present after S_LOCAL; prefer S_LOCAL when present */
            if (func_has_local(curr_func, sym->regrel_v3.name)) break;
            loc.kind = loc_regrel;
            loc.reg = sym->regrel_v3.reg;
            loc.offset = sym->regrel_v3.offset;
            if (top_frame_size == -1) FIXME("S_REGREL32 without frameproc\n");
            symt_add_func_local(msc_dbg->module, curr_func,
                                sym->regrel_v3.offset >= top_frame_size ? DataIsParam : DataIsLocal,
                                &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->regrel_v3.symtype, FALSE)),
                                sym->regrel_v3.name);
            break;

        case S_REGISTER_16t:
            loc.kind = loc_register;
            loc.reg = sym->register_v1.reg;
            loc.offset = 0;
            symt_add_func_local(msc_dbg->module, curr_func,
                                DataIsLocal, &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->register_v1.type, FALSE)),
                                terminate_string(&sym->register_v1.p_name));
            break;
        case S_REGISTER_ST:
            loc.kind = loc_register;
            loc.reg = sym->register_v2.reg;
            loc.offset = 0;
            symt_add_func_local(msc_dbg->module, curr_func,
                                DataIsLocal, &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->register_v2.type, FALSE)),
                                terminate_string(&sym->register_v2.p_name));
            break;
        case S_REGISTER:
            /* S_REGISTER can be present after S_LOCAL; prefer S_LOCAL when present */
            if (func_has_local(curr_func, sym->register_v3.name)) break;
            loc.kind = loc_register;
            loc.reg = sym->register_v3.reg;
            loc.offset = 0;
            symt_add_func_local(msc_dbg->module, curr_func,
                                DataIsLocal, &loc, block,
                                symt_ptr_to_symref(codeview_get_type(sym->register_v3.type, FALSE)),
                                sym->register_v3.name);
            break;

        case S_BLOCK32_ST:
            block = symt_open_func_block(msc_dbg->module, curr_func, block, 1);
            block->ranges[0].low = codeview_get_address(msc_dbg, sym->block_v1.segment, sym->block_v1.offset);
            block->ranges[0].high = block->ranges[0].low + sym->block_v1.length;
            break;
        case S_BLOCK32:
            block = symt_open_func_block(msc_dbg->module, curr_func, block, 1);
            block->ranges[0].low = codeview_get_address(msc_dbg, sym->block_v3.segment, sym->block_v3.offset);
            block->ranges[0].high = block->ranges[0].low + sym->block_v3.length;
            break;

        case S_END:
            if (block)
            {
                block = symt_close_func_block(msc_dbg->module, curr_func, block);
            }
            else if (top_func)
            {
                if (curr_func != top_func) FIXME("shouldn't close a top function with an opened inlined function\n");
                top_func = curr_func = NULL;
                top_frame_size = -1;
            }
            break;

        case S_COMPILE:
            TRACE("S-Compile-V1 machine:%x language:%x %s\n",
                  sym->compile_v1.machine, sym->compile_v1.flags.language, terminate_string(&sym->compile_v1.p_name));
            break;

        case S_COMPILE2_ST:
            TRACE("S-Compile-V2 machine:%x language:%x %s\n",
                  sym->compile2_v2.machine, sym->compile2_v2.flags.iLanguage, terminate_string(&sym->compile2_v2.p_name));
            break;

        case S_COMPILE2:
            TRACE("S-Compile-V3 machine:%x language:%x %s\n",
                  sym->compile2_v3.machine, sym->compile2_v3.flags.iLanguage, debugstr_a(sym->compile2_v3.name));
            break;

        case S_COMPILE3:
            TRACE("S-Compile3-V3 machine:%x language:%x %s\n",
                  sym->compile3_v3.machine, sym->compile3_v3.flags.iLanguage, debugstr_a(sym->compile3_v3.name));
            break;

        case S_ENVBLOCK:
            break;

        case S_OBJNAME:
            TRACE("S-ObjName-V3 %s\n", debugstr_a(sym->objname_v3.name));
            if (!compiland)
                compiland = codeview_new_compiland(msc_dbg, sym->objname_v3.name);
            break;

        case S_OBJNAME_ST:
            TRACE("S-ObjName-V1 %s\n", terminate_string(&sym->objname_v1.p_name));
            if (!compiland)
                compiland = codeview_new_compiland(msc_dbg, terminate_string(&sym->objname_v1.p_name));
            break;

        case S_LABEL32_ST:
            if (curr_func)
            {
                loc.kind = loc_absolute;
                loc.offset = codeview_get_address(msc_dbg, sym->label_v1.segment, sym->label_v1.offset) - curr_func->ranges[0].low;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagLabel, &loc,
                                        terminate_string(&sym->label_v1.p_name));
            }
            else symt_new_label(msc_dbg->module, compiland,
                                terminate_string(&sym->label_v1.p_name),
                                codeview_get_address(msc_dbg, sym->label_v1.segment, sym->label_v1.offset));
            break;
        case S_LABEL32:
            if (curr_func)
            {
                loc.kind = loc_absolute;
                loc.offset = codeview_get_address(msc_dbg, sym->label_v3.segment, sym->label_v3.offset) - curr_func->ranges[0].low;
                symt_add_function_point(msc_dbg->module, curr_func, SymTagLabel,
                                        &loc, sym->label_v3.name);
            }
            else symt_new_label(msc_dbg->module, compiland, sym->label_v3.name,
                                codeview_get_address(msc_dbg, sym->label_v3.segment, sym->label_v3.offset));
            break;

        case S_CONSTANT_16t:
            {
                int                     vlen;
                const struct p_string*  name;
                struct symt*            se;
                VARIANT                 v;

                vlen = leaf_as_variant(&v, sym->constant_v1.data);
                name = (const struct p_string*)&sym->constant_v1.data[vlen];
                se = codeview_get_type(sym->constant_v1.type, FALSE);

                TRACE("S-Constant-V1 %u %s %x\n", V_INT(&v), terminate_string(name), sym->constant_v1.type);
                symt_new_constant(msc_dbg->module, compiland, terminate_string(name),
                                  symt_ptr_to_symref(se), &v);
            }
            break;
        case S_CONSTANT_ST:
            {
                int                     vlen;
                const struct p_string*  name;
                struct symt*            se;
                VARIANT                 v;

                vlen = leaf_as_variant(&v, sym->constant_v2.data);
                name = (const struct p_string*)&sym->constant_v2.data[vlen];
                se = codeview_get_type(sym->constant_v2.type, FALSE);

                TRACE("S-Constant-V2 %u %s %x\n", V_INT(&v), terminate_string(name), sym->constant_v2.type);
                symt_new_constant(msc_dbg->module, compiland, terminate_string(name),
                                  symt_ptr_to_symref(se), &v);
            }
            break;
        case S_CONSTANT:
            {
                int                     vlen;
                const char*             name;
                struct symt*            se;
                VARIANT                 v;

                vlen = leaf_as_variant(&v, sym->constant_v3.data);
                name = (const char*)&sym->constant_v3.data[vlen];
                se = codeview_get_type(sym->constant_v3.type, FALSE);

                TRACE("S-Constant-V3 %u %s %x\n", V_INT(&v), debugstr_a(name), sym->constant_v3.type);
                /* FIXME: we should add this as a constant value */
                symt_new_constant(msc_dbg->module, compiland, name, symt_ptr_to_symref(se), &v);
            }
            break;

        case S_UDT_16t:
            if (sym->udt_v1.type)
            {
                if ((symt = codeview_get_type(sym->udt_v1.type, FALSE)))
                    symt_new_typedef(msc_dbg->module, symt,
                                     terminate_string(&sym->udt_v1.p_name));
                else
                    FIXME("S-Udt %s: couldn't find type 0x%x\n",
                          terminate_string(&sym->udt_v1.p_name), sym->udt_v1.type);
            }
            break;
        case S_UDT_ST:
            if (sym->udt_v2.type)
            {
                if ((symt = codeview_get_type(sym->udt_v2.type, FALSE)))
                    symt_new_typedef(msc_dbg->module, symt,
                                     terminate_string(&sym->udt_v2.p_name));
                else
                    FIXME("S-Udt %s: couldn't find type 0x%x\n",
                          terminate_string(&sym->udt_v2.p_name), sym->udt_v2.type);
            }
            break;
        case S_UDT:
            if (sym->udt_v3.type)
            {
                if ((symt = codeview_get_type(sym->udt_v3.type, FALSE)))
                    symt_new_typedef(msc_dbg->module, symt, sym->udt_v3.name);
                else
                    FIXME("S-Udt %s: couldn't find type 0x%x\n",
                          debugstr_a(sym->udt_v3.name), sym->udt_v3.type);
            }
            break;
        case S_LOCAL:
            /* FIXME: don't store global/static variables accessed through registers... we don't support that
             * in locals... anyway, global data record should be present as well (so the variable will be available
             * through the global definition, but potentially not updated)
             */
            if (!sym->local_v3.varflags.enreg_global && !sym->local_v3.varflags.enreg_static)
            {
                if (stream_id)
                {
                    loc.kind = loc_user + 1 /* loc_cv_defrange for new PDB reader (see pdb.c) */;
                    loc.reg = stream_id;
                    loc.offset = (const BYTE*)sym - root;
                    length += codeview_defrange_length(sym);
                }
                else
                    length += codeview_transform_defrange(msc_dbg, curr_func, sym, &loc);
                symt_add_func_local(msc_dbg->module, curr_func,
                                    sym->local_v3.varflags.is_param ? DataIsParam : DataIsLocal,
                                    &loc, block,
                                    symt_ptr_to_symref(codeview_get_type(sym->local_v3.symtype, FALSE)),
                                    sym->local_v3.name);
            }
            else
                length += codeview_defrange_length(sym);
            break;
        case S_INLINESITE:
            {
                struct symt_function* inlined = codeview_create_inline_site(msc_dbg, cvmod, top_func,
                                                                            block ? &block->symt : &curr_func->symt,
                                                                            sym->inline_site_v3.inlinee,
                                                                            sym->inline_site_v3.binaryAnnotations,
                                                                            (const unsigned char*)sym + length);
                if (inlined)
                {
                    curr_func = inlined;
                    block = NULL;
                }
                else
                {
                    /* skip all records until paired S_INLINESITE_END */
                    sym = (const union codeview_symbol*)(root + sym->inline_site_v3.pEnd);
                    if (sym->generic.id != S_INLINESITE_END) FIXME("complete wreckage\n");
                    length = sym->inline_site_v3.pEnd - i + sym->generic.len;
                }
            }
            break;
        case S_INLINESITE2:
            {
                struct symt_function* inlined = codeview_create_inline_site(msc_dbg, cvmod, top_func,
                                                                            block ? &block->symt : &curr_func->symt,
                                                                            sym->inline_site2_v3.inlinee,
                                                                            sym->inline_site2_v3.binaryAnnotations,
                                                                            (const unsigned char*)sym + length);
                if (inlined)
                {
                    curr_func = inlined;
                    block = NULL;
                }
                else
                {
                    /* skip all records until paired S_INLINESITE_END */
                    sym = (const union codeview_symbol*)(root + sym->inline_site2_v3.pEnd);
                    if (sym->generic.id != S_INLINESITE_END) FIXME("complete wreckage\n");
                    length = sym->inline_site2_v3.pEnd - i + sym->generic.len;
                }
            }
            break;

        case S_INLINESITE_END:
            block = symt_check_tag(curr_func->container, SymTagBlock) ?
                (struct symt_block*)curr_func->container : NULL;
            curr_func = (struct symt_function*)symt_get_upper_inlined(curr_func);
            break;

         /*
         * These are special, in that they are always followed by an
         * additional length-prefixed string which is *not* included
         * into the symbol length count.  We need to skip it.
         */
	case S_PROCREF_ST:
	case S_DATAREF_ST:
	case S_LPROCREF_ST:
            {
                const char* name;

                name = (const char*)sym + length;
                length += (*name + 1 + 3) & ~3;
                break;
            }

        case S_SSEARCH:
            TRACE("Start search: seg=0x%x at offset 0x%08x\n",
                  sym->ssearch_v1.segment, sym->ssearch_v1.offset);
            break;

        case S_ALIGN:
            TRACE("S-Align V1\n");
            break;
        case S_HEAPALLOCSITE:
            TRACE("S-heap site V3: offset=0x%08x at sect_idx 0x%04x, inst_len 0x%08x, index 0x%08x\n",
                  sym->heap_alloc_site_v3.offset, sym->heap_alloc_site_v3.sect_idx,
                  sym->heap_alloc_site_v3.inst_len, sym->heap_alloc_site_v3.index);
            break;

        case S_SEPCODE:
            if (!top_func)
            {
                ULONG_PTR parent_addr = codeview_get_address(msc_dbg, sym->sepcode_v3.sectParent, sym->sepcode_v3.offParent);
                struct symt_ht* parent = symt_find_symbol_at(msc_dbg->module, parent_addr);
                if (symt_check_tag(&parent->symt, SymTagFunction))
                {
                    struct symt_function* pfunc = (struct symt_function*)parent;
                    top_func = symt_new_function(msc_dbg->module, compiland, pfunc->hash_elt.name,
                                                 codeview_get_address(msc_dbg, sym->sepcode_v3.sect, sym->sepcode_v3.off),
                                                 sym->sepcode_v3.length, pfunc->type);
                    curr_func = top_func;
                }
                else
                    WARN("Couldn't find function referenced by S_SEPCODE at %04x:%08x\n",
                         sym->sepcode_v3.sectParent, sym->sepcode_v3.offParent);
            }
            else
                FIXME("S_SEPCODE inside top-level function %s\n", debugstr_a(top_func->hash_elt.name));
            break;

        case S_FRAMEPROC:
            /* expecting only S_FRAMEPROC once for top level functions */
            if (top_frame_size == -1 && curr_func && curr_func == top_func)
                top_frame_size = sym->frame_info_v2.sz_frame;
            else
                FIXME("Unexpected S_FRAMEPROC %d (%p %p) %x\n", top_frame_size, top_func, curr_func, i);
            break;

        /* the symbols we can safely ignore for now */
        case S_SKIP:
        case S_TRAMPOLINE:
        case S_FRAMECOOKIE:
        case S_SECTION:
        case S_COFFGROUP:
        case S_EXPORT:
        case S_CALLSITEINFO:
        case S_ARMSWITCHTABLE:
            /* even if S_LOCAL groks all the S_DEFRANGE* records following itself,
             * those kinds of records can also be present after a S_FILESTATIC record
             * so silence them until (at least) S_FILESTATIC is supported
             */
        case S_DEFRANGE_REGISTER:
        case S_DEFRANGE_FRAMEPOINTER_REL:
        case S_DEFRANGE_SUBFIELD_REGISTER:
        case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
        case S_DEFRANGE_REGISTER_REL:
        case S_BUILDINFO:
        case S_FILESTATIC:
        case S_CALLEES:
        case S_CALLERS:
        case S_UNAMESPACE:
        case S_INLINEES:
        case S_POGODATA:
            TRACE("Unsupported symbol id %x\n", sym->generic.id);
            break;

        default:
            FIXME("Unsupported symbol id %x\n", sym->generic.id);
            dump(sym, 2 + sym->generic.len);
            break;
        }
    }
    return TRUE;
}

static BOOL codeview_is_inside(const struct cv_local_info* locinfo, const struct symt_function* func, DWORD_PTR ip)
{
    unsigned i;
    /* ip must be in local_info range, but not in any of its gaps */
    if (ip < locinfo->start || ip >= locinfo->start + locinfo->rangelen) return FALSE;
    for (i = 0; i < locinfo->ngaps; ++i)
        if (func->ranges[0].low + locinfo->gaps[i].gapStartOffset <= ip &&
            ip < func->ranges[0].low + locinfo->gaps[i].gapStartOffset + locinfo->gaps[i].cbRange)
            return FALSE;
    return TRUE;
}

static void pdb_location_compute(const struct module_format* modfmt,
                                 const struct symt_function* func,
                                 struct location* loc)
{
    const struct cv_local_info* locinfo;

    switch (loc->kind)
    {
    case loc_cv_local_range:
        for (locinfo = (const struct cv_local_info*)loc->offset;
             locinfo->kind != 0;
             locinfo = (const struct cv_local_info*)((const char*)(locinfo + 1) + locinfo->ngaps * sizeof(locinfo->gaps[0])))
        {
            if (!codeview_is_inside(locinfo, func, modfmt->module->process->localscope_pc)) continue;
            switch (locinfo->kind)
            {
            case S_DEFRANGE:
            case S_DEFRANGE_SUBFIELD:
            default:
                FIXME("Unsupported defrange %d\n", locinfo->kind);
                loc->kind = loc_error;
                loc->reg = loc_err_internal;
                return;
            case S_DEFRANGE_SUBFIELD_REGISTER:
                FIXME("sub-field part not handled\n");
                /* fall through */
            case S_DEFRANGE_REGISTER:
                loc->kind = loc_register;
                loc->reg = locinfo->reg;
                return;
            case S_DEFRANGE_REGISTER_REL:
                loc->kind = loc_regrel;
                loc->reg = locinfo->reg;
                loc->offset = locinfo->offset;
                return;
            case S_DEFRANGE_FRAMEPOINTER_REL:
            case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
                loc->kind = loc_regrel;
                loc->reg = modfmt->module->cpu->frame_regno;
                loc->offset = locinfo->offset;
                return;
            }
        }
        break;
    default: break;
    }
    loc->kind = loc_error;
    loc->reg = loc_err_internal;
}

static void* pdb_read_stream(const struct pdb_file_info* pdb_file, DWORD stream_nr);
static unsigned pdb_get_stream_size(const struct pdb_file_info* pdb_file, DWORD stream_nr);

static BOOL codeview_snarf_sym_hashtable(const struct msc_debug_info* msc_dbg, const BYTE* symroot, DWORD symsize,
                                         const BYTE* hashroot, DWORD hashsize,
                                         BOOL (*feed)(const struct msc_debug_info* msc_dbg, const union codeview_symbol*))
{
    const DBI_HASH_HEADER* hash_hdr = (const DBI_HASH_HEADER*)hashroot;
    unsigned num_hash_records, i;
    const DBI_HASH_RECORD* hr;

    if (hashsize < sizeof(DBI_HASH_HEADER) ||
        hash_hdr->signature != 0xFFFFFFFF ||
        hash_hdr->version != 0xeffe0000 + 19990810 ||
        (hash_hdr->hash_records_size % sizeof(DBI_HASH_RECORD)) != 0 ||
        sizeof(DBI_HASH_HEADER) + hash_hdr->hash_records_size + DBI_BITMAP_HASH_SIZE > hashsize ||
        (hashsize - (sizeof(DBI_HASH_HEADER) + hash_hdr->hash_records_size + DBI_BITMAP_HASH_SIZE)) % sizeof(unsigned))
    {
        FIXME("Incorrect hash structure\n");
        return FALSE;
    }

    hr = (DBI_HASH_RECORD*)(hash_hdr + 1);
    num_hash_records = hash_hdr->hash_records_size / sizeof(DBI_HASH_RECORD);

    /* Only iterate over the records listed in the hash table.
     * We assume that records present in stream, but not listed in hash table, are
     * invalid (and thus not loaded).
     */
    for (i = 0; i < num_hash_records; i++)
    {
        if (hr[i].offset && hr[i].offset < symsize)
        {
            const union codeview_symbol* sym = (const union codeview_symbol*)(symroot + hr[i].offset - 1);
            (*feed)(msc_dbg, sym);
        }
    }
    return TRUE;
}

static BOOL pdb_global_feed_types(const struct msc_debug_info* msc_dbg, const union codeview_symbol* sym)
{
    struct symt* symt;
    switch (sym->generic.id)
    {
    case S_UDT_16t:
        if (sym->udt_v1.type)
        {
            if ((symt = codeview_get_type(sym->udt_v1.type, FALSE)))
                symt_new_typedef(msc_dbg->module, symt,
                                 terminate_string(&sym->udt_v1.p_name));
            else
                FIXME("S-Udt %s: couldn't find type 0x%x\n",
                      terminate_string(&sym->udt_v1.p_name), sym->udt_v1.type);
        }
        break;
    case S_UDT_ST:
        if (sym->udt_v2.type)
        {
            if ((symt = codeview_get_type(sym->udt_v2.type, FALSE)))
                symt_new_typedef(msc_dbg->module, symt,
                                 terminate_string(&sym->udt_v2.p_name));
            else
                FIXME("S-Udt %s: couldn't find type 0x%x\n",
                      terminate_string(&sym->udt_v2.p_name), sym->udt_v2.type);
        }
        break;
    case S_UDT:
        if (sym->udt_v3.type)
        {
            if ((symt = codeview_get_type(sym->udt_v3.type, FALSE)))
                symt_new_typedef(msc_dbg->module, symt, sym->udt_v3.name);
            else
                FIXME("S-Udt %s: couldn't find type 0x%x\n",
                      debugstr_a(sym->udt_v3.name), sym->udt_v3.type);
        }
        break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL pdb_global_feed_variables(const struct msc_debug_info* msc_dbg, const union codeview_symbol* sym)
{
    /* The only interest here is to add global variables that haven't been seen
     * in module (=compilation unit) stream.
     * So we don't care about 'local' symbols since we cannot tell their compiland.
     */
    switch (sym->generic.id)
    {
    case S_GDATA32_16t:
        codeview_add_variable(msc_dbg, NULL, NULL, NULL, terminate_string(&sym->data_v1.p_name),
                              sym->data_v1.segment, sym->data_v1.offset, sym->data_v1.symtype,
                              FALSE, FALSE, FALSE);
        break;
    case S_GDATA32_ST:
        codeview_add_variable(msc_dbg, NULL, NULL, NULL, terminate_string(&sym->data_v2.p_name),
                              sym->data_v2.segment, sym->data_v2.offset, sym->data_v2.symtype,
                              FALSE, FALSE, FALSE);
        break;
    case S_GDATA32:
        codeview_add_variable(msc_dbg, NULL, NULL, NULL, sym->data_v3.name,
                              sym->data_v3.segment, sym->data_v3.offset, sym->data_v3.symtype,
                              FALSE, FALSE, FALSE);
        break;
    /* variables with thread storage */
    case S_GTHREAD32_16t:
        codeview_add_variable(msc_dbg, NULL, NULL, NULL, terminate_string(&sym->thread_v1.p_name),
                              sym->thread_v1.segment, sym->thread_v1.offset, sym->thread_v1.symtype,
                              FALSE, TRUE, FALSE);
        break;
    case S_GTHREAD32_ST:
        codeview_add_variable(msc_dbg, NULL, NULL, NULL, terminate_string(&sym->thread_v2.p_name),
                              sym->thread_v2.segment, sym->thread_v2.offset, sym->thread_v2.symtype,
                              FALSE, TRUE, FALSE);
        break;
    case S_GTHREAD32:
        codeview_add_variable(msc_dbg, NULL, NULL, NULL, sym->thread_v3.name,
                              sym->thread_v3.segment, sym->thread_v3.offset, sym->thread_v3.symtype,
                              FALSE, TRUE, FALSE);
        break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL pdb_global_feed_public(const struct msc_debug_info* msc_dbg, const union codeview_symbol* sym)
{
    switch (sym->generic.id)
    {
    case S_PUB32_16t:
        symt_new_public(msc_dbg->module, NULL,
                        terminate_string(&sym->public_v1.p_name),
                        sym->public_v1.pubsymflags == SYMTYPE_FUNCTION,
                        codeview_get_address(msc_dbg, sym->public_v1.segment, sym->public_v1.offset), 1);
        break;
    case S_PUB32_ST:
        symt_new_public(msc_dbg->module, NULL,
                        terminate_string(&sym->public_v2.p_name),
                        sym->public_v2.pubsymflags == SYMTYPE_FUNCTION,
                        codeview_get_address(msc_dbg, sym->public_v2.segment, sym->public_v2.offset), 1);
        break;
    case S_PUB32:
        symt_new_public(msc_dbg->module, NULL,
                        sym->public_v3.name,
                        sym->public_v3.pubsymflags == SYMTYPE_FUNCTION,
                        codeview_get_address(msc_dbg, sym->public_v3.segment, sym->public_v3.offset), 1);
        break;
    default: return FALSE;
    }
    return TRUE;
}

/*========================================================================
 * Process PDB file.
 */

static void* pdb_jg_read(const struct PDB_JG_HEADER* pdb, const WORD* block_list,
                         int size)
{
    int                         i, num_blocks;
    BYTE*                       buffer;

    if (!size) return NULL;

    num_blocks = (size + pdb->block_size - 1) / pdb->block_size;
    buffer = HeapAlloc(GetProcessHeap(), 0, num_blocks * pdb->block_size);
    if (!buffer) return NULL;

    for (i = 0; i < num_blocks; i++)
        memcpy(buffer + i * pdb->block_size,
               (const char*)pdb + block_list[i] * pdb->block_size, pdb->block_size);

    return buffer;
}

static void* pdb_ds_read(const struct PDB_DS_HEADER* pdb, const UINT *block_list,
                         int size)
{
    int                         i, num_blocks;
    BYTE*                       buffer;

    if (!size) return NULL;

    num_blocks = (size + pdb->block_size - 1) / pdb->block_size;
    buffer = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)num_blocks * pdb->block_size);
    if (!buffer) return NULL;

    for (i = 0; i < num_blocks; i++)
        memcpy(buffer + i * pdb->block_size,
               (const char*)pdb + (DWORD_PTR)block_list[i] * pdb->block_size, pdb->block_size);

    return buffer;
}

static void* pdb_read_jg_stream(const struct PDB_JG_HEADER* pdb,
                                const struct PDB_JG_TOC* toc, DWORD stream_nr)
{
    const WORD*                 block_list;
    DWORD                       i;

    if (!toc || stream_nr >= toc->num_streams) return NULL;

    block_list = (const WORD*) &toc->streams[toc->num_streams];
    for (i = 0; i < stream_nr; i++)
        block_list += (toc->streams[i].size + pdb->block_size - 1) / pdb->block_size;

    return pdb_jg_read(pdb, block_list, toc->streams[stream_nr].size);
}

static void* pdb_read_ds_stream(const struct PDB_DS_HEADER* pdb,
                                const struct PDB_DS_TOC* toc, DWORD stream_nr)
{
    const UINT *block_list;
    DWORD                       i;

    if (!toc || stream_nr >= toc->num_streams) return NULL;
    if (toc->stream_size[stream_nr] == 0 || toc->stream_size[stream_nr] == 0xFFFFFFFF) return NULL;

    block_list = &toc->stream_size[toc->num_streams];
    for (i = 0; i < stream_nr; i++)
        block_list += (toc->stream_size[i] + pdb->block_size - 1) / pdb->block_size;

    return pdb_ds_read(pdb, block_list, toc->stream_size[stream_nr]);
}

static void* pdb_read_stream(const struct pdb_file_info* pdb_file,
                             DWORD stream_nr)
{
    switch (pdb_file->kind)
    {
    case PDB_JG:
        return pdb_read_jg_stream((const struct PDB_JG_HEADER*)pdb_file->image,
                                pdb_file->u.jg.toc, stream_nr);
    case PDB_DS:
        return pdb_read_ds_stream((const struct PDB_DS_HEADER*)pdb_file->image,
                                pdb_file->u.ds.toc, stream_nr);
    }
    return NULL;
}

static unsigned pdb_get_stream_size(const struct pdb_file_info* pdb_file, DWORD stream_nr)
{
    switch (pdb_file->kind)
    {
    case PDB_JG: return pdb_file->u.jg.toc->streams[stream_nr].size;
    case PDB_DS: return pdb_file->u.ds.toc->stream_size[stream_nr];
    }
    return 0;
}

static void pdb_free(void* buffer)
{
    HeapFree(GetProcessHeap(), 0, buffer);
}

static void pdb_free_file(struct pdb_file_info* pdb_file, BOOL unmap)
{
    switch (pdb_file->kind)
    {
    case PDB_JG:
        pdb_free(pdb_file->u.jg.toc);
        pdb_file->u.jg.toc = NULL;
        break;
    case PDB_DS:
        pdb_free(pdb_file->u.ds.toc);
        pdb_file->u.ds.toc = NULL;
        break;
    }
    HeapFree(GetProcessHeap(), 0, pdb_file->stream_dict);
    pdb_file->stream_dict = NULL;
    if (unmap)
    {
        UnmapViewOfFile(pdb_file->image);
        pdb_file->image = NULL;
    }
}

static struct pdb_stream_name* pdb_load_stream_name_table(const char* str, unsigned cb)
{
    struct pdb_stream_name*     stream_dict;
    DWORD*                      pdw;
    DWORD*                      ok_bits;
    DWORD                       count, numok;
    unsigned                    i, j;
    char*                       cpstr;

    pdw = (DWORD*)(str + cb);
    numok = *pdw++;
    count = *pdw++;

    stream_dict = HeapAlloc(GetProcessHeap(), 0, (numok + 1) * sizeof(struct pdb_stream_name) + cb);
    if (!stream_dict) return NULL;
    cpstr = (char*)(stream_dict + numok + 1);
    memcpy(cpstr, str, cb);

    /* bitfield: first dword is len (in dword), then data */
    ok_bits = pdw;
    pdw += *ok_bits++ + 1;
    pdw += *pdw + 1; /* skip deleted vector */

    for (i = j = 0; i < count; i++)
    {
        if (ok_bits[i / 32] & (1 << (i % 32)))
        {
            if (j >= numok) break;
            stream_dict[j].name = &cpstr[*pdw++];
            stream_dict[j].index = *pdw++;
            j++;
        }
    }
    /* add sentinel */
    stream_dict[numok].name = NULL;
    return stream_dict;
}

static unsigned pdb_get_stream_by_name(const struct pdb_file_info* pdb_file, const char* name)
{
    struct pdb_stream_name*     psn;

    for (psn = pdb_file->stream_dict; psn && psn->name; psn++)
    {
        if (!strcmp(psn->name, name)) return psn->index;
    }
    return -1;
}

static PDB_STRING_TABLE* pdb_read_strings(const struct pdb_file_info* pdb_file)
{
    unsigned idx;
    PDB_STRING_TABLE *ret;

    idx = pdb_get_stream_by_name(pdb_file, "/names");
    if (idx != -1)
    {
        ret = pdb_read_stream( pdb_file, idx );
        if (ret && ret->magic == 0xeffeeffe &&
            sizeof(*ret) + ret->length <= pdb_get_stream_size(pdb_file, idx)) return ret;
        pdb_free( ret );
    }
    WARN("string table not found\n");
    return NULL;
}

static const char* pdb_get_string_table_entry(const PDB_STRING_TABLE* table, unsigned offset)
{
    return (!table || offset >= table->length) ? NULL : (const char*)(table + 1) + offset;
}

static void pdb_module_remove(struct module_format* modfmt)
{
    unsigned    i;

    for (i = 0; i < modfmt->u.pdb_info->used_subfiles; i++)
    {
        pdb_free_file(&modfmt->u.pdb_info->pdb_files[i], TRUE);
        if (modfmt->u.pdb_info->pdb_files[i].image)
            UnmapViewOfFile(modfmt->u.pdb_info->pdb_files[i].image);
        if (modfmt->u.pdb_info->pdb_files[i].pdb_reader)
            pdb_reader_dispose(modfmt->u.pdb_info->pdb_files[i].pdb_reader);
    }
    HeapFree(GetProcessHeap(), 0, modfmt);
}

static BOOL pdb_convert_types_header(PDB_TYPES* types, const BYTE* image)
{
    if (!image) return FALSE;

    if (*(const DWORD*)image < 19960000)   /* FIXME: correct version? */
    {
        /* Old version of the types record header */
        const PDB_TYPES_OLD*    old = (const PDB_TYPES_OLD*)image;
        memset(types, 0, sizeof(PDB_TYPES));
        types->version     = old->version;
        types->type_offset = sizeof(PDB_TYPES_OLD);
        types->type_size   = old->type_size;
        types->first_index = old->first_index;
        types->last_index  = old->last_index;
        types->hash_stream = old->hash_stream;
    }
    else
    {
        /* New version of the types record header */
        *types = *(const PDB_TYPES*)image;
    }
    return TRUE;
}

static void pdb_convert_symbols_header(PDB_SYMBOLS* symbols,
                                       int* header_size, const BYTE* image)
{
    memset(symbols, 0, sizeof(PDB_SYMBOLS));
    if (!image) return;

    if (*(const DWORD*)image != 0xffffffff)
    {
        /* Old version of the symbols record header */
        const PDB_SYMBOLS_OLD*  old = (const PDB_SYMBOLS_OLD*)image;
        symbols->version            = 0;
        symbols->module_size        = old->module_size;
        symbols->sectcontrib_size   = old->sectcontrib_size;
        symbols->segmap_size        = old->segmap_size;
        symbols->srcmodule_size     = old->srcmodule_size;
        symbols->pdbimport_size     = 0;
        symbols->global_hash_stream = old->global_hash_stream;
        symbols->public_stream      = old->public_stream;
        symbols->gsym_stream        = old->gsym_stream;

        *header_size = sizeof(PDB_SYMBOLS_OLD);
    }
    else
    {
        /* New version of the symbols record header */
        *symbols = *(const PDB_SYMBOLS*)image;
        *header_size = sizeof(PDB_SYMBOLS);
    }
}

static void pdb_convert_symbol_file(const PDB_SYMBOLS* symbols,
                                    PDB_SYMBOL_FILE_EX* sfile,
                                    unsigned* size, const void* image)

{
    if (symbols->version < 19970000)
    {
        const PDB_SYMBOL_FILE *sym_file = image;
        memset(sfile, 0, sizeof(*sfile));
        sfile->stream       = sym_file->stream;
        sfile->range.index  = sym_file->range.index;
        sfile->symbol_size  = sym_file->symbol_size;
        sfile->lineno_size  = sym_file->lineno_size;
        sfile->lineno2_size = sym_file->lineno2_size;
        *size = sizeof(PDB_SYMBOL_FILE);
    }
    else
    {
        memcpy(sfile, image, sizeof(PDB_SYMBOL_FILE_EX));
        *size = sizeof(PDB_SYMBOL_FILE_EX);
    }
}

static void pdb_dispose_type_parse(struct codeview_type_parse* ctp)
{
    pdb_free(ctp->hash_stream);
    free((DWORD*)ctp->offset);
    free((DWORD*)ctp->hash);
    free((DWORD*)ctp->alloc_hash);
}

static BOOL pdb_bitfield_is_bit_set(const unsigned* dw, unsigned len, unsigned i)
{
    if (i >= len * sizeof(unsigned) * 8) return FALSE;
    return (dw[i >> 5] & (1u << (i & 31u))) != 0;
}

static BOOL pdb_init_type_parse(const struct msc_debug_info* msc_dbg,
                                const struct pdb_file_info* pdb_file,
                                struct codeview_type_parse* ctp,
                                BYTE* image)
{
    const BYTE* ptr;
    DWORD* offset;
    int i;

    ctp->hash_stream = NULL;
    ctp->offset = NULL;
    ctp->hash = NULL;
    ctp->alloc_hash = NULL;
    if (!pdb_convert_types_header(&ctp->header, image))
        return FALSE;

    /* Check for unknown versions */
    switch (ctp->header.version)
    {
    case 19950410:      /* VC 4.0 */
    case 19951122:
    case 19961031:      /* VC 5.0 / 6.0 */
    case 19990903:      /* VC 7.0 */
    case 20040203:      /* VC 8.0 */
        break;
    default:
        ERR("-Unknown type info version %d\n", ctp->header.version);
        return FALSE;
    }
    if (ctp->header.hash_value_size != 2 && ctp->header.hash_value_size != 4)
    {
        ERR("-Unsupported hash of size %u\n", ctp->header.hash_value_size);
        return FALSE;
    }
    if (!(ctp->hash_stream = pdb_read_stream(pdb_file, ctp->header.hash_stream)))
    {
        if (ctp->header.last_index > ctp->header.first_index)
        {
            /* may be reconstruct hash table ? */
            FIXME("-No hash table, while types exist\n");
            return FALSE;
        }
    }

    ctp->module = msc_dbg->module;
    /* Reconstruct the types offset table
     * Note: the hash stream of the PDB_TYPES only contains a partial table
     * (not all the indexes are present, so it requires first a binary search in partial table,
     * followed by a linear search...)
     */
    offset = malloc(sizeof(DWORD) * (ctp->header.last_index - ctp->header.first_index));
    if (!offset) goto oom;
    ctp->table = ptr = image + ctp->header.type_offset;
    for (i = ctp->header.first_index; i < ctp->header.last_index; i++)
    {
        offset[i - ctp->header.first_index] = ptr - ctp->table;
        ptr += ((const union codeview_type*)ptr)->generic.len + 2;
    }
    ctp->offset = offset;
    ctp->hash = calloc(ctp->header.hash_num_buckets, sizeof(struct hash_link*));
    if (!ctp->hash) goto oom;
    ctp->alloc_hash = calloc(ctp->header.last_index - ctp->header.first_index, sizeof(struct hash_link));
    if (!ctp->alloc_hash) goto oom;
    for (i = ctp->header.first_index; i < ctp->header.last_index; i++)
    {
        unsigned hash_i = pdb_read_hash_value(ctp, i);
        ctp->alloc_hash[i - ctp->header.first_index].id = i;
        ctp->alloc_hash[i - ctp->header.first_index].next = ctp->hash[hash_i];
        ctp->hash[hash_i] = &ctp->alloc_hash[i - ctp->header.first_index];
    }
    /* parse the remap table
     * => move listed type_id at first position of their hash buckets so that we force remap to them
     */
    if (ctp->hash_stream && ctp->header.type_remap_size)
    {
        const unsigned* remap = (const unsigned*)((const BYTE*)ctp->hash_stream + ctp->header.type_remap_offset);
        unsigned i, capa, count_present;
        const unsigned* present_bitset;
        remap++; /* no need of num */
        capa = *remap++;
        count_present = *remap++;
        present_bitset = remap;
        remap += count_present;
        remap += *remap + 1; /* skip deleted bit set */
        for (i = 0; i < capa; ++i)
        {
            if (pdb_bitfield_is_bit_set(present_bitset, count_present, i))
            {
                unsigned hash_i;
                struct hash_link** phl;
                /* remap[0] is an offset for a string in /string stream, followed by type_id to force */
                hash_i = pdb_read_hash_value(ctp, remap[1]);
                for (phl = &ctp->hash[hash_i]; *phl; phl = &(*phl)->next)
                    if ((*phl)->id == remap[1])
                    {
                        struct hash_link* hl = *phl;
                        /* move hl node at first position of its hash bucket */
                        *phl = hl->next;
                        hl->next = ctp->hash[hash_i];
                        ctp->hash[hash_i] = hl;
                        break;
                    }
                remap += 2;
            }
        }
    }
    return TRUE;
oom:
    pdb_dispose_type_parse(ctp);
    return FALSE;
}

static void pdb_process_types(const struct msc_debug_info* msc_dbg,
                              const struct pdb_file_info* pdb_file)
{
    struct codeview_type_parse ctp;
    BYTE* types_image = pdb_read_stream(pdb_file, 2);

    if (types_image)
    {
        if (pdb_init_type_parse(msc_dbg, pdb_file, &ctp, types_image))
        {
            /* Read type table */
            codeview_parse_type_table(&ctp);
            pdb_dispose_type_parse(&ctp);
        }
        pdb_free(types_image);
    }
}

static const char       PDB_JG_IDENT[] = "Microsoft C/C++ program database 2.00\r\n\032JG\0";
static const char       PDB_DS_IDENT[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0";

/******************************************************************
 *		pdb_init
 *
 * Tries to load a pdb file
 */
static BOOL pdb_init(struct pdb_file_info* pdb_file, const char* image)
{
    /* check the file header, and if ok, load the TOC */
    TRACE("PDB: %.40s\n", debugstr_an(image, 40));

    if (!memcmp(image, PDB_JG_IDENT, sizeof(PDB_JG_IDENT)))
    {
        const struct PDB_JG_HEADER* pdb = (const struct PDB_JG_HEADER*)image;
        struct PDB_JG_ROOT*         root;
        struct PDB_JG_TOC*          jg_toc;

        jg_toc = pdb_jg_read(pdb, (unsigned short *)(pdb + 1), pdb->toc.size);
        if (!jg_toc)
        {
            ERR("-Unable to get TOC from .PDB\n");
            return FALSE;
        }
        root = pdb_read_jg_stream(pdb, jg_toc, 1);
        if (!root)
        {
            ERR("-Unable to get root from .PDB\n");
            pdb_free(jg_toc);
            return FALSE;
        }
        switch (root->Version)
        {
        case 19950623:      /* VC 4.0 */
        case 19950814:
        case 19960307:      /* VC 5.0 */
        case 19970604:      /* VC 6.0 */
            break;
        default:
            ERR("-Unknown root block version %d\n", root->Version);
        }
        pdb_file->kind = PDB_JG;
        pdb_file->u.jg.toc = jg_toc;
        TRACE("found JG: age=%x timestamp=%x\n", root->Age, root->TimeDateStamp);
        pdb_file->stream_dict = pdb_load_stream_name_table(&root->names[0], root->cbNames);
        pdb_file->fpoext_stream = -1;

        pdb_free(root);
    }
    else if (!memcmp(image, PDB_DS_IDENT, sizeof(PDB_DS_IDENT)))
    {
        const struct PDB_DS_HEADER* pdb = (const struct PDB_DS_HEADER*)image;
        struct PDB_DS_ROOT*         root;
        struct PDB_DS_TOC*          ds_toc;

        ds_toc = pdb_ds_read(pdb, (const UINT*)((const char*)pdb + (DWORD_PTR)pdb->toc_block * pdb->block_size),
                             pdb->toc_size);
        if (!ds_toc)
        {
            ERR("-Unable to get TOC from .PDB\n");
            return FALSE;
        }
        root = pdb_read_ds_stream(pdb, ds_toc, 1);
        if (!root)
        {
            ERR("-Unable to get root from .PDB\n");
            pdb_free(ds_toc);
            return FALSE;
        }
        switch (root->Version)
        {
        case 20000404:
            break;
        default:
            ERR("-Unknown root block version %u\n", root->Version);
        }
        pdb_file->kind = PDB_DS;
        pdb_file->u.ds.toc = ds_toc;
        TRACE("found DS for: age=%x guid=%s\n", root->Age, debugstr_guid(&root->guid));
        pdb_file->stream_dict = pdb_load_stream_name_table(&root->names[0], root->cbNames);
        pdb_file->fpoext_stream = -1;

        pdb_free(root);
    }

    if (0) /* some tool to dump the internal files from a PDB file */
    {
        int     i, num_streams;

        switch (pdb_file->kind)
        {
        case PDB_JG: num_streams = pdb_file->u.jg.toc->num_streams; break;
        case PDB_DS: num_streams = pdb_file->u.ds.toc->num_streams; break;
        }

        for (i = 1; i < num_streams; i++)
        {
            unsigned char* x = pdb_read_stream(pdb_file, i);
            FIXME("********************** [%u]: size=%08x\n",
                  i, pdb_get_stream_size(pdb_file, i));
            dump(x, pdb_get_stream_size(pdb_file, i));
            pdb_free(x);
        }
    }
    return pdb_file->stream_dict != NULL;
}

static BOOL pdb_process_internal(const struct process *pcs,
                                 const struct msc_debug_info *msc_dbg,
                                 const WCHAR *filename,
                                 struct pdb_module_info *pdb_module_info,
                                 unsigned module_index,
                                 BOOL *has_linenumber_info);

DWORD pdb_get_file_indexinfo(void* image, DWORD size, SYMSRV_INDEX_INFOW* info)
{
    /* check the file header, and if ok, load the TOC */
    TRACE("PDB: %.40s\n", debugstr_an(image, 40));

    if (!memcmp(image, PDB_JG_IDENT, sizeof(PDB_JG_IDENT)))
    {
        const struct PDB_JG_HEADER* pdb = (const struct PDB_JG_HEADER*)image;
        struct PDB_JG_TOC*          jg_toc;
        struct PDB_JG_ROOT*         root;
        DWORD                       ec = ERROR_SUCCESS;

        jg_toc = pdb_jg_read(pdb, (unsigned short*)(pdb + 1), pdb->toc.size);
        root = pdb_read_jg_stream(pdb, jg_toc, 1);
        if (!root)
        {
            ERR("-Unable to get root from .PDB\n");
            pdb_free(jg_toc);
            return ERROR_FILE_CORRUPT;
        }
        switch (root->Version)
        {
        case 19950623:      /* VC 4.0 */
        case 19950814:
        case 19960307:      /* VC 5.0 */
        case 19970604:      /* VC 6.0 */
            break;
        default:
            ERR("-Unknown root block version %d\n", root->Version);
            ec = ERROR_FILE_CORRUPT;
        }
        if (ec == ERROR_SUCCESS)
        {
            info->dbgfile[0] = '\0';
            memset(&info->guid, 0, sizeof(GUID));
            info->guid.Data1 = root->TimeDateStamp;
            info->pdbfile[0] = '\0';
            info->age = root->Age;
            info->sig = root->TimeDateStamp;
            info->size = 0;
            info->stripped = FALSE;
            info->timestamp = 0;
        }

        pdb_free(jg_toc);
        pdb_free(root);

        return ec;
    }
    if (!memcmp(image, PDB_DS_IDENT, sizeof(PDB_DS_IDENT)))
    {
        const struct PDB_DS_HEADER* pdb = (const struct PDB_DS_HEADER*)image;
        struct PDB_DS_TOC*          ds_toc;
        struct PDB_DS_ROOT*         root;
        DWORD                       ec = ERROR_SUCCESS;

        ds_toc = pdb_ds_read(pdb, (const UINT*)((const char*)pdb + (DWORD_PTR)pdb->toc_block * pdb->block_size),
                             pdb->toc_size);
        root = pdb_read_ds_stream(pdb, ds_toc, 1);
        if (!root)
        {
            pdb_free(ds_toc);
            return ERROR_FILE_CORRUPT;
        }
        switch (root->Version)
        {
        case 20000404:
            break;
        default:
            ERR("-Unknown root block version %u\n", root->Version);
            ec = ERROR_FILE_CORRUPT;
        }
        /* The age field is present twice (in PDB_ROOT and in PDB_SYMBOLS ),
         * It's the one in PDB_SYMBOLS which is reported.
         */
        if (ec == ERROR_SUCCESS)
        {
            PDB_SYMBOLS* symbols = pdb_read_ds_stream(pdb, ds_toc, 3);

            if (symbols && symbols->version == 19990903)
            {
                info->age = symbols->age;
            }
            else
            {
                if (symbols)
                    ERR("-Unknown symbol info version %u %08x\n", symbols->version, symbols->version);
                ec = ERROR_FILE_CORRUPT;
            }
            pdb_free(symbols);
        }
        if (ec == ERROR_SUCCESS)
        {
            info->dbgfile[0] = '\0';
            info->guid = root->guid;
            info->pdbfile[0] = '\0';
            info->sig = 0;
            info->size = 0;
            info->stripped = FALSE;
            info->timestamp = 0;
        }

        pdb_free(ds_toc);
        pdb_free(root);
        return ec;
    }
    return ERROR_BAD_FORMAT;
}

static void pdb_process_symbol_imports(const struct process *pcs,
                                       const struct msc_debug_info *msc_dbg,
                                       const PDB_SYMBOLS *symbols,
                                       const void *symbols_image,
                                       const char *image,
                                       struct pdb_module_info *pdb_module_info,
                                       unsigned module_index)
{
    if (module_index == -1 && symbols && symbols->pdbimport_size)
    {
        const PDB_SYMBOL_IMPORT*imp;
        const void*             first;
        const void*             last;
        const char*             ptr;
        int                     i = 0;

        imp = (const PDB_SYMBOL_IMPORT*)((const char*)symbols_image + sizeof(PDB_SYMBOLS) +
                                         symbols->module_size + symbols->sectcontrib_size +
                                         symbols->segmap_size + symbols->srcmodule_size);
        first = imp;
        last = (const char*)imp + symbols->pdbimport_size;
        while (imp < (const PDB_SYMBOL_IMPORT*)last)
        {
            SYMSRV_INDEX_INFOW info;
            BOOL line_info;

            ptr = (const char*)imp + sizeof(*imp) + strlen(imp->filename);
            if (i >= CV_MAX_MODULES) FIXME("Out of bounds!!!\n");
            TRACE("got for %s: age=%u ts=%x\n",
                  debugstr_a(imp->filename), imp->Age, imp->TimeDateStamp);
            if (path_find_symbol_file(pcs, msc_dbg->module, imp->filename, TRUE, NULL, imp->TimeDateStamp, imp->Age, &info,
                                      &msc_dbg->module->module.PdbUnmatched))
                pdb_process_internal(pcs, msc_dbg, info.pdbfile, pdb_module_info, i, &line_info);
            i++;
            imp = (const PDB_SYMBOL_IMPORT*)((const char*)first + ((ptr - (const char*)first + strlen(ptr) + 1 + 3) & ~3));
        }
        pdb_module_info->used_subfiles = i;
    }
    if (module_index == -1)
    {
        module_index = 0;
        pdb_module_info->used_subfiles = 1;
    }
    cv_current_module = &cv_zmodules[module_index];
    if (cv_current_module->allowed) FIXME("Already allowed??\n");
    cv_current_module->allowed = TRUE;
}

static BOOL pdb_process_internal(const struct process *pcs,
                                 const struct msc_debug_info *msc_dbg,
                                 const WCHAR *filename,
                                 struct pdb_module_info *pdb_module_info,
                                 unsigned module_index,
                                 BOOL *has_linenumber_info)
{
    HANDLE                      hFile = NULL, hMap = NULL;
    char*                       image = NULL;
    BYTE*                       symbols_image = NULL;
    PDB_STRING_TABLE*           files_image = NULL;
    struct pdb_file_info*       pdb_file;

    TRACE("Processing PDB file %ls\n", filename);

    *has_linenumber_info = FALSE;
    pdb_file = &pdb_module_info->pdb_files[module_index == -1 ? 0 : module_index];
    /* Open and map() .PDB file */
    if ((hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE ||
        (hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) == NULL ||
        (image = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) == NULL)
    {
        WARN("Unable to open .PDB file: %ls\n", filename);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hMap);
    /* old pdb reader */
    if (!pdb_init(pdb_file, image))
    {
        CloseHandle(hFile);
        UnmapViewOfFile(image);
        return FALSE;
    }
    if (getenv("WINE_DBGHELP_OLD_PDB")) /* keep using old pdb reader */
    {
        pdb_file->pdb_reader = NULL;
        CloseHandle(hFile);
    }
    else if (!(pdb_file->pdb_reader = pdb_hack_reader_init(msc_dbg->module, hFile, msc_dbg->sectp, msc_dbg->nsect)))
    {
        CloseHandle(hFile);
        UnmapViewOfFile(image);
        return FALSE;
    }

    pdb_file->image = image;
    symbols_image = pdb_read_stream(pdb_file, 3);
    if (symbols_image)
    {
        PDB_SYMBOLS symbols;
        BYTE*       globalimage;
        BYTE*       modimage;
        BYTE*       ipi_image;
        struct codeview_type_parse ipi_ctp;
        BYTE*       file;
        int         header_size = 0;
        unsigned    num_sub_streams;
        const unsigned short* sub_streams;
        BOOL        ipi_ok;

        pdb_convert_symbols_header(&symbols, &header_size, symbols_image);
        switch (symbols.version)
        {
        case 0:            /* VC 4.0 */
        case 19960307:     /* VC 5.0 */
        case 19970606:     /* VC 6.0 */
        case 19990903:
            break;
        default:
            ERR("-Unknown symbol info version %u %08x\n",
                symbols.version, symbols.version);
        }

        num_sub_streams = symbols.stream_index_size / sizeof(sub_streams[0]);
        sub_streams = (const unsigned short*)((const char*)symbols_image + sizeof(PDB_SYMBOLS) +
                                              symbols.module_size + symbols.sectcontrib_size +
                                              symbols.segmap_size + symbols.srcmodule_size +
                                              symbols.pdbimport_size + symbols.unknown2_size);
        if (PDB_SIDX_FPOEXT < num_sub_streams)
            pdb_file->fpoext_stream = sub_streams[PDB_SIDX_FPOEXT];

        files_image = pdb_read_strings(pdb_file);

        pdb_process_symbol_imports(pcs, msc_dbg, &symbols, symbols_image, image,
                                   pdb_module_info, module_index);
        pdb_process_types(msc_dbg, pdb_file);

        ipi_image = pdb_read_stream(pdb_file, 4);
        ipi_ok = pdb_init_type_parse(msc_dbg, pdb_file, &ipi_ctp, ipi_image);

        /* Read global types first, so that lookup by name in module (=compilation unit)
         * streams' loading can succeed them.
         */
        globalimage = pdb_read_stream(pdb_file, symbols.gsym_stream);
        if (globalimage)
        {
            const BYTE* data;
            unsigned global_size = pdb_get_stream_size(pdb_file, symbols.gsym_stream);

            data = pdb_read_stream(pdb_file, symbols.global_hash_stream);
            if (data)
            {
                if (codeview_snarf_sym_hashtable(msc_dbg, globalimage, global_size,
                                                 data, pdb_get_stream_size(pdb_file, symbols.global_hash_stream),
                                                 pdb_global_feed_types))
                    *has_linenumber_info = TRUE;
                pdb_free((void*)data);
            }
        }

        /* Read per-module symbols' tables */
        file = symbols_image + header_size;
        while (file - symbols_image < header_size + symbols.module_size)
        {
            PDB_SYMBOL_FILE_EX          sfile;
            const char*                 file_name;
            unsigned                    size;

            HeapValidate(GetProcessHeap(), 0, NULL);
            pdb_convert_symbol_file(&symbols, &sfile, &size, file);

            modimage = pdb_read_stream(pdb_file, sfile.stream);
            file_name = (const char*)file + size;
            if (modimage)
            {
                struct cv_module_snarf cvmod = {ipi_ok ? &ipi_ctp : NULL, (const void*)(modimage + sfile.symbol_size), sfile.lineno2_size,
                    files_image};
                codeview_snarf(msc_dbg, pdb_file->pdb_reader ? sfile.stream : 0, modimage, sizeof(DWORD), sfile.symbol_size,
                               &cvmod, file_name);

                if (sfile.lineno_size && sfile.lineno2_size)
                    FIXME("Both line info present... preferring second\n");
                if (sfile.lineno2_size)
                {
                    if (pdb_file->pdb_reader ||
                        ((SymGetOptions() & SYMOPT_LOAD_LINES) && codeview_snarf_linetab2(msc_dbg, &cvmod)))
                        *has_linenumber_info = TRUE;
                }
                else if (sfile.lineno_size)
                {
                    if (pdb_file->pdb_reader)
                        FIXME("New PDB reader doesn't support old line format\n");
                    else if ((SymGetOptions() & SYMOPT_LOAD_LINES) &&
                             codeview_snarf_linetab(msc_dbg,
                                                    modimage + sfile.symbol_size,
                                                    sfile.lineno_size,
                                                    pdb_file->kind == PDB_JG))
                        *has_linenumber_info = TRUE;
                }
                pdb_free(modimage);
            }
            file_name += strlen(file_name) + 1;
            /* now at lib_name */
            file = (BYTE*)((DWORD_PTR)(file_name + strlen(file_name) + 1 + 3) & ~3);
        }
        /* Load the global variables and constants (if not yet loaded) and public information */
        if (globalimage)
        {
            const BYTE* data;
            unsigned global_size = pdb_get_stream_size(pdb_file, symbols.gsym_stream);

            data = pdb_read_stream(pdb_file, symbols.global_hash_stream);
            if (data)
            {
                if (codeview_snarf_sym_hashtable(msc_dbg, globalimage, global_size,
                                                 data, pdb_get_stream_size(pdb_file,
                                                                           symbols.global_hash_stream),
                                                 pdb_global_feed_variables))
                    *has_linenumber_info = TRUE;
                pdb_free((void*)data);
            }
            if (!(dbghelp_options & SYMOPT_NO_PUBLICS) && (data = pdb_read_stream(pdb_file, symbols.public_stream)))
            {
                const DBI_PUBLIC_HEADER* pubhdr = (const DBI_PUBLIC_HEADER*)data;
                codeview_snarf_sym_hashtable(msc_dbg, globalimage, pdb_get_stream_size(pdb_file, symbols.gsym_stream),
                                             (const BYTE*)(pubhdr + 1), pubhdr->hash_size, pdb_global_feed_public);
                pdb_free((void*)data);
            }
            pdb_free(globalimage);
        }
        HeapFree(GetProcessHeap(), 0, (DWORD*)ipi_ctp.offset);
        pdb_free(ipi_image);
    }
    else
        pdb_process_symbol_imports(pcs, msc_dbg, NULL, NULL, image,
                                   pdb_module_info, module_index);

    pdb_free(symbols_image);
    pdb_free(files_image);

    pdb_free_file(pdb_file, pdb_file->pdb_reader != NULL);

    return TRUE;
}

static const struct module_format_vtable old_pdb_module_format_vtable =
{
    pdb_module_remove,
    pdb_location_compute,
};

static BOOL pdb_process_file(const struct process *pcs,
                             const struct msc_debug_info *msc_dbg,
                             const char *filename, const GUID *guid, DWORD timestamp, DWORD age)
{
    struct module_format*       modfmt;
    struct pdb_module_info*     pdb_module_info;
    SYMSRV_INDEX_INFOW          info;
    BOOL                        unmatched;

    if (!msc_dbg->module->dont_load_symbols &&
        path_find_symbol_file(pcs, msc_dbg->module, filename, TRUE, guid, timestamp, age, &info, &unmatched) &&
        (modfmt = HeapAlloc(GetProcessHeap(), 0,
                            sizeof(struct module_format) + sizeof(struct pdb_module_info))))
    {
        BOOL ret, has_linenumber_info;

        pdb_module_info = (void*)(modfmt + 1);
        msc_dbg->module->format_info[DFI_PDB] = modfmt;
        modfmt->module      = msc_dbg->module;
        modfmt->vtable      = &old_pdb_module_format_vtable;
        modfmt->u.pdb_info  = pdb_module_info;

        memset(cv_zmodules, 0, sizeof(cv_zmodules));
        codeview_init_basic_types(msc_dbg->module);
        ret = pdb_process_internal(pcs, msc_dbg, info.pdbfile,
                                   msc_dbg->module->format_info[DFI_PDB]->u.pdb_info, -1, &has_linenumber_info);
        codeview_clear_type_table();
        if (ret)
        {
            msc_dbg->module->module.SymType = SymPdb;
            msc_dbg->module->module.PdbSig = info.sig;
            msc_dbg->module->module.PdbAge = info.age;
            msc_dbg->module->module.PdbSig70 = info.guid;
            msc_dbg->module->module.PdbUnmatched = unmatched;
            wcscpy(msc_dbg->module->module.LoadedPdbName, info.pdbfile);

            /* FIXME: we could have a finer grain here */
            msc_dbg->module->module.LineNumbers = has_linenumber_info;
            msc_dbg->module->module.GlobalSymbols = TRUE;
            msc_dbg->module->module.TypeInfo = TRUE;
            msc_dbg->module->module.SourceIndexed = TRUE;
            msc_dbg->module->module.Publics = TRUE;

            return TRUE;
        }
        msc_dbg->module->format_info[DFI_PDB] = NULL;
        HeapFree(GetProcessHeap(), 0, modfmt);
    }
    msc_dbg->module->module.SymType = SymNone;
    if (guid)
        msc_dbg->module->module.PdbSig70 = *guid;
    else
        memset(&msc_dbg->module->module.PdbSig70, 0, sizeof(GUID));
    msc_dbg->module->module.PdbSig = 0;
    msc_dbg->module->module.PdbAge = age;
    return FALSE;
}

/*========================================================================
 * Process CodeView debug information.
 */

#define MAKESIG(a,b,c,d)        ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define CODEVIEW_NB09_SIG       MAKESIG('N','B','0','9')
#define CODEVIEW_NB10_SIG       MAKESIG('N','B','1','0')
#define CODEVIEW_NB11_SIG       MAKESIG('N','B','1','1')
#define CODEVIEW_RSDS_SIG       MAKESIG('R','S','D','S')

static BOOL codeview_process_info(const struct process *pcs,
                                  const struct msc_debug_info *msc_dbg)
{
    const DWORD*                signature = (const DWORD*)msc_dbg->root;
    BOOL                        ret = FALSE;

    TRACE("Processing signature %.4s\n", (const char*)signature);

    switch (*signature)
    {
    case CODEVIEW_NB09_SIG:
    case CODEVIEW_NB11_SIG:
    {
        const OMFSignature*     cv = (const OMFSignature*)msc_dbg->root;
        const OMFDirHeader*     hdr = (const OMFDirHeader*)(msc_dbg->root + cv->filepos);
        const OMFDirEntry*      ent;
        const OMFDirEntry*      prev;
        const OMFDirEntry*      next;
        unsigned int            i;
        BOOL                    has_linenumber_info = FALSE;

        codeview_init_basic_types(msc_dbg->module);

        for (i = 0; i < hdr->cDir; i++)
        {
            ent = (const OMFDirEntry*)((const BYTE*)hdr + hdr->cbDirHeader + i * hdr->cbDirEntry);
            if (ent->SubSection == sstGlobalTypes)
            {
                const OMFGlobalTypes*           types;
                struct codeview_type_parse      ctp;

                types = (const OMFGlobalTypes*)(msc_dbg->root + ent->lfo);
                ctp.module = msc_dbg->module;
                ctp.offset = (const DWORD*)(types + 1);
                memset(&ctp.header, 0, sizeof(ctp.header));
                ctp.header.first_index = T_FIRSTDEFINABLETYPE;
                ctp.header.last_index = ctp.header.first_index + types->cTypes;
                ctp.table  = (const BYTE*)(ctp.offset + types->cTypes);

                cv_current_module = &cv_zmodules[0];
                if (cv_current_module->allowed) FIXME("Already allowed??\n");
                cv_current_module->allowed = TRUE;

                codeview_parse_type_table(&ctp);
                break;
            }
        }

        ent = (const OMFDirEntry*)((const BYTE*)hdr + hdr->cbDirHeader);
        for (i = 0; i < hdr->cDir; i++, ent = next)
        {
            next = (i == hdr->cDir-1) ? NULL :
                   (const OMFDirEntry*)((const BYTE*)ent + hdr->cbDirEntry);
            prev = (i == 0) ? NULL :
                   (const OMFDirEntry*)((const BYTE*)ent - hdr->cbDirEntry);

            if (ent->SubSection == sstAlignSym)
            {
                codeview_snarf(msc_dbg, 0, msc_dbg->root + ent->lfo, sizeof(DWORD), ent->cb, NULL, NULL);

                if (SymGetOptions() & SYMOPT_LOAD_LINES)
                {
                    /*
                     * Check the next and previous entry.  If either is a
                     * sstSrcModule, it contains the line number info for
                     * this file.
                     *
                     * FIXME: This is not a general solution!
                     */
                    if (next && next->iMod == ent->iMod && next->SubSection == sstSrcModule)
                        if (codeview_snarf_linetab(msc_dbg, msc_dbg->root + next->lfo,
                                                   next->cb, TRUE))
                            has_linenumber_info = TRUE;

                    if (prev && prev->iMod == ent->iMod && prev->SubSection == sstSrcModule)
                        if (codeview_snarf_linetab(msc_dbg, msc_dbg->root + prev->lfo,
                                                   prev->cb, TRUE))
                            has_linenumber_info = TRUE;
                }
            }
        }

        msc_dbg->module->module.SymType = SymCv;
        msc_dbg->module->module.LineNumbers = has_linenumber_info;
        /* FIXME: we could have a finer grain here */
        msc_dbg->module->module.GlobalSymbols = TRUE;
        msc_dbg->module->module.TypeInfo = TRUE;
        msc_dbg->module->module.SourceIndexed = TRUE;
        msc_dbg->module->module.Publics = TRUE;
        codeview_clear_type_table();
        ret = TRUE;
        break;
    }

    case CODEVIEW_NB10_SIG:
    {
        const CODEVIEW_PDB_DATA* pdb = (const CODEVIEW_PDB_DATA*)msc_dbg->root;
        ret = pdb_process_file(pcs, msc_dbg, pdb->name, NULL, pdb->timestamp, pdb->age);
        break;
    }
    case CODEVIEW_RSDS_SIG:
    {
        const OMFSignatureRSDS* rsds = (const OMFSignatureRSDS*)msc_dbg->root;

        TRACE("Got RSDS type of PDB file: guid=%s age=%08x name=%s\n",
              wine_dbgstr_guid(&rsds->guid), rsds->age, debugstr_a(rsds->name));
        /* gcc/mingw and clang can emit build-id information, but with an empty PDB filename.
         * Don't search for the .pdb file in that case.
         */
        if (rsds->name[0])
            ret = pdb_process_file(pcs, msc_dbg, rsds->name, &rsds->guid, 0, rsds->age);
        else
            ret = TRUE;
        break;
    }
    default:
        ERR("Unknown CODEVIEW signature %08lx in module %s\n",
            *signature, debugstr_w(msc_dbg->module->modulename));
        break;
    }
    if (ret)
    {
        msc_dbg->module->module.CVSig = *signature;
        if (*signature == CODEVIEW_RSDS_SIG)
            memcpy(msc_dbg->module->module.CVData, msc_dbg->root,
                   sizeof(msc_dbg->module->module.CVData));
    }
    return ret;
}

/*========================================================================
 * Process debug directory.
 */
BOOL pe_load_debug_directory(const struct process* pcs, struct module* module, 
                             const BYTE* mapping,
                             const IMAGE_SECTION_HEADER* sectp, DWORD nsect,
                             const IMAGE_DEBUG_DIRECTORY* dbg, int nDbg)
{
    BOOL                        ret;
    int                         i;
    struct msc_debug_info       msc_dbg;

    msc_dbg.module = module;
    msc_dbg.nsect  = nsect;
    msc_dbg.sectp  = sectp;
    msc_dbg.nomap  = 0;
    msc_dbg.omapp  = NULL;

    __TRY
    {
        ret = FALSE;

        /* First, watch out for OMAP data */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC)
            {
                msc_dbg.nomap = dbg[i].SizeOfData / sizeof(OMAP);
                msc_dbg.omapp = (const OMAP*)(mapping + dbg[i].PointerToRawData);
                break;
            }
        }
  
        /* Now, try to parse CodeView debug info */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                msc_dbg.root = mapping + dbg[i].PointerToRawData;
                if ((ret = codeview_process_info(pcs, &msc_dbg))) goto done;
            }
        }
    
        /* If not found, try to parse COFF debug info */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_COFF)
            {
                msc_dbg.root = mapping + dbg[i].PointerToRawData;
                if ((ret = coff_process_info(&msc_dbg))) goto done;
            }
        }
    done:
	 /* FIXME: this should be supported... this is the debug information for
	  * functions compiled without a frame pointer (FPO = frame pointer omission)
	  * the associated data helps finding out the relevant information
	  */
        for (i = 0; i < nDbg; i++)
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_FPO)
                FIXME("This guy has FPO information\n");
#if 0

#define FRAME_FPO   0
#define FRAME_TRAP  1
#define FRAME_TSS   2

typedef struct _FPO_DATA 
{
	DWORD       ulOffStart;            /* offset 1st byte of function code */
	DWORD       cbProcSize;            /* # bytes in function */
	DWORD       cdwLocals;             /* # bytes in locals/4 */
	WORD        cdwParams;             /* # bytes in params/4 */

	WORD        cbProlog : 8;          /* # bytes in prolog */
	WORD        cbRegs   : 3;          /* # regs saved */
	WORD        fHasSEH  : 1;          /* TRUE if SEH in func */
	WORD        fUseBP   : 1;          /* TRUE if EBP has been allocated */
	WORD        reserved : 1;          /* reserved for future use */
	WORD        cbFrame  : 2;          /* frame type */
} FPO_DATA;
#endif

    }
    __EXCEPT_PAGE_FAULT
    {
        ERR("Got a page fault while loading symbols\n");
        ret = FALSE;
    }
    __ENDTRY

    /* we haven't found yet any debug information, fallback to unmatched pdb */
    if (!ret && module->module.SymType == SymDeferred)
    {
        SYMSRV_INDEX_INFOW info = {.sizeofstruct = sizeof(info)};
        char buffer[MAX_PATH];
        char *ext;
        DWORD options;

        WideCharToMultiByte(CP_ACP, 0, module->module.LoadedImageName, -1, buffer, ARRAY_SIZE(buffer), 0, NULL);
        ext = strrchr(buffer, '.');
        if (ext) strcpy(ext + 1, "pdb"); else strcat(buffer, ".pdb");
        options = SymGetOptions();
        SymSetOptions(options | SYMOPT_LOAD_ANYTHING);
        ret = pdb_process_file(pcs, &msc_dbg, buffer, &null_guid, 0, 0);
        SymSetOptions(options);
        if (!ret && module->dont_load_symbols)
            module->module.TimeDateStamp = 0;
    }

    return ret;
}

DWORD msc_get_file_indexinfo(void* image, const IMAGE_DEBUG_DIRECTORY* debug_dir, DWORD num_dir, SYMSRV_INDEX_INFOW* info)
{
    DWORD i;
    unsigned num_misc_records = 0;

    info->age = 0;
    memset(&info->guid, 0, sizeof(info->guid));
    info->sig = 0;
    info->dbgfile[0] = L'\0';
    info->pdbfile[0] = L'\0';

    for (i = 0; i < num_dir; i++)
    {
        if (debug_dir[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
        {
            const CODEVIEW_PDB_DATA* data = (const CODEVIEW_PDB_DATA*)((char*)image + debug_dir[i].PointerToRawData);
            const OMFSignatureRSDS* rsds_data = (const OMFSignatureRSDS*)data;
            if (!memcmp(data->Signature, "NB10", 4))
            {
                info->age = data->age;
                info->sig = data->timestamp;
                MultiByteToWideChar(CP_ACP, 0, data->name, -1, info->pdbfile, ARRAY_SIZE(info->pdbfile));
            }
            if (!memcmp(rsds_data->Signature, "RSDS", 4))
            {
                info->age = rsds_data->age;
                info->guid = rsds_data->guid;
                MultiByteToWideChar(CP_ACP, 0, rsds_data->name, -1, info->pdbfile, ARRAY_SIZE(info->pdbfile));
            }
        }
        else if (debug_dir[i].Type == IMAGE_DEBUG_TYPE_MISC && info->stripped)
        {
            const IMAGE_DEBUG_MISC* misc = (const IMAGE_DEBUG_MISC*)
                ((const char*)image + debug_dir[i].PointerToRawData);
            if (misc->Unicode)
                wcscpy(info->dbgfile, (WCHAR*)misc->Data);
            else
                MultiByteToWideChar(CP_ACP, 0, (const char*)misc->Data, -1, info->dbgfile, ARRAY_SIZE(info->dbgfile));
            num_misc_records++;
        }
    }
    return (!num_dir || (info->stripped && !num_misc_records)) ? ERROR_BAD_EXE_FORMAT : ERROR_SUCCESS;
}

DWORD dbg_get_file_indexinfo(void* image, DWORD size, SYMSRV_INDEX_INFOW* info)
{
    const IMAGE_SEPARATE_DEBUG_HEADER *header;
    IMAGE_DEBUG_DIRECTORY *dbg;
    DWORD num_directories;

    if (size < sizeof(*header)) return ERROR_BAD_FORMAT;
    header = image;
    if (header->Signature != 0x4944 /* DI */ ||
        size < sizeof(*header) + header->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) + header->ExportedNamesSize + header->DebugDirectorySize)
        return ERROR_BAD_FORMAT;

    info->size = header->SizeOfImage;
    /* seems to use header's timestamp, not debug_directory one */
    info->timestamp = header->TimeDateStamp;
    info->stripped = FALSE; /* FIXME */

    /* header is followed by:
     * - header->NumberOfSections of IMAGE_SECTION_HEADER
     * - header->ExportedNameSize
     * - then num_directories of IMAGE_DEBUG_DIRECTORY
     */
    dbg = (IMAGE_DEBUG_DIRECTORY*)((char*)(header + 1) +
                                   header->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
                                   header->ExportedNamesSize);
    num_directories = header->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);

    return msc_get_file_indexinfo(image, dbg, num_directories, info);
}

BOOL pdb_old_virtual_unwind(struct cpu_stack_walk *csw, DWORD_PTR ip,
                            union ctx *context, struct pdb_cmd_pair *cpair)
{
    struct module_pair          pair;
    struct pdb_module_info*     pdb_info;
    PDB_FPO_DATA*               fpoext;
    unsigned                    i, size;
    PDB_STRING_TABLE*           strbase;
    BOOL                        ret = TRUE;

    if (!module_init_pair(&pair, csw->hProcess, ip)) return FALSE;
    if (!pair.effective->format_info[DFI_PDB]) return FALSE;
    pdb_info = pair.effective->format_info[DFI_PDB]->u.pdb_info;
    TRACE("searching %Ix => %Ix\n", ip, ip - (DWORD_PTR)pair.effective->module.BaseOfImage);
    ip -= (DWORD_PTR)pair.effective->module.BaseOfImage;

    strbase = pdb_read_strings(&pdb_info->pdb_files[0]);
    if (!strbase) return FALSE;
    fpoext = pdb_read_stream(&pdb_info->pdb_files[0], pdb_info->pdb_files[0].fpoext_stream);
    size = pdb_get_stream_size(&pdb_info->pdb_files[0], pdb_info->pdb_files[0].fpoext_stream);
    if (fpoext && (size % sizeof(*fpoext)) == 0)
    {
        size /= sizeof(*fpoext);
        for (i = 0; i < size; i++)
        {
            if (fpoext[i].start <= ip && ip < fpoext[i].start + fpoext[i].func_size)
            {
                TRACE("\t%08x %08x %8x %8x %4x %4x %4x %08x %s\n",
                      fpoext[i].start, fpoext[i].func_size, fpoext[i].locals_size,
                      fpoext[i].params_size, fpoext[i].maxstack_size, fpoext[i].prolog_size,
                      fpoext[i].savedregs_size, fpoext[i].flags,
                      debugstr_a(pdb_get_string_table_entry(strbase, fpoext[i].str_offset)));
                ret = pdb_fpo_unwind_parse_cmd_string(csw, &fpoext[i],
                                                      pdb_get_string_table_entry(strbase, fpoext[i].str_offset),
                                                      cpair);
                break;
            }
        }
    }
    else ret = FALSE;
    pdb_free(fpoext);
    pdb_free(strbase);

    return ret;
}
