#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "arch.h"
#include "neexe.h"

/*
 * Header loading routines for WineLib.
 */

/* LOADSHORT Calls LOAD and swaps the high and the low bytes */

#define LOAD(x)  read (fd, &TAB->x, sizeof (TAB->x))
#define LOADSHORT(x) LOAD(x); TAB->x = CONV_SHORT (TAB->x);
#define LOADLONG(x) LOAD(x);  TAB->x = CONV_LONG (TAB->x);

void load_mz_header (int fd, LPIMAGE_DOS_HEADER mz_header)
{
#define TAB mz_header
	LOAD(e_magic);
	LOADSHORT(e_cblp);
	LOADSHORT(e_cp);
	LOADSHORT(e_crlc);
	LOADSHORT(e_cparhdr);
	LOADSHORT(e_minalloc);
	LOADSHORT(e_maxalloc);
	LOADSHORT(e_ss);
	LOADSHORT(e_sp);
	LOADSHORT(e_csum);
	LOADSHORT(e_ip);
	LOADSHORT(e_cs);
	LOADSHORT(e_lfarlc);
	LOADSHORT(e_ovno);
	LOAD(e_res);
	LOADSHORT(e_oemid);
	LOADSHORT(e_oeminfo);
	LOAD(e_res2);
	LOADLONG(e_lfanew);
}

void load_ne_header (int fd, LPIMAGE_OS2_HEADER ne_header)
{
#undef TAB
#define TAB ne_header
    LOAD (ne_magic);
    LOADSHORT (linker_version);
    LOADSHORT (linker_revision);
    LOADSHORT (entry_tab_offset);
    LOADSHORT (entry_tab_length);
    LOAD (reserved1);
    LOADSHORT (format_flags);
    LOADSHORT (auto_data_seg);
    LOADSHORT (local_heap_length);
    LOADSHORT (stack_length);
    LOADSHORT (ip);
    LOADSHORT (cs);
    LOADSHORT (sp);
    LOADSHORT (ss);
    LOADSHORT (n_segment_tab);
    LOADSHORT (n_mod_ref_tab);
    LOADSHORT (nrname_tab_length);
    LOADSHORT (segment_tab_offset);
    LOADSHORT (resource_tab_offset);
    LOADSHORT (rname_tab_offset);
    LOADSHORT (moduleref_tab_offset);
    LOADSHORT (iname_tab_offset);
    LOADLONG (nrname_tab_offset);
    LOADSHORT (n_mov_entry_points);
    LOADSHORT (align_shift_count);
    LOADSHORT (n_resource_seg);
    LOAD (operating_system);
    LOAD (additional_flags);
    LOADSHORT (fastload_offset);
    LOADSHORT (fastload_length);
    LOADSHORT (reserved2);
    LOADSHORT (expect_version);
}

/*
 * Typeinfo loading routines for non PC-architectures.
 */

int load_typeinfo (int fd, struct resource_typeinfo_s *typeinfo)
{
#undef TAB
#define TAB typeinfo
    LOADSHORT (type_id);
    LOADSHORT (count);
    LOADLONG  (reserved);
    return 1;
}

int load_nameinfo (int fd, struct resource_nameinfo_s *nameinfo)
{
#undef TAB
#define TAB nameinfo
    LOADSHORT (offset);
    LOADSHORT (length);
    LOADSHORT (flags);
    LOADSHORT (id);
    LOADSHORT (handle);
    LOADSHORT (usage);
    return 0;
}
