/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * File module.c - module handling for the wine debugger
 *
 * Copyright (C) 1993, Eric Youngdale.
 * 		 2000, Eric Pouech
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "neexe.h"
#include "module.h"
#include "file.h"
#include "debugger.h"
#include "toolhelp.h"

/***********************************************************************
 * Creates and links a new module to the current process 
 *
 */
DBG_MODULE*	DEBUG_AddModule(const char* name, int type, 
				void* mod_addr, HMODULE hmodule)
{
    DBG_MODULE*	wmod;

    if (!(wmod = (DBG_MODULE*)DBG_alloc(sizeof(*wmod))))
	return NULL;

    memset(wmod, 0, sizeof(*wmod));

    wmod->next = DEBUG_CurrProcess->modules;
    wmod->status = DM_STATUS_NEW;
    wmod->type = type;
    wmod->load_addr = mod_addr;
    wmod->handle = hmodule;
    wmod->dbg_index = DEBUG_CurrProcess->next_index;
    wmod->module_name = DBG_strdup(name);
    DEBUG_CurrProcess->modules = wmod;

    return wmod;
}

/***********************************************************************
 *	DEBUG_FindModuleByName
 *
 */
DBG_MODULE*	DEBUG_FindModuleByName(const char* name, int type)
{
    DBG_MODULE*	wmod;
    
    for (wmod = DEBUG_CurrProcess->modules; wmod; wmod = wmod->next) {
	if ((type == DM_TYPE_UNKNOWN || type == wmod->type) &&
	    !strcasecmp(name, wmod->module_name)) break;
    }
    return wmod;
}

/***********************************************************************
 *	DEBUG_FindModuleByAddr
 *
 * either the addr where module is loaded, or any address inside the 
 * module
 */
DBG_MODULE*	DEBUG_FindModuleByAddr(void* addr, int type)
{
    DBG_MODULE*	wmod;
    DBG_MODULE*	res = NULL;
    
    for (wmod = DEBUG_CurrProcess->modules; wmod; wmod = wmod->next) {
	if ((type == DM_TYPE_UNKNOWN || type == wmod->type) &&
	    (u_long)addr >= (u_long)wmod->load_addr &&
	    (!res || res->load_addr < wmod->load_addr))
	    res = wmod;
    }
    return res;
}

/***********************************************************************
 *		DEBUG_FindModuleByHandle
 */
DBG_MODULE*	DEBUG_FindModuleByHandle(HANDLE handle, int type)
{
    DBG_MODULE*	wmod;
    
    for (wmod = DEBUG_CurrProcess->modules; wmod; wmod = wmod->next) {
	if ((type == DM_TYPE_UNKNOWN || type == wmod->type) && handle == wmod->handle) break;
    }
    return wmod;
}

/***********************************************************************
 *			DEBUG_RegisterELFModule
 *
 * ELF modules are also entered into the list - this is so that we
 * can make 'info shared' types of displays possible.
 */
DBG_MODULE* DEBUG_RegisterELFModule(u_long load_addr, const char* name)
{
    DBG_MODULE*	wmod = DEBUG_AddModule(name, DM_TYPE_ELF, (void*)load_addr, 0);

    if (!wmod) return NULL;

    wmod->status = DM_STATUS_LOADED;
    DEBUG_CurrProcess->next_index++;

    return wmod;
}

/***********************************************************************
 *			DEBUG_RegisterPEModule
 *
 */
DBG_MODULE* DEBUG_RegisterPEModule(HMODULE hModule, u_long load_addr, const char *module_name)
{
    DBG_MODULE*	wmod = DEBUG_AddModule(module_name, DM_TYPE_PE, (void*)load_addr, hModule);

    if (!wmod) return NULL;

    DEBUG_CurrProcess->next_index++;

    return wmod;
}

/***********************************************************************
 *			DEBUG_RegisterNEModule
 *
 */
DBG_MODULE* DEBUG_RegisterNEModule(HMODULE hModule, void* load_addr, const char *module_name)
{
    DBG_MODULE*	wmod = DEBUG_AddModule(module_name, DM_TYPE_NE, load_addr, hModule);

    if (!wmod) return NULL;

    wmod->status = DM_STATUS_LOADED;
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
	sprintf(epname, "%s.%.*s", name, buf[0], &buf[1]);
	if (DEBUG_GetEP16(moduleAddr, module, *(WORD*)&buf[1 + buf[0]], &value.addr)) {
	    DEBUG_AddSymbol(epname, &value, NULL, SYM_WIN32 | SYM_FUNC);
	}
	cpnt += buf[0] + 1 + sizeof(WORD);
    }
    
    /* Now search the non-resident names table */
    if (!module->nrname_handle) return;  /* No non-resident table */
    cpnt = (char *)GlobalLock16(module->nrname_handle);
    while (DEBUG_READ_MEM_VERBOSE(cpnt, buf, sizeof(buf)) && buf[0]) {
	sprintf(epname, "%s.%.*s", name, buf[0], &buf[1]);
	if (DEBUG_GetEP16(moduleAddr, module, *(WORD*)&buf[1 + buf[0]], &value.addr)) {
	    DEBUG_AddSymbol(epname, &value, NULL, SYM_WIN32 | SYM_FUNC);
	}
	cpnt += buf[0] + 1 + sizeof(WORD);
    }
    GlobalUnlock16(module->nrname_handle);
}

/***********************************************************************
 *			DEBUG_LoadModule32
 */
void	DEBUG_LoadModule32(const char* name, HANDLE hFile, DWORD base)
{
    DBG_VALUE			value;
    char			buffer[256];
    char			bufstr[256];
    int 			i;
    IMAGE_NT_HEADERS		pe_header;
    DWORD			pe_header_ofs;
    IMAGE_SECTION_HEADER 	pe_seg;
    DWORD			pe_seg_ofs;
    IMAGE_DATA_DIRECTORY 	dir;
    DWORD			dir_ofs;
    DBG_MODULE*			wmod;

    /* FIXME: we make the assumption that hModule == base */
    wmod = DEBUG_RegisterPEModule((HMODULE)base, base, name);

    DEBUG_Printf(DBG_CHN_TRACE, "Registring 32bit DLL '%s' at %08lx\n", name, base);
    
    value.type = NULL;
    value.cookie = DV_TARGET;
    value.addr.seg = 0;
    value.addr.off = 0;
    
    /* grab PE Header */
    if (!DEBUG_READ_MEM_VERBOSE((void*)(base + OFFSET_OF(IMAGE_DOS_HEADER, e_lfanew)),
				&pe_header_ofs, sizeof(pe_header_ofs)) ||
	!DEBUG_READ_MEM_VERBOSE((void*)(base + pe_header_ofs), 
				&pe_header, sizeof(pe_header)))
	return;
    
    if (wmod) {
	DEBUG_RegisterStabsDebugInfo(wmod, hFile, &pe_header, pe_header_ofs);
	DEBUG_RegisterMSCDebugInfo(wmod, hFile, &pe_header, pe_header_ofs);	
    }

    /* Add start of DLL */
    value.addr.off = base;
    DEBUG_AddSymbol(name, &value, NULL, SYM_WIN32 | SYM_FUNC);
    
    /* Add entry point */
    sprintf(buffer, "%s.EntryPoint", name);
    value.addr.off = base + pe_header.OptionalHeader.AddressOfEntryPoint;
    DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);

    /* Add start of sections */
    pe_seg_ofs = pe_header_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
	pe_header.FileHeader.SizeOfOptionalHeader;
    
    for (i = 0; i < pe_header.FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg)) {
	if (!DEBUG_READ_MEM_VERBOSE((void*)(base + pe_seg_ofs), &pe_seg, sizeof(pe_seg)))
	    continue;
	sprintf(buffer, "%s.%s", name, pe_seg.Name);
	value.addr.off = base + pe_seg.VirtualAddress;
	DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
    }
    
    /* Add exported functions */
    dir_ofs = pe_header_ofs + 
	OFFSET_OF(IMAGE_NT_HEADERS, 
		  OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    if (DEBUG_READ_MEM_VERBOSE((void*)(base + dir_ofs), &dir, sizeof(dir)) && dir.Size) {
	IMAGE_EXPORT_DIRECTORY 	exports;
	WORD*			ordinals = NULL;
	void**			functions = NULL;
	DWORD*			names = NULL;
	int			j;
	
	if (DEBUG_READ_MEM_VERBOSE((void*)(base + dir.VirtualAddress), 
				   &exports, sizeof(exports)) &&
	    
	    ((functions = DBG_alloc(sizeof(functions[0]) * exports.NumberOfFunctions))) &&
	    DEBUG_READ_MEM_VERBOSE((void*)(base + (DWORD)exports.AddressOfFunctions),
				   functions, sizeof(functions[0]) * exports.NumberOfFunctions) &&
	    
	    ((ordinals = DBG_alloc(sizeof(ordinals[0]) * exports.NumberOfNames))) &&
	    DEBUG_READ_MEM_VERBOSE((void*)(base + (DWORD)exports.AddressOfNameOrdinals),
				   ordinals, sizeof(ordinals[0]) * exports.NumberOfNames) &&
	    
	    ((names = DBG_alloc(sizeof(names[0]) * exports.NumberOfNames))) &&
	    DEBUG_READ_MEM_VERBOSE((void*)(base + (DWORD)exports.AddressOfNames),
				   names, sizeof(names[0]) * exports.NumberOfNames)) {
	    
	    for (i = 0; i < exports.NumberOfNames; i++) {
		if (!names[i] ||
		    !DEBUG_READ_MEM_VERBOSE((void*)(base + names[i]), bufstr, sizeof(bufstr)))
		    continue;
		bufstr[sizeof(bufstr) - 1] = 0;
		sprintf(buffer, "%s.%s", name, bufstr);
		value.addr.off = base + (DWORD)functions[ordinals[i]];
		DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
	    }
	    
	    for (i = 0; i < exports.NumberOfFunctions; i++) {
		if (!functions[i]) continue;
		/* Check if we already added it with a name */
		for (j = 0; j < exports.NumberOfNames; j++)
		    if ((ordinals[j] == i) && names[j]) break;
		if (j < exports.NumberOfNames) continue;
		sprintf(buffer, "%s.%ld", name, i + exports.Base);
		value.addr.off = base + (DWORD)functions[i];
		DEBUG_AddSymbol(buffer, &value, NULL, SYM_WIN32 | SYM_FUNC);
	    }
	}
	DBG_free(functions);
	DBG_free(ordinals);
	DBG_free(names);
    }
}

/***********************************************************************
 *           	DEBUG_LoadEntryPoints
 *
 * Load the entry points of all the modules into the hash table.
 */
int DEBUG_LoadEntryPoints(const char* pfx)
{
    MODULEENTRY	entry;
    NE_MODULE	module;
    void*	moduleAddr;
    int		first = 0;
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
	    if (pfx) DEBUG_Printf(DBG_CHN_MESG, pfx);
	    DEBUG_Printf(DBG_CHN_MESG, "   ");
	    rowcount = 3 + (pfx ? strlen(pfx) : 0);
	    first = 1;
	}
	
	len = strlen(entry.szModule);
	if ((rowcount + len) > 76) {
	    DEBUG_Printf(DBG_CHN_MESG, "\n   ");
	    rowcount = 3;
	}
	DEBUG_Printf(DBG_CHN_MESG, " %s", entry.szModule);
	rowcount += len + 1;
	
	DEBUG_LoadModule16(entry.hModule, &module, moduleAddr, entry.szModule);
    } while (ModuleNext16(&entry));
    
    if (first) DEBUG_Printf(DBG_CHN_MESG, "\n"); 
    return first;
}

/***********************************************************************
 *           DEBUG_InfoShare
 *
 * Display shared libarary information.
 */
void DEBUG_InfoShare(void)
{
    DBG_MODULE*	wmod;
    const char*	xtype;

    DEBUG_Printf(DBG_CHN_MESG, "Address\t\tModule\tName\n");

    for (wmod = DEBUG_CurrProcess->modules; wmod; wmod = wmod->next) {
	switch (wmod->type) {
	case DM_TYPE_NE:	xtype = "NE"; break;
	case DM_TYPE_PE:	xtype = "PE"; break;
	case DM_TYPE_ELF:	xtype = "ELF"; break;
	default:		xtype = "???"; break;
	}
	DEBUG_Printf(DBG_CHN_MESG, "0x%8.8x\t(%s)\t%s\n", (unsigned int)wmod->load_addr,
		     xtype, wmod->module_name);
    }
}

static const char*	DEBUG_GetModuleType(int type)
{
    switch (type) {
    case DM_TYPE_NE:	return "NE";
    case DM_TYPE_PE:	return "PE";
    case DM_TYPE_ELF:	return "ELF";
    default:		return "???";;
    }
}

static const char*	DEBUG_GetModuleStatus(int status)
{
    switch (status) {
    case DM_STATUS_NEW:		return "deferred"; 
    case DM_STATUS_LOADED:	return "ok"; 
    case DM_STATUS_ERROR:	return "error"; 
    default:			return "???"; 
    }
}

/***********************************************************************
 *           DEBUG_
 * Display information about a given module (DLL or EXE)
 */
void DEBUG_DumpModule(DWORD mod)
{
    DBG_MODULE*	wmod;

    if (!(wmod = DEBUG_FindModuleByHandle(mod, DM_TYPE_UNKNOWN)) &&
	!(wmod = DEBUG_FindModuleByAddr((void*)mod, DM_TYPE_UNKNOWN))) {
	DEBUG_Printf(DBG_CHN_MESG, "'0x%08lx' is not a valid module handle or address\n", mod);
	return;
    }

    DEBUG_Printf(DBG_CHN_MESG, "Module '%s' (handle=0x%08x) at 0x%8.8x (%s/%s)\n",
		 wmod->module_name, wmod->handle, (unsigned int)wmod->load_addr,
		 DEBUG_GetModuleType(wmod->type), DEBUG_GetModuleStatus(wmod->status));
}

/***********************************************************************
 *           DEBUG_WalkModules
 *
 * Display information about all modules (DLLs and EXEs)
 */
void DEBUG_WalkModules(void)
{
    DBG_MODULE*	wmod;
    const char*	xtype;

    DEBUG_Printf(DBG_CHN_MESG, "Address\t\tModule\tName\n");

    for (wmod = DEBUG_CurrProcess->modules; wmod; wmod = wmod->next) {
	switch (wmod->type) {
	case DM_TYPE_NE:	xtype = "NE"; break;
	case DM_TYPE_PE:	xtype = "PE"; break;
	case DM_TYPE_ELF:	continue;
	default:		xtype = "???"; break;
	}
	
	DEBUG_Printf(DBG_CHN_MESG, "0x%8.8x\t(%s)\t%s\n", 
		     (unsigned int)wmod->load_addr, DEBUG_GetModuleType(wmod->type), 
		     wmod->module_name);
    }
}

