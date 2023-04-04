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

#include <stdio.h>
#include <stdlib.h>

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

void source_show_path(void)
{
    const char* ptr;
    const char* next;

    dbg_printf("Search list:\n");
    for (ptr = dbg_curr_process->search_path; ptr; ptr = next)
    {
        next = strchr(ptr, ';');
        if (next)
            dbg_printf("\t%.*s\n", (int)(next++ - ptr), ptr);
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
    if (dbg_curr_process->search_path)
    {
        unsigned pos = strlen(dbg_curr_process->search_path) + 1;
        new = realloc(dbg_curr_process->search_path, pos + size);
        if (!new) return;
        new[pos - 1] = ';';
        strcpy(&new[pos], path);
    }
    else
    {
        new = malloc(size);
        if (!new) return;
        strcpy(new, path);
    }
    dbg_curr_process->search_path = new;
}

void source_nuke_path(struct dbg_process* p)
{
    free(p->search_path);
    p->search_path = NULL;
}

static  void*   source_map_file(const char* name, HANDLE* hMap, unsigned* size)
{
    HANDLE              hFile;

    hFile = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return (void*)-1;
    if (size != NULL && (*size = GetFileSize(hFile, NULL)) == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return (void*)-1;
    }
    *hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
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

    for (ol = dbg_curr_process->source_ofiles; ol; ol = ol->next)
    {
        if (strcmp(ol->path, name) == 0) break;
    }
    return ol;
}

static BOOL     source_locate_file(const char* srcfile, char* path)
{
    BOOL        found = FALSE;

    if (GetFileAttributesA(srcfile) != INVALID_FILE_ATTRIBUTES)
    {
        strcpy(path, srcfile);
        found = TRUE;
    }
    else
    {
        const char* spath;
        const char* sp = dbg_curr_process->search_path;
        if (!sp) sp = ".";
        spath = srcfile;
        while (!found)
        {
            while (*spath && *spath != '/' && *spath != '\\') spath++;
            if (!*spath) break;
            spath++;
            found = SearchPathA(sp, spath, NULL, MAX_PATH, path, NULL);
        }
    }
    return found;
}

static struct open_file_list* source_add_file(const char* name, const char* realpath)
{
    struct open_file_list*      ol;
    size_t                      sz, nlen;

    sz = sizeof(*ol);
    nlen = strlen(name) + 1;
    if (realpath) sz += strlen(realpath) + 1;
    ol = malloc(sz + nlen);
    if (!ol) return NULL;
    strcpy(ol->path = (char*)(ol + 1), name);
    if (realpath)
        strcpy(ol->real_path = ol->path + nlen, realpath);
    else
        ol->real_path = NULL;
    ol->next = dbg_curr_process->source_ofiles;
    ol->nlines = 0;
    ol->linelist = NULL;
    ol->size = 0;
    return dbg_curr_process->source_ofiles = ol;
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
    char			tmppath[MAX_PATH];

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
        if (!source_locate_file(sourcefile, tmppath))
        {
            if (dbg_interactiveP)
            {
                char zbuf[256];

                for (;;)
                {
                    size_t      len;
                    /*
                     * Still couldn't find it.  Ask user for path to add.
                     */
                    snprintf(zbuf, sizeof(zbuf), "Enter path to file '%s' (<cr> to end search): ", sourcefile);
                    input_read_line(zbuf, tmppath, sizeof(tmppath));
                    if (!(len = strlen(tmppath))) break;

                    /* append '/' if missing at the end */
                    if (tmppath[len - 1] != '/' && tmppath[len - 1] != '\\')
                        tmppath[len++] = '/';
                    strcpy(&tmppath[len], basename);
                    if (GetFileAttributesA(tmppath) != INVALID_FILE_ATTRIBUTES)
                        break;
                    dbg_printf("Unable to access file '%s'\n", tmppath);
                }
            }
            else
            {
                dbg_printf("Unable to access file '%s'\n", sourcefile);
                tmppath[0] = '\0';
            }

            if (!tmppath[0])
            {
                /*
                 * OK, I guess the user doesn't really want to see it
                 * after all.
                 */
                source_add_file(sourcefile, NULL);
                return FALSE;
            }
        }
        /*
         * Create header for file.
         */
        ol = source_add_file(sourcefile, tmppath);

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
        ol->linelist = malloc(ol->nlines * sizeof(unsigned int));

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

void source_list(IMAGEHLP_LINE64* src1, IMAGEHLP_LINE64* src2, int delta)
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
    if (!sourcefile) sourcefile = dbg_curr_process->source_current_file;

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
            end = dbg_curr_process->source_start_line;
            start = end + delta;
	}
        else
	{
            start = dbg_curr_process->source_end_line;
            end = start + delta;
	}
    }
    else if (start == -1) start = end + delta;
    else if (end == -1)   end = start + delta;

    /*
     * Now call this function to do the dirty work.
     */
    source_display(sourcefile, start, end);

    if (sourcefile != dbg_curr_process->source_current_file)
        strcpy(dbg_curr_process->source_current_file, sourcefile);
    dbg_curr_process->source_start_line = start;
    dbg_curr_process->source_end_line = end;
}

void source_list_from_addr(const ADDRESS64* addr, int nlines)
{
    IMAGEHLP_LINE64     il;
    ADDRESS64           la;
    DWORD               disp;

    if (!addr)
    {
        memory_get_current_pc(&la);
        addr = &la;
    }

    il.SizeOfStruct = sizeof(il);
    if (SymGetLineFromAddr64(dbg_curr_process->handle,
                             (DWORD_PTR)memory_to_linear_addr(addr),
			     &disp, &il))
        source_list(&il, NULL, nlines);
}

void source_free_files(struct dbg_process* p)
{
    struct open_file_list*      ofile;
    struct open_file_list*      ofile_next;

    for (ofile = p->source_ofiles; ofile; ofile = ofile_next)
    {
        ofile_next = ofile->next;
        free(ofile->linelist);
        free(ofile);
    }
}
