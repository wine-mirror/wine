/*
 * File module.c - module handling for the wine debugger
 *
 * Copyright (C) 1993,      Eric Youngdale.
 * 		 2000-2004, Eric Pouech
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
#include <assert.h>

#include "dbghelp_private.h"
#include "psapi.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static void module_fill_module(const char* in, char* out, unsigned size)
{
    const char          *ptr,*endptr;
    unsigned            len;

    endptr = in + strlen(in);
    for (ptr = endptr - 1;
         ptr >= in && *ptr != '/' && *ptr != '\\';
         ptr--);
    ptr++;
    len = min(endptr-ptr,size-1);
    memcpy(out, ptr, len);
    out[len] = '\0';
    if (len > 4 && 
        (!strcasecmp(&out[len - 4], ".dll") || !strcasecmp(&out[len - 4], ".exe")))
        out[len - 4] = '\0';
    else if (((len > 12 && out[len - 13] == '/') || len == 12) && 
             (!strcasecmp(out + len - 12, "wine-pthread") || 
              !strcasecmp(out + len - 12, "wine-kthread")))
        lstrcpynA(out, "<wine-loader>",size);
    else
    {
        if (len > 7 && 
            (!strcasecmp(&out[len - 7], ".dll.so") || !strcasecmp(&out[len - 7], ".exe.so")))
            strcpy(&out[len - 7], "<elf>");
        else if (len > 7 &&
                 out[len - 7] == '.' && !strcasecmp(&out[len - 3], ".so"))
        {
            if (len + 3 < size) strcpy(&out[len - 3], "<elf>");
            else WARN("Buffer too short: %s\n", out);
        }
    }
    while ((*out = tolower(*out))) out++;
}

/***********************************************************************
 * Creates and links a new module to a process 
 */
struct module* module_new(struct process* pcs, const char* name, 
                          enum module_type type, 
                          unsigned long mod_addr, unsigned long size,
                          unsigned long stamp, unsigned long checksum) 
{
    struct module*      module;

    assert(type == DMT_ELF || type == DMT_PE);
    if (!(module = HeapAlloc(GetProcessHeap(), 0, sizeof(*module))))
	return NULL;

    memset(module, 0, sizeof(*module));

    module->next = pcs->lmodules;
    pcs->lmodules = module;

    TRACE("=> %s %08lx-%08lx %s\n", 
          type == DMT_ELF ? "ELF" : (type == DMT_PE ? "PE" : "---"),
          mod_addr, mod_addr + size, name);

    pool_init(&module->pool, 65536);
    
    module->module.SizeOfStruct = sizeof(module->module);
    module->module.BaseOfImage = mod_addr;
    module->module.ImageSize = size;
    module_fill_module(name, module->module.ModuleName,
                       sizeof(module->module.ModuleName));
    module->module.ImageName[0] = '\0';
    lstrcpynA(module->module.LoadedImageName, name, sizeof(module->module.LoadedImageName));
    module->module.SymType = SymNone;
    module->module.NumSyms = 0;
    module->module.TimeDateStamp = stamp;
    module->module.CheckSum = checksum;

    module->type              = type;
    module->sortlist_valid    = FALSE;
    module->addr_sorttab      = NULL;
    /* FIXME: this seems a bit too high (on a per module basis)
     * need some statistics about this
     */
    hash_table_init(&module->pool, &module->ht_symbols, 4096);
    hash_table_init(&module->pool, &module->ht_types,   4096);
    vector_init(&module->vtypes, sizeof(struct symt*),  32);

    module->sources_used      = 0;
    module->sources_alloc     = 0;
    module->sources           = 0;

    return module;
}

/***********************************************************************
 *	module_find_by_name
 *
 */
struct module* module_find_by_name(const struct process* pcs, 
                                   const char* name, enum module_type type)
{
    struct module*      module;

    if (type == DMT_UNKNOWN)
    {
        if ((module = module_find_by_name(pcs, name, DMT_PE)) ||
            (module = module_find_by_name(pcs, name, DMT_ELF)))
            return module;
    }
    else
    {
        char                modname[MAX_PATH];

        for (module = pcs->lmodules; module; module = module->next)
        {
            if (type == module->type &&
                !strcasecmp(name, module->module.LoadedImageName)) 
                return module;
        }
        module_fill_module(name, modname, sizeof(modname));
        for (module = pcs->lmodules; module; module = module->next)
        {
            if (type == module->type &&
                !strcasecmp(modname, module->module.ModuleName)) 
                return module;
        }
    }
    SetLastError(ERROR_INVALID_NAME);
    return NULL;
}

/***********************************************************************
 *           module_get_container
 *
 */
struct module* module_get_container(const struct process* pcs, 
                                    const struct module* inner)
{
    struct module*      module;
     
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module != inner &&
            module->module.BaseOfImage <= inner->module.BaseOfImage &&
            module->module.BaseOfImage + module->module.ImageSize >=
            inner->module.BaseOfImage + inner->module.ImageSize)
            return module;
    }
    return NULL;
}

/***********************************************************************
 *           module_get_containee
 *
 */
struct module* module_get_containee(const struct process* pcs, 
                                    const struct module* outter)
{
    struct module*      module;
     
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module != outter &&
            outter->module.BaseOfImage <= module->module.BaseOfImage &&
            outter->module.BaseOfImage + outter->module.ImageSize >=
            module->module.BaseOfImage + module->module.ImageSize)
            return module;
    }
    return NULL;
}

/******************************************************************
 *		module_get_debug
 *
 * get the debug information from a module:
 * - if the module's type is deferred, then force loading of debug info (and return
 *   the module itself)
 * - if the module has no debug info and has an ELF container, then return the ELF
 *   container (and also force the ELF container's debug info loading if deferred)
 * - otherwise return the module itself if it has some debug info
 */
struct module* module_get_debug(const struct process* pcs, struct module* module)
{
    struct module*      parent;

    if (!module) return NULL;
    /* for a PE builtin, always get info from parent */
    if ((parent = module_get_container(pcs, module)))
        module = parent;
    /* if deferred, force loading */
    if (module->module.SymType == SymDeferred)
    {
        BOOL ret;

        switch (module->type)
        {
        case DMT_ELF: ret = elf_load_debug_info(module, NULL);  break;
        case DMT_PE:  ret = pe_load_debug_info(pcs, module);    break;
        default:      ret = FALSE;                              break;
        }
        if (!ret) module->module.SymType = SymNone;
        assert(module->module.SymType != SymDeferred);
    }
    return (module && module->module.SymType != SymNone) ? module : NULL;
}

/***********************************************************************
 *	module_find_by_addr
 *
 * either the addr where module is loaded, or any address inside the 
 * module
 */
struct module* module_find_by_addr(const struct process* pcs, unsigned long addr, 
                                   enum module_type type)
{
    struct module*      module;
    
    if (type == DMT_UNKNOWN)
    {
        if ((module = module_find_by_addr(pcs, addr, DMT_PE)) ||
            (module = module_find_by_addr(pcs, addr, DMT_ELF)))
            return module;
    }
    else
    {
        for (module = pcs->lmodules; module; module = module->next)
        {
            if (type == module->type && addr >= module->module.BaseOfImage &&
                addr < module->module.BaseOfImage + module->module.ImageSize) 
                return module;
        }
    }
    SetLastError(ERROR_INVALID_ADDRESS);
    return module;
}

static BOOL module_is_elf_container_loaded(struct process* pcs, const char* ImageName,
                                           const char* ModuleName)
{
    char                buffer[MAX_PATH];
    size_t              len;
    struct module*      module;

    if (!ModuleName)
    {
        module_fill_module(ImageName, buffer, sizeof(buffer));
        ModuleName = buffer;
    }
    len = strlen(ModuleName);
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!strncasecmp(module->module.ModuleName, ModuleName, len) &&
            module->type == DMT_ELF &&
            !strcmp(module->module.ModuleName + len, "<elf>"))
            return TRUE;
    }
    return FALSE;
}

/******************************************************************
 *		module_get_type_by_name
 *
 * Guesses a filename type from its extension
 */
enum module_type module_get_type_by_name(const char* name)
{
    const char* ptr;
    int         len = strlen(name);

    /* check for terminating .so or .so.[digit] */
    ptr = strrchr(name, '.');
    if (ptr)
    {
        if (!strcmp(ptr, ".so") ||
            (isdigit(ptr[1]) && !ptr[2] && ptr >= name + 3 && !memcmp(ptr - 3, ".so", 3)))
            return DMT_ELF;
        else if (!strcasecmp(ptr, ".pdb"))
            return DMT_PDB;
    }
    /* wine-[kp]thread is also an ELF module */
    else if (((len > 12 && name[len - 13] == '/') || len == 12) && 
             (!strcasecmp(name + len - 12, "wine-pthread") || 
              !strcasecmp(name + len - 12, "wine-kthread")))
    {
        return DMT_ELF;
    }
    return DMT_PE;
}

/***********************************************************************
 *			SymLoadModule (DBGHELP.@)
 */
DWORD WINAPI SymLoadModule(HANDLE hProcess, HANDLE hFile, char* ImageName,
                           char* ModuleName, DWORD BaseOfDll, DWORD SizeOfDll)
{
    struct process*     pcs;
    struct module*	module = NULL;

    TRACE("(%p %p %s %s %08lx %08lx)\n", 
          hProcess, hFile, debugstr_a(ImageName), debugstr_a(ModuleName), 
          BaseOfDll, SizeOfDll);

    pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;

    /* force transparent ELF loading / unloading */
    elf_synchronize_module_list(pcs);

    /* this is a Wine extension to the API just to redo the synchronisation */
    if (!ImageName && !hFile) return 0;

    if (module_is_elf_container_loaded(pcs, ImageName, ModuleName))
    {
        /* force the loading of DLL as builtin */
        if ((module = pe_load_module_from_pcs(pcs, ImageName, ModuleName,
                                              BaseOfDll, SizeOfDll)))
            goto done;
        WARN("Couldn't locate %s\n", ImageName);
        return 0;
    }
    TRACE("Assuming %s as native DLL\n", ImageName);
    if (!(module = pe_load_module(pcs, ImageName, hFile, BaseOfDll, SizeOfDll)))
    {
        if (module_get_type_by_name(ImageName) == DMT_ELF &&
            (module = elf_load_module(pcs, ImageName, BaseOfDll)))
            goto done;
        FIXME("Should have successfully loaded debug information for image %s\n",
              ImageName);
        if ((module = pe_load_module_from_pcs(pcs, ImageName, ModuleName,
                                              BaseOfDll, SizeOfDll)))
            goto done;
        WARN("Couldn't locate %s\n", ImageName);
        return 0;
    }

done:
    /* by default pe_load_module fills module.ModuleName from a derivation 
     * of ImageName. Overwrite it, if we have better information
     */
    if (ModuleName)
        lstrcpynA(module->module.ModuleName, ModuleName, sizeof(module->module.ModuleName));
    lstrcpynA(module->module.ImageName, ImageName, sizeof(module->module.ImageName));

    return module->module.BaseOfImage;
}

/***********************************************************************
 *			SymLoadModuleEx (DBGHELP.@)
 */
DWORD64 WINAPI  SymLoadModuleEx(HANDLE hProcess, HANDLE hFile, PCSTR ImageName,
                                PCSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize,
                                PMODLOAD_DATA Data, DWORD Flags)
{
    if (Data || Flags)
    {
        FIXME("Unsupported parameters (%p, %lx) for %s\n", Data, Flags, ImageName);
        if (Flags & 1) return TRUE;
    }
    if (!validate_addr64(BaseOfDll)) return FALSE;
    return SymLoadModule(hProcess, hFile, (char*)ImageName, (char*)ModuleName,
                         (DWORD)BaseOfDll, DllSize);
}

/***********************************************************************
 *                     SymLoadModule64 (DBGHELP.@)
 */
DWORD WINAPI SymLoadModule64(HANDLE hProcess, HANDLE hFile, char* ImageName,
                             char* ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll)
{
    if (!validate_addr64(BaseOfDll)) return FALSE;
    return SymLoadModule(hProcess, hFile, ImageName, ModuleName, (DWORD)BaseOfDll, SizeOfDll);
}

/******************************************************************
 *		module_remove
 *
 */
BOOL module_remove(struct process* pcs, struct module* module)
{
    struct module**     p;

    TRACE("%s (%p)\n", module->module.ModuleName, module);
    hash_table_destroy(&module->ht_symbols);
    hash_table_destroy(&module->ht_types);
    HeapFree(GetProcessHeap(), 0, (char*)module->sources);
    HeapFree(GetProcessHeap(), 0, module->addr_sorttab);
    pool_destroy(&module->pool);

    for (p = &pcs->lmodules; *p; p = &(*p)->next)
    {
        if (*p == module)
        {
            *p = module->next;
            HeapFree(GetProcessHeap(), 0, module);
            return TRUE;
        }
    }
    FIXME("This shouldn't happen\n");
    return FALSE;
}

/******************************************************************
 *		SymUnloadModule (DBGHELP.@)
 *
 */
BOOL WINAPI SymUnloadModule(HANDLE hProcess, DWORD BaseOfDll)
{
    struct process*     pcs;
    struct module*      module;

    pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module) return FALSE;
    return module_remove(pcs, module);
}

/******************************************************************
 *		SymUnloadModule64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymUnloadModule64(HANDLE hProcess, DWORD64 BaseOfDll)
{
    struct process*     pcs;
    struct module*      module;

    pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;
    if (!validate_addr64(BaseOfDll)) return FALSE;
    module = module_find_by_addr(pcs, (DWORD)BaseOfDll, DMT_UNKNOWN);
    if (!module) return FALSE;
    return module_remove(pcs, module);
}

/******************************************************************
 *		SymEnumerateModules (DBGHELP.@)
 *
 */
BOOL  WINAPI SymEnumerateModules(HANDLE hProcess,
                                 PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,  
                                 PVOID UserContext)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return FALSE;
    
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!(dbghelp_options & SYMOPT_WINE_WITH_ELF_MODULES) && module->type != DMT_PE)
            continue;
        if (!EnumModulesCallback(module->module.ModuleName, 
                                 module->module.BaseOfImage, UserContext))
            break;
    }
    return TRUE;
}

/******************************************************************
 *		EnumerateLoadedModules (DBGHELP.@)
 *
 */
BOOL  WINAPI EnumerateLoadedModules(HANDLE hProcess,
                                    PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
                                    PVOID UserContext)
{
    HMODULE*    hMods;
    char        base[256], mod[256];
    DWORD       i, sz;
    MODULEINFO  mi;

    hMods = HeapAlloc(GetProcessHeap(), 0, 256 * sizeof(hMods[0]));
    if (!hMods) return FALSE;

    if (!EnumProcessModules(hProcess, hMods, 256 * sizeof(hMods[0]), &sz))
    {
        /* hProcess should also be a valid process handle !! */
        FIXME("If this happens, bump the number in mod\n");
        HeapFree(GetProcessHeap(), 0, hMods);
        return FALSE;
    }
    sz /= sizeof(HMODULE);
    for (i = 0; i < sz; i++)
    {
        if (!GetModuleInformation(hProcess, hMods[i], &mi, sizeof(mi)) ||
            !GetModuleBaseNameA(hProcess, hMods[i], base, sizeof(base)))
            continue;
        module_fill_module(base, mod, sizeof(mod));
        EnumLoadedModulesCallback(mod, (DWORD)mi.lpBaseOfDll, mi.SizeOfImage, 
                                  UserContext);
    }
    HeapFree(GetProcessHeap(), 0, hMods);

    return sz != 0 && i == sz;
}

/******************************************************************
 *		SymGetModuleInfo (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfo(HANDLE hProcess, DWORD dwAddr, 
                              PIMAGEHLP_MODULE ModuleInfo)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return FALSE;
    if (ModuleInfo->SizeOfStruct < sizeof(*ModuleInfo)) return FALSE;
    module = module_find_by_addr(pcs, dwAddr, DMT_UNKNOWN);
    if (!module) return FALSE;

    *ModuleInfo = module->module;
    if (module->module.SymType == SymNone)
    {
        module = module_get_container(pcs, module);
        if (module && module->module.SymType != SymNone)
            ModuleInfo->SymType = module->module.SymType;
    }

    return TRUE;
}

/******************************************************************
 *		SymGetModuleInfo64 (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfo64(HANDLE hProcess, DWORD64 dwAddr, 
                                PIMAGEHLP_MODULE64 ModuleInfo)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    DWORD               sz;
    IMAGEHLP_MODULE64   mod;

    TRACE("%p %s %p\n", hProcess, wine_dbgstr_longlong(dwAddr), ModuleInfo);

    if (!pcs) return FALSE;
    if (ModuleInfo->SizeOfStruct > sizeof(*ModuleInfo)) return FALSE;
    module = module_find_by_addr(pcs, dwAddr, DMT_UNKNOWN);
    if (!module) return FALSE;

    mod.BaseOfImage = module->module.BaseOfImage;
    mod.ImageSize = module->module.ImageSize;
    mod.TimeDateStamp = module->module.TimeDateStamp;
    mod.CheckSum = module->module.CheckSum;
    mod.NumSyms = module->module.NumSyms;
    mod.SymType = module->module.SymType;
    strcpy(mod.ModuleName, module->module.ModuleName);
    strcpy(mod.ImageName, module->module.ImageName);
    strcpy(mod.LoadedImageName, module->module.LoadedImageName);
    /* FIXME: all following attributes need to be set */
    mod.LoadedPdbName[0] = '\0';
    mod.CVSig = 0;
    memset(mod.CVData, 0, sizeof(mod.CVData));
    mod.PdbSig = 0;
    memset(&mod.PdbSig70, 0, sizeof(mod.PdbSig70));
    mod.PdbAge = 0;
    mod.PdbUnmatched = 0;
    mod.DbgUnmatched = 0;
    mod.LineNumbers = 0;
    mod.GlobalSymbols = 0;
    mod.TypeInfo = 0;
    mod.SourceIndexed = 0;
    mod.Publics = 0;

    if (module->module.SymType == SymNone)
    {
        module = module_get_container(pcs, module);
        if (module && module->module.SymType != SymNone)
            mod.SymType = module->module.SymType;
    }
    sz = ModuleInfo->SizeOfStruct;
    memcpy(ModuleInfo, &mod, sz);
    ModuleInfo->SizeOfStruct = sz;
    return TRUE;
}

/***********************************************************************
 *		SymGetModuleBase (IMAGEHLP.@)
 */
DWORD WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return 0;
    module = module_find_by_addr(pcs, dwAddr, DMT_UNKNOWN);
    if (!module) return 0;
    return module->module.BaseOfImage;
}

/******************************************************************
 *		module_reset_debug_info
 * Removes any debug information linked to a given module.
 */
void module_reset_debug_info(struct module* module)
{
    module->sortlist_valid = TRUE;
    module->addr_sorttab = NULL;
    hash_table_destroy(&module->ht_symbols);
    module->ht_symbols.num_buckets = 0;
    module->ht_symbols.buckets = NULL;
    hash_table_destroy(&module->ht_types);
    module->ht_types.num_buckets = 0;
    module->ht_types.buckets = NULL;
    module->vtypes.num_elts = 0;
    hash_table_destroy(&module->ht_symbols);
    module->sources_used = module->sources_alloc = 0;
    module->sources = NULL;
}
