/*
 * File source.c - source file handling for internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#include "win.h"
#include "pe_image.h"
#include "peexe.h"
#include "debugger.h"
#include "peexe.h"
#include "xmalloc.h"

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
DEBUG_ShowDir()
{
  struct searchlist * sl;

  fprintf(stderr,"Search list :\n");
  for(sl = listhead; sl; sl = sl->next)
    {
      fprintf(stderr, "\t%s\n", sl->path);
    }
  fprintf(stderr, "\n");
}

void
DEBUG_AddPath(const char * path)
{
  struct searchlist * sl;

  sl = (struct searchlist *) xmalloc(sizeof(struct searchlist));
  if( sl == NULL )
    {
      return;
    }

  sl->next = listhead;
  sl->path = xstrdup(path);
  listhead = sl;
}

void
DEBUG_NukePath()
{
  struct searchlist * sl;
  struct searchlist * nxt;

  for(sl = listhead; sl; sl = nxt)
    {
      nxt = sl->next;
      free(sl->path);
      free(sl);
    }

  listhead = NULL;
}

static
int
DEBUG_DisplaySource(char * sourcefile, int start, int end)
{
  char			      * addr;
  char			        buffer[1024];
  int				fd;
  int				i;
  struct open_filelist	      * ol;
  int				nlines;
  char			      * pnt;
  int				rtn;
  struct searchlist	      * sl;
  struct stat			statbuf;
  int				status;
  char				tmppath[PATH_MAX];

  /*
   * First see whether we have the file open already.  If so, then
   * use that, otherwise we have to try and open it.
   */
  for(ol = ofiles; ol; ol = ol->next)
    {
      if( strcmp(ol->path, sourcefile) == 0 )
	{
	  break;
	}
    }

  if( ol == NULL )
    {
      /*
       * Try again, stripping the path from the opened file.
       */
      for(ol = ofiles; ol; ol = ol->next)
	{
	  pnt = strrchr(ol->path, '/');
	  if( pnt != NULL && strcmp(pnt + 1, sourcefile) == 0 )
	    {
	      break;
	    }
	}
      
    }

  if( ol == NULL )
    {
      /*
       * See if this is a DOS style name or not.
       */
      pnt = strchr(sourcefile, '\\' );
      if( pnt == NULL )
	{
	  pnt = strchr(sourcefile, '/' );
	  if( pnt == NULL )
	    {
	      pnt = sourcefile;
	    }
	}

      /*
       * Crapola.  We need to try and open the file.
       */
      status = stat(sourcefile, &statbuf);
      if( status != -1 )
	{
	  strcpy(tmppath, sourcefile);
	}
      else
	{
	  for(sl = listhead; sl; sl = sl->next)
	    {
	      strcpy(tmppath, sl->path);
	      if( tmppath[strlen(tmppath)-1] != '/' )
		{
		  strcat(tmppath, "/");
		}
	      /*
	       * Now append the base file name.
	       */
	      strcat(tmppath, pnt);
	      
	      status = stat(tmppath, &statbuf);
	      if( status != -1 )
		{
		  break;
		}
	    }
	  
	  if( sl == NULL )
	    {
	      /*
	       * Still couldn't find it.  Ask user for path to add.
	       */
	      fprintf(stderr,"Enter path to file %s: ", sourcefile);
	      fgets(tmppath, sizeof(tmppath), stdin);
	      
	      if( tmppath[strlen(tmppath)-1] == '\n' )
		{
		  tmppath[strlen(tmppath)-1] = '\0';
		}
	      
	      if( tmppath[strlen(tmppath)-1] != '/' )
		{
		  strcat(tmppath, "/");
		}
	      /*
	       * Now append the base file name.
	       */
	      strcat(tmppath, pnt);
	      
	      status = stat(tmppath, &statbuf);
	      if( status == -1 )
		{
		  /*
		   * OK, I guess the user doesn't really want to see it
		   * after all.
		   */
		  ol = (struct open_filelist *) xmalloc(sizeof(*ol));
		  ol->path = xstrdup(sourcefile);
		  ol->real_path = NULL;
		  ol->next = ofiles;
		  ol->nlines = 0;
		  ol->linelist = NULL;
		  ofiles = ol;
		  fprintf(stderr,"Unable to open file %s\n", tmppath);
		  return FALSE;
		}
	    }
	}
      /*
       * Create header for file.
       */
      ol = (struct open_filelist *) xmalloc(sizeof(*ol));
      ol->path = xstrdup(sourcefile);
      ol->real_path = xstrdup(tmppath);
      ol->next = ofiles;
      ol->nlines = 0;
      ol->linelist = NULL;
      ol->size = statbuf.st_size;
      ofiles = ol;

      /*
       * Now open and map the file.
       */
      fd = open(tmppath, O_RDONLY);
      if( fd == -1 )
	{
	  return FALSE;
	}

      addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      if( addr == (char *) -1 )
	{
	  return FALSE;
	}

      /*
       * Now build up the line number mapping table.
       */
      ol->nlines = 1;
      pnt = addr;
      while(pnt < addr + ol->size )
	{
	  if( *pnt++ == '\n' )
	    {
	      ol->nlines++;
	    }
	}

      ol->nlines++;
      ol->linelist = (unsigned int*) xmalloc(ol->nlines * sizeof(unsigned int) );

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
      /*
       * We know what the file is, we just need to reopen it and remap it.
       */
      fd = open(ol->real_path, O_RDONLY);
      if( fd == -1 )
	{
	  return FALSE;
	}
      
      addr = mmap(0, ol->size, PROT_READ, MAP_PRIVATE, fd, 0);
      if( addr == (char *) -1 )
	{
	  return FALSE;
	}
    }
  
  /*
   * All we need to do is to display the source lines here.
   */
  rtn = FALSE;
  for(i=start - 1; i <= end - 1; i++)
    {
      if( i < 0 || i >= ol->nlines - 1)
	{
	  continue;
	}

      rtn = TRUE;
      memset(&buffer, 0, sizeof(buffer));
      if( ol->linelist[i+1] != ol->linelist[i] )
	{
	  memcpy(&buffer, addr + ol->linelist[i], 
		 (ol->linelist[i+1] - ol->linelist[i]) - 1);
	}
      fprintf(stderr,"%d\t%s\n", i + 1,  buffer);
    }

  munmap(addr, ol->size);
  close(fd);

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
      fprintf(stderr, "Ambiguous source file specification.\n");
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
      fprintf(stderr, "No source file specified.\n");
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

DBG_ADDR DEBUG_LastDisassemble={NULL,0,0};

static int
_disassemble(DBG_ADDR *addr)
{
	DEBUG_PrintAddress( addr, dbg_mode, TRUE );
	fprintf(stderr,": ");
	if (!DBG_CHECK_READ_PTR( addr, 1 )) return 0;
	DEBUG_Disasm( addr, TRUE );
	fprintf(stderr,"\n");
	return 1;
}

void
_disassemble_fixaddr(DBG_ADDR *addr) {
    DWORD seg2;
    struct datatype *testtype;

    DBG_FIX_ADDR_SEG(addr,CS_reg(&DEBUG_context));
    if( addr->type != NULL )
      {
        if( addr->type == DEBUG_TypeIntConst )
          {
            /*
             * We know that we have the actual offset stored somewhere
             * else in 32-bit space.  Grab it, and we
             * should be all set.
             */
            seg2 = addr->seg;
            addr->seg = 0;
            addr->off = DEBUG_GetExprValue(addr, NULL);
            addr->seg = seg2;
          }
        else
          {
            if (!DBG_CHECK_READ_PTR( addr, 1 )) return;
            DEBUG_TypeDerefPointer(addr, &testtype);
            if( testtype != NULL || addr->type == DEBUG_TypeIntConst )
                addr->off = DEBUG_GetExprValue(addr, NULL);
          }
      }
    else if (!addr->seg && !addr->off)
    {
        fprintf(stderr,"Invalid expression\n");
        return;
    }
}

void
DEBUG_Disassemble(const DBG_ADDR *xstart,const DBG_ADDR *xend,int offset)
{
  int i;
  DBG_ADDR	last;
  DBG_ADDR	end,start;


  if (xstart) {
    start=*xstart;
    _disassemble_fixaddr(&start);
  }
  if (xend) {
    end=*xend;
    _disassemble_fixaddr(&end);
  }
  if (!xstart && !xend) {
    last = DEBUG_LastDisassemble;
    if (!last.seg && !last.off) {
      last.seg = (CS_reg(&DEBUG_context)==WINE_CODE_SELECTOR)?0:CS_reg(&DEBUG_context);
      last.off = EIP_reg(&DEBUG_context);
    }
    for (i=0;i<offset;i++)
      if (!_disassemble(&last)) break;
    memcpy(&DEBUG_LastDisassemble,&last,sizeof(last));
    return;
  }
  last = start;
  if (!xend) {
    for (i=0;i<offset;i++)
      if (!_disassemble(&last)) break;
    memcpy(&DEBUG_LastDisassemble,&last,sizeof(last));
    return;
  }
  while (last.off <= end.off)
    if (!_disassemble(&last)) break;
  memcpy(&DEBUG_LastDisassemble,&last,sizeof(last));
  return;
}



#if 0
main()
{
  int i, j;
  DEBUG_AddPath("../../de");
  while(1==1)
    {
      fscanf(stdin,"%d %d", &i, &j);
      DEBUG_DisplaySource("dumpexe.c", i, j);
    }
  return 0;
}
#endif
