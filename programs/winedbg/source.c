/*
 * File source.c - source file handling for internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
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
#include <stdio.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include "debugger.h"

struct open_file_list
{
    char*                       path;
    char*                       real_path;
    struct open_file_list*      next;
    unsigned int	        size;
    signed int		        nlines;
    unsigned int*               linelist;
};

static struct open_file_list*   source_ofiles;

static char* search_path; /* = NULL; */
static char source_current_file[PATH_MAX];
static int source_start_line = -1;
static int source_end_line = -1;

void source_show_path(void)
{
    const char* ptr;
    const char* next;

    dbg_printf("Search list:\n");
    for (ptr = search_path; ptr; ptr = next)
    {
        next = strchr(ptr, ';');
        if (next)
            dbg_printf("\t%.*s\n", next++ - ptr, ptr);
        else
            dbg_printf("\t%s\n", ptr);
    }
    dbg_printf("\n");
}

void source_add_path(const char* path)
{
    char*       new;
    unsigned    size;

    size = strlen(path) + 1;
    if (search_path)
    {
        unsigned pos = strlen(search_path) + 1;
        new = HeapReAlloc(GetProcessHeap(), 0, search_path, pos + size);
        if (!new) return;
        new[pos - 1] = ';';
        strcpy(&new[pos], path);
    }
    else
    {
        new = HeapAlloc(GetProcessHeap(), 0, size);
        if (!new) return;
        strcpy(new, path);
    }
    search_path = new;
}

void source_nuke_path(void)
{
    HeapFree(GetProcessHeap(), 0, search_path);
    search_path = NULL;
}

static  void*   source_map_file(const char* name, HANDLE* hMap, unsigned* size)
{
    HANDLE              hFile;

    hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return (void*)-1;
    if (size != NULL && (*size = GetFileSize(hFile, NULL)) == -1) return (void*)-1;
    *hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (!*hMap) return (void*)-1;
    return MapViewOfFile(*hMap, FILE_MAP_READ, 0, 0, 0);
}

static void     source_unmap_file(void* addr, HANDLE hMap)
{
    UnmapViewOfFile(addr);
    CloseHandle(hMap);
}

static struct open_file_list* source_search_open_file(const char* name)
{
    struct open_file_list*      ol;

    for (ol = source_ofiles; ol; ol = ol->next)
    {
        if (strcmp(ol->path, name) == 0) break;
    }
    return ol;
}

static int source_display(const char* sourcefile, int start, int end)
{
    char*                       addr;
    int				i;
    struct open_file_list*      ol;
    int				nlines;
    const char*                 basename = NULL;
    char*                       pnt;
    int				rtn;
    HANDLE                      hMap;
    char			tmppath[PATH_MAX];

    /*
     * First see whether we have the file open already.  If so, then
     * use that, otherwise we have to try and open it.
     */
    ol = source_search_open_file(sourcefile);

    if (ol == NULL)
    {
        /*
         * Try again, stripping the path from the opened file.
         */
        basename = strrchr(sourcefile, '\\');
        if (!basename) basename = strrchr(sourcefile, '/');
        if (!basename) basename = sourcefile;
        else basename++;

        ol = source_search_open_file(basename);
    }

    if (ol == NULL)
    {
        /*
         * Crapola.  We need to try and open the file.
         */
        strcpy(tmppath, sourcefile);
        if (GetFileAttributes(sourcefile) == INVALID_FILE_ATTRIBUTES &&
            (!search_path || !SearchPathA(search_path, sourcefile, NULL, MAX_PATH, tmppath, NULL)))
        {
            if (dbg_interactiveP)
            {
                char zbuf[256];
                /*
                 * Still couldn't find it.  Ask user for path to add.
                 */
                snprintf(zbuf, sizeof(zbuf), "Enter path to file '%s': ", sourcefile);
                input_read_line(zbuf, tmppath, sizeof(tmppath));

                if (tmppath[strlen(tmppath) - 1] != '/')
                {
                    strcat(tmppath, "/");
                }
                /*
                 * Now append the base file name.
                 */
                strcat(tmppath, basename);
            }

            if (GetFileAttributes(tmppath) == INVALID_FILE_ATTRIBUTES)
            {
                /*
                 * OK, I guess the user doesn't really want to see it
                 * after all.
                 */
                ol = HeapAlloc(GetProcessHeap(), 0, sizeof(*ol));
                ol->path = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(sourcefile) + 1), sourcefile);
                ol->real_path = NULL;
                ol->next = source_ofiles;
                ol->nlines = 0;
                ol->linelist = NULL;
                ol->size = 0;
                source_ofiles = ol;
                dbg_printf("Unable to open file '%s'\n", tmppath);
                return FALSE;
            }
        }
        /*
         * Create header for file.
         */
        ol = HeapAlloc(GetProcessHeap(), 0, sizeof(*ol));
        ol->path = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(sourcefile) + 1), sourcefile);
        ol->real_path = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(tmppath) + 1), tmppath);
        ol->next = source_ofiles;
        ol->nlines = 0;
        ol->linelist = NULL;
        ol->size = 0;
        source_ofiles = ol;

        addr = source_map_file(tmppath, &hMap, &ol->size);
        if (addr == (char*)-1) return FALSE;
        /*
         * Now build up the line number mapping table.
         */
        ol->nlines = 1;
        pnt = addr;
        while (pnt < addr + ol->size)
        {
            if (*pnt++ == '\n') ol->nlines++;
        }

        ol->nlines++;
        ol->linelist = HeapAlloc(GetProcessHeap(), 0, ol->nlines * sizeof(unsigned int));

        nlines = 0;
        pnt = addr;
        ol->linelist[nlines++] = 0;
        while (pnt < addr + ol->size)
        {
            if (*pnt++ == '\n') ol->linelist[nlines++] = pnt - addr;
        }
        ol->linelist[nlines++] = pnt - addr;

    }
    else
    {
        addr = source_map_file(ol->real_path, &hMap, NULL);
        if (addr == (char*)-1) return FALSE;
    }
    /*
     * All we need to do is to display the source lines here.
     */
    rtn = FALSE;
    for (i = start - 1; i <= end - 1; i++)
    {
        char    buffer[1024];

        if (i < 0 || i >= ol->nlines - 1) continue;

        rtn = TRUE;
        memset(&buffer, 0, sizeof(buffer));
        if (ol->linelist[i+1] != ol->linelist[i])
	{
            memcpy(&buffer, addr + ol->linelist[i],
                   (ol->linelist[i+1] - ol->linelist[i]) - 1);
	}
        dbg_printf("%d\t%s\n", i + 1, buffer);
    }

    source_unmap_file(addr, hMap);
    return rtn;
}

void source_list(IMAGEHLP_LINE* src1, IMAGEHLP_LINE* src2, int delta)
{
    int         end;
    int         start;
    const char* sourcefile;

    /*
     * We need to see what source file we need.  Hopefully we only have
     * one specified, otherwise we might as well punt.
     */
    if (src1 && src2 && src1->FileName && src2->FileName &&
        strcmp(src1->FileName, src2->FileName) != 0)
    {
        dbg_printf("Ambiguous source file specification.\n");
        return;
    }

    sourcefile = NULL;
    if (src1 && src1->FileName) sourcefile = src1->FileName;
    if (!sourcefile && src2 && src2->FileName) sourcefile = src2->FileName;
    if (!sourcefile) sourcefile = source_current_file;

    /*
     * Now figure out the line number range to be listed.
     */
    start = end = -1;

    if (src1) start = src1->LineNumber;
    if (src2) end   = src2->LineNumber;
    if (start == -1 && end == -1)
    {
        if (delta < 0)
	{
            end = source_start_line;
            start = end + delta;
	}
        else
	{
            start = source_end_line;
            end = start + delta;
	}
    }
    else if (start == -1) start = end + delta;
    else if (end == -1)   end = start + delta;

    /*
     * Now call this function to do the dirty work.
     */
    source_display(sourcefile, start, end);

    if (sourcefile != source_current_file)
        strcpy(source_current_file, sourcefile);
    source_start_line = start;
    source_end_line = end;
}

void source_list_from_addr(const ADDRESS64* addr, int nlines)
{
    IMAGEHLP_LINE       il;
    ADDRESS64           la;
    DWORD               disp;

    if (!addr)
    {
        memory_get_current_pc(&la);
        addr = &la;
    }

    il.SizeOfStruct = sizeof(il);
    if (SymGetLineFromAddr(dbg_curr_process->handle,
                           (unsigned long)memory_to_linear_addr(addr),
                           &disp, &il))
        source_list(&il, NULL, nlines);
}
