/*
 * File module.c - module handling for the wine debugger
 *
 * Copyright (C) 1993,      Eric Youngdale.
 * 		 2000-2007, Eric Pouech
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "image_private.h"
#include "psapi.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

#define NOTE_GNU_BUILD_ID  3

const WCHAR S_WineLoaderW[] = L"<wine-loader>";
static const WCHAR * const ext[] = {L".acm", L".dll", L".drv", L".exe", L".ocx", L".vxd", NULL};

static int match_ext(const WCHAR* ptr, size_t len)
{
    const WCHAR* const *e;
    size_t      l;

    for (e = ext; *e; e++)
    {
        l = lstrlenW(*e);
        if (l >= len) return 0;
        if (wcsnicmp(&ptr[len - l], *e, l)) continue;
        return l;
    }
    return 0;
}

/* FIXME: implemented from checking on modulename (ie foo.dll.so)
 * and Wine loader, but fails to identify unixlib.
 * Would require a stronger tagging of ELF modules.
 */
BOOL module_is_wine_host(const WCHAR* module_name, const WCHAR* ext)
{
    size_t len, extlen;
    if (!wcscmp(module_name, S_WineLoaderW)) return TRUE;
    len = wcslen(module_name);
    extlen = wcslen(ext);
    return len > extlen && !wcsicmp(&module_name[len - extlen], ext) &&
        match_ext(module_name, len - extlen);
}

static const WCHAR* get_filename(const WCHAR* name, const WCHAR* endptr)
{
    const WCHAR*        ptr;

    if (!endptr) endptr = name + lstrlenW(name);
    for (ptr = endptr - 1; ptr >= name; ptr--)
    {
        if (*ptr == '/' || *ptr == '\\') break;
    }
    return ++ptr;
}

static BOOL is_wine_loader(const WCHAR *module)
{
    const WCHAR *filename = get_filename(module, NULL);

    return !wcscmp( filename, L"wine" );
}

static void module_fill_module(const WCHAR* in, WCHAR* out, size_t size)
{
    const WCHAR *ptr, *endptr;
    size_t      len;

    endptr = in + lstrlenW(in);
    endptr -= match_ext(in, endptr - in);
    ptr = get_filename(in, endptr);
    len = min(endptr - ptr, size - 1);
    memcpy(out, ptr, len * sizeof(WCHAR));
    out[len] = '\0';
    if (is_wine_loader(out))
        lstrcpynW(out, S_WineLoaderW, size);
    while ((*out = towlower(*out))) out++;
}

void module_set_module(struct module* module, const WCHAR* name)
{
    module_fill_module(name, module->module.ModuleName, ARRAY_SIZE(module->module.ModuleName));
    module_fill_module(name, module->modulename, ARRAY_SIZE(module->modulename));
}

const WCHAR *get_wine_loader_name(struct process *pcs)
{
    return process_getenv(pcs, L"WINELOADER");
}

static const char*      get_module_type(struct module* module)
{
    switch (module->type)
    {
    case DMT_ELF: return "ELF";
    case DMT_MACHO: return "Mach-O";
    case DMT_PE: return module->is_wine_builtin ? "PE (builtin)" : "PE";
    default: return "---";
    }
}

/***********************************************************************
 * Creates and links a new module to a process
 */
struct module* module_new(struct process* pcs, const WCHAR* name,
                          enum dhext_module_type type, BOOL builtin, BOOL virtual,
                          DWORD64 mod_addr, DWORD64 size,
                          ULONG_PTR stamp, ULONG_PTR checksum, WORD machine)
{
    struct module*      module;
    struct module**     pmodule;
    unsigned            i;

    assert(type == DMT_ELF || type == DMT_PE || type == DMT_MACHO);
    if (!(module = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*module))))
	return NULL;

    for (pmodule = &pcs->lmodules; *pmodule; pmodule = &(*pmodule)->next);
    module->next = NULL;
    *pmodule = module;

    TRACE("=> %s%s%s %I64x-%I64x %s\n", virtual ? "virtual " : "", builtin ? "built-in " : "",
          get_module_type(module), mod_addr, mod_addr + size, debugstr_w(name));

    pool_init(&module->pool, 65536);

    module->process = pcs;
    module->module.SizeOfStruct = sizeof(module->module);
    module->module.BaseOfImage = mod_addr;
    module->module.ImageSize = size;
    module_set_module(module, name);
    module->alt_modulename = NULL;
    module->module.ImageName[0] = '\0';
    lstrcpynW(module->module.LoadedImageName, name, ARRAY_SIZE(module->module.LoadedImageName));
    module->module.SymType = SymDeferred;
    module->module.NumSyms = 0;
    module->module.TimeDateStamp = stamp;
    module->module.CheckSum = checksum;

    memset(module->module.LoadedPdbName, 0, sizeof(module->module.LoadedPdbName));
    module->module.CVSig = 0;
    memset(module->module.CVData, 0, sizeof(module->module.CVData));
    module->module.PdbSig = 0;
    memset(&module->module.PdbSig70, 0, sizeof(module->module.PdbSig70));
    module->module.PdbAge = 0;
    module->module.PdbUnmatched = FALSE;
    module->module.DbgUnmatched = FALSE;
    module->module.LineNumbers = FALSE;
    module->module.GlobalSymbols = FALSE;
    module->module.TypeInfo = FALSE;
    module->module.SourceIndexed = FALSE;
    module->module.Publics = FALSE;
    module->module.MachineType = machine;
    module->module.Reserved = 0;

    module->reloc_delta       = 0;
    module->type              = type;
    module->is_virtual        = !!virtual;
    module->is_wine_builtin   = !!builtin;
    module->has_file_image    = TRUE;

    for (i = 0; i < DFI_LAST; i++) module->format_info[i] = NULL;
    module->sortlist_valid    = FALSE;
    module->sorttab_size      = 0;
    module->addr_sorttab      = NULL;
    module->num_sorttab       = 0;
    module->num_symbols       = 0;
    module->cpu               = cpu_find(machine);
    if (!module->cpu)
        module->cpu = dbghelp_current_cpu;
    module->debug_format_bitmask = 0;

    vector_init(&module->vsymt, sizeof(symref_t), 0);
    vector_init(&module->vcustom_symt, sizeof(symref_t), 0);
    /* FIXME: this seems a bit too high (on a per module basis)
     * need some statistics about this
     */
    hash_table_init(&module->pool, &module->ht_symbols, 4096);
    hash_table_init(&module->pool, &module->ht_types,   4096);

    module->sources_used      = 0;
    module->sources_alloc     = 0;
    module->sources           = 0;
    wine_rb_init(&module->sources_offsets_tree, source_rb_compare);

    /* add top level symbol */
    module->top = symt_new_module(module);

    return module;
}

BOOL module_init_pair(struct module_pair* pair, HANDLE hProcess, DWORD64 addr)
{
    if (!(pair->pcs = process_find_by_handle(hProcess))) return FALSE;
    pair->requested = module_find_by_addr(pair->pcs, addr);
    return module_get_debug(pair);
}

/***********************************************************************
 *	module_find_by_nameW
 *
 */
struct module* module_find_by_nameW(const struct process* pcs, const WCHAR* name)
{
    struct module*      module;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!wcsicmp(name, module->modulename)) return module;
        if (module->alt_modulename && !wcsicmp(name, module->alt_modulename)) return module;
    }
    SetLastError(ERROR_INVALID_NAME);
    return NULL;
}

struct module* module_find_by_nameA(const struct process* pcs, const char* name)
{
    WCHAR wname[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, name, -1, wname, ARRAY_SIZE(wname));
    return module_find_by_nameW(pcs, wname);
}

/***********************************************************************
 *	module_is_already_loaded
 *
 */
struct module* module_is_already_loaded(const struct process* pcs, const WCHAR* name)
{
    struct module*      module;
    const WCHAR*        filename;

    /* first compare the loaded image name... */
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!wcsicmp(name, module->module.LoadedImageName))
            return module;
    }
    /* then compare the standard filenames (without the path) ... */
    filename = get_filename(name, NULL);
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!wcsicmp(filename, get_filename(module->module.LoadedImageName, NULL)))
            return module;
    }
    SetLastError(ERROR_INVALID_NAME);
    return NULL;
}

/***********************************************************************
 *           module_get_container
 *
 */
static struct module* module_get_container(const struct process* pcs,
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
struct module* module_get_containee(const struct process* pcs, const struct module* outer)
{
    struct module*      module;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module != outer &&
            outer->module.BaseOfImage <= module->module.BaseOfImage &&
            outer->module.BaseOfImage + outer->module.ImageSize >=
            module->module.BaseOfImage + module->module.ImageSize)
            return module;
    }
    return NULL;
}

BOOL module_load_debug(struct module* module)
{
    IMAGEHLP_DEFERRED_SYMBOL_LOADW64    idslW64;

    /* if deferred, force loading */
    if (module->module.SymType == SymDeferred)
    {
        BOOL ret;

        if (module->is_virtual)
        {
            module->module.SymType = SymVirtual;
            ret = TRUE;
        }
        else if (module->type == DMT_PE)
        {
            idslW64.SizeOfStruct = sizeof(idslW64);
            idslW64.BaseOfImage = module->module.BaseOfImage;
            idslW64.CheckSum = module->module.CheckSum;
            idslW64.TimeDateStamp = module->module.TimeDateStamp;
            memcpy(idslW64.FileName, module->module.ImageName,
                   sizeof(module->module.ImageName));
            idslW64.Reparse = FALSE;
            idslW64.hFile = INVALID_HANDLE_VALUE;

            pcs_callback(module->process, CBA_DEFERRED_SYMBOL_LOAD_START, &idslW64);
            ret = pe_load_debug_info(module);
            pcs_callback(module->process,
                         ret ? CBA_DEFERRED_SYMBOL_LOAD_COMPLETE : CBA_DEFERRED_SYMBOL_LOAD_FAILURE,
                         &idslW64);
        }
        else ret = module->process->loader->load_debug_info(module->process, module);

        if (!ret) module->module.SymType = SymNone;
        assert(module->module.SymType != SymDeferred);
        module->module.NumSyms = module->ht_symbols.num_elts;
        return ret;
    }
    return TRUE;
}

/******************************************************************
 *		module_get_debug
 *
 * get the debug information from a module:
 * - if the module's type is deferred, then force loading of debug info (and return
 *   the module itself)
 * - if the module has no debug info and has an ELF container, then return the ELF
 *   container (and also force the ELF container's debug info loading if deferred)
 */
BOOL module_get_debug(struct module_pair* pair)
{
    if (!pair->requested) return FALSE;
    /* for a PE builtin, always get info from container */
    if (!(pair->effective = module_get_container(pair->pcs, pair->requested)))
        pair->effective = pair->requested;
    return module_load_debug(pair->effective);
}

/***********************************************************************
 *	module_find_by_addr
 *
 * either the addr where module is loaded, or any address inside the
 * module
 */
struct module* module_find_by_addr(const struct process* pcs, DWORD64 addr)
{
    struct module* module;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_PE && addr >= module->module.BaseOfImage &&
            addr < module->module.BaseOfImage + module->module.ImageSize)
            return module;
    }
    for (module = pcs->lmodules; module; module = module->next)
    {
        if ((module->type == DMT_ELF || module->type == DMT_MACHO) &&
            addr >= module->module.BaseOfImage &&
            addr < module->module.BaseOfImage + module->module.ImageSize)
            return module;
    }
    SetLastError(ERROR_MOD_NOT_FOUND);
    return module;
}

/******************************************************************
 *		module_is_container_loaded
 *
 * checks whether the native container, for a (supposed) PE builtin is
 * already loaded
 */
static BOOL module_is_container_loaded(const struct process* pcs,
                                       const WCHAR* ImageName, DWORD64 base)
{
    size_t              len;
    struct module*      module;
    PCWSTR              filename, modname;

    if (!base) return FALSE;
    filename = get_filename(ImageName, NULL);
    len = lstrlenW(filename);

    for (module = pcs->lmodules; module; module = module->next)
    {
        if ((module->type == DMT_ELF || module->type == DMT_MACHO) &&
            base >= module->module.BaseOfImage &&
            base < module->module.BaseOfImage + module->module.ImageSize)
        {
            modname = get_filename(module->module.LoadedImageName, NULL);
            if (!wcsnicmp(modname, filename, len) &&
                !memcmp(modname + len, L".so", 3 * sizeof(WCHAR)))
            {
                return TRUE;
            }
        }
    }
    /* likely a native PE module */
    WARN("Couldn't find container for %s\n", debugstr_w(ImageName));
    return FALSE;
}

static BOOL image_check_debug_link_crc(const WCHAR* file, struct image_file_map* fmap, DWORD link_crc)
{
    DWORD read_bytes;
    HANDLE handle;
    WCHAR *path;
    WORD magic;
    DWORD crc;
    BOOL ret;

    path = get_dos_file_name(file);
    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HeapFree(GetProcessHeap(), 0, path);
    if (handle == INVALID_HANDLE_VALUE) return FALSE;

    crc = calc_crc32(handle);
    if (crc != link_crc)
    {
        WARN("Bad CRC for file %s (got %08lx while expecting %08lx)\n",  debugstr_w(file), crc, link_crc);
        CloseHandle(handle);
        return FALSE;
    }

    SetFilePointer(handle, 0, 0, FILE_BEGIN);
    if (ReadFile(handle, &magic, sizeof(magic), &read_bytes, NULL) && magic == IMAGE_DOS_SIGNATURE)
        ret = pe_map_file(handle, fmap);
    else
        ret = elf_map_handle(handle, fmap);
    CloseHandle(handle);
    return ret;
}

static BOOL image_check_debug_link_gnu_id(const WCHAR* file, struct image_file_map* fmap, const BYTE* id, unsigned idlen)
{
    struct image_section_map buildid_sect;
    DWORD read_bytes;
    GUID guid;
    HANDLE handle;
    WCHAR *path;
    WORD magic;
    BOOL ret;

    path = get_dos_file_name(file);
    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HeapFree(GetProcessHeap(), 0, path);
    if (handle == INVALID_HANDLE_VALUE) return FALSE;

    TRACE("Located debug information file at %s\n", debugstr_w(file));

    if (ReadFile(handle, &magic, sizeof(magic), &read_bytes, NULL) && magic == IMAGE_DOS_SIGNATURE)
        ret = pe_map_file(handle, fmap);
    else
        ret = elf_map_handle(handle, fmap);
    CloseHandle(handle);

    if (ret && pe_has_buildid_debug(fmap, &guid))
        return TRUE;

    if (ret && image_find_section(fmap, ".note.gnu.build-id", &buildid_sect))
    {
        const UINT32* note;

        note = (const UINT32*)image_map_section(&buildid_sect);
        if (note != IMAGE_NO_MAP)
        {
            /* the usual ELF note structure: name-size desc-size type <name> <desc> */
            if (note[2] == NOTE_GNU_BUILD_ID)
            {
                if (note[1] == idlen && !memcmp(note + 3 + ((note[0] + 3) >> 2), id, idlen))
                    return TRUE;
            }
        }
        image_unmap_section(&buildid_sect);
        image_unmap_file(fmap);
    }
    if (ret)
        WARN("mismatch in buildid information for %s\n", debugstr_w(file));
    return FALSE;
}

/******************************************************************
 *		image_locate_debug_link
 *
 * Locate a filename from a .gnu_debuglink section, using the same
 * strategy as gdb:
 * "If the full name of the directory containing the executable is
 * execdir, and the executable has a debug link that specifies the
 * name debugfile, then GDB will automatically search for the
 * debugging information file in three places:
 *  - the directory containing the executable file (that is, it
 *    will look for a file named `execdir/debugfile',
 *  - a subdirectory of that directory named `.debug' (that is, the
 *    file `execdir/.debug/debugfile', and
 *  - a subdirectory of the global debug file directory that includes
 *    the executable's full path, and the name from the link (that is,
 *    the file `globaldebugdir/execdir/debugfile', where globaldebugdir
 *    is the global debug file directory, and execdir has been turned
 *    into a relative path)." (from GDB manual)
 */
static struct image_file_map* image_locate_debug_link(const struct module* module, const char* filename, DWORD crc)
{
    static const WCHAR globalDebugDirW[] = {'/','u','s','r','/','l','i','b','/','d','e','b','u','g','/'};
    static const WCHAR dotDebugW[] = {'.','d','e','b','u','g','/'};
    const size_t globalDebugDirLen = ARRAY_SIZE(globalDebugDirW);
    size_t filename_len, path_len;
    WCHAR* p = NULL;
    WCHAR* slash;
    WCHAR* slash2;
    struct image_file_map* fmap_link = NULL;

    fmap_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*fmap_link));
    if (!fmap_link) return NULL;

    filename_len = MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, NULL, 0);
    path_len = lstrlenW(module->module.LoadedImageName);
    if (module->real_path) path_len = max(path_len, lstrlenW(module->real_path));
    p = HeapAlloc(GetProcessHeap(), 0,
                  (globalDebugDirLen + path_len + 6 + 1 + filename_len + 1) * sizeof(WCHAR));
    if (!p) goto found;

    /* we prebuild the string with "execdir" */
    lstrcpyW(p, module->module.LoadedImageName);
    slash = p;
    if ((slash2 = wcsrchr(slash, '/'))) slash = slash2 + 1;
    if ((slash2 = wcsrchr(slash, '\\'))) slash = slash2 + 1;

    /* testing execdir/filename */
    MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, slash, filename_len);
    if (image_check_debug_link_crc(p, fmap_link, crc)) goto found;

    /* testing execdir/.debug/filename */
    memcpy(slash, dotDebugW, sizeof(dotDebugW));
    MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, slash + ARRAY_SIZE(dotDebugW), filename_len);
    if (image_check_debug_link_crc(p, fmap_link, crc)) goto found;

    if (module->real_path)
    {
        lstrcpyW(p, module->real_path);
        slash = p;
        if ((slash2 = wcsrchr(slash, '/'))) slash = slash2 + 1;
        if ((slash2 = wcsrchr(slash, '\\'))) slash = slash2 + 1;
        MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, slash, filename_len);
        if (image_check_debug_link_crc(p, fmap_link, crc)) goto found;
    }

    /* testing globaldebugdir/execdir/filename */
    memmove(p + globalDebugDirLen, p, (slash - p) * sizeof(WCHAR));
    memcpy(p, globalDebugDirW, globalDebugDirLen * sizeof(WCHAR));
    slash += globalDebugDirLen;
    MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, slash, filename_len);
    if (image_check_debug_link_crc(p, fmap_link, crc)) goto found;

    /* finally testing filename */
    if (image_check_debug_link_crc(slash, fmap_link, crc)) goto found;


    WARN("Couldn't locate or map %s\n", debugstr_a(filename));
    HeapFree(GetProcessHeap(), 0, p);
    HeapFree(GetProcessHeap(), 0, fmap_link);
    return NULL;

found:
    TRACE("Located debug information file %s at %s\n", debugstr_a(filename), debugstr_w(p));
    HeapFree(GetProcessHeap(), 0, p);
    return fmap_link;
}

static WCHAR* append_hex(WCHAR* dst, const BYTE* id, const BYTE* end)
{
    while (id < end)
    {
        *dst++ = "0123456789abcdef"[*id >> 4  ];
        *dst++ = "0123456789abcdef"[*id & 0x0F];
        id++;
    }
    return dst;
}

static BOOL image_locate_build_id_target_in_dir(struct image_file_map *fmap_link, const BYTE* id, unsigned idlen, const WCHAR *from)
{
    size_t from_len = wcslen(from);
    BOOL found = FALSE;
    WCHAR *p, *z;

    if ((p = malloc((from_len + idlen * 2 + 1) * sizeof(WCHAR) + sizeof(L".debug"))))
    {
        memcpy(p, from, from_len * sizeof(WCHAR));
        z = p + from_len;
        z = append_hex(z, id, id + 1);
        if (idlen > 1)
        {
            *z++ = L'/';
            z = append_hex(z, id + 1, id + idlen);
        }
        wcscpy(z, L".debug");
        TRACE("checking %s\n", debugstr_w(p));
        found = image_check_debug_link_gnu_id(p, fmap_link, id, idlen);

        free(p);
    }
    return found;
}

/******************************************************************
 *		image_locate_build_id_target
 *
 * Try to find an image file containing the debug info out of the build-id
 * note information.
 */
static struct image_file_map* image_locate_build_id_target(const BYTE* id, unsigned idlen)
{
    struct image_file_map* fmap_link;
    DWORD sz;

    if (!idlen) return NULL;

    if (!(fmap_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*fmap_link))))
        return NULL;
    if (image_locate_build_id_target_in_dir(fmap_link, id, idlen, L"/usr/lib/debug/.build-id/"))
        return fmap_link;
    if (image_locate_build_id_target_in_dir(fmap_link, id, idlen, L"/usr/lib/.build-id/"))
        return fmap_link;

    sz = GetEnvironmentVariableW(L"WINEHOMEDIR", NULL, 0);
    if (sz)
    {
        WCHAR *p, *z;
        p = malloc(sz * sizeof(WCHAR) +
                   sizeof(L"\\.cache\\debuginfod_client\\") +
                   idlen * 2 * sizeof(WCHAR) + sizeof(L"\\debuginfo") + 500);
        if (p && GetEnvironmentVariableW(L"WINEHOMEDIR", p, sz) == sz - 1)
        {
            BOOL found;

            wcscpy(p + sz - 1, L"\\.cache\\debuginfod_client\\");
            z = p + wcslen(p);
            z = append_hex(z, id, id + idlen);
            wcscpy(z, L"\\debuginfo");
            TRACE("checking %ls\n", p);
            found = image_check_debug_link_gnu_id(p, fmap_link, id, idlen);
            free(p);
            if (found) return fmap_link;
        }
    }

    TRACE("not found\n");

    HeapFree(GetProcessHeap(), 0, fmap_link);
    return NULL;
}

/******************************************************************
 *		image_load_debugaltlink
 *
 * Handle a (potential) .gnu_debugaltlink section and the link to
 * (another) alternate debug file.
 * Return an heap-allocated image_file_map when the section .gnu_debugaltlink is present,
 * and a matching debug file has been located.
 */
struct image_file_map* image_load_debugaltlink(struct image_file_map* fmap, struct module* module)
{
    struct image_section_map debugaltlink_sect;
    const char* data;
    struct image_file_map* fmap_link = NULL;
    BOOL ret = FALSE;

    if (!image_find_section(fmap, ".gnu_debugaltlink", &debugaltlink_sect))
    {
        TRACE("No .gnu_debugaltlink section found for %s\n", debugstr_w(module->modulename));
        return NULL;
    }

    data = image_map_section(&debugaltlink_sect);
    if (data != IMAGE_NO_MAP)
    {
        unsigned sect_len;
        const BYTE* id;
        /* The content of the section is:
         * + a \0 terminated string (filename)
         * + followed by the build-id
         * We try loading the dwz_alternate:
         * - from the filename: either as absolute path, or relative to the embedded build-id
         * - from the build-id
         * In both cases, checking that found .so file matches the requested build-id
         */
        sect_len = image_get_map_size(&debugaltlink_sect);
        id = memchr(data, '\0', sect_len);
        if (id++)
        {
            unsigned idlen = (const BYTE*)data + sect_len - id;
            fmap_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*fmap_link));
            if (fmap_link)
            {
                unsigned filename_len = MultiByteToWideChar(CP_UNIXCP, 0, data, -1, NULL, 0);
                /* Trying absolute path */
                WCHAR* dst = HeapAlloc(GetProcessHeap(), 0, filename_len * sizeof(WCHAR));
                if (dst)
                {
                    MultiByteToWideChar(CP_UNIXCP, 0, data, -1, dst, filename_len);
                    ret = image_check_debug_link_gnu_id(dst, fmap_link, id, idlen);
                    HeapFree(GetProcessHeap(), 0, dst);
                }
                /* Trying relative path to build-id directory */
                if (!ret)
                {
                    dst = HeapAlloc(GetProcessHeap(), 0,
                                    sizeof(L"/usr/lib/debug/.build-id/") + (3 + filename_len + idlen * 2) * sizeof(WCHAR));
                    if (dst)
                    {
                        WCHAR* p;

                        /* SIGH....
                         * some relative links are relative to /usr/lib/debug/.build-id, some others are from the directory
                         * where the alternate file is...
                         * so try both
                         */
                        p = memcpy(dst, L"/usr/lib/debug/.build-id/", sizeof(L"/usr/lib/debug/.build-id/"));
                        p += wcslen(dst);
                        MultiByteToWideChar(CP_UNIXCP, 0, data, -1, p, filename_len);
                        ret = image_check_debug_link_gnu_id(dst, fmap_link, id, idlen);
                        if (!ret)
                        {
                            p = append_hex(p, id, id + idlen);
                            *p++ = '/';
                            MultiByteToWideChar(CP_UNIXCP, 0, data, -1, p, filename_len);
                            ret = image_check_debug_link_gnu_id(dst, fmap_link, id, idlen);
                        }
                        HeapFree(GetProcessHeap(), 0, dst);
                    }
                }
                if (!ret)
                {
                    HeapFree(GetProcessHeap(), 0, fmap_link);
                    /* didn't work out with filename, try file lookup based on build-id */
                    if (!(fmap_link = image_locate_build_id_target(id, idlen)))
                        WARN("Couldn't find a match for .gnu_debugaltlink section %s for %s\n",
                             debugstr_a(data), debugstr_w(module->modulename));
                }
            }
        }
    }
    image_unmap_section(&debugaltlink_sect);
    if (fmap_link) TRACE("Found match .gnu_debugaltlink section for %s\n", debugstr_w(module->modulename));
    return fmap_link;
}

/******************************************************************
 *		image_check_alternate
 *
 * Load alternate files for a given image file, looking at either .note.gnu_build-id
 * or .gnu_debuglink sections.
 */
BOOL image_check_alternate(struct image_file_map* fmap, const struct module* module)
{
    struct image_section_map buildid_sect, debuglink_sect;
    struct image_file_map* fmap_link = NULL;

    /* if present, add the .gnu_debuglink file as an alternate to current one */
    if (fmap->modtype == DMT_PE)
    {
        GUID guid;

        if (pe_has_buildid_debug(fmap, &guid))
        {
            /* reorder bytes to match little endian order */
            fmap_link = image_locate_build_id_target((const BYTE*)&guid, sizeof(guid));
        }
    }
    /* if present, add the .note.gnu.build-id as an alternate to current one */
    if (!fmap_link && image_find_section(fmap, ".note.gnu.build-id", &buildid_sect))
    {
        const UINT32* note;

        note = (const UINT32*)image_map_section(&buildid_sect);
        if (note != IMAGE_NO_MAP)
        {
            /* the usual ELF note structure: name-size desc-size type <name> <desc> */
            if (note[2] == NOTE_GNU_BUILD_ID)
            {
                fmap_link = image_locate_build_id_target((const BYTE*)(note + 3 + ((note[0] + 3) >> 2)), note[1]);
            }
        }
        image_unmap_section(&buildid_sect);
    }
    /* if present, add the .gnu_debuglink file as an alternate to current one */
    if (!fmap_link && image_find_section(fmap, ".gnu_debuglink", &debuglink_sect))
    {
        const char* dbg_link;

        dbg_link = image_map_section(&debuglink_sect);
        if (dbg_link != IMAGE_NO_MAP)
        {
            /* The content of a debug link section is:
             * 1/ a NULL terminated string, containing the file name for the
             *    debug info
             * 2/ padding on 4 byte boundary
             * 3/ CRC of the linked file
             */
            DWORD crc = *(const DWORD*)(dbg_link + ((DWORD_PTR)(strlen(dbg_link) + 4) & ~3));
            if (!(fmap_link = image_locate_debug_link(module, dbg_link, crc)))
                WARN("Couldn't load linked debug file for %s\n", debugstr_w(module->modulename));
        }
        image_unmap_section(&debuglink_sect);
    }
    if (fmap_link)
    {
        fmap->alternate = fmap_link;
        return TRUE;
    }
    return FALSE;
}

#ifndef _WIN64
/***********************************************************************
 *			SymLoadModule (DBGHELP.@)
 */
DWORD WINAPI SymLoadModule(HANDLE hProcess, HANDLE hFile, PCSTR ImageName,
                           PCSTR ModuleName, DWORD BaseOfDll, DWORD SizeOfDll)
{
    return SymLoadModuleEx(hProcess, hFile, ImageName, ModuleName, BaseOfDll,
                           SizeOfDll, NULL, 0);
}
#endif

/***********************************************************************
 *			SymLoadModuleEx (DBGHELP.@)
 */
DWORD64 WINAPI  SymLoadModuleEx(HANDLE hProcess, HANDLE hFile, PCSTR ImageName,
                                PCSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize,
                                PMODLOAD_DATA Data, DWORD Flags)
{
    PWSTR       wImageName, wModuleName;
    unsigned    len;
    DWORD64     ret;

    TRACE("(%p %p %s %s %I64x %08lx %p %08lx)\n",
          hProcess, hFile, debugstr_a(ImageName), debugstr_a(ModuleName),
          BaseOfDll, DllSize, Data, Flags);

    if (ImageName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, ImageName, -1, NULL, 0);
        wImageName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, ImageName, -1, wImageName, len);
    }
    else wImageName = NULL;
    if (ModuleName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, ModuleName, -1, NULL, 0);
        wModuleName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, ModuleName, -1, wModuleName, len);
    }
    else wModuleName = NULL;

    ret = SymLoadModuleExW(hProcess, hFile, wImageName, wModuleName,
                          BaseOfDll, DllSize, Data, Flags);
    HeapFree(GetProcessHeap(), 0, wImageName);
    HeapFree(GetProcessHeap(), 0, wModuleName);
    return ret;
}

/***********************************************************************
 *			SymLoadModuleExW (DBGHELP.@)
 */
DWORD64 WINAPI  SymLoadModuleExW(HANDLE hProcess, HANDLE hFile, PCWSTR wImageName,
                                 PCWSTR wModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll,
                                 PMODLOAD_DATA Data, DWORD Flags)
{
    struct process*     pcs;
    struct module*      module = NULL;
    struct module*      altmodule;

    TRACE("(%p %p %s %s %I64x %08lx %p %08lx)\n",
          hProcess, hFile, debugstr_w(wImageName), debugstr_w(wModuleName),
          BaseOfDll, SizeOfDll, Data, Flags);

    if (Data)
        FIXME("Unsupported load data parameter %p for %s\n",
              Data, debugstr_w(wImageName));

    if (!(pcs = process_find_by_handle(hProcess))) return 0;

    if (Flags & ~(SLMFLAG_VIRTUAL | SLMFLAG_NO_SYMBOLS))
        FIXME("Unsupported Flags %08lx for %s\n", Flags, debugstr_w(wImageName));

    /* Trying to load a new module at the same address of an existing one,
     * native simply keeps the old one in place.
     */
    if (BaseOfDll)
        for (altmodule = pcs->lmodules; altmodule; altmodule = altmodule->next)
        {
            if (altmodule->type == DMT_PE && BaseOfDll == altmodule->module.BaseOfImage)
            {
                SetLastError(ERROR_SUCCESS);
                return 0;
            }
        }

    /* this is a Wine extension to the API just to redo the synchronisation */
    if (!wImageName && !hFile && !Flags)
    {
        pcs->loader->synchronize_module_list(pcs);
        return 0;
    }

    if (Flags & SLMFLAG_VIRTUAL)
    {
        if (!wImageName) wImageName = L"";
        module = module_new(pcs, wImageName, DMT_PE, FALSE, TRUE, BaseOfDll, SizeOfDll, 0, 0, IMAGE_FILE_MACHINE_UNKNOWN);
        if (!module) return 0;
    }
    else
    {
        /* try PE image */
        module = pe_load_native_module(pcs, wImageName, hFile, BaseOfDll, SizeOfDll);
        if (!module && wImageName)
        {
            /* It could be either a dll.so file (for which we need the corresponding
             * system module) or a system module.
             * In both cases, ensure system module list is up-to-date.
             */
            pcs->loader->synchronize_module_list(pcs);
            if (module_is_container_loaded(pcs, wImageName, BaseOfDll))
                module = pe_load_builtin_module(pcs, wImageName, BaseOfDll, SizeOfDll);
            /* at last, try ELF or Mach-O module */
            if (!module)
                module = pcs->loader->load_module(pcs, wImageName, BaseOfDll);
        }
        if (!module)
        {
            WARN("Couldn't locate %s\n", debugstr_w(wImageName));
            SetLastError(ERROR_NO_MORE_FILES);
            return 0;
        }
    }
    if (Flags & SLMFLAG_NO_SYMBOLS) module->dont_load_symbols = 1;

    /* Store alternate name for module when provided. */
    if (wModuleName)
        module->alt_modulename = pool_wcsdup(&module->pool, wModuleName);
    if (wImageName)
        lstrcpynW(module->module.ImageName, wImageName, ARRAY_SIZE(module->module.ImageName));

    for (altmodule = pcs->lmodules; altmodule; altmodule = altmodule->next)
    {
        if (altmodule != module && altmodule->type == module->type &&
            module->module.BaseOfImage >= altmodule->module.BaseOfImage &&
            module->module.BaseOfImage < altmodule->module.BaseOfImage + altmodule->module.ImageSize)
            break;
    }
    if (altmodule)
    {
        /* We have a conflict as the new module cannot be found by its base address
         * (it's hidden by altmodule).
         * We need to decide which one the two modules we need to get rid of.
         */
        /* loading same module at same address... we can only get here when BaseOfDll is 0 */
        if (module->module.BaseOfImage == altmodule->module.BaseOfImage)
        {
            module_remove(pcs, module);
            SetLastError(ERROR_INVALID_ADDRESS);
            return 0;
        }
        /* replace old module with new one */
        WARN("Replace module %ls at %I64x by module %ls at %I64x\n",
             altmodule->module.ImageName, altmodule->module.BaseOfImage,
             module->module.ImageName, module->module.BaseOfImage);
        module_remove(pcs, altmodule);
    }

    if ((dbghelp_options & SYMOPT_DEFERRED_LOADS) == 0 && !module_get_container(pcs, module))
        module_load_debug(module);
    return module->module.BaseOfImage;
}

/***********************************************************************
 *                     SymLoadModule64 (DBGHELP.@)
 */
DWORD64 WINAPI SymLoadModule64(HANDLE hProcess, HANDLE hFile, PCSTR ImageName,
                               PCSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll)
{
    return SymLoadModuleEx(hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll,
                           NULL, 0);
}

/******************************************************************
 *		module_remove
 *
 */
BOOL module_remove(struct process* pcs, struct module* module)
{
    struct module_format_vtable_iterator iter = {};
    struct module**     p;

    TRACE("%s (%p)\n", debugstr_w(module->modulename), module);

    /* remove local scope if symbol is from this module */
    if (pcs->localscope_symt)
    {
        struct symt* locsym = pcs->localscope_symt;
        if (symt_check_tag(locsym, SymTagInlineSite))
            locsym = &symt_get_function_from_inlined((struct symt_function*)locsym)->symt;
        if (symt_check_tag(locsym, SymTagFunction))
        {
            struct symt_compiland *compiland = (struct symt_compiland*)SYMT_SYMREF_TO_PTR(((struct symt_function*)locsym)->container);
            if (symt_check_tag(&compiland->symt, SymTagCompiland))
            {
                if (module == ((struct symt_module*)SYMT_SYMREF_TO_PTR(compiland->container))->module)
                {
                    pcs->localscope_pc = 0;
                    pcs->localscope_symt = NULL;
                }
            }
        }
    }
    while (module_format_vtable_iterator_next(module, &iter, MODULE_FORMAT_VTABLE_INDEX(remove)))
        iter.modfmt->vtable->remove(iter.modfmt);

    hash_table_destroy(&module->ht_symbols);
    hash_table_destroy(&module->ht_types);
    HeapFree(GetProcessHeap(), 0, module->sources);
    HeapFree(GetProcessHeap(), 0, module->addr_sorttab);
    pool_destroy(&module->pool);
    /* native dbghelp doesn't invoke registered callback(,CBA_SYMBOLS_UNLOADED,) here
     * so do we
     */
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

#ifndef _WIN64
/******************************************************************
 *		SymUnloadModule (DBGHELP.@)
 *
 */
BOOL WINAPI SymUnloadModule(HANDLE hProcess, DWORD BaseOfDll)
{
    return SymUnloadModule64(hProcess, BaseOfDll);
}
#endif

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
    module = module_find_by_addr(pcs, BaseOfDll);
    if (!module) return FALSE;
    module_remove(pcs, module);
    return TRUE;
}

#ifndef _WIN64
/******************************************************************
 *		SymEnumerateModules (DBGHELP.@)
 *
 */
struct enum_modW64_32
{
    PSYM_ENUMMODULES_CALLBACK   cb;
    PVOID                       user;
    char                        module[MAX_PATH];
};

static BOOL CALLBACK enum_modW64_32(PCWSTR name, DWORD64 base, PVOID user)
{
    struct enum_modW64_32*      x = user;

    WideCharToMultiByte(CP_ACP, 0, name, -1, x->module, sizeof(x->module), NULL, NULL);
    return x->cb(x->module, (DWORD)base, x->user);
}

BOOL  WINAPI SymEnumerateModules(HANDLE hProcess,
                                 PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,  
                                 PVOID UserContext)
{
    struct enum_modW64_32       x;

    x.cb = EnumModulesCallback;
    x.user = UserContext;

    return SymEnumerateModulesW64(hProcess, enum_modW64_32, &x);
}
#endif

/******************************************************************
 *		SymEnumerateModules64 (DBGHELP.@)
 *
 */
struct enum_modW64_64
{
    PSYM_ENUMMODULES_CALLBACK64 cb;
    PVOID                       user;
    char                        module[MAX_PATH];
};

static BOOL CALLBACK enum_modW64_64(PCWSTR name, DWORD64 base, PVOID user)
{
    struct enum_modW64_64*      x = user;

    WideCharToMultiByte(CP_ACP, 0, name, -1, x->module, sizeof(x->module), NULL, NULL);
    return x->cb(x->module, base, x->user);
}

BOOL  WINAPI SymEnumerateModules64(HANDLE hProcess,
                                   PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,  
                                   PVOID UserContext)
{
    struct enum_modW64_64       x;

    x.cb = EnumModulesCallback;
    x.user = UserContext;

    return SymEnumerateModulesW64(hProcess, enum_modW64_64, &x);
}

/******************************************************************
 *		SymEnumerateModulesW64 (DBGHELP.@)
 *
 */
BOOL  WINAPI SymEnumerateModulesW64(HANDLE hProcess,
                                    PSYM_ENUMMODULES_CALLBACKW64 EnumModulesCallback,
                                    PVOID UserContext)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return FALSE;
    
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!dbghelp_opt_native &&
            (module->type == DMT_ELF || module->type == DMT_MACHO))
            continue;
        if (!EnumModulesCallback(module->modulename,
                                 module->module.BaseOfImage, UserContext))
            break;
    }
    return TRUE;
}

/******************************************************************
 *		EnumerateLoadedModules64 (DBGHELP.@)
 *
 */
struct enum_load_modW64_64
{
    PENUMLOADED_MODULES_CALLBACK64      cb;
    PVOID                               user;
    char                                module[MAX_PATH];
};

static BOOL CALLBACK enum_load_modW64_64(PCWSTR name, DWORD64 base, ULONG size,
                                         PVOID user)
{
    struct enum_load_modW64_64* x = user;

    WideCharToMultiByte(CP_ACP, 0, name, -1, x->module, sizeof(x->module), NULL, NULL);
    return x->cb(x->module, base, size, x->user);
}

BOOL  WINAPI EnumerateLoadedModules64(HANDLE hProcess,
                                      PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,
                                      PVOID UserContext)
{
    struct enum_load_modW64_64  x;

    x.cb = EnumLoadedModulesCallback;
    x.user = UserContext;

    return EnumerateLoadedModulesW64(hProcess, enum_load_modW64_64, &x);
}

#ifndef _WIN64
/******************************************************************
 *		EnumerateLoadedModules (DBGHELP.@)
 *
 */
struct enum_load_modW64_32
{
    PENUMLOADED_MODULES_CALLBACK        cb;
    PVOID                               user;
    char                                module[MAX_PATH];
};

static BOOL CALLBACK enum_load_modW64_32(PCWSTR name, DWORD64 base, ULONG size,
                                         PVOID user)
{
    struct enum_load_modW64_32* x = user;
    WideCharToMultiByte(CP_ACP, 0, name, -1, x->module, sizeof(x->module), NULL, NULL);
    return x->cb(x->module, (DWORD)base, size, x->user);
}

BOOL  WINAPI EnumerateLoadedModules(HANDLE hProcess,
                                    PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
                                    PVOID UserContext)
{
    struct enum_load_modW64_32  x;

    x.cb = EnumLoadedModulesCallback;
    x.user = UserContext;

    return EnumerateLoadedModulesW64(hProcess, enum_load_modW64_32, &x);
}
#endif

static unsigned int load_and_grow_modules(HANDLE process, HMODULE** hmods, unsigned start, unsigned* alloc, DWORD filter)
{
    DWORD needed;
    BOOL ret;

    while ((ret = EnumProcessModulesEx(process, *hmods + start, (*alloc - start) * sizeof(HMODULE),
                                       &needed, filter)) &&
           needed > (*alloc - start) * sizeof(HMODULE))
    {
        HMODULE* new = HeapReAlloc(GetProcessHeap(), 0, *hmods, (*alloc) * 2 * sizeof(HMODULE));
        if (!new) return 0;
        *hmods = new;
        *alloc *= 2;
    }
    return ret ? needed / sizeof(HMODULE) : 0;
}

/******************************************************************
 *		EnumerateLoadedModulesW64 (DBGHELP.@)
 *
 */
BOOL  WINAPI EnumerateLoadedModulesW64(HANDLE process,
                                       PENUMLOADED_MODULES_CALLBACKW64 enum_cb,
                                       PVOID user)
{
    OBJECT_BASIC_INFORMATION obi;
    HMODULE*            hmods;
    unsigned            alloc = 256, count, count32, i;
    USHORT              pcs_machine, native_machine;
    BOOL                with_32bit_modules;
    WCHAR               imagenameW[MAX_PATH];
    MODULEINFO          mi;
    WCHAR*              sysdir = NULL;
    WCHAR*              wowdir = NULL;
    size_t              sysdir_len = 0, wowdir_len = 0;

    if (process != GetCurrentProcess() &&
        RtlIsCurrentProcess( process ) &&
        !NtQueryObject(process, ObjectBasicInformation, &obi, sizeof(obi), NULL) &&
        obi.GrantedAccess & PROCESS_VM_READ)
    {
        TRACE("same process.\n");
        process = GetCurrentProcess();
    }

    /* process might not be a handle to a live process */
    if (!IsWow64Process2(process, &pcs_machine, &native_machine))
    {
        SetLastError(STATUS_INVALID_CID);
        return FALSE;
    }
    with_32bit_modules = sizeof(void*) > sizeof(int) &&
        pcs_machine != IMAGE_FILE_MACHINE_UNKNOWN &&
        (dbghelp_options & SYMOPT_INCLUDE_32BIT_MODULES);

    if (!(hmods = HeapAlloc(GetProcessHeap(), 0, alloc * sizeof(hmods[0]))))
        return FALSE;

    /* Note:
     * - we report modules returned from kernelbase.EnumProcessModulesEx
     * - appending 32bit modules when possible and requested
     *
     * When considering 32bit modules in a wow64 child process, required from
     * a 64bit process:
     * - native returns from kernelbase.EnumProcessModulesEx
     *   redirected paths (that is in system32 directory), while
     *   dbghelp.EnumerateLoadedModulesWine returns the effective path
     *   (eg. syswow64 for x86_64).
     * - (Except for the main module, if gotten from syswow64, where kernelbase
     *    will return the effective path)
     * - Wine kernelbase (and ntdll) incorrectly return these modules from
     *   syswow64 (except for ntdll which is returned from system32).
     * => for these modules, always perform a system32 => syswow64 path
     *    conversion (it'll work even if ntdll/kernelbase is fixed).
     */
    if ((count = load_and_grow_modules(process, &hmods, 0, &alloc, LIST_MODULES_DEFAULT)) && with_32bit_modules)
    {
        /* append 32bit modules when required */
        if ((count32 = load_and_grow_modules(process, &hmods, count, &alloc, LIST_MODULES_32BIT)))
        {
            sysdir_len = GetSystemDirectoryW(NULL, 0);
            wowdir_len = GetSystemWow64Directory2W(NULL, 0, pcs_machine);

            if (!sysdir_len || !wowdir_len ||
                !(sysdir = HeapAlloc(GetProcessHeap(), 0, (sysdir_len + 1 + wowdir_len + 1) * sizeof(WCHAR))))
            {
                HeapFree(GetProcessHeap(), 0, hmods);
                return FALSE;
            }
            wowdir = sysdir + sysdir_len + 1;
            if (GetSystemDirectoryW(sysdir, sysdir_len) >= sysdir_len)
                FIXME("shouldn't happen\n");
            if (GetSystemWow64Directory2W(wowdir, wowdir_len, pcs_machine) >= wowdir_len)
                FIXME("shouldn't happen\n");
            wcscat(sysdir, L"\\");
            wcscat(wowdir, L"\\");
        }
    }
    else count32 = 0;

    for (i = 0; i < count + count32; i++)
    {
        if (GetModuleInformation(process, hmods[i], &mi, sizeof(mi)) &&
            GetModuleFileNameExW(process, hmods[i], imagenameW, ARRAY_SIZE(imagenameW)))
        {
            /* rewrite path in system32 into syswow64 for 32bit modules */
            if (i >= count)
            {
                size_t len = wcslen(imagenameW);

                if (!wcsnicmp(imagenameW, sysdir, sysdir_len) &&
                    (len - sysdir_len + wowdir_len) + 1 <= ARRAY_SIZE(imagenameW))
                {
                    memmove(&imagenameW[wowdir_len], &imagenameW[sysdir_len], (len - sysdir_len) * sizeof(WCHAR));
                    memcpy(imagenameW, wowdir, wowdir_len * sizeof(WCHAR));
                }
            }
            if (!enum_cb(imagenameW, (DWORD_PTR)mi.lpBaseOfDll, mi.SizeOfImage, user))
                break;
        }
    }

    HeapFree(GetProcessHeap(), 0, hmods);
    HeapFree(GetProcessHeap(), 0, sysdir);

    return count != 0;
}

static void dbghelp_str_WtoA(const WCHAR *src, char *dst, int dst_len)
{
    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, dst_len - 1, NULL, NULL);
    dst[dst_len - 1] = 0;
}

#ifndef _WIN64
/******************************************************************
 *		SymGetModuleInfo (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfo(HANDLE hProcess, DWORD dwAddr,
                              PIMAGEHLP_MODULE ModuleInfo)
{
    IMAGEHLP_MODULE     mi;
    IMAGEHLP_MODULEW64  miw64;

    if (sizeof(mi) < ModuleInfo->SizeOfStruct) FIXME("Wrong size\n");

    miw64.SizeOfStruct = sizeof(miw64);
    if (!SymGetModuleInfoW64(hProcess, dwAddr, &miw64)) return FALSE;

    mi.SizeOfStruct  = ModuleInfo->SizeOfStruct;
    mi.BaseOfImage   = miw64.BaseOfImage;
    mi.ImageSize     = miw64.ImageSize;
    mi.TimeDateStamp = miw64.TimeDateStamp;
    mi.CheckSum      = miw64.CheckSum;
    mi.NumSyms       = miw64.NumSyms;
    mi.SymType       = miw64.SymType;
    dbghelp_str_WtoA(miw64.ModuleName, mi.ModuleName, sizeof(mi.ModuleName));
    dbghelp_str_WtoA(miw64.ImageName, mi.ImageName, sizeof(mi.ImageName));
    dbghelp_str_WtoA(miw64.LoadedImageName, mi.LoadedImageName, sizeof(mi.LoadedImageName));

    memcpy(ModuleInfo, &mi, ModuleInfo->SizeOfStruct);

    return TRUE;
}

/******************************************************************
 *		SymGetModuleInfoW (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfoW(HANDLE hProcess, DWORD dwAddr,
                               PIMAGEHLP_MODULEW ModuleInfo)
{
    IMAGEHLP_MODULEW64  miw64;
    IMAGEHLP_MODULEW    miw;

    if (sizeof(miw) < ModuleInfo->SizeOfStruct) FIXME("Wrong size\n");

    miw64.SizeOfStruct = sizeof(miw64);
    if (!SymGetModuleInfoW64(hProcess, dwAddr, &miw64)) return FALSE;

    miw.SizeOfStruct  = ModuleInfo->SizeOfStruct;
    miw.BaseOfImage   = miw64.BaseOfImage;
    miw.ImageSize     = miw64.ImageSize;
    miw.TimeDateStamp = miw64.TimeDateStamp;
    miw.CheckSum      = miw64.CheckSum;
    miw.NumSyms       = miw64.NumSyms;
    miw.SymType       = miw64.SymType;
    lstrcpyW(miw.ModuleName, miw64.ModuleName);
    lstrcpyW(miw.ImageName, miw64.ImageName);
    lstrcpyW(miw.LoadedImageName, miw64.LoadedImageName);
    memcpy(ModuleInfo, &miw, ModuleInfo->SizeOfStruct);

    return TRUE;
}
#endif

/******************************************************************
 *		SymGetModuleInfo64 (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfo64(HANDLE hProcess, DWORD64 dwAddr,
                                PIMAGEHLP_MODULE64 ModuleInfo)
{
    IMAGEHLP_MODULE64   mi64;
    IMAGEHLP_MODULEW64  miw64;

    if (sizeof(mi64) < ModuleInfo->SizeOfStruct)
    {
        SetLastError(ERROR_MOD_NOT_FOUND); /* NOTE: native returns this error */
        WARN("Wrong size %lu\n", ModuleInfo->SizeOfStruct);
        return FALSE;
    }

    miw64.SizeOfStruct = sizeof(miw64);
    if (!SymGetModuleInfoW64(hProcess, dwAddr, &miw64)) return FALSE;

    mi64.SizeOfStruct  = ModuleInfo->SizeOfStruct;
    mi64.BaseOfImage   = miw64.BaseOfImage;
    mi64.ImageSize     = miw64.ImageSize;
    mi64.TimeDateStamp = miw64.TimeDateStamp;
    mi64.CheckSum      = miw64.CheckSum;
    mi64.NumSyms       = miw64.NumSyms;
    mi64.SymType       = miw64.SymType;
    dbghelp_str_WtoA(miw64.ModuleName, mi64.ModuleName, sizeof(mi64.ModuleName));
    dbghelp_str_WtoA(miw64.ImageName, mi64.ImageName, sizeof(mi64.ImageName));
    dbghelp_str_WtoA(miw64.LoadedImageName, mi64.LoadedImageName, sizeof(mi64.LoadedImageName));
    dbghelp_str_WtoA(miw64.LoadedPdbName, mi64.LoadedPdbName, sizeof(mi64.LoadedPdbName));

    mi64.CVSig         = miw64.CVSig;
    dbghelp_str_WtoA(miw64.CVData, mi64.CVData, sizeof(mi64.CVData));
    mi64.PdbSig        = miw64.PdbSig;
    mi64.PdbSig70      = miw64.PdbSig70;
    mi64.PdbAge        = miw64.PdbAge;
    mi64.PdbUnmatched  = miw64.PdbUnmatched;
    mi64.DbgUnmatched  = miw64.DbgUnmatched;
    mi64.LineNumbers   = miw64.LineNumbers;
    mi64.GlobalSymbols = miw64.GlobalSymbols;
    mi64.TypeInfo      = miw64.TypeInfo;
    mi64.SourceIndexed = miw64.SourceIndexed;
    mi64.Publics       = miw64.Publics;
    mi64.MachineType   = miw64.MachineType;
    mi64.Reserved      = miw64.Reserved;

    memcpy(ModuleInfo, &mi64, ModuleInfo->SizeOfStruct);

    return TRUE;
}

/******************************************************************
 *		SymGetModuleInfoW64 (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfoW64(HANDLE hProcess, DWORD64 dwAddr,
                                 PIMAGEHLP_MODULEW64 ModuleInfo)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    IMAGEHLP_MODULEW64  miw64;

    TRACE("%p %I64x %p\n", hProcess, dwAddr, ModuleInfo);

    if (!pcs) return FALSE;
    if (ModuleInfo->SizeOfStruct > sizeof(*ModuleInfo)) return FALSE;
    module = module_find_by_addr(pcs, dwAddr);
    if (!module) return FALSE;

    miw64 = module->module;

    if (dbghelp_opt_real_path && module->real_path)
        lstrcpynW(miw64.LoadedImageName, module->real_path, ARRAY_SIZE(miw64.LoadedImageName));
    else if (miw64.SymType == SymDeferred)
    {
        miw64.LoadedImageName[0] = '\0';
        miw64.TimeDateStamp = 0;
    }

    /* update debug information from container if any */
    if (module->module.SymType == SymNone)
    {
        module = module_get_container(pcs, module);
        if (module && module->module.SymType != SymNone)
        {
            miw64.SymType = module->module.SymType;
            miw64.NumSyms = module->module.NumSyms;
        }
    }
    memcpy(ModuleInfo, &miw64, ModuleInfo->SizeOfStruct);
    return TRUE;
}

#ifndef _WIN64
/***********************************************************************
 *		SymGetModuleBase (DBGHELP.@)
 */
DWORD WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
    return (DWORD)SymGetModuleBase64(hProcess, dwAddr);
}
#endif

/***********************************************************************
 *		SymGetModuleBase64 (DBGHELP.@)
 */
DWORD64 WINAPI SymGetModuleBase64(HANDLE hProcess, DWORD64 dwAddr)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return 0;
    module = module_find_by_addr(pcs, dwAddr);
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
    module->sorttab_size = 0;
    module->addr_sorttab = NULL;
    module->num_sorttab = module->num_symbols = 0;
    hash_table_destroy(&module->ht_symbols);
    module->ht_symbols.num_buckets = 0;
    module->ht_symbols.buckets = NULL;
    hash_table_destroy(&module->ht_types);
    module->ht_types.num_buckets = 0;
    module->ht_types.buckets = NULL;
    hash_table_destroy(&module->ht_symbols);
    module->sources_used = module->sources_alloc = 0;
    module->sources = NULL;
}

static BOOL WINAPI process_invade_cb(PCWSTR name, ULONG64 base, ULONG size, PVOID user)
{
    HANDLE      hProcess = user;

    /* Note: this follows native behavior:
     * If a PE module has been unloaded from debuggee, it's not immediately removed
     * from module list in dbghelp.
     * Removal may eventually happen when loading a another module with SymLoadModule:
     * if the module to be loaded overlaps an existing one, SymLoadModule will
     * automatically unload the eldest one.
     */
    SymLoadModuleExW(hProcess, 0, name, NULL, base, size, NULL, 0);
    return TRUE;
}

BOOL module_refresh_list(struct process *pcs)
{
    BOOL ret;

    ret = pcs->loader->synchronize_module_list(pcs);
    ret = EnumerateLoadedModulesW64(pcs->handle, process_invade_cb, pcs->handle) && ret;
    return ret;
}

/******************************************************************
 *              SymRefreshModuleList (DBGHELP.@)
 */
BOOL WINAPI SymRefreshModuleList(HANDLE hProcess)
{
    struct process *pcs;

    TRACE("(%p)\n", hProcess);

    if (!(pcs = process_find_by_handle(hProcess))) return FALSE;
    return module_refresh_list(pcs);
}

#ifndef _WIN64
/***********************************************************************
 *		SymFunctionTableAccess (DBGHELP.@)
 */
PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
    return SymFunctionTableAccess64(hProcess, AddrBase);
}
#endif

/***********************************************************************
 *		SymFunctionTableAccess64 (DBGHELP.@)
 */
PVOID WINAPI SymFunctionTableAccess64(HANDLE hProcess, DWORD64 AddrBase)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return NULL;
    module = module_find_by_addr(pcs, AddrBase);
    if (!module || !module->cpu->find_runtime_function) return NULL;

    return module->cpu->find_runtime_function(module, AddrBase);
}

static struct module* native_load_module(struct process* pcs, const WCHAR* name, ULONG_PTR addr)
{
    return NULL;
}

static BOOL native_load_debug_info(struct process* process, struct module* module)
{
    module->module.SymType = SymNone;
    return FALSE;
}

static BOOL native_fetch_file_info(struct process* process, const WCHAR* name, ULONG_PTR load_addr, DWORD_PTR* base,
                                   DWORD* size, DWORD* checksum)
{
    return FALSE;
}

static BOOL noloader_synchronize_module_list(struct process* pcs)
{
    return FALSE;
}

static BOOL noloader_enum_modules(struct process *process, enum_modules_cb cb, void* user)
{
    return FALSE;
}

static BOOL empty_synchronize_module_list(struct process* pcs)
{
    return TRUE;
}

static BOOL empty_enum_modules(struct process *process, enum_modules_cb cb, void* user)
{
    return TRUE;
}

/* to be used when debuggee isn't a live target */
const struct loader_ops no_loader_ops =
{
    noloader_synchronize_module_list,
    native_load_module,
    native_load_debug_info,
    noloader_enum_modules,
    native_fetch_file_info,
};

/* to be used when debuggee is a live target, but which system information isn't available */
const struct loader_ops empty_loader_ops =
{
    empty_synchronize_module_list,
    native_load_module,
    native_load_debug_info,
    empty_enum_modules,
    native_fetch_file_info,
};

BOOL WINAPI wine_get_module_information(HANDLE proc, DWORD64 base, struct dhext_module_information* wmi, unsigned len)
{
    struct process*     pcs;
    struct module*      module;
    struct dhext_module_information dhmi;

    /* could be interpreted as a WinDbg extension */
    if (!dbghelp_opt_extension_api)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    TRACE("(%p %I64x %p %u\n", proc, base, wmi, len);

    if (!(pcs = process_find_by_handle(proc))) return FALSE;
    if (len > sizeof(*wmi)) return FALSE;

    module = module_find_by_addr(pcs, base);
    if (!module) return FALSE;

    dhmi.type = module->type;
    dhmi.is_virtual = module->is_virtual;
    dhmi.is_wine_builtin = module->is_wine_builtin;
    dhmi.has_file_image = module->has_file_image;
    dhmi.debug_format_bitmask = module->debug_format_bitmask;
    if ((module = module_get_container(pcs, module)))
    {
        dhmi.debug_format_bitmask |= module->debug_format_bitmask;
    }
    memcpy(wmi, &dhmi, len);

    return TRUE;
}
