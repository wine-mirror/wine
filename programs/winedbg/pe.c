/*
 * File pe.c - handle PE module information
 *
 * Copyright (C) 1996, Eric Youngdale.
 * Copyright (C) 1999, 2000, Ulrich Weigand.
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
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include "wine/exception.h"
#include "wine/debug.h"
#include "excpt.h"
#include "debugger.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

#define MAX_PATHNAME_LEN 1024

typedef struct
{
    DWORD  from;
    DWORD  to;

} OMAP_DATA;

typedef struct tagMSC_DBG_INFO
{
    int			        nsect;
    PIMAGE_SECTION_HEADER       sectp;
    int			        nomap;
    OMAP_DATA*                  omapp;

} MSC_DBG_INFO;

/***********************************************************************
 *           DEBUG_LocateDebugInfoFile
 *
 * NOTE: dbg_filename must be at least MAX_PATHNAME_LEN bytes in size
 */
static void DEBUG_LocateDebugInfoFile(const char* filename, char* dbg_filename)
{
    char*       str1 = DBG_alloc(MAX_PATHNAME_LEN);
    char*       str2 = DBG_alloc(MAX_PATHNAME_LEN*10);
    const char* file;
    char*       name_part;

    file = strrchr(filename, '\\');
    if (file == NULL) file = filename; else file++;

    if ((GetEnvironmentVariable("_NT_SYMBOL_PATH", str1, MAX_PATHNAME_LEN) &&
	 (SearchPath(str1, file, NULL, MAX_PATHNAME_LEN*10, str2, &name_part))) ||
	(GetEnvironmentVariable("_NT_ALT_SYMBOL_PATH", str1, MAX_PATHNAME_LEN) &&
	 (SearchPath(str1, file, NULL, MAX_PATHNAME_LEN*10, str2, &name_part))) ||
	(SearchPath(NULL, file, NULL, MAX_PATHNAME_LEN*10, str2, &name_part)))
        lstrcpyn(dbg_filename, str2, MAX_PATHNAME_LEN);
    else
        lstrcpyn(dbg_filename, filename, MAX_PATHNAME_LEN);
    DBG_free(str1);
    DBG_free(str2);
}

/***********************************************************************
 *           DEBUG_MapDebugInfoFile
 */
void*	DEBUG_MapDebugInfoFile(const char* name, DWORD offset, DWORD size,
                               HANDLE* hFile, HANDLE* hMap)
{
    DWORD	g_offset;	/* offset aligned on map granuality */
    DWORD	g_size;		/* size to map, with offset aligned */
    char*	ret;

    *hMap = 0;

    if (name != NULL)
    {
        char 	filename[MAX_PATHNAME_LEN];

        DEBUG_LocateDebugInfoFile(name, filename);
        if ((*hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
            return NULL;
    }

    if (!size)
    {
        DWORD file_size = GetFileSize(*hFile, NULL);
        if (file_size == (DWORD)-1) return NULL;
        size = file_size - offset;
    }

    g_offset = offset & ~0xFFFF; /* FIXME: is granularity portable ? */
    g_size = offset + size - g_offset;

    if ((*hMap = CreateFileMapping(*hFile, NULL, PAGE_READONLY, 0, 0, NULL)) == 0)
        return NULL;

    if ((ret = MapViewOfFile(*hMap, FILE_MAP_READ, 0, g_offset, g_size)) != NULL)
        ret += offset - g_offset;

    return ret;
}

/***********************************************************************
 *           DEBUG_UnmapDebugInfoFile
 */
void	DEBUG_UnmapDebugInfoFile(HANDLE hFile, HANDLE hMap, const void* addr)
{
    if (addr) UnmapViewOfFile((void*)addr);
    if (hMap) CloseHandle(hMap);
    if (hFile!=INVALID_HANDLE_VALUE) CloseHandle(hFile);
}

/*========================================================================
 * Process DBG file.
 */
static enum DbgInfoLoad DEBUG_ProcessDBGFile(DBG_MODULE* module,
                                             const char* filename, DWORD timestamp)
{
    enum DbgInfoLoad            dil = DIL_ERROR;
    HANDLE                      hFile = INVALID_HANDLE_VALUE, hMap = 0;
    const BYTE*                 file_map = NULL;
    PIMAGE_SEPARATE_DEBUG_HEADER hdr;
    PIMAGE_DEBUG_DIRECTORY      dbg;
    int                         nDbg;

    WINE_TRACE("Processing DBG file %s\n", filename);

    file_map = DEBUG_MapDebugInfoFile(filename, 0, 0, &hFile, &hMap);
    if (!file_map)
    {
        WINE_ERR("-Unable to peruse .DBG file %s\n", filename);
        goto leave;
    }

    hdr = (PIMAGE_SEPARATE_DEBUG_HEADER) file_map;

    if (hdr->TimeDateStamp != timestamp)
    {
        WINE_ERR("Warning - %s has incorrect internal timestamp\n", filename);
        /*
         *  Well, sometimes this happens to DBG files which ARE REALLY the right .DBG
         *  files but nonetheless this check fails. Anyway, WINDBG (debugger for
         *  Windows by Microsoft) loads debug symbols which have incorrect timestamps.
         */
    }


    dbg = (PIMAGE_DEBUG_DIRECTORY) 
        (file_map + sizeof(*hdr) + hdr->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)
         + hdr->ExportedNamesSize);

    nDbg = hdr->DebugDirectorySize / sizeof(*dbg);

    dil = DEBUG_ProcessDebugDirectory(module, file_map, dbg, nDbg);

leave:
    DEBUG_UnmapDebugInfoFile(hFile, hMap, file_map);
    return dil;
}


/*========================================================================
 * Process MSC debug information in PE file.
 */
enum DbgInfoLoad DEBUG_RegisterMSCDebugInfo(DBG_MODULE* module, HANDLE hFile,
                                            void* _nth, unsigned long nth_ofs)
{
    enum DbgInfoLoad	   dil = DIL_ERROR;
    PIMAGE_NT_HEADERS      nth = (PIMAGE_NT_HEADERS)_nth;
    PIMAGE_DATA_DIRECTORY  dir = nth->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DEBUG;
    PIMAGE_DEBUG_DIRECTORY dbg = NULL;
    int                    nDbg;
    MSC_DBG_INFO           extra_info = { 0, NULL, 0, NULL };
    HANDLE	           hMap = 0;
    const BYTE*            file_map = NULL;

    /* Read in section data */

    module->msc_dbg_info = &extra_info;
    extra_info.nsect = nth->FileHeader.NumberOfSections;
    extra_info.sectp = DBG_alloc(extra_info.nsect * sizeof(IMAGE_SECTION_HEADER));
    if (!extra_info.sectp)
        goto leave;

    if (!DEBUG_READ_MEM_VERBOSE((char*)module->load_addr +
                                nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
                                nth->FileHeader.SizeOfOptionalHeader,
                                extra_info.sectp,
                                extra_info.nsect * sizeof(IMAGE_SECTION_HEADER)))
        goto leave;

    /* Read in debug directory */

    nDbg = dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    if (!nDbg)
        goto leave;

    dbg = (PIMAGE_DEBUG_DIRECTORY) DBG_alloc(nDbg * sizeof(IMAGE_DEBUG_DIRECTORY));
    if (!dbg)
        goto leave;

    if (!DEBUG_READ_MEM_VERBOSE((char*)module->load_addr + dir->VirtualAddress,
                                dbg, nDbg * sizeof(IMAGE_DEBUG_DIRECTORY)))
        goto leave;


    /* Map in PE file */
    file_map = DEBUG_MapDebugInfoFile(NULL, 0, 0, &hFile, &hMap);
    if (!file_map)
        goto leave;


    /* Parse debug directory */

    if (nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    {
        /* Debug info is stripped to .DBG file */

        PIMAGE_DEBUG_MISC misc = (PIMAGE_DEBUG_MISC)(file_map + dbg->PointerToRawData);

        if (nDbg != 1 || dbg->Type != IMAGE_DEBUG_TYPE_MISC
                       || misc->DataType != IMAGE_DEBUG_MISC_EXENAME)
        {
            WINE_ERR("-Debug info stripped, but no .DBG file in module %s\n",
                     module->module_name);
            goto leave;
        }

        dil = DEBUG_ProcessDBGFile(module, misc->Data, nth->FileHeader.TimeDateStamp);
    }
    else
    {
        /* Debug info is embedded into PE module */
        /* FIXME: the nDBG information we're manipulating comes from the debuggee
         * address space. However, the following code will be made against the
         * version mapped in the debugger address space. There are cases (for example
         * when the PE sections are compressed in the file and become decompressed
         * in the debuggee address space) where the two don't match.
         * Therefore, redo the DBG information lookup with the mapped data
         */
        PIMAGE_NT_HEADERS      mpd_nth = (PIMAGE_NT_HEADERS)(file_map + nth_ofs);
        PIMAGE_DATA_DIRECTORY  mpd_dir;
        PIMAGE_DEBUG_DIRECTORY mpd_dbg = NULL;

        /* sanity checks */
        if (mpd_nth->Signature != IMAGE_NT_SIGNATURE ||
             mpd_nth->FileHeader.NumberOfSections != nth->FileHeader.NumberOfSections ||
             (mpd_nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) != 0)
            goto leave;
        mpd_dir = mpd_nth->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DEBUG;

        if ((mpd_dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY)) != nDbg)
            goto leave;

        mpd_dbg = (PIMAGE_DEBUG_DIRECTORY)(file_map + mpd_dir->VirtualAddress);

        dil = DEBUG_ProcessDebugDirectory(module, file_map, mpd_dbg, nDbg);
    }


leave:
    module->msc_dbg_info = NULL;

    DEBUG_UnmapDebugInfoFile(0, hMap, file_map);
    if (extra_info.sectp) DBG_free(extra_info.sectp);
    if (dbg) DBG_free(dbg);
    return dil;
}


/*========================================================================
 * look for stabs information in PE header (it's how mingw compiler provides its
 * debugging information), and also wine PE <-> ELF linking through .wsolnk sections
 */
enum DbgInfoLoad DEBUG_RegisterStabsDebugInfo(DBG_MODULE* module, HANDLE hFile,
					      void* _nth, unsigned long nth_ofs)
{
    IMAGE_SECTION_HEADER	pe_seg;
    unsigned long		pe_seg_ofs;
    int 		      	i, stabsize = 0, stabstrsize = 0;
    unsigned int 		stabs = 0, stabstr = 0;
    PIMAGE_NT_HEADERS		nth = (PIMAGE_NT_HEADERS)_nth;
    enum DbgInfoLoad		dil = DIL_ERROR;

    pe_seg_ofs = nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
	nth->FileHeader.SizeOfOptionalHeader;

    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg))
    {
        if (!DEBUG_READ_MEM_VERBOSE((void*)((char*)module->load_addr + pe_seg_ofs),
                                    &pe_seg, sizeof(pe_seg)))
            continue;

        if (!strcasecmp(pe_seg.Name, ".stab"))
        {
            stabs = pe_seg.VirtualAddress;
            stabsize = pe_seg.SizeOfRawData;
        }
        else if (!strncasecmp(pe_seg.Name, ".stabstr", 8))
        {
            stabstr = pe_seg.VirtualAddress;
            stabstrsize = pe_seg.SizeOfRawData;
        }
    }

    if (stabstrsize && stabsize)
    {
        char*	s1 = DBG_alloc(stabsize+stabstrsize);

        if (s1)
        {
            if (DEBUG_READ_MEM_VERBOSE((char*)module->load_addr + stabs, s1, stabsize) &&
                DEBUG_READ_MEM_VERBOSE((char*)module->load_addr + stabstr,
                                       s1 + stabsize, stabstrsize))
            {
                dil = DEBUG_ParseStabs(s1, 0, 0, stabsize, stabsize, stabstrsize);
            }
            else
            {
                DEBUG_Printf("couldn't read data block\n");
            }
            DBG_free(s1);
        }
        else 
        {
            DEBUG_Printf("couldn't alloc %d bytes\n", stabsize + stabstrsize);
        }
    }
    else
    {
        dil = DIL_NOINFO;
    }
    return dil;
}

/***********************************************************************
 *			DEBUG_RegisterPEDebugInfo
 */
enum DbgInfoLoad	DEBUG_RegisterPEDebugInfo(DBG_MODULE* wmod, HANDLE hFile,
						  void* _nth, unsigned long nth_ofs)
{
    DBG_VALUE			value;
    char			buffer[512];
    char			bufstr[256];
    unsigned int 		i;
    IMAGE_SECTION_HEADER 	pe_seg;
    DWORD			pe_seg_ofs;
    IMAGE_DATA_DIRECTORY 	dir;
    DWORD			dir_ofs;
    const char*			prefix;
    IMAGE_NT_HEADERS*		nth = (PIMAGE_NT_HEADERS)_nth;
    void*			base = wmod->load_addr;

    value.type = NULL;
    value.cookie = DV_TARGET;
    value.addr.seg = 0;
    value.addr.off = 0;

    /* Add start of DLL */
    value.addr.off = (unsigned long)base;
    if ((prefix = strrchr(wmod->module_name, '\\'))) prefix++;
    else prefix = wmod->module_name;

    DEBUG_AddSymbol(prefix, &value, NULL, SYM_WIN32 | SYM_FUNC);

    /* Add entry point */
    snprintf(buffer, sizeof(buffer), "%s.EntryPoint", prefix);
    value.addr.off = (unsigned long)base + nth->OptionalHeader.AddressOfEntryPoint;
    DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);

    /* Add start of sections */
    pe_seg_ofs = nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
	nth->FileHeader.SizeOfOptionalHeader;

    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg))
    {
	if (!DEBUG_READ_MEM_VERBOSE((char*)base + pe_seg_ofs, &pe_seg, sizeof(pe_seg)))
	    continue;
	snprintf(buffer, sizeof(buffer), "%s.%s", prefix, pe_seg.Name);
	value.addr.off = (unsigned long)base + pe_seg.VirtualAddress;
	DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
    }

    /* Add exported functions */
    dir_ofs = nth_ofs +
	OFFSET_OF(IMAGE_NT_HEADERS,
		  OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    if (DEBUG_READ_MEM_VERBOSE((char*)base + dir_ofs, &dir, sizeof(dir)) && dir.Size)
    {
	IMAGE_EXPORT_DIRECTORY 	exports;
	WORD*			ordinals = NULL;
	void**			functions = NULL;
	DWORD*			names = NULL;
	unsigned int		j;

	if (DEBUG_READ_MEM_VERBOSE((char*)base + dir.VirtualAddress,
				   &exports, sizeof(exports)) &&

	    ((functions = DBG_alloc(sizeof(functions[0]) * exports.NumberOfFunctions))) &&
	    DEBUG_READ_MEM_VERBOSE((char*)base + exports.AddressOfFunctions,
				   functions, sizeof(functions[0]) * exports.NumberOfFunctions) &&

	    ((ordinals = DBG_alloc(sizeof(ordinals[0]) * exports.NumberOfNames))) &&
	    DEBUG_READ_MEM_VERBOSE((char*)base + (DWORD)exports.AddressOfNameOrdinals,
				   ordinals, sizeof(ordinals[0]) * exports.NumberOfNames) &&

	    ((names = DBG_alloc(sizeof(names[0]) * exports.NumberOfNames))) &&
	    DEBUG_READ_MEM_VERBOSE((char*)base + (DWORD)exports.AddressOfNames,
				   names, sizeof(names[0]) * exports.NumberOfNames))
        {

	    for (i = 0; i < exports.NumberOfNames; i++)
            {
		if (!names[i] ||
		    !DEBUG_READ_MEM_VERBOSE((char*)base + names[i], bufstr, sizeof(bufstr)))
		    continue;
		bufstr[sizeof(bufstr) - 1] = 0;
		snprintf(buffer, sizeof(buffer), "%s.%s", prefix, bufstr);
		value.addr.off = (unsigned long)base + (DWORD)functions[ordinals[i]];
		DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
	    }

	    for (i = 0; i < exports.NumberOfFunctions; i++)
            {
		if (!functions[i]) continue;
		/* Check if we already added it with a name */
		for (j = 0; j < exports.NumberOfNames; j++)
		    if ((ordinals[j] == i) && names[j]) break;
		if (j < exports.NumberOfNames) continue;
		snprintf(buffer, sizeof(buffer), "%s.%ld", prefix, i + exports.Base);
		value.addr.off = (unsigned long)base + (DWORD)functions[i];
		DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
	    }
	}
	DBG_free(functions);
	DBG_free(ordinals);
	DBG_free(names);
    }
    /* no real debug info, only entry points */
    return DIL_NOINFO;
}

/***********************************************************************
 *			DEBUG_LoadPEModule
 */
void	DEBUG_LoadPEModule(const char* name, HANDLE hFile, void* base)
{
     IMAGE_NT_HEADERS		pe_header;
     DWORD			nth_ofs;
     DBG_MODULE*		wmod = NULL;
     int 			i;
     IMAGE_SECTION_HEADER 	pe_seg;
     DWORD			pe_seg_ofs;
     DWORD			size = 0;
     enum DbgInfoLoad		dil = DIL_ERROR;

     /* grab PE Header */
     if (!DEBUG_READ_MEM_VERBOSE((char*)base + OFFSET_OF(IMAGE_DOS_HEADER, e_lfanew),
				 &nth_ofs, sizeof(nth_ofs)) ||
	 !DEBUG_READ_MEM_VERBOSE((char*)base + nth_ofs, &pe_header, sizeof(pe_header)))
	 return;

     pe_seg_ofs = nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
	 pe_header.FileHeader.SizeOfOptionalHeader;

     for (i = 0; i < pe_header.FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg))
     {
	 if (!DEBUG_READ_MEM_VERBOSE((char*)base + pe_seg_ofs, &pe_seg, sizeof(pe_seg)))
	     continue;
	 if (size < pe_seg.VirtualAddress + pe_seg.SizeOfRawData)
	     size = pe_seg.VirtualAddress + pe_seg.SizeOfRawData;
     }

     /* FIXME: we make the assumption that hModule == base */
     wmod = DEBUG_AddModule(name, DMT_PE, base, size, (HMODULE)base);

     if (wmod)
     {
	 dil = DEBUG_RegisterStabsDebugInfo(wmod, hFile, &pe_header, nth_ofs);
	 if (dil != DIL_LOADED)
	     dil = DEBUG_RegisterMSCDebugInfo(wmod, hFile, &pe_header, nth_ofs);
	 if (dil != DIL_LOADED)
	     dil = DEBUG_RegisterPEDebugInfo(wmod, hFile, &pe_header, nth_ofs);
	 wmod->dil = dil;
     }

     DEBUG_ReportDIL(dil, "32bit DLL", name, base);
}
