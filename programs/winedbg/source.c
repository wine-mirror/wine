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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include "debugger.h"

struct searchlist
{
  char * path;
  struct searchlist * next;
};


struct open_filelist
{
  char			* path;
  char			* real_path;
  struct open_filelist  * next;
  unsigned int		  size;
  signed int		  nlines;
  unsigned int		* linelist;
};

static struct open_filelist * ofiles;

static struct searchlist * listhead;
static char DEBUG_current_sourcefile[PATH_MAX];
static int DEBUG_start_sourceline = -1;
static int DEBUG_end_sourceline = -1;

void
DEBUG_ShowDir(void)
{
  struct searchlist * sl;

  DEBUG_Printf(DBG_CHN_MESG,"Search list :\n");
  for(sl = listhead; sl; sl = sl->next)
    {
      DEBUG_Printf(DBG_CHN_MESG, "\t%s\n", sl->path);
    }
  DEBUG_Printf(DBG_CHN_MESG, "\n");
}

void
DEBUG_AddPath(const char * path)
{
  struct searchlist * sl;

  sl = (struct searchlist *) DBG_alloc(sizeof(struct searchlist));
  if( sl == NULL )
    {
      return;
    }

  sl->next = listhead;
  sl->path = DBG_strdup(path);
  listhead = sl;
}

void
DEBUG_NukePath(void)
{
  struct searchlist * sl;
  struct searchlist * nxt;

  for(sl = listhead; sl; sl = nxt)
    {
      nxt = sl->next;
      DBG_free(sl->path);
      DBG_free(sl);
    }

  listhead = NULL;
}

static  void*   DEBUG_MapFile(const char* name, HANDLE* hMap, unsigned* size)
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

static void     DEBUG_UnmapFile(void* addr, HANDLE hMap)
{
    UnmapViewOfFile(addr);
    CloseHandle(hMap);
}

static struct open_filelist*    DEBUG_SearchOpenFile(const char* name)
{
    struct open_filelist*       ol;

    for (ol = ofiles; ol; ol = ol->next)
    {
        if (strcmp(ol->path, name) == 0) break;
    }
    return ol;
}

static
int
DEBUG_DisplaySource(char * sourcefile, int start, int end)
{
    char*                       addr;
    int				i;
    struct open_filelist*       ol;
    int				nlines;
    char*                       basename = NULL;
    char*                       pnt;
    int				rtn;
    struct searchlist*          sl;
    HANDLE                      hMap;
    DWORD			status;
    char			tmppath[PATH_MAX];

    /*
     * First see whether we have the file open already.  If so, then
     * use that, otherwise we have to try and open it.
     */
    ol = DEBUG_SearchOpenFile(sourcefile);

    if ( ol == NULL )
    {
        /*
         * Try again, stripping the path from the opened file.
         */
        basename = strrchr(sourcefile, '\\' );
        if ( !basename )
            basename = strrchr(sourcefile, '/' );
        if ( !basename )
            basename = sourcefile;
        else
            basename++;

        ol = DEBUG_SearchOpenFile(basename);
    }

    if ( ol == NULL )
    {
        /*
         * Crapola.  We need to try and open the file.
         */
        status = GetFileAttributes(sourcefile);
        if ( status != -1 )
        {
            strcpy(tmppath, sourcefile);
        }
        else if ( (status = GetFileAttributes(basename)) != -1 )
        {
            strcpy(tmppath, basename);
        }
        else
        {
            for (sl = listhead; sl; sl = sl->next)
            {
                strcpy(tmppath, sl->path);
                if ( tmppath[strlen(tmppath)-1] != '/' && tmppath[strlen(tmppath)-1] != '\\' )
                {
                    strcat(tmppath, "/");
                }
                /*
                 * Now append the base file name.
                 */
                strcat(tmppath, basename);

                status = GetFileAttributes(tmppath);
                if ( status != -1 ) break;
            }

            if ( sl == NULL )
            {
                if (DEBUG_InteractiveP)
                {
                    char zbuf[256];
                    /*
                     * Still couldn't find it.  Ask user for path to add.
                     */
                    snprintf(zbuf, sizeof(zbuf), "Enter path to file '%s': ", sourcefile);
                    DEBUG_ReadLine(zbuf, tmppath, sizeof(tmppath));

                    if ( tmppath[strlen(tmppath)-1] != '/' )
                    {
                        strcat(tmppath, "/");
                    }
                    /*
                     * Now append the base file name.
                     */
                    strcat(tmppath, basename);

                    status = GetFileAttributes(tmppath);
                }
                else
                {
                    status = -1;
                    strcpy(tmppath, sourcefile);
                }

                if ( status == -1 )
                {
                    /*
                     * OK, I guess the user doesn't really want to see it
                     * after all.
                     */
                    ol = (struct open_filelist *) DBG_alloc(sizeof(*ol));
                    ol->path = DBG_strdup(sourcefile);
                    ol->real_path = NULL;
                    ol->next = ofiles;
                    ol->nlines = 0;
                    ol->linelist = NULL;
                    ofiles = ol;
                    DEBUG_Printf(DBG_CHN_MESG,"Unable to open file %s\n", tmppath);
                    return FALSE;
                }
            }
        }
        /*
         * Create header for file.
         */
        ol = (struct open_filelist *) DBG_alloc(sizeof(*ol));
        ol->path = DBG_strdup(sourcefile);
        ol->real_path = DBG_strdup(tmppath);
        ol->next = ofiles;
        ol->nlines = 0;
        ol->linelist = NULL;
        ol->size = 0;
        ofiles = ol;

        addr = DEBUG_MapFile(tmppath, &hMap, &ol->size);
        if ( addr == (char *) -1 )
        {
            return FALSE;
        }
        /*
         * Now build up the line number mapping table.
         */
        ol->nlines = 1;
        pnt = addr;
        while (pnt < addr + ol->size )
        {
            if ( *pnt++ == '\n' )
            {
                ol->nlines++;
            }
        }

        ol->nlines++;
        ol->linelist = (unsigned int*) DBG_alloc( ol->nlines * sizeof(unsigned int) );

        nlines = 0;
        pnt = addr;
        ol->linelist[nlines++] = 0;
        while(pnt < addr + ol->size )
        {
            if( *pnt++ == '\n' )
            {
                ol->linelist[nlines++] = pnt - addr;
            }
        }
        ol->linelist[nlines++] = pnt - addr;

    }
    else
    {
        addr = DEBUG_MapFile(ol->real_path, &hMap, NULL);
        if ( addr == (char *) -1 )
        {
            return FALSE;
        }
    }
    /*
     * All we need to do is to display the source lines here.
     */
    rtn = FALSE;
    for (i = start - 1; i <= end - 1; i++)
    {
        char    buffer[1024];

        if (i < 0 || i >= ol->nlines - 1)
	{
            continue;
	}

        rtn = TRUE;
        memset(&buffer, 0, sizeof(buffer));
        if ( ol->linelist[i+1] != ol->linelist[i] )
	{
            memcpy(&buffer, addr + ol->linelist[i],
                   (ol->linelist[i+1] - ol->linelist[i]) - 1);
	}
        DEBUG_Printf(DBG_CHN_MESG,"%d\t%s\n", i + 1,  buffer);
    }

    DEBUG_UnmapFile(addr, hMap);
    return rtn;
}

void
DEBUG_List(struct list_id * source1, struct list_id * source2,
			 int delta)
{
  int    end;
  int    rtn;
  int    start;
  char * sourcefile;

  /*
   * We need to see what source file we need.  Hopefully we only have
   * one specified, otherwise we might as well punt.
   */
  if( source1 != NULL
      && source2 != NULL
      && source1->sourcefile != NULL
      && source2->sourcefile != NULL
      && strcmp(source1->sourcefile, source2->sourcefile) != 0 )
    {
      DEBUG_Printf(DBG_CHN_MESG, "Ambiguous source file specification.\n");
      return;
    }

  sourcefile = NULL;
  if( source1 != NULL && source1->sourcefile != NULL )
    {
      sourcefile = source1->sourcefile;
    }

  if( sourcefile == NULL
      && source2 != NULL
      && source2->sourcefile != NULL )
    {
      sourcefile = source2->sourcefile;
    }

  if( sourcefile == NULL )
    {
      sourcefile = (char *) &DEBUG_current_sourcefile;
    }

  if( sourcefile == NULL )
    {
      DEBUG_Printf(DBG_CHN_MESG, "No source file specified.\n");
      return;
    }

  /*
   * Now figure out the line number range to be listed.
   */
  start = -1;
  end = -1;

  if( source1 != NULL )
    {
      start = source1->line;
    }

  if( source2 != NULL )
    {
      end = source2->line;
    }

  if( start == -1 && end == -1 )
    {
      if( delta < 0 )
	{
	  end = DEBUG_start_sourceline;
	  start = end + delta;
	}
      else
	{
	  start = DEBUG_end_sourceline;
	  end = start + delta;
	}
    }
  else if( start == -1 )
    {
      start = end + delta;
    }
  else if (end == -1)
    {
      end = start + delta;
    }

  /*
   * Now call this function to do the dirty work.
   */
  rtn = DEBUG_DisplaySource(sourcefile, start, end);

  if( sourcefile != (char *) &DEBUG_current_sourcefile )
    {
      strcpy(DEBUG_current_sourcefile, sourcefile);
    }
  DEBUG_start_sourceline = start;
  DEBUG_end_sourceline = end;
}

DBG_ADDR DEBUG_LastDisassemble={0,0};

BOOL DEBUG_DisassembleInstruction(DBG_ADDR *addr)
{
   char		ch;
   BOOL		ret = TRUE;

   DEBUG_PrintAddress(addr, DEBUG_CurrThread->dbg_mode, TRUE);
   DEBUG_Printf(DBG_CHN_MESG, ": ");
   if (!DEBUG_READ_MEM_VERBOSE((void*)DEBUG_ToLinear(addr), &ch, sizeof(ch))) {
      DEBUG_Printf(DBG_CHN_MESG, "-- no code --");
      ret = FALSE;
   } else {
      DEBUG_Disasm(addr, TRUE);
   }
   DEBUG_Printf(DBG_CHN_MESG,"\n");
   return ret;
}

void
DEBUG_Disassemble(const DBG_VALUE *xstart,const DBG_VALUE *xend,int offset)
{
  int i;
  DBG_ADDR	last;
  DBG_VALUE	end,start;

  if (xstart) {
    start = *xstart;
    DEBUG_GrabAddress(&start, TRUE);
  }
  if (xend) {
    end = *xend;
    DEBUG_GrabAddress(&end, TRUE);
  }
  if (!xstart && !xend) {
    last = DEBUG_LastDisassemble;
    if (!last.seg && !last.off)
      DEBUG_GetCurrentAddress( &last );

    for (i=0;i<offset;i++)
      if (!DEBUG_DisassembleInstruction(&last)) break;
    DEBUG_LastDisassemble = last;
    return;
  }
  last = start.addr;
  if (!xend) {
    for (i=0;i<offset;i++)
      if (!DEBUG_DisassembleInstruction(&last)) break;
    DEBUG_LastDisassemble = last;
    return;
  }
  while (last.off <= end.addr.off)
    if (!DEBUG_DisassembleInstruction(&last)) break;
  DEBUG_LastDisassemble = last;
  return;
}
