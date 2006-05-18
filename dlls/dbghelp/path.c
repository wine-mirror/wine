/*
 * File path.c - managing path in debugging environments
 *
 * Copyright (C) 2004, Eric Pouech
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

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbghelp_private.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static inline BOOL is_sep(char ch) {return ch == '/' || ch == '\\';}

static inline const char* file_name(const char* str)
{
    const char*       p;

    for (p = str + strlen(str) - 1; p >= str && !is_sep(*p); p--);
    return p + 1;
}

/******************************************************************
 *		FindDebugInfoFile (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFile(PCSTR FileName, PCSTR SymbolPath, PSTR DebugFilePath)
{
    HANDLE      h;

    h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        if (!SearchPathA(SymbolPath, file_name(FileName), NULL, MAX_PATH, DebugFilePath, NULL))
            return NULL;
        h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}
 
/******************************************************************
 *		FindDebugInfoFileEx (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFileEx(PCSTR FileName, PCSTR SymbolPath,
                                  PSTR DebugFilePath, 
                                  PFIND_DEBUG_FILE_CALLBACK Callback,
                                  PVOID CallerData)
{
    FIXME("(%s %s %p %p %p): stub\n", 
          FileName, SymbolPath, DebugFilePath, Callback, CallerData);
    return NULL;
}

/******************************************************************
 *		FindExecutableImage (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImage(PCSTR FileName, PCSTR SymbolPath, PSTR ImageFilePath)
{
    HANDLE h;
    if (!SearchPathA(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileA(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, 
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/***********************************************************************
 *           MakeSureDirectoryPathExists (DBGHELP.@)
 */
BOOL WINAPI MakeSureDirectoryPathExists(LPCSTR DirPath)
{
    char path[MAX_PATH];
    const char *p = DirPath;
    int  n;

    if (p[0] && p[1] == ':') p += 2;
    while (*p == '\\') p++; /* skip drive root */
    while ((p = strchr(p, '\\')) != NULL)
    {
       n = p - DirPath + 1;
       memcpy(path, DirPath, n);
       path[n] = '\0';
       if( !CreateDirectoryA(path, NULL)            &&
           (GetLastError() != ERROR_ALREADY_EXISTS))
           return FALSE;
       p++;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
       SetLastError(ERROR_SUCCESS);

    return TRUE;
}

/******************************************************************
 *		SymMatchFileName (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchFileName(char* file, char* match,
                             char** filestop, char** matchstop)
{
    char*       fptr;
    char*       mptr;

    TRACE("(%s %s %p %p)\n", file, match, filestop, matchstop);

    fptr = file + strlen(file) - 1;
    mptr = match + strlen(match) - 1;

    while (fptr >= file && mptr >= match)
    {
        if (toupper(*fptr) != toupper(*mptr) && !(is_sep(*fptr) && is_sep(*mptr)))
            break;
        fptr--; mptr--;
    }
    if (filestop) *filestop = fptr;
    if (matchstop) *matchstop = mptr;

    return mptr == match - 1;
}

static BOOL do_search(const char* file, char* buffer, BOOL recurse,
                      PENUMDIRTREE_CALLBACK cb, void* user)
{
    HANDLE              h;
    WIN32_FIND_DATAA    fd;
    unsigned            pos;
    BOOL                found = FALSE;

    pos = strlen(buffer);
    if (buffer[pos - 1] != '\\') buffer[pos++] = '\\';
    strcpy(buffer + pos, "*.*");
    if ((h = FindFirstFileA(buffer, &fd)) == INVALID_HANDLE_VALUE)
        return FALSE;
    /* doc doesn't specify how the tree is enumerated... 
     * doing a depth first based on, but may be wrong
     */
    do
    {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;

        strcpy(buffer + pos, fd.cFileName);
        if (recurse && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            found = do_search(file, buffer, TRUE, cb, user);
        else if (SymMatchFileName(buffer, (char*)file, NULL, NULL))
        {
            if (!cb || cb(buffer, user)) found = TRUE;
        }
    } while (!found && FindNextFileA(h, &fd));
    if (!found) buffer[--pos] = '\0';
    FindClose(h);

    return found;
}

/***********************************************************************
 *           SearchTreeForFile (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFile(PCSTR root, PCSTR file, PSTR buffer)
{
    TRACE("(%s, %s, %p)\n", 
          debugstr_a(root), debugstr_a(file), buffer);
    strcpy(buffer, root);
    return do_search(file, buffer, TRUE, NULL, NULL);
}

/******************************************************************
 *		EnumDirTree (DBGHELP.@)
 *
 *
 */
BOOL WINAPI EnumDirTree(HANDLE hProcess, PCSTR root, PCSTR file,
                        LPSTR buffer, PENUMDIRTREE_CALLBACK cb, PVOID user)
{
    TRACE("(%p %s %s %p %p %p)\n", hProcess, root, file, buffer, cb, user);

    strcpy(buffer, root);
    return do_search(file, buffer, TRUE, cb, user);
}

struct sffip
{
    enum module_type            kind;
    /* pe:  id  -> DWORD:timestamp
     *      two -> size of image (from PE header)
     * pdb: id  -> PDB signature
     *            I think either DWORD:timestamp or GUID:guid depending on PDB version
     *      two -> PDB age ???
     * elf: id  -> DWORD:CRC 32 of ELF image (Wine only)
     */
    PVOID                       id;
    DWORD                       two;
    DWORD                       three;
    DWORD                       flags;
    PFINDFILEINPATHCALLBACK     cb;
    void*                       user;
};

/* checks that buffer (as found by matching the name) matches the info
 * (information is based on file type)
 * returns TRUE when file is found, FALSE to continue searching
 * (NB this is the opposite conventions as for SymFindFileInPathProc)
 */
static BOOL CALLBACK sffip_cb(LPCSTR buffer, void* user)
{
    struct sffip*       s = (struct sffip*)user;
    DWORD               size, checksum;

    /* FIXME: should check that id/two/three match the file pointed
     * by buffer
     */
    switch (s->kind)
    {
    case DMT_PE:
        {
            HANDLE  hFile, hMap;
            void*   mapping;
            DWORD   timestamp;

            timestamp = ~(DWORD_PTR)s->id;
            size = ~s->two;
            hFile = CreateFileA(buffer, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) return FALSE;
            if ((hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
            {
                if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
                {
                    IMAGE_NT_HEADERS*   nth = RtlImageNtHeader(mapping);
                    timestamp = nth->FileHeader.TimeDateStamp;
                    size = nth->OptionalHeader.SizeOfImage;
                    UnmapViewOfFile(mapping);
                }
                CloseHandle(hMap);
            }
            CloseHandle(hFile);
            if (timestamp != (DWORD_PTR)s->id || size != s->two)
            {
                WARN("Found %s, but wrong size or timestamp\n", buffer);
                return FALSE;
            }
        }
        break;
    case DMT_ELF:
        if (elf_fetch_file_info(buffer, 0, &size, &checksum))
        {
            if (checksum != (DWORD_PTR)s->id)
            {
                WARN("Found %s, but wrong checksums: %08lx %08lx\n",
                      buffer, checksum, (DWORD_PTR)s->id);
                return FALSE;
            }
        }
        else
        {
            WARN("Couldn't read %s\n", buffer);
            return FALSE;
        }
        break;
    case DMT_PDB:
        {
            struct pdb_lookup   pdb_lookup;

            pdb_lookup.filename = buffer;

            if (!pdb_fetch_file_info(&pdb_lookup)) return FALSE;
            switch (pdb_lookup.kind)
            {
            case PDB_JG:
                if (s->flags & SSRVOPT_GUIDPTR)
                {
                    WARN("Found %s, but wrong PDB version\n", buffer);
                    return FALSE;
                }
                if (pdb_lookup.u.jg.timestamp != (DWORD_PTR)s->id)
                {
                    WARN("Found %s, but wrong signature: %08lx %08lx\n",
                         buffer, pdb_lookup.u.jg.timestamp, (DWORD_PTR)s->id);
                    return FALSE;
                }
                break;
            case PDB_DS:
                if (!(s->flags & SSRVOPT_GUIDPTR))
                {
                    WARN("Found %s, but wrong PDB version\n", buffer);
                    return FALSE;
                }
                if (!(memcmp(&pdb_lookup.u.ds.guid, (GUID*)s->id, sizeof(GUID))))
                {
                    WARN("Found %s, but wrong GUID: %s %s\n",
                         buffer, debugstr_guid(&pdb_lookup.u.ds.guid),
                         debugstr_guid((GUID*)s->id));
                    return FALSE;
                }
                break;
            }
            if (pdb_lookup.age != s->two)
            {
                WARN("Found %s, but wrong age: %08lx %08lx\n",
                     buffer, pdb_lookup.age, s->two);
                return FALSE;
            }
        }
        break;
    default:
        FIXME("What the heck??\n");
        return FALSE;
    }
    /* yes, EnumDirTree/do_search and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return !(s->cb)((char*)buffer, s->user);
}

/******************************************************************
 *		SymFindFileInPath (DBGHELP.@)
 *
 */
BOOL WINAPI SymFindFileInPath(HANDLE hProcess, PCSTR inSearchPath, PCSTR full_path,
                              PVOID id, DWORD two, DWORD three, DWORD flags,
                              LPSTR buffer, PFINDFILEINPATHCALLBACK cb,
                              PVOID user)
{
    struct sffip        s;
    struct process*     pcs = process_find_by_handle(hProcess);
    char                tmp[MAX_PATH];
    char*               ptr;
    const char*         filename;
    const char*         searchPath = inSearchPath;

    TRACE("(%p %s %s %p %08lx %08lx %08lx %p %p %p)\n",
          hProcess, searchPath, full_path, id, two, three, flags, 
          buffer, cb, user);

    if (!pcs) return FALSE;
    if (!searchPath)
    {
        unsigned len = WideCharToMultiByte(CP_ACP, 0, pcs->search_path, -1, NULL, 0, NULL, NULL);
        char* buf;

        searchPath = buf = HeapAlloc(GetProcessHeap(), 0, len);
        if (!searchPath) return FALSE;
        WideCharToMultiByte(CP_ACP, 0, pcs->search_path, -1, buf, len, NULL, NULL);
    }

    s.id = id;
    s.two = two;
    s.three = three;
    s.flags = flags;
    s.cb = cb;
    s.user = user;

    filename = file_name(full_path);
    s.kind = module_get_type_by_name(filename);

    /* first check full path to file */
    if (sffip_cb(full_path, &s))
    {
        strcpy(buffer, full_path);
        if (searchPath != inSearchPath)
            HeapFree(GetProcessHeap(), 0, (char*)searchPath);
        return TRUE;
    }

    while (searchPath)
    {
        ptr = strchr(searchPath, ';');
        if (ptr)
        {
            memcpy(tmp, searchPath, ptr - searchPath);
            tmp[ptr - searchPath] = 0;
            searchPath = ptr + 1;
        }
        else
        {
            strcpy(tmp, searchPath);
            searchPath = NULL;
        }
        if (do_search(filename, tmp, FALSE, sffip_cb, &s))
        {
            strcpy(buffer, tmp);
            if (searchPath != inSearchPath)
                HeapFree(GetProcessHeap(), 0, (char*)searchPath);
            return TRUE;
        }
    }
    if (searchPath != inSearchPath)
        HeapFree(GetProcessHeap(), 0, (char*)searchPath);
    return FALSE;
}
