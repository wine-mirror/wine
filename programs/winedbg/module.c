/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * File module.c - module handling for the wine debugger
 *
 * Copyright (C) 1993, Eric Youngdale.
 * 		 2000, Eric Pouech
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
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debugger.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

/***********************************************************************
 * Creates and links a new module to the current process
 *
 */
static DBG_MODULE* DEBUG_AddModule(const char* name, enum DbgModuleType type,
                                   void* mod_addr, unsigned long size, HMODULE hmodule)
{
    DBG_MODULE*	wmod;

    if (!(wmod = (DBG_MODULE*)DBG_alloc(sizeof(*wmod))))
	return NULL;

    memset(wmod, 0, sizeof(*wmod));

    wmod->dil = DIL_DEFERRED;
    wmod->main = (DEBUG_CurrProcess->num_modules == 0);
    wmod->type = type;
    wmod->load_addr = mod_addr;
    wmod->size = size;
    wmod->handle = hmodule;
    wmod->dbg_index = DEBUG_CurrProcess->next_index;
    wmod->module_name = DBG_strdup(name);

    DEBUG_CurrProcess->modules = DBG_realloc(DEBUG_CurrProcess->modules,
					     ++DEBUG_CurrProcess->num_modules * sizeof(DBG_MODULE*));
    DEBUG_CurrProcess->modules[DEBUG_CurrProcess->num_modules - 1] = wmod;

    return wmod;
}

/***********************************************************************
 *	DEBUG_FindModuleByName
 *
 */
DBG_MODULE*	DEBUG_FindModuleByName(const char* name, enum DbgModuleType type)
{
     int		i;
     DBG_MODULE**	amod = DEBUG_CurrProcess->modules;

     for (i = 0; i < DEBUG_CurrProcess->num_modules; i++) {
	 if ((type == DMT_UNKNOWN || type == amod[i]->type) &&
	     !strcasecmp(name, amod[i]->module_name))
	     return amod[i];
     }
     return NULL;
}

/***********************************************************************
 *	DEBUG_FindModuleByAddr
 *
 * either the addr where module is loaded, or any address inside the
 * module
 */
DBG_MODULE*	DEBUG_FindModuleByAddr(void* addr, enum DbgModuleType type)
{
     int		i;
     DBG_MODULE**	amod = DEBUG_CurrProcess->modules;
     DBG_MODULE*	res = NULL;

     for (i = 0; i < DEBUG_CurrProcess->num_modules; i++) {
	 if ((type == DMT_UNKNOWN || type == amod[i]->type) &&
	     (char *)addr >= (char *)amod[i]->load_addr &&
	     (char *)addr < (char *)amod[i]->load_addr + amod[i]->size) {
	     /* amod[i] contains it... check against res now */
	     if (!res || res->load_addr < amod[i]->load_addr)
		 res = amod[i];
	 }
     }
     return res;
}

/***********************************************************************
 *		DEBUG_FindModuleByHandle
 */
DBG_MODULE*	DEBUG_FindModuleByHandle(HANDLE handle, enum DbgModuleType type)
{
     int		i;
     DBG_MODULE**	amod = DEBUG_CurrProcess->modules;

     for (i = 0; i < DEBUG_CurrProcess->num_modules; i++) {
	 if ((type == DMT_UNKNOWN || type == amod[i]->type) &&
	     handle == amod[i]->handle)
	     return amod[i];
     }
     return NULL;
}

/***********************************************************************
 *		DEBUG_GetProcessMainModule
 */
DBG_MODULE*	DEBUG_GetProcessMainModule(DBG_PROCESS* process)
{
    if (!process || !process->num_modules)	return NULL;

    /* main module is the first to be loaded on a given process, so it's the first
     * in the array */
    assert(process->modules[0]->main);
    return process->modules[0];
}

/***********************************************************************
 *			DEBUG_RegisterELFModule
 *
 * ELF modules are also entered into the list - this is so that we
 * can make 'info shared' types of displays possible.
 */
DBG_MODULE* DEBUG_RegisterELFModule(void *load_addr, unsigned long size, const char* name)
{
    DBG_MODULE*	wmod = DEBUG_AddModule(name, DMT_ELF, load_addr, size, 0);

    if (!wmod) return NULL;

    DEBUG_CurrProcess->next_index++;

    return wmod;
}

/***********************************************************************
 *			DEBUG_RegisterPEModule
 *
 */
static DBG_MODULE* DEBUG_RegisterPEModule(HMODULE hModule, void *load_addr,
                                          unsigned long size, const char *module_name)
{
    DBG_MODULE*	wmod = DEBUG_AddModule(module_name, DMT_PE, load_addr, size, hModule);

    if (!wmod) return NULL;

    DEBUG_CurrProcess->next_index++;

    return wmod;
}

#if 0
/***********************************************************************
 *			DEBUG_RegisterNEModule
 *
 */
static DBG_MODULE* DEBUG_RegisterNEModule(HMODULE hModule, void* load_addr,
                                          unsigned long size, const char *module_name)
{
    DBG_MODULE*	wmod = DEBUG_AddModule(module_name, DMT_NE, load_addr, size, hModule);

    if (!wmod) return NULL;

    DEBUG_CurrProcess->next_index++;
    return wmod;
}

/***********************************************************************
 *           DEBUG_GetEP16
 *
 * Helper function fo DEBUG_LoadModuleEPs16:
 *	finds the address of a given entry point from a given module
 */
static BOOL DEBUG_GetEP16(char* moduleAddr, const NE_MODULE* module,
			  WORD ordinal, DBG_ADDR* addr)
{
    void*		idx;
    ET_ENTRY		entry;
    ET_BUNDLE		bundle;
    SEGTABLEENTRY	ste;

    bundle.next = module->entry_table;
    do {
	if (!bundle.next)
	    return FALSE;
	idx = moduleAddr + bundle.next;
	if (!DEBUG_READ_MEM_VERBOSE(idx, &bundle, sizeof(bundle)))
	    return FALSE;
    } while ((ordinal < bundle.first + 1) || (ordinal > bundle.last));

    if (!DEBUG_READ_MEM_VERBOSE((char*)idx + sizeof(ET_BUNDLE) +
				(ordinal - bundle.first - 1) * sizeof(ET_ENTRY),
				&entry, sizeof(ET_ENTRY)))
	return FALSE;

    addr->seg = entry.segnum;
    addr->off = entry.offs;

    if (addr->seg == 0xfe) addr->seg = 0xffff;  /* constant entry */
    else {
	if (!DEBUG_READ_MEM_VERBOSE(moduleAddr + module->seg_table +
				    sizeof(ste) * (addr->seg - 1),
				    &ste, sizeof(ste)))
	    return FALSE;
	addr->seg = GlobalHandleToSel16(ste.hSeg);
    }
    return TRUE;
}

/***********************************************************************
 *           DEBUG_LoadModule16
 *
 * Load the entry points of a Win16 module into the hash table.
 */
static void DEBUG_LoadModule16(HMODULE hModule, NE_MODULE* module, char* moduleAddr, const char* name)
{
    DBG_VALUE	value;
    BYTE	buf[1 + 256 + 2];
    char 	epname[512];
    char*	cpnt;
    DBG_MODULE*	wmod;

    wmod = DEBUG_RegisterNEModule(hModule, moduleAddr, name);

    value.type = NULL;
    value.cookie = DV_TARGET;
    value.addr.seg = 0;
    value.addr.off = 0;

    cpnt = moduleAddr + module->name_table;

    /* First search the resident names */

    /* skip module name */
    if (!DEBUG_READ_MEM_VERBOSE(cpnt, buf, sizeof(buf)) || !buf[0])
	return;
    cpnt += 1 + buf[0] + sizeof(WORD);

    while (DEBUG_READ_MEM_VERBOSE(cpnt, buf, sizeof(buf)) && buf[0]) {
	snprintf(epname, sizeof(epname), "%s.%.*s", name, buf[0], &buf[1]);
	if (DEBUG_GetEP16(moduleAddr, module, *(WORD*)&buf[1 + buf[0]], &value.addr)) {
	    DEBUG_AddSymbol(epname, &value, NULL, SYM_WIN32 | SYM_FUNC);
	}
	cpnt += buf[0] + 1 + sizeof(WORD);
    }

    /* Now search the non-resident names table */
    if (!module->nrname_handle) return;  /* No non-resident table */
    cpnt = (char *)GlobalLock16(module->nrname_handle);
    while (DEBUG_READ_MEM_VERBOSE(cpnt, buf, sizeof(buf)) && buf[0]) {
	snprintf(epname, sizeof(epname), "%s.%.*s", name, buf[0], &buf[1]);
	if (DEBUG_GetEP16(moduleAddr, module, *(WORD*)&buf[1 + buf[0]], &value.addr)) {
	    DEBUG_AddSymbol(epname, &value, NULL, SYM_WIN32 | SYM_FUNC);
	}
	cpnt += buf[0] + 1 + sizeof(WORD);
    }
    GlobalUnlock16(module->nrname_handle);
}
#endif

/***********************************************************************
 *			DEBUG_LoadModule32
 */
void	DEBUG_LoadModule32(const char* name, HANDLE hFile, void *base)
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
     if (!DEBUG_READ_MEM_VERBOSE( (char *)base + OFFSET_OF(IMAGE_DOS_HEADER, e_lfanew),
				 &nth_ofs, sizeof(nth_ofs)) ||
	 !DEBUG_READ_MEM_VERBOSE( (char *)base + nth_ofs, &pe_header, sizeof(pe_header)))
	 return;

     pe_seg_ofs = nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
	 pe_header.FileHeader.SizeOfOptionalHeader;

     for (i = 0; i < pe_header.FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg)) {
	 if (!DEBUG_READ_MEM_VERBOSE( (char *)base + pe_seg_ofs, &pe_seg, sizeof(pe_seg)))
	     continue;
	 if (size < pe_seg.VirtualAddress + pe_seg.SizeOfRawData)
	     size = pe_seg.VirtualAddress + pe_seg.SizeOfRawData;
     }

     /* FIXME: we make the assumption that hModule == base */
     wmod = DEBUG_RegisterPEModule((HMODULE)base, base, size, name);
     if (wmod) {
	 dil = DEBUG_RegisterStabsDebugInfo(wmod, hFile, &pe_header, nth_ofs);
	 if (dil != DIL_LOADED)
	     dil = DEBUG_RegisterMSCDebugInfo(wmod, hFile, &pe_header, nth_ofs);
	 if (dil != DIL_LOADED)
	     dil = DEBUG_RegisterPEDebugInfo(wmod, hFile, &pe_header, nth_ofs);
	 wmod->dil = dil;
     }

     DEBUG_ReportDIL(dil, "32bit DLL", name, base);
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
    void *			base = wmod->load_addr;

    value.type = NULL;
    value.cookie = DV_TARGET;
    value.addr.seg = 0;
    value.addr.off = 0;

    /* Add start of DLL */
    value.addr.off = (unsigned long)base;
    if ((prefix = strrchr(wmod->module_name, '\\' ))) prefix++;
    else prefix = wmod->module_name;

    DEBUG_AddSymbol(prefix, &value, NULL, SYM_WIN32 | SYM_FUNC);

    /* Add entry point */
    snprintf(buffer, sizeof(buffer), "%s.EntryPoint", prefix);
    value.addr.off = (unsigned long)base + nth->OptionalHeader.AddressOfEntryPoint;
    DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);

    /* Add start of sections */
    pe_seg_ofs = nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
	nth->FileHeader.SizeOfOptionalHeader;

    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg)) {
	if (!DEBUG_READ_MEM_VERBOSE( (char *)base + pe_seg_ofs, &pe_seg, sizeof(pe_seg)))
	    continue;
	snprintf(buffer, sizeof(buffer), "%s.%s", prefix, pe_seg.Name);
	value.addr.off = (unsigned long)base + pe_seg.VirtualAddress;
	DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
    }

    /* Add exported functions */
    dir_ofs = nth_ofs +
	OFFSET_OF(IMAGE_NT_HEADERS,
		  OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    if (DEBUG_READ_MEM_VERBOSE( (char *)base + dir_ofs, &dir, sizeof(dir)) && dir.Size) {
	IMAGE_EXPORT_DIRECTORY 	exports;
	WORD*			ordinals = NULL;
	void**			functions = NULL;
	DWORD*			names = NULL;
	unsigned int		j;

	if (DEBUG_READ_MEM_VERBOSE( (char *)base + dir.VirtualAddress,
				   &exports, sizeof(exports)) &&

	    ((functions = DBG_alloc(sizeof(functions[0]) * exports.NumberOfFunctions))) &&
	    DEBUG_READ_MEM_VERBOSE( (char *)base + exports.AddressOfFunctions,
				   functions, sizeof(functions[0]) * exports.NumberOfFunctions) &&

	    ((ordinals = DBG_alloc(sizeof(ordinals[0]) * exports.NumberOfNames))) &&
	    DEBUG_READ_MEM_VERBOSE( (char *)base + (DWORD)exports.AddressOfNameOrdinals,
				   ordinals, sizeof(ordinals[0]) * exports.NumberOfNames) &&

	    ((names = DBG_alloc(sizeof(names[0]) * exports.NumberOfNames))) &&
	    DEBUG_READ_MEM_VERBOSE( (char *)base + (DWORD)exports.AddressOfNames,
				   names, sizeof(names[0]) * exports.NumberOfNames)) {

	    for (i = 0; i < exports.NumberOfNames; i++) {
		if (!names[i] ||
		    !DEBUG_READ_MEM_VERBOSE( (char *)base + names[i], bufstr, sizeof(bufstr)))
		    continue;
		bufstr[sizeof(bufstr) - 1] = 0;
		snprintf(buffer, sizeof(buffer), "%s.%s", prefix, bufstr);
		value.addr.off = (unsigned long)base + (DWORD)functions[ordinals[i]];
		DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
	    }

	    for (i = 0; i < exports.NumberOfFunctions; i++) {
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
 *           	DEBUG_LoadEntryPoints
 *
 * Load the entry points of all the modules into the hash table.
 */
int DEBUG_LoadEntryPoints(const char* pfx)
{
    int		first = 0;
    /* FIXME: with address space separation in space, this is plain wrong
     *	      it requires the 16 bit WOW debugging interface...
     */
#if 0
    MODULEENTRY	entry;
    NE_MODULE	module;
    void*	moduleAddr;
    int		rowcount = 0;
    int		len;

    /* FIXME: we assume that a module is never removed from memory */
    /* FIXME: this is (currently plain wrong when debugger is started by
     *	      attaching to an existing program => the 16 bit modules will
     *        not be shared... not much to do on debugger side... sigh
     */
    if (ModuleFirst16(&entry)) do {
	if (DEBUG_FindModuleByName(entry.szModule, DM_TYPE_UNKNOWN) ||
	    !(moduleAddr = NE_GetPtr(entry.hModule)) ||
	    !DEBUG_READ_MEM_VERBOSE(moduleAddr, &module, sizeof(module)) ||
	    (module.flags & NE_FFLAGS_WIN32) /* NE module */)
	    continue;
	if (!first) {
	    if (pfx) DEBUG_Printf(pfx);
	    DEBUG_Printf("   ");
	    rowcount = 3 + (pfx ? strlen(pfx) : 0);
	    first = 1;
	}

	len = strlen(entry.szModule);
	if ((rowcount + len) > 76) {
	    DEBUG_Printf("\n   ");
	    rowcount = 3;
	}
	DEBUG_Printf(" %s", entry.szModule);
	rowcount += len + 1;

	DEBUG_LoadModule16(entry.hModule, &module, moduleAddr, entry.szModule);
    } while (ModuleNext16(&entry));
#endif

    if (first) DEBUG_Printf("\n");
    return first;
}

void	DEBUG_ReportDIL(enum DbgInfoLoad dil, const char* pfx, const char* filename, void *load_addr)
{
    const char*	fmt;

    switch (dil) {
    case DIL_DEFERRED:
	fmt = "Deferring debug information loading for %s '%s' (%p)\n";
	break;
    case DIL_LOADED:
	fmt = "Loaded debug information from %s '%s' (%p)\n";
	break;
    case DIL_NOINFO:
	fmt = "No debug information in %s '%s' (%p)\n";
	break;
    case DIL_ERROR:
	fmt = "Can't find file for %s '%s' (%p)\n";
	break;
    default:
	WINE_ERR("Oooocch (%d)\n", dil);
	return;
    }

    DEBUG_Printf(fmt, pfx, filename, load_addr);
}

static const char*      DEBUG_GetModuleType(enum DbgModuleType type)
{
    switch (type) {
    case DMT_NE:    return "NE";
    case DMT_PE:    return "PE";
    case DMT_ELF:   return "ELF";
    default:        return "???";
    }
}

static const char*      DEBUG_GetDbgInfo(enum DbgInfoLoad dil)
{
    switch (dil) {
    case DIL_LOADED:	return "loaded";
    case DIL_DEFERRED: 	return "deferred";
    case DIL_NOINFO:	return "none";
    case DIL_ERROR:	return "error";
    default:            return "?";
    }
}

/***********************************************************************
 *           DEBUG_ModuleCompare
 *
 * returns -1 is p1 < p2, 0 is p1 == p2, +1 if p1 > p2
 * order used is order on load_addr of a module
 */
static int	DEBUG_ModuleCompare(const void* p1, const void* p2)
{
    return (char*)(*((const DBG_MODULE**)p1))->load_addr -
	   (char*)(*((const DBG_MODULE**)p2))->load_addr;
}

/***********************************************************************
 *           DEBUG_IsContainer
 *
 * returns TRUE is wmod_child is contained (inside bounds) of wmod_cntnr
 */
static inline BOOL DEBUG_IsContainer(const DBG_MODULE* wmod_cntnr,
				     const DBG_MODULE* wmod_child)
{
    return wmod_cntnr->load_addr < wmod_child->load_addr &&
	(DWORD)wmod_cntnr->load_addr + wmod_cntnr->size >
	(DWORD)wmod_child->load_addr + wmod_child->size;
}

static void	DEBUG_InfoShareModule(const DBG_MODULE* module, int ident)
{
    if (ident) DEBUG_Printf("  \\-");
    DEBUG_Printf("%s\t0x%08lx-%08lx\t%s\n",
		 DEBUG_GetModuleType(module->type),
		 (DWORD)module->load_addr, (DWORD)module->load_addr + module->size,
		 module->module_name);
}

/***********************************************************************
 *           DEBUG_InfoShare
 *
 * Display shared libarary information.
 */
void DEBUG_InfoShare(void)
{
    DBG_MODULE**	ref;
    int			i, j;

    ref = DBG_alloc(sizeof(DBG_MODULE*) * DEBUG_CurrProcess->num_modules);
    if (!ref) return;

    DEBUG_Printf("Module\tAddress\t\t\tName\t%d modules\n",
		 DEBUG_CurrProcess->num_modules);

    memcpy(ref, DEBUG_CurrProcess->modules,
	   sizeof(DBG_MODULE*) * DEBUG_CurrProcess->num_modules);
    qsort(ref, DEBUG_CurrProcess->num_modules, sizeof(DBG_MODULE*),
	  DEBUG_ModuleCompare);
    for (i = 0; i < DEBUG_CurrProcess->num_modules; i++) {
	switch (ref[i]->type) {
	case DMT_ELF:
	    DEBUG_InfoShareModule(ref[i], 0);
	    for (j = 0; j < DEBUG_CurrProcess->num_modules; j++) {
		if (ref[j]->type != DMT_ELF && DEBUG_IsContainer(ref[i], ref[j]))
		    DEBUG_InfoShareModule(ref[j], 1);
	    }
	    break;
	case DMT_NE:
	case DMT_PE:
	    /* check module is not in ELF */
	    for (j = 0; j < DEBUG_CurrProcess->num_modules; j++) {
		if (ref[j]->type == DMT_ELF &&
		    DEBUG_IsContainer(ref[j], ref[i]))
		    break;
	    }
	    if (j >= DEBUG_CurrProcess->num_modules)
		DEBUG_InfoShareModule(ref[i], 0);
	    break;
	default:
	    WINE_ERR("Unknown type (%d)\n", ref[i]->type);
	}
    }
    DBG_free(ref);
}

/***********************************************************************
 *           DEBUG_DumpModule
 * Display information about a given module (DLL or EXE)
 */
void DEBUG_DumpModule(DWORD mod)
{
    DBG_MODULE*	wmod;

    if (!(wmod = DEBUG_FindModuleByHandle((HANDLE)mod, DMT_UNKNOWN)) &&
	!(wmod = DEBUG_FindModuleByAddr((void*)mod, DMT_UNKNOWN))) {
	DEBUG_Printf("'0x%08lx' is not a valid module handle or address\n", mod);
	return;
    }

    DEBUG_Printf("Module '%s' (handle=%p) 0x%08lx-0x%08lx (%s, debug info %s)\n",
		 wmod->module_name, wmod->handle, (DWORD)wmod->load_addr,
		 (DWORD)wmod->load_addr + wmod->size,
		 DEBUG_GetModuleType(wmod->type), DEBUG_GetDbgInfo(wmod->dil));
}

/***********************************************************************
 *           DEBUG_WalkModules
 *
 * Display information about all modules (DLLs and EXEs)
 */
void DEBUG_WalkModules(void)
{
    DBG_MODULE**	amod;
    int			i;

    if (!DEBUG_CurrProcess)
    {
        DEBUG_Printf("Cannot walk classes while no process is loaded\n");
        return;
    }

    DEBUG_Printf("Address\t\t\tModule\tName\n");

    amod = DBG_alloc(sizeof(DBG_MODULE*) * DEBUG_CurrProcess->num_modules);
    if (!amod) return;

    memcpy(amod, DEBUG_CurrProcess->modules,
	   sizeof(DBG_MODULE*) * DEBUG_CurrProcess->num_modules);
    qsort(amod, DEBUG_CurrProcess->num_modules, sizeof(DBG_MODULE*),
	  DEBUG_ModuleCompare);
    for (i = 0; i < DEBUG_CurrProcess->num_modules; i++) {
	if (amod[i]->type == DMT_ELF)	continue;

	DEBUG_Printf("0x%08lx-%08lx\t(%s)\t%s\n",
		     (DWORD)amod[i]->load_addr,
		     (DWORD)amod[i]->load_addr + amod[i]->size,
		     DEBUG_GetModuleType(amod[i]->type), amod[i]->module_name);
    }
    DBG_free(amod);
}
