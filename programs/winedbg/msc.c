/*
 * File msc.c - read VC++ debug information from COFF and eventually
 * from PDB files.
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
 * Note - this handles reading debug information for 32 bit applications
 * that run under Windows-NT for example.  I doubt that this would work well
 * for 16 bit applications, but I don't think it really matters since the
 * file format is different, and we should never get in here in such cases.
 *
 * TODO:
 *	Get 16 bit CV stuff working.
 *	Add symbol size to internal symbol table.
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include "wine/exception.h"
#include "excpt.h"
#include "debugger.h"

#define MAX_PATHNAME_LEN 1024

typedef struct
{
    DWORD  from;
    DWORD  to;

} OMAP_DATA;

typedef struct tagMSC_DBG_INFO
{
    int			  nsect;
    PIMAGE_SECTION_HEADER sectp;

    int			  nomap;
    OMAP_DATA *           omapp;

} MSC_DBG_INFO;

/*========================================================================
 * Debug file access helper routines
 */

static WINE_EXCEPTION_FILTER(page_fault)
{
      if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
                return EXCEPTION_EXECUTE_HANDLER;
          return EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           DEBUG_LocateDebugInfoFile
 *
 * NOTE: dbg_filename must be at least MAX_PATHNAME_LEN bytes in size
 */
static void DEBUG_LocateDebugInfoFile(const char *filename, char *dbg_filename)
{
    char	  *str1 = DBG_alloc(MAX_PATHNAME_LEN);
    char	  *str2 = DBG_alloc(MAX_PATHNAME_LEN*10);
    const char	  *file;
    char	  *name_part;

    file = strrchr(filename, '\\');
    if( file == NULL ) file = filename; else file++;

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
static void*	DEBUG_MapDebugInfoFile(const char* name, DWORD offset, DWORD size,
				       HANDLE* hFile, HANDLE* hMap)
{
    DWORD	g_offset;	/* offset aligned on map granuality */
    DWORD	g_size;		/* size to map, with offset aligned */
    char*	ret;

    *hMap = 0;

    if (name != NULL) {
       char 	filename[MAX_PATHNAME_LEN];

       DEBUG_LocateDebugInfoFile(name, filename);
       if ((*hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	  return NULL;
    }

    if (!size) {
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
static void	DEBUG_UnmapDebugInfoFile(HANDLE hFile, HANDLE hMap, void* addr)
{
   if (addr) UnmapViewOfFile(addr);
   if (hMap) CloseHandle(hMap);
   if (hFile!=INVALID_HANDLE_VALUE) CloseHandle(hFile);
}



/*========================================================================
 * Process COFF debug information.
 */

struct CoffFile
{
    unsigned int       startaddr;
    unsigned int       endaddr;
    const char        *filename;
    int                linetab_offset;
    int                linecnt;
    struct name_hash **entries;
    int		       neps;
    int		       neps_alloc;
};

struct CoffFileSet
{
  struct CoffFile     *files;
  int		       nfiles;
  int		       nfiles_alloc;
};

static const char*	DEBUG_GetCoffName( PIMAGE_SYMBOL coff_sym, const char* coff_strtab )
{
   static	char	namebuff[9];
   const char*		nampnt;

   if( coff_sym->N.Name.Short )
      {
	 memcpy(namebuff, coff_sym->N.ShortName, 8);
	 namebuff[8] = '\0';
	 nampnt = &namebuff[0];
      }
   else
      {
	 nampnt = coff_strtab + coff_sym->N.Name.Long;
      }

   if( nampnt[0] == '_' )
      nampnt++;
   return nampnt;
}

static int DEBUG_AddCoffFile( struct CoffFileSet* coff_files, const char* filename )
{
   struct CoffFile* file;

   if( coff_files->nfiles + 1 >= coff_files->nfiles_alloc )
     {
	coff_files->nfiles_alloc += 10;
	coff_files->files = (struct CoffFile *) DBG_realloc(coff_files->files,
							    coff_files->nfiles_alloc * sizeof(struct CoffFile));
     }
   file = coff_files->files + coff_files->nfiles;
   file->startaddr = 0xffffffff;
   file->endaddr   = 0;
   file->filename = filename;
   file->linetab_offset = -1;
   file->linecnt = 0;
   file->entries = NULL;
   file->neps = file->neps_alloc = 0;

   return coff_files->nfiles++;
}

static void DEBUG_AddCoffSymbol( struct CoffFile* coff_file, struct name_hash* sym )
{
   if( coff_file->neps + 1 >= coff_file->neps_alloc )
      {
	 coff_file->neps_alloc += 10;
	 coff_file->entries = (struct name_hash **)
	    DBG_realloc(coff_file->entries,
			coff_file->neps_alloc * sizeof(struct name_hash *));
      }
   coff_file->entries[coff_file->neps++] = sym;
}

static enum DbgInfoLoad DEBUG_ProcessCoff( DBG_MODULE *module, LPBYTE root )
{
  PIMAGE_AUX_SYMBOL		aux;
  PIMAGE_COFF_SYMBOLS_HEADER	coff;
  PIMAGE_LINENUMBER		coff_linetab;
  PIMAGE_LINENUMBER		linepnt;
  char		     	      * coff_strtab;
  PIMAGE_SYMBOL			coff_sym;
  PIMAGE_SYMBOL			coff_symbols;
  struct CoffFileSet	        coff_files;
  int				curr_file_idx = -1;
  unsigned int		       	i;
  int			       	j;
  int			       	k;
  int			       	l;
  int		       		linetab_indx;
  const char	     	      * nampnt;
  int		       		naux;
  DBG_VALUE	       		new_value;
  enum DbgInfoLoad		dil = DIL_ERROR;

  DEBUG_Printf(DBG_CHN_TRACE, "Processing COFF symbols...\n");

  assert(sizeof(IMAGE_SYMBOL) == IMAGE_SIZEOF_SYMBOL);
  assert(sizeof(IMAGE_LINENUMBER) == IMAGE_SIZEOF_LINENUMBER);

  coff_files.files = NULL;
  coff_files.nfiles = coff_files.nfiles_alloc = 0;

  coff = (PIMAGE_COFF_SYMBOLS_HEADER) root;

  coff_symbols = (PIMAGE_SYMBOL) ((unsigned int) coff + coff->LvaToFirstSymbol);
  coff_linetab = (PIMAGE_LINENUMBER) ((unsigned int) coff + coff->LvaToFirstLinenumber);
  coff_strtab = (char *) (coff_symbols + coff->NumberOfSymbols);

  linetab_indx = 0;

  new_value.cookie = DV_TARGET;
  new_value.type = NULL;

  for(i=0; i < coff->NumberOfSymbols; i++ )
    {
      coff_sym = coff_symbols + i;
      naux = coff_sym->NumberOfAuxSymbols;

      if( coff_sym->StorageClass == IMAGE_SYM_CLASS_FILE )
	{
	  curr_file_idx = DEBUG_AddCoffFile( &coff_files, (char *) (coff_sym + 1) );
	  DEBUG_Printf(DBG_CHN_TRACE,"New file %s\n", coff_files.files[curr_file_idx].filename);
	  i += naux;
	  continue;
	}

      if (curr_file_idx < 0) {
	  assert(coff_files.nfiles == 0 && coff_files.nfiles_alloc == 0);
	  curr_file_idx = DEBUG_AddCoffFile( &coff_files, "<none>" );
	  DEBUG_Printf(DBG_CHN_TRACE,"New file %s\n", coff_files.files[curr_file_idx].filename);
      }

      /*
       * This guy marks the size and location of the text section
       * for the current file.  We need to keep track of this so
       * we can figure out what file the different global functions
       * go with.
       */
      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC)
	  && (naux != 0)
	  && (coff_sym->Type == 0)
	  && (coff_sym->SectionNumber == 1) )
	{
	  aux = (PIMAGE_AUX_SYMBOL) (coff_sym + 1);

	  if( coff_files.files[curr_file_idx].linetab_offset != -1 )
	    {
	      /*
	       * Save this so we can still get the old name.
	       */
	      const char* fn = coff_files.files[curr_file_idx].filename;

#ifdef MORE_DBG
	      DEBUG_Printf(DBG_CHN_TRACE, "Duplicating sect from %s: %lx %x %x %d %d\n",
			   coff_files.files[curr_file_idx].filename,
			   aux->Section.Length,
			   aux->Section.NumberOfRelocations,
			   aux->Section.NumberOfLinenumbers,
			   aux->Section.Number,
			   aux->Section.Selection);
	      DEBUG_Printf(DBG_CHN_TRACE, "More sect %d %s %08lx %d %d %d\n",
			   coff_sym->SectionNumber,
			   DEBUG_GetCoffName( coff_sym, coff_strtab ),
			   coff_sym->Value,
			   coff_sym->Type,
			   coff_sym->StorageClass,
			   coff_sym->NumberOfAuxSymbols);
#endif

	      /*
	       * Duplicate the file entry.  We have no way to describe
	       * multiple text sections in our current way of handling things.
	       */
	      DEBUG_AddCoffFile( &coff_files, fn );
	    }
#ifdef MORE_DBG
	  else
	    {
	      DEBUG_Printf(DBG_CHN_TRACE, "New text sect from %s: %lx %x %x %d %d\n",
			   coff_files.files[curr_file_idx].filename,
			   aux->Section.Length,
			   aux->Section.NumberOfRelocations,
			   aux->Section.NumberOfLinenumbers,
			   aux->Section.Number,
			   aux->Section.Selection);
	    }
#endif

	  if( coff_files.files[curr_file_idx].startaddr > coff_sym->Value )
	    {
	      coff_files.files[curr_file_idx].startaddr = coff_sym->Value;
	    }

	  if( coff_files.files[curr_file_idx].endaddr < coff_sym->Value + aux->Section.Length )
	    {
	      coff_files.files[curr_file_idx].endaddr = coff_sym->Value + aux->Section.Length;
	    }

	  coff_files.files[curr_file_idx].linetab_offset = linetab_indx;
	  coff_files.files[curr_file_idx].linecnt = aux->Section.NumberOfLinenumbers;
	  linetab_indx += aux->Section.NumberOfLinenumbers;
	  i += naux;
	  continue;
	}

      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC)
	  && (naux == 0)
	  && (coff_sym->SectionNumber == 1) )
	{
	  DWORD	base = module->msc_info->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
	  /*
	   * This is a normal static function when naux == 0.
	   * Just register it.  The current file is the correct
	   * one in this instance.
	   */
	  nampnt = DEBUG_GetCoffName( coff_sym, coff_strtab );

	  new_value.addr.seg = 0;
	  new_value.addr.off = (int) ((char *)module->load_addr + base + coff_sym->Value);

#ifdef MORE_DBG
	  DEBUG_Printf(DBG_CHN_TRACE,"\tAdding static symbol %s\n", nampnt);
#endif

	  /* FIXME: was adding symbol to this_file ??? */
	  DEBUG_AddCoffSymbol( &coff_files.files[curr_file_idx],
			       DEBUG_AddSymbol( nampnt, &new_value,
						coff_files.files[curr_file_idx].filename,
						SYM_WIN32 | SYM_FUNC ) );
	  i += naux;
	  continue;
	}

      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL)
	  && ISFCN(coff_sym->Type)
          && (coff_sym->SectionNumber > 0) )
	{
	  const char* this_file = NULL;
	  DWORD	base = module->msc_info->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
	  nampnt = DEBUG_GetCoffName( coff_sym, coff_strtab );

	  new_value.addr.seg = 0;
	  new_value.addr.off = (int) ((char *)module->load_addr + base + coff_sym->Value);

#ifdef MORE_DBG
	  DEBUG_Printf(DBG_CHN_TRACE, "%d: %lx %s\n", i, new_value.addr.off, nampnt);

	  DEBUG_Printf(DBG_CHN_TRACE,"\tAdding global symbol %s (sect=%s)\n",
		       nampnt, MSC_INFO(module)->sectp[coff_sym->SectionNumber - 1].Name);
#endif

	  /*
	   * Now we need to figure out which file this guy belongs to.
	   */
	  for(j=0; j < coff_files.nfiles; j++)
	    {
	      if( coff_files.files[j].startaddr <= base + coff_sym->Value
		  && coff_files.files[j].endaddr > base + coff_sym->Value )
		{
		  this_file = coff_files.files[j].filename;
		  break;
		}
	    }
	  if (j < coff_files.nfiles) {
	     DEBUG_AddCoffSymbol( &coff_files.files[j],
				  DEBUG_AddSymbol( nampnt, &new_value, this_file, SYM_WIN32 | SYM_FUNC ) );
	  } else {
	     DEBUG_AddSymbol( nampnt, &new_value, NULL, SYM_WIN32 | SYM_FUNC );
	  }
	  i += naux;
	  continue;
	}

      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL)
          && (coff_sym->SectionNumber > 0) )
	{
	  DWORD	base = module->msc_info->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
	  /*
	   * Similar to above, but for the case of data symbols.
	   * These aren't treated as entrypoints.
	   */
	  nampnt = DEBUG_GetCoffName( coff_sym, coff_strtab );

	  new_value.addr.seg = 0;
	  new_value.addr.off = (int) ((char *)module->load_addr + base + coff_sym->Value);

#ifdef MORE_DBG
	  DEBUG_Printf(DBG_CHN_TRACE, "%d: %lx %s\n", i, new_value.addr.off, nampnt);

	  DEBUG_Printf(DBG_CHN_TRACE,"\tAdding global data symbol %s\n", nampnt);
#endif

	  /*
	   * Now we need to figure out which file this guy belongs to.
	   */
	  DEBUG_AddSymbol( nampnt, &new_value, NULL, SYM_WIN32 | SYM_DATA );
	  i += naux;
	  continue;
	}

      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC)
	  && (naux == 0) )
	{
	  /*
	   * Ignore these.  They don't have anything to do with
	   * reality.
	   */
	  i += naux;
	  continue;
	}

#ifdef MORE_DBG
      DEBUG_Printf(DBG_CHN_TRACE,"Skipping unknown entry '%s' %d %d %d\n",
		   DEBUG_GetCoffName( coff_sym, coff_strtab ),
		   coff_sym->StorageClass, coff_sym->SectionNumber, naux);
#endif

      /*
       * For now, skip past the aux entries.
       */
      i += naux;

    }

  /*
   * OK, we now should have a list of files, and we should have a list
   * of entrypoints.  We need to sort the entrypoints so that we are
   * able to tie the line numbers with the given functions within the
   * file.
   */
  if( coff_files.files != NULL )
    {
      for(j=0; j < coff_files.nfiles; j++)
	{
	  if( coff_files.files[j].entries != NULL )
	    {
	      qsort(coff_files.files[j].entries, coff_files.files[j].neps,
		    sizeof(struct name_hash *), DEBUG_cmp_sym);
	    }
	}

      /*
       * Now pick apart the line number tables, and attach the entries
       * to the given functions.
       */
      for(j=0; j < coff_files.nfiles; j++)
	{
	  l = 0;
	  if( coff_files.files[j].neps != 0 )
	    for(k=0; k < coff_files.files[j].linecnt; k++)
	    {
	      linepnt = coff_linetab + coff_files.files[j].linetab_offset + k;
	      /*
	       * If we have spilled onto the next entrypoint, then
	       * bump the counter..
	       */
	      while(TRUE)
		{
		  if (l+1 >= coff_files.files[j].neps) break;
		  DEBUG_GetSymbolAddr(coff_files.files[j].entries[l+1], &new_value.addr);
		  if( (((unsigned int)module->load_addr +
		        linepnt->Type.VirtualAddress) >= new_value.addr.off) )
		  {
		      l++;
		  } else break;
		}

	      /*
	       * Add the line number.  This is always relative to the
	       * start of the function, so we need to subtract that offset
	       * first.
	       */
	      DEBUG_GetSymbolAddr(coff_files.files[j].entries[l], &new_value.addr);
	      DEBUG_AddLineNumber(coff_files.files[j].entries[l],
				  linepnt->Linenumber,
				  (unsigned int) module->load_addr
				  + linepnt->Type.VirtualAddress
				  - new_value.addr.off);
	    }
	}
    }

  dil = DIL_LOADED;

  if( coff_files.files != NULL )
    {
      for(j=0; j < coff_files.nfiles; j++)
	{
	  if( coff_files.files[j].entries != NULL )
	    {
	      DBG_free(coff_files.files[j].entries);
	    }
	}
      DBG_free(coff_files.files);
    }

  return dil;

}



/*========================================================================
 * Process CodeView type information.
 */

union codeview_type
{
  struct
  {
    unsigned short int  len;
    short int           id;
  } generic;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          attribute;
    short int          datatype;
    unsigned char      variant[1];
  } pointer;

  struct
  {
    unsigned short int len;
    short int          id;
    unsigned int       datatype;
    unsigned int       attribute;
    unsigned char      variant[1];
  } pointer32;

  struct
  {
    unsigned short int len;
    short int          id;
    unsigned char      nbits;
    unsigned char      bitoff;
    unsigned short     type;
  } bitfield;

  struct
  {
    unsigned short int len;
    short int          id;
    unsigned int       type;
    unsigned char      nbits;
    unsigned char      bitoff;
  } bitfield32;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          elemtype;
    short int          idxtype;
    unsigned short int arrlen;     /* numeric leaf */
#if 0
    unsigned char      name[1];
#endif
  } array;

  struct
  {
    unsigned short int len;
    short int          id;
    unsigned int       elemtype;
    unsigned int       idxtype;
    unsigned short int arrlen;    /* numeric leaf */
#if 0
    unsigned char      name[1];
#endif
  } array32;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          n_element;
    short int          fieldlist;
    short int          property;
    short int          derived;
    short int          vshape;
    unsigned short int structlen;  /* numeric leaf */
#if 0
    unsigned char      name[1];
#endif
  } structure;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          n_element;
    short int          property;
    unsigned int       fieldlist;
    unsigned int       derived;
    unsigned int       vshape;
    unsigned short int structlen;  /* numeric leaf */
#if 0
    unsigned char      name[1];
#endif
  } structure32;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          count;
    short int          fieldlist;
    short int          property;
    unsigned short int un_len;     /* numeric leaf */
#if 0
    unsigned char      name[1];
#endif
  } t_union;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          count;
    short int          property;
    unsigned int       fieldlist;
    unsigned short int un_len;     /* numeric leaf */
#if 0
    unsigned char      name[1];
#endif
  } t_union32;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          count;
    short int          type;
    short int          field;
    short int          property;
    unsigned char      name[1];
  } enumeration;

  struct
  {
    unsigned short int len;
    short int          id;
    short int          count;
    short int          property;
    unsigned int       type;
    unsigned int       field;
    unsigned char      name[1];
  } enumeration32;

  struct
  {
    unsigned short int len;
    short int          id;
    unsigned char      list[1];
  } fieldlist;
};

union codeview_fieldtype
{
  struct
  {
    short int		id;
  } generic;

  struct
  {
    short int		id;
    short int		type;
    short int		attribute;
    unsigned short int	offset;     /* numeric leaf */
  } bclass;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	type;
    unsigned short int	offset;     /* numeric leaf */
  } bclass32;

  struct
  {
    short int		id;
    short int		btype;
    short int		vbtype;
    short int		attribute;
    unsigned short int	vbpoff;     /* numeric leaf */
#if 0
    unsigned short int	vboff;      /* numeric leaf */
#endif
  } vbclass;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	btype;
    unsigned int	vbtype;
    unsigned short int	vbpoff;     /* numeric leaf */
#if 0
    unsigned short int	vboff;      /* numeric leaf */
#endif
  } vbclass32;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned short int	value;     /* numeric leaf */
#if 0
    unsigned char	name[1];
#endif
  } enumerate;

  struct
  {
    short int		id;
    short int		type;
    unsigned char	name[1];
  } friendfcn;

  struct
  {
    short int		id;
    short int		_pad0;
    unsigned int	type;
    unsigned char	name[1];
  } friendfcn32;

  struct
  {
    short int		id;
    short int		type;
    short int		attribute;
    unsigned short int	offset;    /* numeric leaf */
#if 0
    unsigned char	name[1];
#endif
  } member;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	type;
    unsigned short int	offset;    /* numeric leaf */
#if 0
    unsigned char	name[1];
#endif
  } member32;

  struct
  {
    short int		id;
    short int		type;
    short int		attribute;
    unsigned char	name[1];
  } stmember;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	type;
    unsigned char	name[1];
  } stmember32;

  struct
  {
    short int		id;
    short int		count;
    short int		mlist;
    unsigned char	name[1];
  } method;

  struct
  {
    short int		id;
    short int		count;
    unsigned int	mlist;
    unsigned char	name[1];
  } method32;

  struct
  {
    short int		id;
    short int		index;
    unsigned char	name[1];
  } nesttype;

  struct
  {
    short int		id;
    short int		_pad0;
    unsigned int	index;
    unsigned char	name[1];
  } nesttype32;

  struct
  {
    short int		id;
    short int		type;
  } vfunctab;

  struct
  {
    short int		id;
    short int		_pad0;
    unsigned int	type;
  } vfunctab32;

  struct
  {
    short int		id;
    short int		type;
  } friendcls;

  struct
  {
    short int		id;
    short int		_pad0;
    unsigned int	type;
  } friendcls32;


  struct
  {
    short int		id;
    short int		attribute;
    short int		type;
    unsigned char	name[1];
  } onemethod;
  struct
  {
    short int		id;
    short int		attribute;
    short int		type;
    unsigned int	vtab_offset;
    unsigned char	name[1];
  } onemethod_virt;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int 	type;
    unsigned char	name[1];
  } onemethod32;
  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	type;
    unsigned int	vtab_offset;
    unsigned char	name[1];
  } onemethod32_virt;

  struct
  {
    short int		id;
    short int		type;
    unsigned int	offset;
  } vfuncoff;

  struct
  {
    short int		id;
    short int		_pad0;
    unsigned int	type;
    unsigned int	offset;
  } vfuncoff32;

  struct
  {
    short int		id;
    short int		attribute;
    short int		index;
    unsigned char	name[1];
  } nesttypeex;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	index;
    unsigned char	name[1];
  } nesttypeex32;

  struct
  {
    short int		id;
    short int		attribute;
    unsigned int	type;
    unsigned char	name[1];
  } membermodify;
};


/*
 * This covers the basic datatypes that VC++ seems to be using these days.
 * 32 bit mode only.  There are additional numbers for the pointers in 16
 * bit mode.  There are many other types listed in the documents, but these
 * are apparently not used by the compiler, or represent pointer types
 * that are not used.
 */
#define T_NOTYPE	0x0000	/* Notype */
#define T_ABS		0x0001	/* Abs */
#define T_VOID		0x0003	/* Void */
#define T_CHAR		0x0010	/* signed char */
#define T_SHORT		0x0011	/* short */
#define T_LONG		0x0012	/* long */
#define T_QUAD		0x0013	/* long long */
#define T_UCHAR		0x0020	/* unsigned  char */
#define T_USHORT	0x0021	/* unsigned short */
#define T_ULONG		0x0022	/* unsigned long */
#define T_UQUAD		0x0023	/* unsigned long long */
#define T_REAL32	0x0040	/* float */
#define T_REAL64	0x0041	/* double */
#define T_RCHAR		0x0070	/* real char */
#define T_WCHAR		0x0071	/* wide char */
#define T_INT4		0x0074	/* int */
#define T_UINT4		0x0075	/* unsigned int */

#define T_32PVOID	0x0403	/* 32 bit near pointer to void */
#define T_32PCHAR	0x0410  /* 16:32 near pointer to signed char */
#define T_32PSHORT	0x0411  /* 16:32 near pointer to short */
#define T_32PLONG	0x0412  /* 16:32 near pointer to int */
#define T_32PQUAD	0x0413  /* 16:32 near pointer to long long */
#define T_32PUCHAR	0x0420  /* 16:32 near pointer to unsigned char */
#define T_32PUSHORT	0x0421  /* 16:32 near pointer to unsigned short */
#define T_32PULONG	0x0422	/* 16:32 near pointer to unsigned int */
#define T_32PUQUAD	0x0423  /* 16:32 near pointer to long long */
#define T_32PREAL32	0x0440	/* 16:32 near pointer to float */
#define T_32PREAL64	0x0441	/* 16:32 near pointer to float */
#define T_32PRCHAR	0x0470	/* 16:32 near pointer to real char */
#define T_32PWCHAR	0x0471	/* 16:32 near pointer to real char */
#define T_32PINT4	0x0474	/* 16:32 near pointer to int */
#define T_32PUINT4	0x0475  /* 16:32 near pointer to unsigned int */


#define LF_MODIFIER     0x0001
#define LF_POINTER      0x0002
#define LF_ARRAY        0x0003
#define LF_CLASS        0x0004
#define LF_STRUCTURE    0x0005
#define LF_UNION        0x0006
#define LF_ENUM         0x0007
#define LF_PROCEDURE    0x0008
#define LF_MFUNCTION    0x0009
#define LF_VTSHAPE      0x000a
#define LF_COBOL0       0x000b
#define LF_COBOL1       0x000c
#define LF_BARRAY       0x000d
#define LF_LABEL        0x000e
#define LF_NULL         0x000f
#define LF_NOTTRAN      0x0010
#define LF_DIMARRAY     0x0011
#define LF_VFTPATH      0x0012
#define LF_PRECOMP      0x0013
#define LF_ENDPRECOMP   0x0014
#define LF_OEM          0x0015
#define LF_TYPESERVER   0x0016

#define LF_MODIFIER_32  0x1001     /* variants with new 32-bit type indices */
#define LF_POINTER_32   0x1002
#define LF_ARRAY_32     0x1003
#define LF_CLASS_32     0x1004
#define LF_STRUCTURE_32 0x1005
#define LF_UNION_32     0x1006
#define LF_ENUM_32      0x1007
#define LF_PROCEDURE_32 0x1008
#define LF_MFUNCTION_32 0x1009
#define LF_COBOL0_32    0x100a
#define LF_BARRAY_32    0x100b
#define LF_DIMARRAY_32  0x100c
#define LF_VFTPATH_32   0x100d
#define LF_PRECOMP_32   0x100e
#define LF_OEM_32       0x100f

#define LF_SKIP         0x0200
#define LF_ARGLIST      0x0201
#define LF_DEFARG       0x0202
#define LF_LIST         0x0203
#define LF_FIELDLIST    0x0204
#define LF_DERIVED      0x0205
#define LF_BITFIELD     0x0206
#define LF_METHODLIST   0x0207
#define LF_DIMCONU      0x0208
#define LF_DIMCONLU     0x0209
#define LF_DIMVARU      0x020a
#define LF_DIMVARLU     0x020b
#define LF_REFSYM       0x020c

#define LF_SKIP_32      0x1200    /* variants with new 32-bit type indices */
#define LF_ARGLIST_32   0x1201
#define LF_DEFARG_32    0x1202
#define LF_FIELDLIST_32 0x1203
#define LF_DERIVED_32   0x1204
#define LF_BITFIELD_32  0x1205
#define LF_METHODLIST_32 0x1206
#define LF_DIMCONU_32   0x1207
#define LF_DIMCONLU_32  0x1208
#define LF_DIMVARU_32   0x1209
#define LF_DIMVARLU_32  0x120a

#define LF_BCLASS       0x0400
#define LF_VBCLASS      0x0401
#define LF_IVBCLASS     0x0402
#define LF_ENUMERATE    0x0403
#define LF_FRIENDFCN    0x0404
#define LF_INDEX        0x0405
#define LF_MEMBER       0x0406
#define LF_STMEMBER     0x0407
#define LF_METHOD       0x0408
#define LF_NESTTYPE     0x0409
#define LF_VFUNCTAB     0x040a
#define LF_FRIENDCLS    0x040b
#define LF_ONEMETHOD    0x040c
#define LF_VFUNCOFF     0x040d
#define LF_NESTTYPEEX   0x040e
#define LF_MEMBERMODIFY 0x040f

#define LF_BCLASS_32    0x1400    /* variants with new 32-bit type indices */
#define LF_VBCLASS_32   0x1401
#define LF_IVBCLASS_32  0x1402
#define LF_FRIENDFCN_32 0x1403
#define LF_INDEX_32     0x1404
#define LF_MEMBER_32    0x1405
#define LF_STMEMBER_32  0x1406
#define LF_METHOD_32    0x1407
#define LF_NESTTYPE_32  0x1408
#define LF_VFUNCTAB_32  0x1409
#define LF_FRIENDCLS_32 0x140a
#define LF_ONEMETHOD_32 0x140b
#define LF_VFUNCOFF_32  0x140c
#define LF_NESTTYPEEX_32 0x140d

#define LF_NUMERIC      0x8000    /* numeric leaf types */
#define LF_CHAR         0x8000
#define LF_SHORT        0x8001
#define LF_USHORT       0x8002
#define LF_LONG         0x8003
#define LF_ULONG        0x8004
#define LF_REAL32       0x8005
#define LF_REAL64       0x8006
#define LF_REAL80       0x8007
#define LF_REAL128      0x8008
#define LF_QUADWORD     0x8009
#define LF_UQUADWORD    0x800a
#define LF_REAL48       0x800b
#define LF_COMPLEX32    0x800c
#define LF_COMPLEX64    0x800d
#define LF_COMPLEX80    0x800e
#define LF_COMPLEX128   0x800f
#define LF_VARSTRING    0x8010



#define MAX_BUILTIN_TYPES	0x480
static struct datatype * cv_basic_types[MAX_BUILTIN_TYPES];
static unsigned int num_cv_defined_types = 0;
static struct datatype **cv_defined_types = NULL;

void
DEBUG_InitCVDataTypes(void)
{
  /*
   * These are the common builtin types that are used by VC++.
   */
  cv_basic_types[T_NOTYPE] = NULL;
  cv_basic_types[T_ABS] = NULL;
  cv_basic_types[T_VOID] = DEBUG_GetBasicType(DT_BASIC_VOID);
  cv_basic_types[T_CHAR] = DEBUG_GetBasicType(DT_BASIC_CHAR);
  cv_basic_types[T_SHORT] = DEBUG_GetBasicType(DT_BASIC_SHORTINT);
  cv_basic_types[T_LONG] = DEBUG_GetBasicType(DT_BASIC_LONGINT);
  cv_basic_types[T_QUAD] = DEBUG_GetBasicType(DT_BASIC_LONGLONGINT);
  cv_basic_types[T_UCHAR] = DEBUG_GetBasicType(DT_BASIC_UCHAR);
  cv_basic_types[T_USHORT] = DEBUG_GetBasicType(DT_BASIC_USHORTINT);
  cv_basic_types[T_ULONG] = DEBUG_GetBasicType(DT_BASIC_ULONGINT);
  cv_basic_types[T_UQUAD] = DEBUG_GetBasicType(DT_BASIC_ULONGLONGINT);
  cv_basic_types[T_REAL32] = DEBUG_GetBasicType(DT_BASIC_FLOAT);
  cv_basic_types[T_REAL64] = DEBUG_GetBasicType(DT_BASIC_DOUBLE);
  cv_basic_types[T_RCHAR] = DEBUG_GetBasicType(DT_BASIC_CHAR);
  cv_basic_types[T_WCHAR] = DEBUG_GetBasicType(DT_BASIC_SHORTINT);
  cv_basic_types[T_INT4] = DEBUG_GetBasicType(DT_BASIC_INT);
  cv_basic_types[T_UINT4] = DEBUG_GetBasicType(DT_BASIC_UINT);

  cv_basic_types[T_32PVOID] = DEBUG_FindOrMakePointerType(cv_basic_types[T_VOID]);
  cv_basic_types[T_32PCHAR] = DEBUG_FindOrMakePointerType(cv_basic_types[T_CHAR]);
  cv_basic_types[T_32PSHORT] = DEBUG_FindOrMakePointerType(cv_basic_types[T_SHORT]);
  cv_basic_types[T_32PLONG] = DEBUG_FindOrMakePointerType(cv_basic_types[T_LONG]);
  cv_basic_types[T_32PQUAD] = DEBUG_FindOrMakePointerType(cv_basic_types[T_QUAD]);
  cv_basic_types[T_32PUCHAR] = DEBUG_FindOrMakePointerType(cv_basic_types[T_UCHAR]);
  cv_basic_types[T_32PUSHORT] = DEBUG_FindOrMakePointerType(cv_basic_types[T_USHORT]);
  cv_basic_types[T_32PULONG] = DEBUG_FindOrMakePointerType(cv_basic_types[T_ULONG]);
  cv_basic_types[T_32PUQUAD] = DEBUG_FindOrMakePointerType(cv_basic_types[T_UQUAD]);
  cv_basic_types[T_32PREAL32] = DEBUG_FindOrMakePointerType(cv_basic_types[T_REAL32]);
  cv_basic_types[T_32PREAL64] = DEBUG_FindOrMakePointerType(cv_basic_types[T_REAL64]);
  cv_basic_types[T_32PRCHAR] = DEBUG_FindOrMakePointerType(cv_basic_types[T_RCHAR]);
  cv_basic_types[T_32PWCHAR] = DEBUG_FindOrMakePointerType(cv_basic_types[T_WCHAR]);
  cv_basic_types[T_32PINT4] = DEBUG_FindOrMakePointerType(cv_basic_types[T_INT4]);
  cv_basic_types[T_32PUINT4] = DEBUG_FindOrMakePointerType(cv_basic_types[T_UINT4]);
}


static int
numeric_leaf( int *value, unsigned short int *leaf )
{
    unsigned short int type = *leaf++;
    int length = 2;

    if ( type < LF_NUMERIC )
    {
        *value = type;
    }
    else
    {
        switch ( type )
        {
        case LF_CHAR:
            length += 1;
            *value = *(char *)leaf;
            break;

        case LF_SHORT:
            length += 2;
            *value = *(short *)leaf;
            break;

        case LF_USHORT:
            length += 2;
            *value = *(unsigned short *)leaf;
            break;

        case LF_LONG:
            length += 4;
            *value = *(int *)leaf;
            break;

        case LF_ULONG:
            length += 4;
            *value = *(unsigned int *)leaf;
            break;

        case LF_QUADWORD:
        case LF_UQUADWORD:
            length += 8;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL32:
            length += 4;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL48:
            length += 6;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL64:
            length += 8;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL80:
            length += 10;
            *value = 0;    /* FIXME */
            break;

        case LF_REAL128:
            length += 16;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX32:
            length += 4;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX64:
            length += 8;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX80:
            length += 10;
            *value = 0;    /* FIXME */
            break;

        case LF_COMPLEX128:
            length += 16;
            *value = 0;    /* FIXME */
            break;

        case LF_VARSTRING:
            length += 2 + *leaf;
            *value = 0;    /* FIXME */
            break;

        default:
	    DEBUG_Printf( DBG_CHN_MESG, "Unknown numeric leaf type %04x\n", type );
            *value = 0;
            break;
        }
    }

    return length;
}

static char *
terminate_string( unsigned char *name )
{
    static char symname[256];

    int namelen = name[0];
    assert( namelen >= 0 && namelen < 256 );

    memcpy( symname, name+1, namelen );
    symname[namelen] = '\0';

    if ( !*symname || strcmp( symname, "__unnamed" ) == 0 )
        return NULL;
    else
        return symname;
}

static
struct datatype * DEBUG_GetCVType(unsigned int typeno)
{
    struct datatype * dt = NULL;

    /*
     * Convert Codeview type numbers into something we can grok internally.
     * Numbers < 0x1000 are all fixed builtin types.  Numbers from 0x1000 and
     * up are all user defined (structs, etc).
     */
    if ( typeno < 0x1000 )
    {
        if ( typeno < MAX_BUILTIN_TYPES )
	    dt = cv_basic_types[typeno];
    }
    else
    {
        if ( typeno - 0x1000 < num_cv_defined_types )
	    dt = cv_defined_types[typeno - 0x1000];
    }

    return dt;
}

static int
DEBUG_AddCVType( unsigned int typeno, struct datatype *dt )
{
    while ( typeno - 0x1000 >= num_cv_defined_types )
    {
        num_cv_defined_types += 0x100;
        cv_defined_types = (struct datatype **)
            DBG_realloc( cv_defined_types,
		         num_cv_defined_types * sizeof(struct datatype *) );

        memset( cv_defined_types + num_cv_defined_types - 0x100,
		0,
		0x100 * sizeof(struct datatype *) );

        if ( cv_defined_types == NULL )
            return FALSE;
    }

    cv_defined_types[ typeno - 0x1000 ] = dt;
    return TRUE;
}

static void
DEBUG_ClearTypeTable( void )
{
    if ( cv_defined_types )
        DBG_free( cv_defined_types );

    cv_defined_types = NULL;
    num_cv_defined_types = 0;
}

static int
DEBUG_AddCVType_Pointer( unsigned int typeno, unsigned int datatype )
{
    struct datatype *dt =
	    DEBUG_FindOrMakePointerType( DEBUG_GetCVType( datatype ) );

    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_AddCVType_Array( unsigned int typeno, char *name,
                       unsigned int elemtype, unsigned int arr_len )
{
    struct datatype *dt    = DEBUG_NewDataType( DT_ARRAY, name );
    struct datatype *elem  = DEBUG_GetCVType( elemtype );
    unsigned int elem_size = elem? DEBUG_GetObjectSize( elem ) : 0;
    unsigned int arr_max   = elem_size? arr_len / elem_size : 0;

    DEBUG_SetArrayParams( dt, 0, arr_max, elem );
    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_AddCVType_Bitfield( unsigned int typeno,
                          unsigned int bitoff, unsigned int nbits,
                          unsigned int basetype )
{
    struct datatype *dt   = DEBUG_NewDataType( DT_BITFIELD, NULL );
    struct datatype *base = DEBUG_GetCVType( basetype );

    DEBUG_SetBitfieldParams( dt, bitoff, nbits, base );
    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_AddCVType_EnumFieldList( unsigned int typeno, unsigned char *list, int len )
{
    struct datatype *dt = DEBUG_NewDataType( DT_ENUM, NULL );
    unsigned char *ptr = list;

    while ( ptr - list < len )
    {
        union codeview_fieldtype *type = (union codeview_fieldtype *)ptr;

        if ( *ptr >= 0xf0 )       /* LF_PAD... */
        {
            ptr += *ptr & 0x0f;
            continue;
        }

        switch ( type->generic.id )
        {
        case LF_ENUMERATE:
        {
            int value, vlen = numeric_leaf( &value, &type->enumerate.value );
            unsigned char *name = (unsigned char *)&type->enumerate.value + vlen;

            DEBUG_AddStructElement( dt, terminate_string( name ),
                                        NULL, value, 0 );

            ptr += 2 + 2 + vlen + (1 + name[0]);
            break;
        }

        default:
            DEBUG_Printf( DBG_CHN_MESG, "Unhandled type %04x in ENUM field list\n",
                                         type->generic.id );
            return FALSE;
        }
    }

    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_AddCVType_StructFieldList( unsigned int typeno, unsigned char *list, int len )
{
    struct datatype *dt = DEBUG_NewDataType( DT_STRUCT, NULL );
    unsigned char *ptr = list;

    while ( ptr - list < len )
    {
        union codeview_fieldtype *type = (union codeview_fieldtype *)ptr;

        if ( *ptr >= 0xf0 )       /* LF_PAD... */
        {
            ptr += *ptr & 0x0f;
            continue;
        }

        switch ( type->generic.id )
        {
        case LF_BCLASS:
        {
            int offset, olen = numeric_leaf( &offset, &type->bclass.offset );

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 2 + olen;
            break;
        }

        case LF_BCLASS_32:
        {
            int offset, olen = numeric_leaf( &offset, &type->bclass32.offset );

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 4 + olen;
            break;
        }

        case LF_VBCLASS:
        case LF_IVBCLASS:
        {
            int vbpoff, vbplen = numeric_leaf( &vbpoff, &type->vbclass.vbpoff );
            unsigned short int *p_vboff = (unsigned short int *)((char *)&type->vbclass.vbpoff + vbpoff);
            int vpoff, vplen = numeric_leaf( &vpoff, p_vboff );

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 2 + 2 + vbplen + vplen;
            break;
        }

        case LF_VBCLASS_32:
        case LF_IVBCLASS_32:
        {
            int vbpoff, vbplen = numeric_leaf( &vbpoff, &type->vbclass32.vbpoff );
            unsigned short int *p_vboff = (unsigned short int *)((char *)&type->vbclass32.vbpoff + vbpoff);
            int vpoff, vplen = numeric_leaf( &vpoff, p_vboff );

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 4 + 4 + vbplen + vplen;
            break;
        }

        case LF_MEMBER:
        {
            int offset, olen = numeric_leaf( &offset, &type->member.offset );
            unsigned char *name = (unsigned char *)&type->member.offset + olen;

            struct datatype *subtype = DEBUG_GetCVType( type->member.type );
            int elem_size = subtype? DEBUG_GetObjectSize( subtype ) : 0;

            DEBUG_AddStructElement( dt, terminate_string( name ),
                                        subtype, offset << 3, elem_size << 3 );

            ptr += 2 + 2 + 2 + olen + (1 + name[0]);
            break;
        }

        case LF_MEMBER_32:
        {
            int offset, olen = numeric_leaf( &offset, &type->member32.offset );
            unsigned char *name = (unsigned char *)&type->member32.offset + olen;

            struct datatype *subtype = DEBUG_GetCVType( type->member32.type );
            int elem_size = subtype? DEBUG_GetObjectSize( subtype ) : 0;

            DEBUG_AddStructElement( dt, terminate_string( name ),
                                        subtype, offset << 3, elem_size << 3 );

            ptr += 2 + 2 + 4 + olen + (1 + name[0]);
            break;
        }

        case LF_STMEMBER:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 2 + (1 + type->stmember.name[0]);
            break;

        case LF_STMEMBER_32:
            /* FIXME: ignored for now */
            ptr += 2 + 4 + 2 + (1 + type->stmember32.name[0]);
            break;

        case LF_METHOD:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 2 + (1 + type->method.name[0]);
            break;

        case LF_METHOD_32:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (1 + type->method32.name[0]);
            break;

        case LF_NESTTYPE:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + (1 + type->nesttype.name[0]);
            break;

        case LF_NESTTYPE_32:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (1 + type->nesttype32.name[0]);
            break;

        case LF_VFUNCTAB:
            /* FIXME: ignored for now */
            ptr += 2 + 2;
            break;

        case LF_VFUNCTAB_32:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4;
            break;

        case LF_ONEMETHOD:
            /* FIXME: ignored for now */
            switch ( (type->onemethod.attribute >> 2) & 7 )
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 2 + 4 + (1 + type->onemethod_virt.name[0]);
                break;

            default:
                ptr += 2 + 2 + 2 + (1 + type->onemethod.name[0]);
                break;
            }
            break;

        case LF_ONEMETHOD_32:
            /* FIXME: ignored for now */
            switch ( (type->onemethod32.attribute >> 2) & 7 )
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 4 + 4 + (1 + type->onemethod32_virt.name[0]);
                break;

            default:
                ptr += 2 + 2 + 4 + (1 + type->onemethod32.name[0]);
                break;
            }
            break;

        default:
            DEBUG_Printf( DBG_CHN_MESG, "Unhandled type %04x in STRUCT field list\n",
                                        type->generic.id );
            return FALSE;
        }
    }

    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_AddCVType_Enum( unsigned int typeno, char *name, unsigned int fieldlist )
{
    struct datatype *dt   = DEBUG_NewDataType( DT_ENUM, name );
    struct datatype *list = DEBUG_GetCVType( fieldlist );

    if ( list )
        if(DEBUG_CopyFieldlist( dt, list ) == FALSE)
            return FALSE;

    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_AddCVType_Struct( unsigned int typeno, char *name, int structlen, unsigned int fieldlist )
{
    struct datatype *dt   = DEBUG_NewDataType( DT_STRUCT, name );
    struct datatype *list = DEBUG_GetCVType( fieldlist );

    if ( list )
    {
        DEBUG_SetStructSize( dt, structlen );
        if(DEBUG_CopyFieldlist( dt, list ) == FALSE)
            return FALSE;
    }

    return DEBUG_AddCVType( typeno, dt );
}

static int
DEBUG_ParseTypeTable( char *table, int len )
{
    unsigned int curr_type = 0x1000;
    char *ptr = table;

    while ( ptr - table < len )
    {
        union codeview_type *type = (union codeview_type *) ptr;
        int retv = TRUE;

        switch ( type->generic.id )
        {
        case LF_POINTER:
            retv = DEBUG_AddCVType_Pointer( curr_type, type->pointer.datatype );
            break;
        case LF_POINTER_32:
            retv = DEBUG_AddCVType_Pointer( curr_type, type->pointer32.datatype );
            break;

        case LF_ARRAY:
        {
            int arrlen, alen = numeric_leaf( &arrlen, &type->array.arrlen );
            unsigned char *name = (unsigned char *)&type->array.arrlen + alen;

            retv = DEBUG_AddCVType_Array( curr_type, terminate_string( name ),
                                          type->array.elemtype, arrlen );
            break;
        }
        case LF_ARRAY_32:
        {
            int arrlen, alen = numeric_leaf( &arrlen, &type->array32.arrlen );
            unsigned char *name = (unsigned char *)&type->array32.arrlen + alen;

            retv = DEBUG_AddCVType_Array( curr_type, terminate_string( name ),
                                          type->array32.elemtype, type->array32.arrlen );
            break;
        }

        case LF_BITFIELD:
            retv = DEBUG_AddCVType_Bitfield( curr_type, type->bitfield.bitoff,
                                                        type->bitfield.nbits,
                                                        type->bitfield.type );
            break;
        case LF_BITFIELD_32:
            retv = DEBUG_AddCVType_Bitfield( curr_type, type->bitfield32.bitoff,
                                                        type->bitfield32.nbits,
                                                        type->bitfield32.type );
            break;

        case LF_FIELDLIST:
        case LF_FIELDLIST_32:
        {
            /*
             * A 'field list' is a CodeView-specific data type which doesn't
             * directly correspond to any high-level data type.  It is used
             * to hold the collection of members of a struct, class, union
             * or enum type.  The actual definition of that type will follow
             * later, and refer to the field list definition record.
             *
             * As we don't have a field list type ourselves, we look ahead
             * in the field list to try to find out whether this field list
             * will be used for an enum or struct type, and create a dummy
             * type of the corresponding sort.  Later on, the definition of
             * the 'real' type will copy the member / enumeration data.
             */

            char *list = type->fieldlist.list;
            int   len  = (ptr + type->generic.len + 2) - list;

            if ( ((union codeview_fieldtype *)list)->generic.id == LF_ENUMERATE )
                retv = DEBUG_AddCVType_EnumFieldList( curr_type, list, len );
            else
                retv = DEBUG_AddCVType_StructFieldList( curr_type, list, len );
            break;
        }

        case LF_STRUCTURE:
        case LF_CLASS:
        {
            int structlen, slen = numeric_leaf( &structlen, &type->structure.structlen );
            unsigned char *name = (unsigned char *)&type->structure.structlen + slen;

            retv = DEBUG_AddCVType_Struct( curr_type, terminate_string( name ),
                                           structlen, type->structure.fieldlist );
            break;
        }
        case LF_STRUCTURE_32:
        case LF_CLASS_32:
        {
            int structlen, slen = numeric_leaf( &structlen, &type->structure32.structlen );
            unsigned char *name = (unsigned char *)&type->structure32.structlen + slen;

            retv = DEBUG_AddCVType_Struct( curr_type, terminate_string( name ),
                                           structlen, type->structure32.fieldlist );
            break;
        }

        case LF_UNION:
        {
            int un_len, ulen = numeric_leaf( &un_len, &type->t_union.un_len );
            unsigned char *name = (unsigned char *)&type->t_union.un_len + ulen;

            retv = DEBUG_AddCVType_Struct( curr_type, terminate_string( name ),
                                           un_len, type->t_union.fieldlist );
            break;
        }
        case LF_UNION_32:
        {
            int un_len, ulen = numeric_leaf( &un_len, &type->t_union32.un_len );
            unsigned char *name = (unsigned char *)&type->t_union32.un_len + ulen;

            retv = DEBUG_AddCVType_Struct( curr_type, terminate_string( name ),
                                           un_len, type->t_union32.fieldlist );
            break;
        }

        case LF_ENUM:
            retv = DEBUG_AddCVType_Enum( curr_type, terminate_string( type->enumeration.name ),
                                         type->enumeration.field );
            break;
        case LF_ENUM_32:
            retv = DEBUG_AddCVType_Enum( curr_type, terminate_string( type->enumeration32.name ),
                                         type->enumeration32.field );
            break;

        default:
            break;
        }

        if ( !retv )
            return FALSE;

        curr_type++;
        ptr += type->generic.len + 2;
    }

    return TRUE;
}


/*========================================================================
 * Process CodeView line number information.
 */

union any_size
{
  char		 * c;
  short		 * s;
  int		 * i;
  unsigned int   * ui;
};

struct startend
{
  unsigned int	  start;
  unsigned int	  end;
};

struct codeview_linetab_hdr
{
  unsigned int		   nline;
  unsigned int		   segno;
  unsigned int		   start;
  unsigned int		   end;
  char			 * sourcefile;
  unsigned short	 * linetab;
  unsigned int		 * offtab;
};

static struct codeview_linetab_hdr *
DEBUG_SnarfLinetab(char * linetab,
		   int    size)
{
  int				  file_segcount;
  char				  filename[PATH_MAX];
  unsigned int			* filetab;
  char				* fn;
  int				  i;
  int				  k;
  struct codeview_linetab_hdr   * lt_hdr;
  unsigned int			* lt_ptr;
  int				  nfile;
  int				  nseg;
  union any_size		  pnt;
  union any_size		  pnt2;
  struct startend		* start;
  int				  this_seg;

  /*
   * Now get the important bits.
   */
  pnt.c = linetab;
  nfile = *pnt.s++;
  nseg = *pnt.s++;

  filetab = (unsigned int *) pnt.c;

  /*
   * Now count up the number of segments in the file.
   */
  nseg = 0;
  for(i=0; i<nfile; i++)
    {
      pnt2.c = linetab + filetab[i];
      nseg += *pnt2.s;
    }

  /*
   * Next allocate the header we will be returning.
   * There is one header for each segment, so that we can reach in
   * and pull bits as required.
   */
  lt_hdr = (struct codeview_linetab_hdr *)
    DBG_alloc((nseg + 1) * sizeof(*lt_hdr));
  if( lt_hdr == NULL )
    {
      goto leave;
    }

  memset(lt_hdr, 0, sizeof(*lt_hdr) * (nseg+1));

  /*
   * Now fill the header we will be returning, one for each segment.
   * Note that this will basically just contain pointers into the existing
   * line table, and we do not actually copy any additional information
   * or allocate any additional memory.
   */

  this_seg = 0;
  for(i=0; i<nfile; i++)
    {
      /*
       * Get the pointer into the segment information.
       */
      pnt2.c = linetab + filetab[i];
      file_segcount = *pnt2.s;

      pnt2.ui++;
      lt_ptr = (unsigned int *) pnt2.c;
      start = (struct startend *) (lt_ptr + file_segcount);

      /*
       * Now snarf the filename for all of the segments for this file.
       */
      fn = (unsigned char *) (start + file_segcount);
      memset(filename, 0, sizeof(filename));
      memcpy(filename, fn + 1, *fn);
      fn = DBG_strdup(filename);

      for(k = 0; k < file_segcount; k++, this_seg++)
	{
	  pnt2.c = linetab + lt_ptr[k];
	  lt_hdr[this_seg].start      = start[k].start;
	  lt_hdr[this_seg].end	      = start[k].end;
	  lt_hdr[this_seg].sourcefile = fn;
	  lt_hdr[this_seg].segno      = *pnt2.s++;
	  lt_hdr[this_seg].nline      = *pnt2.s++;
	  lt_hdr[this_seg].offtab     =  pnt2.ui;
	  lt_hdr[this_seg].linetab    = (unsigned short *)
	    (pnt2.ui + lt_hdr[this_seg].nline);
	}
    }

leave:

  return lt_hdr;

}


/*========================================================================
 * Process CodeView symbol information.
 */

union codeview_symbol
{
  struct
  {
    short int	len;
    short int	id;
  } generic;

  struct
  {
	short int	len;
	short int	id;
	unsigned int	offset;
	unsigned short	seg;
	unsigned short	symtype;
	unsigned char	namelen;
	unsigned char	name[1];
  } data;

  struct
  {
	short int	len;
	short int	id;
	unsigned int	symtype;
	unsigned int	offset;
	unsigned short	seg;
	unsigned char	namelen;
	unsigned char	name[1];
  } data32;

  struct
  {
	short int	len;
	short int	id;
	unsigned int	pparent;
	unsigned int	pend;
	unsigned int	next;
	unsigned int	offset;
	unsigned short	segment;
	unsigned short	thunk_len;
	unsigned char	thtype;
	unsigned char	namelen;
	unsigned char	name[1];
  } thunk;

  struct
  {
	short int	len;
	short int	id;
	unsigned int	pparent;
	unsigned int	pend;
	unsigned int	next;
	unsigned int	proc_len;
	unsigned int	debug_start;
	unsigned int	debug_end;
	unsigned int	offset;
	unsigned short	segment;
	unsigned short	proctype;
	unsigned char	flags;
	unsigned char	namelen;
	unsigned char	name[1];
  } proc;

  struct
  {
	short int	len;
	short int	id;
	unsigned int	pparent;
	unsigned int	pend;
	unsigned int	next;
	unsigned int	proc_len;
	unsigned int	debug_start;
	unsigned int	debug_end;
	unsigned int	proctype;
	unsigned int	offset;
	unsigned short	segment;
	unsigned char	flags;
	unsigned char	namelen;
	unsigned char	name[1];
  } proc32;

  struct
  {
	short int	len;	/* Total length of this entry */
	short int	id;		/* Always S_BPREL32 */
	unsigned int	offset;	/* Stack offset relative to BP */
	unsigned short	symtype;
	unsigned char	namelen;
	unsigned char	name[1];
  } stack;

  struct
  {
	short int	len;	/* Total length of this entry */
	short int	id;		/* Always S_BPREL32 */
	unsigned int	offset;	/* Stack offset relative to BP */
	unsigned int	symtype;
	unsigned char	namelen;
	unsigned char	name[1];
  } stack32;

};

#define S_COMPILE       0x0001
#define S_REGISTER      0x0002
#define S_CONSTANT      0x0003
#define S_UDT           0x0004
#define S_SSEARCH       0x0005
#define S_END           0x0006
#define S_SKIP          0x0007
#define S_CVRESERVE     0x0008
#define S_OBJNAME       0x0009
#define S_ENDARG        0x000a
#define S_COBOLUDT      0x000b
#define S_MANYREG       0x000c
#define S_RETURN        0x000d
#define S_ENTRYTHIS     0x000e

#define S_BPREL         0x0200
#define S_LDATA         0x0201
#define S_GDATA         0x0202
#define S_PUB           0x0203
#define S_LPROC         0x0204
#define S_GPROC         0x0205
#define S_THUNK         0x0206
#define S_BLOCK         0x0207
#define S_WITH          0x0208
#define S_LABEL         0x0209
#define S_CEXMODEL      0x020a
#define S_VFTPATH       0x020b
#define S_REGREL        0x020c
#define S_LTHREAD       0x020d
#define S_GTHREAD       0x020e

#define S_PROCREF       0x0400
#define S_DATAREF       0x0401
#define S_ALIGN         0x0402
#define S_LPROCREF      0x0403

#define S_REGISTER_32   0x1001 /* Variants with new 32-bit type indices */
#define S_CONSTANT_32   0x1002
#define S_UDT_32        0x1003
#define S_COBOLUDT_32   0x1004
#define S_MANYREG_32    0x1005

#define S_BPREL_32      0x1006
#define S_LDATA_32      0x1007
#define S_GDATA_32      0x1008
#define S_PUB_32        0x1009
#define S_LPROC_32      0x100a
#define S_GPROC_32      0x100b
#define S_VFTTABLE_32   0x100c
#define S_REGREL_32     0x100d
#define S_LTHREAD_32    0x100e
#define S_GTHREAD_32    0x100f



static unsigned int
DEBUG_MapCVOffset( DBG_MODULE *module, unsigned int offset )
{
    int        nomap = module->msc_info->nomap;
    OMAP_DATA *omapp = module->msc_info->omapp;
    int i;

    if ( !nomap || !omapp )
        return offset;

    /* FIXME: use binary search */
    for ( i = 0; i < nomap-1; i++ )
        if ( omapp[i].from <= offset && omapp[i+1].from > offset )
            return !omapp[i].to? 0 : omapp[i].to + (offset - omapp[i].from);

    return 0;
}

static struct name_hash *
DEBUG_AddCVSymbol( DBG_MODULE *module, char *name, int namelen,
                   int type, unsigned int seg, unsigned int offset,
                   int size, int cookie, int flags,
                   struct codeview_linetab_hdr *linetab )
{
    int			  nsect = module->msc_info->nsect;
    PIMAGE_SECTION_HEADER sectp = module->msc_info->sectp;

    struct name_hash *symbol;
    char symname[PATH_MAX];
    DBG_VALUE value;

    /*
     * Some sanity checks
     */

    if ( !name || !namelen )
        return NULL;

    if ( !seg || seg > nsect )
        return NULL;

    /*
     * Convert type, address, and symbol name
     */
    value.type = type? DEBUG_GetCVType( type ) : NULL;
    value.cookie = cookie;

    value.addr.seg = 0;
    value.addr.off = (unsigned int) module->load_addr +
        DEBUG_MapCVOffset( module, sectp[seg-1].VirtualAddress + offset );

    memcpy( symname, name, namelen );
    symname[namelen] = '\0';


    /*
     * Check whether we have line number information
     */
    if ( linetab )
    {
        for ( ; linetab->linetab; linetab++ )
            if (    linetab->segno == seg
                 && linetab->start <= offset
                 && linetab->end   >  offset )
                break;

        if ( !linetab->linetab )
            linetab = NULL;
    }


    /*
     * Create Wine symbol record
     */
    symbol = DEBUG_AddSymbol( symname, &value,
                              linetab? linetab->sourcefile : NULL, flags );

    if ( size )
        DEBUG_SetSymbolSize( symbol, size );


    /*
     * Add line numbers if found
     */
    if ( linetab )
    {
        unsigned int i;
        for ( i = 0; i < linetab->nline; i++ )
            if (    linetab->offtab[i] >= offset
                 && linetab->offtab[i] <  offset + size )
            {
                DEBUG_AddLineNumber( symbol, linetab->linetab[i],
                                             linetab->offtab[i] - offset );
            }
    }

    return symbol;
}

static struct wine_locals *
DEBUG_AddCVLocal( struct name_hash *func, char *name, int namelen,
                  int type, int offset )
{
    struct wine_locals *local;
    char symname[PATH_MAX];

    memcpy( symname, name, namelen );
    symname[namelen] = '\0';

    local = DEBUG_AddLocal( func, 0, offset, 0, 0, symname );
    DEBUG_SetLocalSymbolType( local, DEBUG_GetCVType( type ) );

    return local;
}

static int
DEBUG_SnarfCodeView( DBG_MODULE	*module, LPBYTE root, int offset, int size,
                     struct codeview_linetab_hdr *linetab )
{
    struct name_hash *curr_func = NULL;
    int i, length;


    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    for ( i = offset; i < size; i += length )
    {
        union codeview_symbol *sym = (union codeview_symbol *)(root + i);
        length = sym->generic.len + 2;

        switch ( sym->generic.id )
        {
        /*
         * Global and local data symbols.  We don't associate these
         * with any given source file.
         */
	case S_GDATA:
	case S_LDATA:
	case S_PUB:
            DEBUG_AddCVSymbol( module, sym->data.name, sym->data.namelen,
                               sym->data.symtype, sym->data.seg,
                               sym->data.offset, 0,
                               DV_TARGET, SYM_WIN32 | SYM_DATA, NULL );
	    break;
	case S_GDATA_32:
	case S_LDATA_32:
	case S_PUB_32:
            DEBUG_AddCVSymbol( module, sym->data32.name, sym->data32.namelen,
                               sym->data32.symtype, sym->data32.seg,
                               sym->data32.offset, 0,
                               DV_TARGET, SYM_WIN32 | SYM_DATA, NULL );
	    break;

        /*
         * Sort of like a global function, but it just points
         * to a thunk, which is a stupid name for what amounts to
         * a PLT slot in the normal jargon that everyone else uses.
         */
	case S_THUNK:
            DEBUG_AddCVSymbol( module, sym->thunk.name, sym->thunk.namelen,
                               0, sym->thunk.segment,
                               sym->thunk.offset, sym->thunk.thunk_len,
                               DV_TARGET, SYM_WIN32 | SYM_FUNC, NULL );
	    break;

        /*
         * Global and static functions.
         */
	case S_GPROC:
	case S_LPROC:
 	    DEBUG_Normalize( curr_func );

            curr_func = DEBUG_AddCVSymbol( module, sym->proc.name, sym->proc.namelen,
                                           sym->proc.proctype, sym->proc.segment,
                                           sym->proc.offset, sym->proc.proc_len,
                                           DV_TARGET, SYM_WIN32 | SYM_FUNC, linetab );

            DEBUG_SetSymbolBPOff( curr_func, sym->proc.debug_start );
	    break;
	case S_GPROC_32:
	case S_LPROC_32:
 	    DEBUG_Normalize( curr_func );

            curr_func = DEBUG_AddCVSymbol( module, sym->proc32.name, sym->proc32.namelen,
                                           sym->proc32.proctype, sym->proc32.segment,
                                           sym->proc32.offset, sym->proc32.proc_len,
                                           DV_TARGET, SYM_WIN32 | SYM_FUNC, linetab );

            DEBUG_SetSymbolBPOff( curr_func, sym->proc32.debug_start );
	    break;


        /*
         * Function parameters and stack variables.
         */
	case S_BPREL:
            DEBUG_AddCVLocal( curr_func, sym->stack.name, sym->stack.namelen,
                                         sym->stack.symtype, sym->stack.offset );
            break;
	case S_BPREL_32:
            DEBUG_AddCVLocal( curr_func, sym->stack32.name, sym->stack32.namelen,
                                         sym->stack32.symtype, sym->stack32.offset );
            break;


        /*
         * These are special, in that they are always followed by an
         * additional length-prefixed string which is *not* included
         * into the symbol length count.  We need to skip it.
         */
	case S_PROCREF:
	case S_DATAREF:
	case S_LPROCREF:
            {
                LPBYTE name = (LPBYTE)sym + length;
                length += (*name + 1 + 3) & ~3;
                break;
            }
        }
    }

    DEBUG_Normalize( curr_func );

    if ( linetab ) DBG_free(linetab);
    return TRUE;
}



/*========================================================================
 * Process PDB file.
 */

#pragma pack(1)
typedef struct _PDB_FILE
{
    DWORD size;
    DWORD unknown;

} PDB_FILE, *PPDB_FILE;

typedef struct _PDB_HEADER
{
    CHAR     ident[40];
    DWORD    signature;
    DWORD    blocksize;
    WORD     freelist;
    WORD     total_alloc;
    PDB_FILE toc;
    WORD     toc_block[ 1 ];

} PDB_HEADER, *PPDB_HEADER;

typedef struct _PDB_TOC
{
    DWORD    nFiles;
    PDB_FILE file[ 1 ];

} PDB_TOC, *PPDB_TOC;

typedef struct _PDB_ROOT
{
    DWORD  version;
    DWORD  TimeDateStamp;
    DWORD  unknown;
    DWORD  cbNames;
    CHAR   names[ 1 ];

} PDB_ROOT, *PPDB_ROOT;

typedef struct _PDB_TYPES_OLD
{
    DWORD  version;
    WORD   first_index;
    WORD   last_index;
    DWORD  type_size;
    WORD   file;
    WORD   pad;

} PDB_TYPES_OLD, *PPDB_TYPES_OLD;

typedef struct _PDB_TYPES
{
    DWORD  version;
    DWORD  type_offset;
    DWORD  first_index;
    DWORD  last_index;
    DWORD  type_size;
    WORD   file;
    WORD   pad;
    DWORD  hash_size;
    DWORD  hash_base;
    DWORD  hash_offset;
    DWORD  hash_len;
    DWORD  search_offset;
    DWORD  search_len;
    DWORD  unknown_offset;
    DWORD  unknown_len;

} PDB_TYPES, *PPDB_TYPES;

typedef struct _PDB_SYMBOL_RANGE
{
    WORD   segment;
    WORD   pad1;
    DWORD  offset;
    DWORD  size;
    DWORD  characteristics;
    WORD   index;
    WORD   pad2;

} PDB_SYMBOL_RANGE, *PPDB_SYMBOL_RANGE;

typedef struct _PDB_SYMBOL_RANGE_EX
{
    WORD   segment;
    WORD   pad1;
    DWORD  offset;
    DWORD  size;
    DWORD  characteristics;
    WORD   index;
    WORD   pad2;
    DWORD  timestamp;
    DWORD  unknown;

} PDB_SYMBOL_RANGE_EX, *PPDB_SYMBOL_RANGE_EX;

typedef struct _PDB_SYMBOL_FILE
{
    DWORD  unknown1;
    PDB_SYMBOL_RANGE range;
    WORD   flag;
    WORD   file;
    DWORD  symbol_size;
    DWORD  lineno_size;
    DWORD  unknown2;
    DWORD  nSrcFiles;
    DWORD  attribute;
    CHAR   filename[ 1 ];

} PDB_SYMBOL_FILE, *PPDB_SYMBOL_FILE;

typedef struct _PDB_SYMBOL_FILE_EX
{
    DWORD  unknown1;
    PDB_SYMBOL_RANGE_EX range;
    WORD   flag;
    WORD   file;
    DWORD  symbol_size;
    DWORD  lineno_size;
    DWORD  unknown2;
    DWORD  nSrcFiles;
    DWORD  attribute;
    DWORD  reserved[ 2 ];
    CHAR   filename[ 1 ];

} PDB_SYMBOL_FILE_EX, *PPDB_SYMBOL_FILE_EX;

typedef struct _PDB_SYMBOL_SOURCE
{
    WORD   nModules;
    WORD   nSrcFiles;
    WORD   table[ 1 ];

} PDB_SYMBOL_SOURCE, *PPDB_SYMBOL_SOURCE;

typedef struct _PDB_SYMBOL_IMPORT
{
    DWORD  unknown1;
    DWORD  unknown2;
    DWORD  TimeDateStamp;
    DWORD  nRequests;
    CHAR   filename[ 1 ];

} PDB_SYMBOL_IMPORT, *PPDB_SYMBOL_IMPORT;

typedef struct _PDB_SYMBOLS_OLD
{
    WORD   hash1_file;
    WORD   hash2_file;
    WORD   gsym_file;
    WORD   pad;
    DWORD  module_size;
    DWORD  offset_size;
    DWORD  hash_size;
    DWORD  srcmodule_size;

} PDB_SYMBOLS_OLD, *PPDB_SYMBOLS_OLD;

typedef struct _PDB_SYMBOLS
{
    DWORD  signature;
    DWORD  version;
    DWORD  unknown;
    DWORD  hash1_file;
    DWORD  hash2_file;
    DWORD  gsym_file;
    DWORD  module_size;
    DWORD  offset_size;
    DWORD  hash_size;
    DWORD  srcmodule_size;
    DWORD  pdbimport_size;
    DWORD  resvd[ 5 ];

} PDB_SYMBOLS, *PPDB_SYMBOLS;
#pragma pack()


static void *pdb_read( LPBYTE image, WORD *block_list, int size )
{
    PPDB_HEADER pdb = (PPDB_HEADER)image;
    int i, nBlocks;
    LPBYTE buffer;

    if ( !size ) return NULL;

    nBlocks = (size + pdb->blocksize-1) / pdb->blocksize;
    buffer = DBG_alloc( nBlocks * pdb->blocksize );

    for ( i = 0; i < nBlocks; i++ )
        memcpy( buffer + i*pdb->blocksize,
                image + block_list[i]*pdb->blocksize, pdb->blocksize );

    return buffer;
}

static void *pdb_read_file( LPBYTE image, PPDB_TOC toc, DWORD fileNr )
{
    PPDB_HEADER pdb = (PPDB_HEADER)image;
    WORD *block_list;
    DWORD i;

    if ( !toc || fileNr >= toc->nFiles )
        return NULL;

    block_list = (WORD *) &toc->file[ toc->nFiles ];
    for ( i = 0; i < fileNr; i++ )
        block_list += (toc->file[i].size + pdb->blocksize-1) / pdb->blocksize;

    return pdb_read( image, block_list, toc->file[fileNr].size );
}

static void pdb_free( void *buffer )
{
    DBG_free( buffer );
}

static void pdb_convert_types_header( PDB_TYPES *types, char *image )
{
    memset( types, 0, sizeof(PDB_TYPES) );
    if ( !image ) return;

    if ( *(DWORD *)image < 19960000 )   /* FIXME: correct version? */
    {
        /* Old version of the types record header */
        PDB_TYPES_OLD *old = (PDB_TYPES_OLD *)image;
        types->version     = old->version;
        types->type_offset = sizeof(PDB_TYPES_OLD);
        types->type_size   = old->type_size;
        types->first_index = old->first_index;
        types->last_index  = old->last_index;
        types->file        = old->file;
    }
    else
    {
        /* New version of the types record header */
        *types = *(PDB_TYPES *)image;
    }
}

static void pdb_convert_symbols_header( PDB_SYMBOLS *symbols,
                                        int *header_size, char *image )
{
    memset( symbols, 0, sizeof(PDB_SYMBOLS) );
    if ( !image ) return;

    if ( *(DWORD *)image != 0xffffffff )
    {
        /* Old version of the symbols record header */
        PDB_SYMBOLS_OLD *old     = (PDB_SYMBOLS_OLD *)image;
        symbols->version         = 0;
        symbols->module_size     = old->module_size;
        symbols->offset_size     = old->offset_size;
        symbols->hash_size       = old->hash_size;
        symbols->srcmodule_size  = old->srcmodule_size;
        symbols->pdbimport_size  = 0;
        symbols->hash1_file      = old->hash1_file;
        symbols->hash2_file      = old->hash2_file;
        symbols->gsym_file       = old->gsym_file;

        *header_size = sizeof(PDB_SYMBOLS_OLD);
    }
    else
    {
        /* New version of the symbols record header */
        *symbols = *(PDB_SYMBOLS *)image;

        *header_size = sizeof(PDB_SYMBOLS);
    }
}

static enum DbgInfoLoad DEBUG_ProcessPDBFile( DBG_MODULE *module,
					      const char *filename, DWORD timestamp )
{
    enum DbgInfoLoad dil = DIL_ERROR;
    HANDLE hFile, hMap;
    char *image = NULL;
    PDB_HEADER *pdb = NULL;
    PDB_TOC *toc = NULL;
    PDB_ROOT *root = NULL;
    char *types_image = NULL;
    char *symbols_image = NULL;
    PDB_TYPES types;
    PDB_SYMBOLS symbols;
    int header_size = 0;
    char *modimage, *file;

    DEBUG_Printf( DBG_CHN_TRACE, "Processing PDB file %s\n", filename );

    /*
     * Open and map() .PDB file
     */
    image = DEBUG_MapDebugInfoFile( filename, 0, 0, &hFile, &hMap );
    if ( !image )
    {
        DEBUG_Printf( DBG_CHN_ERR, "-Unable to peruse .PDB file %s\n", filename );
        goto leave;
    }

    /*
     * Read in TOC and well-known files
     */

    pdb = (PPDB_HEADER)image;
    toc = pdb_read( image, pdb->toc_block, pdb->toc.size );
    root = pdb_read_file( image, toc, 1 );
    types_image = pdb_read_file( image, toc, 2 );
    symbols_image = pdb_read_file( image, toc, 3 );

    pdb_convert_types_header( &types, types_image );
    pdb_convert_symbols_header( &symbols, &header_size, symbols_image );

    if ( !root )
    {
        DEBUG_Printf( DBG_CHN_ERR,
                      "-Unable to get root from .PDB file %s\n",
                      filename );
        goto leave;
    }

    /*
     * Check for unknown versions
     */

    switch ( root->version )
    {
        case 19950623:      /* VC 4.0 */
        case 19950814:
        case 19960307:      /* VC 5.0 */
        case 19970604:      /* VC 6.0 */
            break;
        default:
            DEBUG_Printf( DBG_CHN_ERR, "-Unknown root block version %ld\n", root->version );
    }

    switch ( types.version )
    {
        case 19950410:      /* VC 4.0 */
        case 19951122:
        case 19961031:      /* VC 5.0 / 6.0 */
            break;
        default:
            DEBUG_Printf( DBG_CHN_ERR, "-Unknown type info version %ld\n", types.version );
    }

    switch ( symbols.version )
    {
        case 0:            /* VC 4.0 */
        case 19960307:     /* VC 5.0 */
        case 19970606:     /* VC 6.0 */
            break;
        default:
            DEBUG_Printf( DBG_CHN_ERR, "-Unknown symbol info version %ld\n", symbols.version );
    }


    /*
     * Check .PDB time stamp
     */

    if ( root->TimeDateStamp != timestamp )
    {
        DEBUG_Printf( DBG_CHN_ERR, "-Wrong time stamp of .PDB file %s (0x%08lx, 0x%08lx)\n",
		      filename, root->TimeDateStamp, timestamp );
    }

    /*
     * Read type table
     */

    DEBUG_ParseTypeTable( types_image + types.type_offset, types.type_size );

    /*
     * Read type-server .PDB imports
     */

    if ( symbols.pdbimport_size )
    {
        /* FIXME */
        DEBUG_Printf(DBG_CHN_ERR, "-Type server .PDB imports ignored!\n" );
    }

    /*
     * Read global symbol table
     */

    modimage = pdb_read_file( image, toc, symbols.gsym_file );
    if ( modimage )
    {
        DEBUG_SnarfCodeView( module, modimage, 0,
                             toc->file[symbols.gsym_file].size, NULL );
        pdb_free( modimage );
    }

    /*
     * Read per-module symbol / linenumber tables
     */

    file = symbols_image + header_size;
    while ( file - symbols_image < header_size + symbols.module_size )
    {
        int file_nr, file_index, symbol_size, lineno_size;
        char *file_name;

        if ( symbols.version < 19970000 )
        {
            PDB_SYMBOL_FILE *sym_file = (PDB_SYMBOL_FILE *) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            file_index  = sym_file->range.index;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
        }
        else
        {
            PDB_SYMBOL_FILE_EX *sym_file = (PDB_SYMBOL_FILE_EX *) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            file_index  = sym_file->range.index;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
        }

        modimage = pdb_read_file( image, toc, file_nr );
        if ( modimage )
        {
            struct codeview_linetab_hdr *linetab = NULL;

            if ( lineno_size )
                linetab = DEBUG_SnarfLinetab( modimage + symbol_size, lineno_size );

            if ( symbol_size )
                DEBUG_SnarfCodeView( module, modimage, sizeof(DWORD),
                                     symbol_size, linetab );

            pdb_free( modimage );
        }

        file_name += strlen(file_name) + 1;
        file = (char *)( (DWORD)(file_name + strlen(file_name) + 1 + 3) & ~3 );
    }

    dil = DIL_LOADED;

 leave:

    /*
     * Cleanup
     */

    DEBUG_ClearTypeTable();

    if ( symbols_image ) pdb_free( symbols_image );
    if ( types_image ) pdb_free( types_image );
    if ( root ) pdb_free( root );
    if ( toc ) pdb_free( toc );

    DEBUG_UnmapDebugInfoFile(hFile, hMap, image);

    return dil;
}




/*========================================================================
 * Process CodeView debug information.
 */

#define CODEVIEW_NB09_SIG  ( 'N' | ('B' << 8) | ('0' << 16) | ('9' << 24) )
#define CODEVIEW_NB10_SIG  ( 'N' | ('B' << 8) | ('1' << 16) | ('0' << 24) )
#define CODEVIEW_NB11_SIG  ( 'N' | ('B' << 8) | ('1' << 16) | ('1' << 24) )

typedef struct _CODEVIEW_HEADER
{
    DWORD  dwSignature;
    DWORD  lfoDirectory;

} CODEVIEW_HEADER, *PCODEVIEW_HEADER;

typedef struct _CODEVIEW_PDB_DATA
{
    DWORD  timestamp;
    DWORD  unknown;
    CHAR   name[ 1 ];

} CODEVIEW_PDB_DATA, *PCODEVIEW_PDB_DATA;

typedef struct _CV_DIRECTORY_HEADER
{
    WORD   cbDirHeader;
    WORD   cbDirEntry;
    DWORD  cDir;
    DWORD  lfoNextDir;
    DWORD  flags;

} CV_DIRECTORY_HEADER, *PCV_DIRECTORY_HEADER;

typedef struct _CV_DIRECTORY_ENTRY
{
    WORD   subsection;
    WORD   iMod;
    DWORD  lfo;
    DWORD  cb;

} CV_DIRECTORY_ENTRY, *PCV_DIRECTORY_ENTRY;


#define	sstAlignSym		0x125
#define	sstSrcModule		0x127


static enum DbgInfoLoad DEBUG_ProcessCodeView( DBG_MODULE *module, LPBYTE root )
{
    PCODEVIEW_HEADER cv = (PCODEVIEW_HEADER)root;
    enum DbgInfoLoad dil = DIL_ERROR;

    switch ( cv->dwSignature )
    {
    case CODEVIEW_NB09_SIG:
    case CODEVIEW_NB11_SIG:
    {
        PCV_DIRECTORY_HEADER hdr = (PCV_DIRECTORY_HEADER)(root + cv->lfoDirectory);
        PCV_DIRECTORY_ENTRY ent, prev, next;
        unsigned int i;

        ent = (PCV_DIRECTORY_ENTRY)((LPBYTE)hdr + hdr->cbDirHeader);
        for ( i = 0; i < hdr->cDir; i++, ent = next )
        {
            next = (i == hdr->cDir-1)? NULL :
                   (PCV_DIRECTORY_ENTRY)((LPBYTE)ent + hdr->cbDirEntry);
            prev = (i == 0)? NULL :
                   (PCV_DIRECTORY_ENTRY)((LPBYTE)ent - hdr->cbDirEntry);

            if ( ent->subsection == sstAlignSym )
            {
                /*
                 * Check the next and previous entry.  If either is a
                 * sstSrcModule, it contains the line number info for
                 * this file.
                 *
                 * FIXME: This is not a general solution!
                 */
                struct codeview_linetab_hdr *linetab = NULL;

                if ( next && next->iMod == ent->iMod
                          && next->subsection == sstSrcModule )
                     linetab = DEBUG_SnarfLinetab( root + next->lfo, next->cb );

                if ( prev && prev->iMod == ent->iMod
                          && prev->subsection == sstSrcModule )
                     linetab = DEBUG_SnarfLinetab( root + prev->lfo, prev->cb );


                DEBUG_SnarfCodeView( module, root + ent->lfo, sizeof(DWORD),
                                     ent->cb, linetab );
            }
        }

        dil = DIL_LOADED;
        break;
    }

    case CODEVIEW_NB10_SIG:
    {
        PCODEVIEW_PDB_DATA pdb = (PCODEVIEW_PDB_DATA)(cv + 1);

        dil = DEBUG_ProcessPDBFile( module, pdb->name, pdb->timestamp );
        break;
    }

    default:
        DEBUG_Printf( DBG_CHN_ERR, "Unknown CODEVIEW signature %08lX in module %s\n",
                      cv->dwSignature, module->module_name );
        break;
    }

    return dil;
}


/*========================================================================
 * Process debug directory.
 */
static enum DbgInfoLoad DEBUG_ProcessDebugDirectory( DBG_MODULE *module,
						     LPBYTE file_map,
						     PIMAGE_DEBUG_DIRECTORY dbg,
						     int nDbg )
{
    enum DbgInfoLoad dil;
    int i;
    __TRY {
        dil = DIL_ERROR;
        /* First, watch out for OMAP data */
        for ( i = 0; i < nDbg; i++ )
        {
            if ( dbg[i].Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC )
            {
                module->msc_info->nomap = dbg[i].SizeOfData / sizeof(OMAP_DATA);
                module->msc_info->omapp = (OMAP_DATA *)(file_map + dbg[i].PointerToRawData);
                break;
            }
        }
  
      /* Now, try to parse CodeView debug info */
        for ( i = 0; dil != DIL_LOADED && i < nDbg; i++ )
        {
            if ( dbg[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW )
            {
                dil = DEBUG_ProcessCodeView( module, file_map + dbg[i].PointerToRawData );
            }
        }
    
        /* If not found, try to parse COFF debug info */
        for ( i = 0; dil != DIL_LOADED && i < nDbg; i++ )
        {
            if ( dbg[i].Type == IMAGE_DEBUG_TYPE_COFF )
                dil = DEBUG_ProcessCoff( module, file_map + dbg[i].PointerToRawData );
        }
#if 0
	 /* FIXME: this should be supported... this is the debug information for
	  * functions compiled without a frame pointer (FPO = frame pointer omission)
	  * the associated data helps finding out the relevant information
	  */
        for ( i = 0; i < nDbg; i++ )
            if ( dbg[i].Type == IMAGE_DEBUG_TYPE_FPO )
                DEBUG_Printf(DBG_CHN_MESG, "This guy has FPO information\n");

#define FRAME_FPO   0
#define FRAME_TRAP  1
#define FRAME_TSS   2

typedef struct _FPO_DATA {
	DWORD       ulOffStart;            /* offset 1st byte of function code */
	DWORD       cbProcSize;            /* # bytes in function */
	DWORD       cdwLocals;             /* # bytes in locals/4 */
	WORD        cdwParams;             /* # bytes in params/4 */

	WORD        cbProlog : 8;          /* # bytes in prolog */
	WORD        cbRegs   : 3;          /* # regs saved */
	WORD        fHasSEH  : 1;          /* TRUE if SEH in func */
	WORD        fUseBP   : 1;          /* TRUE if EBP has been allocated */
	WORD        reserved : 1;          /* reserved for future use */
	WORD        cbFrame  : 2;          /* frame type */
} FPO_DATA;
#endif
    }
    __EXCEPT(page_fault)
    {
        return DIL_ERROR;
    }
    __ENDTRY
    return dil;
}


/*========================================================================
 * Process DBG file.
 */
static enum DbgInfoLoad DEBUG_ProcessDBGFile( DBG_MODULE *module,
					      const char *filename, DWORD timestamp )
{
    enum DbgInfoLoad dil = DIL_ERROR;
    HANDLE hFile = INVALID_HANDLE_VALUE, hMap = 0;
    LPBYTE file_map = NULL;
    PIMAGE_SEPARATE_DEBUG_HEADER hdr;
    PIMAGE_DEBUG_DIRECTORY dbg;
    int nDbg;


    DEBUG_Printf( DBG_CHN_TRACE, "Processing DBG file %s\n", filename );

    file_map = DEBUG_MapDebugInfoFile( filename, 0, 0, &hFile, &hMap );
    if ( !file_map )
    {
        DEBUG_Printf( DBG_CHN_ERR, "-Unable to peruse .DBG file %s\n", filename );
        goto leave;
    }

    hdr = (PIMAGE_SEPARATE_DEBUG_HEADER) file_map;

    if ( hdr->TimeDateStamp != timestamp )
    {
        DEBUG_Printf( DBG_CHN_ERR, "Warning - %s has incorrect internal timestamp\n",
	              filename );
        /*
         *  Well, sometimes this happens to DBG files which ARE REALLY the right .DBG
         *  files but nonetheless this check fails. Anyway, WINDBG (debugger for
         *  Windows by Microsoft) loads debug symbols which have incorrect timestamps.
         */
    }


    dbg = (PIMAGE_DEBUG_DIRECTORY) ( file_map + sizeof(*hdr)
		 + hdr->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)
		 + hdr->ExportedNamesSize );

    nDbg = hdr->DebugDirectorySize / sizeof(*dbg);

    dil = DEBUG_ProcessDebugDirectory( module, file_map, dbg, nDbg );


 leave:
    DEBUG_UnmapDebugInfoFile( hFile, hMap, file_map );
    return dil;
}


/*========================================================================
 * Process MSC debug information in PE file.
 */
enum DbgInfoLoad DEBUG_RegisterMSCDebugInfo( DBG_MODULE *module, HANDLE hFile,
					     void *_nth, unsigned long nth_ofs )
{
    enum DbgInfoLoad	   dil = DIL_ERROR;
    PIMAGE_NT_HEADERS      nth = (PIMAGE_NT_HEADERS)_nth;
    PIMAGE_DATA_DIRECTORY  dir = nth->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DEBUG;
    PIMAGE_DEBUG_DIRECTORY dbg = NULL;
    int                    nDbg;
    MSC_DBG_INFO           extra_info = { 0, NULL, 0, NULL };
    HANDLE	           hMap = 0;
    LPBYTE                 file_map = NULL;


    /* Read in section data */

    module->msc_info = &extra_info;
    extra_info.nsect = nth->FileHeader.NumberOfSections;
    extra_info.sectp = DBG_alloc( extra_info.nsect * sizeof(IMAGE_SECTION_HEADER) );
    if ( !extra_info.sectp )
        goto leave;

    if ( !DEBUG_READ_MEM_VERBOSE( (char *)module->load_addr +
		                  nth_ofs + OFFSET_OF(IMAGE_NT_HEADERS, OptionalHeader) +
		                  nth->FileHeader.SizeOfOptionalHeader,
                                  extra_info.sectp,
                                  extra_info.nsect * sizeof(IMAGE_SECTION_HEADER) ) )
        goto leave;

    /* Read in debug directory */

    nDbg = dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    if ( !nDbg )
        goto leave;

    dbg = (PIMAGE_DEBUG_DIRECTORY) DBG_alloc( nDbg * sizeof(IMAGE_DEBUG_DIRECTORY) );
    if ( !dbg )
        goto leave;

    if ( !DEBUG_READ_MEM_VERBOSE( (char *)module->load_addr + dir->VirtualAddress,
                                  dbg, nDbg * sizeof(IMAGE_DEBUG_DIRECTORY) ) )
        goto leave;


    /* Map in PE file */
    file_map = DEBUG_MapDebugInfoFile( NULL, 0, 0, &hFile, &hMap );
    if ( !file_map )
        goto leave;


    /* Parse debug directory */

    if ( nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED )
    {
        /* Debug info is stripped to .DBG file */

        PIMAGE_DEBUG_MISC misc = (PIMAGE_DEBUG_MISC)(file_map + dbg->PointerToRawData);

        if ( nDbg != 1 || dbg->Type != IMAGE_DEBUG_TYPE_MISC
                       || misc->DataType != IMAGE_DEBUG_MISC_EXENAME )
        {
            DEBUG_Printf( DBG_CHN_ERR, "-Debug info stripped, but no .DBG file in module %s\n",
	                  module->module_name );
            goto leave;
        }

        dil = DEBUG_ProcessDBGFile( module, misc->Data, nth->FileHeader.TimeDateStamp );
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
        if ( mpd_nth->Signature != IMAGE_NT_SIGNATURE ||
             mpd_nth->FileHeader.NumberOfSections != nth->FileHeader.NumberOfSections ||
             (mpd_nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) != 0)
            goto leave;
        mpd_dir = mpd_nth->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DEBUG;

        if ((mpd_dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY)) != nDbg)
            goto leave;

        mpd_dbg = (PIMAGE_DEBUG_DIRECTORY)(file_map + mpd_dir->VirtualAddress);

        dil = DEBUG_ProcessDebugDirectory( module, file_map, mpd_dbg, nDbg );
    }


 leave:
    module->msc_info = NULL;

    DEBUG_UnmapDebugInfoFile( 0, hMap, file_map );
    if ( extra_info.sectp ) DBG_free( extra_info.sectp );
    if ( dbg ) DBG_free( dbg );
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

    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, pe_seg_ofs += sizeof(pe_seg)) {
      if (!DEBUG_READ_MEM_VERBOSE((void*)((char *)module->load_addr + pe_seg_ofs),
				  &pe_seg, sizeof(pe_seg)))
	  continue;

      if (!strcasecmp(pe_seg.Name, ".stab")) {
	stabs = pe_seg.VirtualAddress;
	stabsize = pe_seg.SizeOfRawData;
      } else if (!strncasecmp(pe_seg.Name, ".stabstr", 8)) {
	stabstr = pe_seg.VirtualAddress;
	stabstrsize = pe_seg.SizeOfRawData;
      }
    }

    if (stabstrsize && stabsize) {
       char*	s1 = DBG_alloc(stabsize+stabstrsize);

       if (s1) {
	  if (DEBUG_READ_MEM_VERBOSE((char*)module->load_addr + stabs, s1, stabsize) &&
	      DEBUG_READ_MEM_VERBOSE((char*)module->load_addr + stabstr,
				     s1 + stabsize, stabstrsize)) {
	     dil = DEBUG_ParseStabs(s1, 0, 0, stabsize, stabsize, stabstrsize);
	  } else {
	     DEBUG_Printf(DBG_CHN_MESG, "couldn't read data block\n");
	  }
	  DBG_free(s1);
       } else {
	  DEBUG_Printf(DBG_CHN_MESG, "couldn't alloc %d bytes\n",
		       stabsize + stabstrsize);
       }
    } else {
       dil = DIL_NOINFO;
    }
    return dil;
}
