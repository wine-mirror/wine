/*
 * File msc.c - read VC++ debug information from COFF and eventually
 * from PDB files.
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004,      Eric Pouech.
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

/*
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

#include <assert.h>
#include <stdlib.h>

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"

#include "wine/exception.h"
#include "wine/debug.h"
#include "excpt.h"
#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_msc);

#define MAX_PATHNAME_LEN 1024

typedef struct
{
    DWORD  from;
    DWORD  to;
} OMAP_DATA;

struct msc_debug_info
{
    struct module*              module;
    int			        nsect;
    const IMAGE_SECTION_HEADER* sectp;
    int			        nomap;
    const OMAP_DATA*            omapp;
    const BYTE*                 root;
};

/*========================================================================
 * Debug file access helper routines
 */

static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/*========================================================================
 * Process COFF debug information.
 */

struct CoffFile
{
    unsigned int                startaddr;
    unsigned int                endaddr;
    struct symt_compiland*      compiland;
    int                         linetab_offset;
    int                         linecnt;
    struct symt**               entries;
    int		                neps;
    int		                neps_alloc;
};

struct CoffFileSet
{
    struct CoffFile*    files;
    int		        nfiles;
    int		        nfiles_alloc;
};

static const char*	coff_get_name(const IMAGE_SYMBOL* coff_sym, 
                                      const char* coff_strtab)
{
    static	char	namebuff[9];
    const char*		nampnt;

    if (coff_sym->N.Name.Short)
    {
        memcpy(namebuff, coff_sym->N.ShortName, 8);
        namebuff[8] = '\0';
        nampnt = &namebuff[0];
    }
    else
    {
        nampnt = coff_strtab + coff_sym->N.Name.Long;
    }

    if (nampnt[0] == '_') nampnt++;
    return nampnt;
}

static int coff_add_file(struct CoffFileSet* coff_files, struct module* module,
                         const char* filename)
{
    struct CoffFile* file;

    if (coff_files->nfiles + 1 >= coff_files->nfiles_alloc)
    {
	coff_files->nfiles_alloc += 10;
        coff_files->files = (coff_files->files) ?
            HeapReAlloc(GetProcessHeap(), 0, coff_files->files,
                        coff_files->nfiles_alloc * sizeof(struct CoffFile)) :
            HeapAlloc(GetProcessHeap(), 0,
                      coff_files->nfiles_alloc * sizeof(struct CoffFile));
    }
    file = coff_files->files + coff_files->nfiles;
    file->startaddr = 0xffffffff;
    file->endaddr   = 0;
    file->compiland = symt_new_compiland(module, filename);
    file->linetab_offset = -1;
    file->linecnt = 0;
    file->entries = NULL;
    file->neps = file->neps_alloc = 0;

    return coff_files->nfiles++;
}

static void coff_add_symbol(struct CoffFile* coff_file, struct symt* sym)
{
    if (coff_file->neps + 1 >= coff_file->neps_alloc)
    {
        coff_file->neps_alloc += 10;
        coff_file->entries = (coff_file->entries) ?
            HeapReAlloc(GetProcessHeap(), 0, coff_file->entries,
                        coff_file->neps_alloc * sizeof(struct symt*)) :
            HeapAlloc(GetProcessHeap(), 0, 
                      coff_file->neps_alloc * sizeof(struct symt*));
    }
    coff_file->entries[coff_file->neps++] = sym;
}

static SYM_TYPE coff_process_info(const struct msc_debug_info* msc_dbg)
{
    const IMAGE_AUX_SYMBOL*		aux;
    const IMAGE_COFF_SYMBOLS_HEADER*	coff;
    const IMAGE_LINENUMBER*		coff_linetab;
    const IMAGE_LINENUMBER*		linepnt;
    const char*                         coff_strtab;
    const IMAGE_SYMBOL* 		coff_sym;
    const IMAGE_SYMBOL* 		coff_symbols;
    struct CoffFileSet	                coff_files;
    int				        curr_file_idx = -1;
    unsigned int		        i;
    int			       	        j;
    int			       	        k;
    int			       	        l;
    int		       		        linetab_indx;
    const char*                         nampnt;
    int		       		        naux;
    SYM_TYPE                            sym_type = SymNone;
    DWORD                               addr;

    TRACE("Processing COFF symbols...\n");

    assert(sizeof(IMAGE_SYMBOL) == IMAGE_SIZEOF_SYMBOL);
    assert(sizeof(IMAGE_LINENUMBER) == IMAGE_SIZEOF_LINENUMBER);

    coff_files.files = NULL;
    coff_files.nfiles = coff_files.nfiles_alloc = 0;

    coff = (const IMAGE_COFF_SYMBOLS_HEADER*)msc_dbg->root;

    coff_symbols = (const IMAGE_SYMBOL*)((unsigned int)coff + 
                                         coff->LvaToFirstSymbol);
    coff_linetab = (const IMAGE_LINENUMBER*)((unsigned int)coff + 
                                             coff->LvaToFirstLinenumber);
    coff_strtab = (const char*)(coff_symbols + coff->NumberOfSymbols);

    linetab_indx = 0;

    for (i = 0; i < coff->NumberOfSymbols; i++)
    {
        coff_sym = coff_symbols + i;
        naux = coff_sym->NumberOfAuxSymbols;

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_FILE)
	{
            curr_file_idx = coff_add_file(&coff_files, msc_dbg->module, 
                                          (const char*)(coff_sym + 1));
            TRACE("New file %s\n", (const char*)(coff_sym + 1));
            i += naux;
            continue;
	}

        if (curr_file_idx < 0)
        {
            assert(coff_files.nfiles == 0 && coff_files.nfiles_alloc == 0);
            curr_file_idx = coff_add_file(&coff_files, msc_dbg->module, "<none>");
            TRACE("New file <none>\n");
        }

        /*
         * This guy marks the size and location of the text section
         * for the current file.  We need to keep track of this so
         * we can figure out what file the different global functions
         * go with.
         */
        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC &&
            naux != 0 && coff_sym->Type == 0 && coff_sym->SectionNumber == 1)
	{
            aux = (const IMAGE_AUX_SYMBOL*) (coff_sym + 1);

            if (coff_files.files[curr_file_idx].linetab_offset != -1)
	    {
                /*
                 * Save this so we can still get the old name.
                 */
                const char* fn;

                fn = source_get(msc_dbg->module, 
                                coff_files.files[curr_file_idx].compiland->source);

                TRACE("Duplicating sect from %s: %lx %x %x %d %d\n",
                      fn, aux->Section.Length,
                      aux->Section.NumberOfRelocations,
                      aux->Section.NumberOfLinenumbers,
                      aux->Section.Number, aux->Section.Selection);
                TRACE("More sect %d %s %08lx %d %d %d\n",
                      coff_sym->SectionNumber,
                      coff_get_name(coff_sym, coff_strtab),
                      coff_sym->Value, coff_sym->Type,
                      coff_sym->StorageClass, coff_sym->NumberOfAuxSymbols);

                /*
                 * Duplicate the file entry.  We have no way to describe
                 * multiple text sections in our current way of handling things.
                 */
                coff_add_file(&coff_files, msc_dbg->module, fn);
	    }
            else
	    {
                TRACE("New text sect from %s: %lx %x %x %d %d\n",
                      source_get(msc_dbg->module, coff_files.files[curr_file_idx].compiland->source),
                      aux->Section.Length,
                      aux->Section.NumberOfRelocations,
                      aux->Section.NumberOfLinenumbers,
                      aux->Section.Number, aux->Section.Selection);
	    }

            if (coff_files.files[curr_file_idx].startaddr > coff_sym->Value)
	    {
                coff_files.files[curr_file_idx].startaddr = coff_sym->Value;
	    }

            if (coff_files.files[curr_file_idx].endaddr < coff_sym->Value + aux->Section.Length)
	    {
                coff_files.files[curr_file_idx].endaddr = coff_sym->Value + aux->Section.Length;
	    }

            coff_files.files[curr_file_idx].linetab_offset = linetab_indx;
            coff_files.files[curr_file_idx].linecnt = aux->Section.NumberOfLinenumbers;
            linetab_indx += aux->Section.NumberOfLinenumbers;
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC && naux == 0 && 
            coff_sym->SectionNumber == 1)
	{
            DWORD base = msc_dbg->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
            /*
             * This is a normal static function when naux == 0.
             * Just register it.  The current file is the correct
             * one in this instance.
             */
            nampnt = coff_get_name(coff_sym, coff_strtab);

            TRACE("\tAdding static symbol %s\n", nampnt);

            /* FIXME: was adding symbol to this_file ??? */
            coff_add_symbol(&coff_files.files[curr_file_idx],
                            &symt_new_function(msc_dbg->module, 
                                               coff_files.files[curr_file_idx].compiland, 
                                               nampnt,
                                               msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                               0 /* FIXME */,
                                               NULL /* FIXME */)->symt);
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            ISFCN(coff_sym->Type) && coff_sym->SectionNumber > 0)
	{
            struct symt_compiland* compiland = NULL;
            DWORD base = msc_dbg->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
            nampnt = coff_get_name(coff_sym, coff_strtab);

            TRACE("%d: %lx %s\n",
                  i, msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                  nampnt);
            TRACE("\tAdding global symbol %s (sect=%s)\n",
                  nampnt, msc_dbg->sectp[coff_sym->SectionNumber - 1].Name);

            /*
             * Now we need to figure out which file this guy belongs to.
             */
            for (j = 0; j < coff_files.nfiles; j++)
	    {
                if (coff_files.files[j].startaddr <= base + coff_sym->Value
                     && coff_files.files[j].endaddr > base + coff_sym->Value)
		{
                    compiland = coff_files.files[j].compiland;
                    break;
		}
	    }
            if (j < coff_files.nfiles)
            {
                coff_add_symbol(&coff_files.files[j],
                                &symt_new_function(msc_dbg->module, compiland, nampnt, 
                                                   msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                                   0 /* FIXME */, NULL /* FIXME */)->symt);
            } 
            else 
            {
                symt_new_function(msc_dbg->module, NULL, nampnt, 
                                  msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                  0 /* FIXME */, NULL /* FIXME */);
            }
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            coff_sym->SectionNumber > 0)
	{
            DWORD base = msc_dbg->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
            /*
             * Similar to above, but for the case of data symbols.
             * These aren't treated as entrypoints.
             */
            nampnt = coff_get_name(coff_sym, coff_strtab);

            TRACE("%d: %lx %s\n",
                  i, msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                  nampnt);
            TRACE("\tAdding global data symbol %s\n", nampnt);

            /*
             * Now we need to figure out which file this guy belongs to.
             */
            symt_new_global_variable(msc_dbg->module, NULL, nampnt, TRUE /* FIXME */,
                                     msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                     0 /* FIXME */, NULL /* FIXME */);
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC && naux == 0)
	{
            /*
             * Ignore these.  They don't have anything to do with
             * reality.
             */
            i += naux;
            continue;
	}

        TRACE("Skipping unknown entry '%s' %d %d %d\n",
              coff_get_name(coff_sym, coff_strtab),
              coff_sym->StorageClass, coff_sym->SectionNumber, naux);
        
        /*
         * For now, skip past the aux entries.
         */
        i += naux;
    }

    if (coff_files.files != NULL)
    {
        /*
         * OK, we now should have a list of files, and we should have a list
         * of entrypoints.  We need to sort the entrypoints so that we are
         * able to tie the line numbers with the given functions within the
         * file.
         */
        for (j = 0; j < coff_files.nfiles; j++)
        {
            if (coff_files.files[j].entries != NULL)
            {
                qsort(coff_files.files[j].entries, coff_files.files[j].neps,
                      sizeof(struct symt*), symt_cmp_addr);
            }
        }

        /*
         * Now pick apart the line number tables, and attach the entries
         * to the given functions.
         */
        for (j = 0; j < coff_files.nfiles; j++)
        {
            l = 0;
            if (coff_files.files[j].neps != 0)
            {
                for (k = 0; k < coff_files.files[j].linecnt; k++)
                {
                    linepnt = coff_linetab + coff_files.files[j].linetab_offset + k;
                    /*
                     * If we have spilled onto the next entrypoint, then
                     * bump the counter..
                     */
                    for (;;)
                    {
                        if (l+1 >= coff_files.files[j].neps) break;
                        symt_get_info(coff_files.files[j].entries[l+1], TI_GET_ADDRESS, &addr);
                        if (((msc_dbg->module->module.BaseOfImage + linepnt->Type.VirtualAddress) < addr))
                            break;
                        l++;
                    }

                    if (coff_files.files[j].entries[l+1]->tag == SymTagFunction)
                    {
                        /*
                         * Add the line number.  This is always relative to the
                         * start of the function, so we need to subtract that offset
                         * first.
                         */
                        symt_get_info(coff_files.files[j].entries[l+1], TI_GET_ADDRESS, &addr);
                        symt_add_func_line(msc_dbg->module, (struct symt_function*)coff_files.files[j].entries[l+1], 
                                           coff_files.files[j].compiland->source, linepnt->Linenumber,
                                           msc_dbg->module->module.BaseOfImage + linepnt->Type.VirtualAddress - addr);
                    }
                }
            }
        }

        sym_type = SymCoff;

        for (j = 0; j < coff_files.nfiles; j++)
	{
            if (coff_files.files[j].entries != NULL)
	    {
                HeapFree(GetProcessHeap(), 0, coff_files.files[j].entries);
	    }
	}
        HeapFree(GetProcessHeap(), 0, coff_files.files);
    }

    return sym_type;
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
        unsigned int	        type;
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
        unsigned int	        btype;
        unsigned int	        vbtype;
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
        unsigned char	        name[1];
#endif
    } enumerate;

    struct
    {
        short int		id;
        short int		type;
        unsigned char	        name[1];
    } friendfcn;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        type;
        unsigned char	        name[1];
    } friendfcn32;

    struct
    {
        short int		id;
        short int		type;
        short int		attribute;
        unsigned short int	offset;    /* numeric leaf */
#if 0
        unsigned char	        name[1];
#endif
    } member;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned short int	offset;    /* numeric leaf */
#if 0
        unsigned char	        name[1];
#endif
    } member32;

    struct
    {
        short int		id;
        short int		type;
        short int		attribute;
        unsigned char	        name[1];
    } stmember;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned char	        name[1];
    } stmember32;

    struct
    {
        short int		id;
        short int		count;
        short int		mlist;
        unsigned char	        name[1];
    } method;

    struct
    {
        short int		id;
        short int		count;
        unsigned int	        mlist;
        unsigned char	        name[1];
    } method32;

    struct
    {
        short int		id;
        short int		index;
        unsigned char	        name[1];
    } nesttype;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        index;
        unsigned char	        name[1];
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
        unsigned int	        type;
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
        unsigned int	        type;
    } friendcls32;

    struct
    {
        short int		id;
        short int		attribute;
        short int		type;
        unsigned char	        name[1];
    } onemethod;

    struct
    {
        short int		id;
        short int		attribute;
        short int		type;
        unsigned int	        vtab_offset;
        unsigned char	        name[1];
    } onemethod_virt;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int 	        type;
        unsigned char	        name[1];
    } onemethod32;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned int	        vtab_offset;
        unsigned char	        name[1];
    } onemethod32_virt;

    struct
    {
        short int		id;
        short int		type;
        unsigned int	        offset;
    } vfuncoff;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        type;
        unsigned int	        offset;
    } vfuncoff32;

    struct
    {
        short int		id;
        short int		attribute;
        short int		index;
        unsigned char	        name[1];
    } nesttypeex;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        index;
        unsigned char	        name[1];
    } nesttypeex32;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned char	        name[1];
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
static struct symt*     cv_basic_types[MAX_BUILTIN_TYPES];
static unsigned int     num_cv_defined_types = 0;
static struct symt**    cv_defined_types = NULL;
#define SymTagCVBitField        (SymTagMax + 0x100)
struct codeview_bitfield
{
    struct symt symt;
    unsigned    subtype;
    unsigned    bitposition;
    unsigned    bitlength;
};
static struct codeview_bitfield*        cv_bitfields;
static unsigned                         num_cv_bitfields;
static unsigned                         used_cv_bitfields;

static void codeview_init_basic_types(struct module* module)
{
    /*
     * These are the common builtin types that are used by VC++.
     */
    cv_basic_types[T_NOTYPE] = NULL;
    cv_basic_types[T_ABS]    = NULL;
    cv_basic_types[T_VOID]   = &symt_new_basic(module, btVoid,  "void", 0)->symt;
    cv_basic_types[T_CHAR]   = &symt_new_basic(module, btChar,  "char", 1)->symt;
    cv_basic_types[T_SHORT]  = &symt_new_basic(module, btInt,   "short int", 2)->symt;
    cv_basic_types[T_LONG]   = &symt_new_basic(module, btInt,   "long int", 4)->symt;
    cv_basic_types[T_QUAD]   = &symt_new_basic(module, btInt,   "long long int", 8)->symt;
    cv_basic_types[T_UCHAR]  = &symt_new_basic(module, btUInt,  "unsigned char", 1)->symt;
    cv_basic_types[T_USHORT] = &symt_new_basic(module, btUInt,  "unsigned short", 2)->symt;
    cv_basic_types[T_ULONG]  = &symt_new_basic(module, btUInt,  "unsigned long", 4)->symt;
    cv_basic_types[T_UQUAD]  = &symt_new_basic(module, btUInt,  "unsigned long long", 8)->symt;
    cv_basic_types[T_REAL32] = &symt_new_basic(module, btFloat, "float", 4)->symt;
    cv_basic_types[T_REAL64] = &symt_new_basic(module, btFloat, "double", 8)->symt;
    cv_basic_types[T_RCHAR]  = &symt_new_basic(module, btInt,   "signed char", 1)->symt;
    cv_basic_types[T_WCHAR]  = &symt_new_basic(module, btWChar, "wchar_t", 2)->symt;
    cv_basic_types[T_INT4]   = &symt_new_basic(module, btInt,   "INT4", 4)->symt;
    cv_basic_types[T_UINT4]  = &symt_new_basic(module, btUInt,  "UINT4", 4)->symt;

    cv_basic_types[T_32PVOID]   = &symt_new_pointer(module, cv_basic_types[T_VOID])->symt;
    cv_basic_types[T_32PCHAR]   = &symt_new_pointer(module, cv_basic_types[T_CHAR])->symt;
    cv_basic_types[T_32PSHORT]  = &symt_new_pointer(module, cv_basic_types[T_SHORT])->symt;
    cv_basic_types[T_32PLONG]   = &symt_new_pointer(module, cv_basic_types[T_LONG])->symt;
    cv_basic_types[T_32PQUAD]   = &symt_new_pointer(module, cv_basic_types[T_QUAD])->symt;
    cv_basic_types[T_32PUCHAR]  = &symt_new_pointer(module, cv_basic_types[T_UCHAR])->symt;
    cv_basic_types[T_32PUSHORT] = &symt_new_pointer(module, cv_basic_types[T_USHORT])->symt;
    cv_basic_types[T_32PULONG]  = &symt_new_pointer(module, cv_basic_types[T_ULONG])->symt;
    cv_basic_types[T_32PUQUAD]  = &symt_new_pointer(module, cv_basic_types[T_UQUAD])->symt;
    cv_basic_types[T_32PREAL32] = &symt_new_pointer(module, cv_basic_types[T_REAL32])->symt;
    cv_basic_types[T_32PREAL64] = &symt_new_pointer(module, cv_basic_types[T_REAL64])->symt;
    cv_basic_types[T_32PRCHAR]  = &symt_new_pointer(module, cv_basic_types[T_RCHAR])->symt;
    cv_basic_types[T_32PWCHAR]  = &symt_new_pointer(module, cv_basic_types[T_WCHAR])->symt;
    cv_basic_types[T_32PINT4]   = &symt_new_pointer(module, cv_basic_types[T_INT4])->symt;
    cv_basic_types[T_32PUINT4]  = &symt_new_pointer(module, cv_basic_types[T_UINT4])->symt;
}

static int numeric_leaf(int* value, const unsigned short int* leaf)
{
    unsigned short int type = *leaf++;
    int length = 2;

    if (type < LF_NUMERIC)
    {
        *value = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:
            length += 1;
            *value = *(const char*)leaf;
            break;

        case LF_SHORT:
            length += 2;
            *value = *(const short*)leaf;
            break;

        case LF_USHORT:
            length += 2;
            *value = *(const unsigned short*)leaf;
            break;

        case LF_LONG:
            length += 4;
            *value = *(const int*)leaf;
            break;

        case LF_ULONG:
            length += 4;
            *value = *(const unsigned int*)leaf;
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
	    FIXME("Unknown numeric leaf type %04x\n", type);
            *value = 0;
            break;
        }
    }

    return length;
}

static const char* terminate_string(const unsigned char* name)
{
    static char symname[256];

    int namelen = name[0];
    assert(namelen >= 0 && namelen < 256);

    memcpy(symname, name + 1, namelen);
    symname[namelen] = '\0';

    return (!*symname || strcmp(symname, "__unnamed") == 0) ? NULL : symname;
}

static struct symt*  codeview_get_type(unsigned int typeno, BOOL allow_special)
{
    struct symt*        symt = NULL;

    /*
     * Convert Codeview type numbers into something we can grok internally.
     * Numbers < 0x1000 are all fixed builtin types.  Numbers from 0x1000 and
     * up are all user defined (structs, etc).
     */
    if (typeno < 0x1000)
    {
        if (typeno < MAX_BUILTIN_TYPES)
	    symt = cv_basic_types[typeno];
    }
    else
    {
        if (typeno - 0x1000 < num_cv_defined_types)
	    symt = cv_defined_types[typeno - 0x1000];
    }
    if (!allow_special && symt && symt->tag == SymTagCVBitField)
        FIXME("bitfields are only handled for UDTs\n");
    return symt;
}

static int codeview_add_type(unsigned int typeno, struct symt* dt)
{
    while (typeno - 0x1000 >= num_cv_defined_types)
    {
        num_cv_defined_types += 0x100;
        if (cv_defined_types)
            cv_defined_types = (struct symt**)
                HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cv_defined_types,
                            num_cv_defined_types * sizeof(struct symt*));
        else
            cv_defined_types = (struct symt**)
                HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                          num_cv_defined_types * sizeof(struct symt*));

        if (cv_defined_types == NULL) return FALSE;
    }

    cv_defined_types[typeno - 0x1000] = dt;
    return TRUE;
}

static void codeview_clear_type_table(void)
{
    if (cv_defined_types) HeapFree(GetProcessHeap(), 0, cv_defined_types);
    cv_defined_types = NULL;
    num_cv_defined_types = 0;

    if (cv_bitfields) HeapFree(GetProcessHeap(), 0, cv_bitfields);
    cv_bitfields = NULL;
    num_cv_bitfields = used_cv_bitfields = 0;
}

static int codeview_add_type_pointer(struct module* module, unsigned int typeno, 
                                     unsigned int datatype)
{
    struct symt* symt = &symt_new_pointer(module, codeview_get_type(datatype, FALSE))->symt;
    return codeview_add_type(typeno, symt);
}

static int codeview_add_type_array(struct module* module, 
                                   unsigned int typeno, const char* name,
                                   unsigned int elemtype, unsigned int arr_len)
{
    struct symt*        symt;
    struct symt*        elem = codeview_get_type(elemtype, FALSE);
    DWORD               arr_max = 0;

    if (elem)
    {
        DWORD elem_size;
        symt_get_info(elem, TI_GET_LENGTH, &elem_size);
        if (elem_size) arr_max = arr_len / elem_size;
    }
    symt = &symt_new_array(module, 0, arr_max, elem)->symt;
    return codeview_add_type(typeno, symt);
}

static int codeview_add_type_bitfield(unsigned int typeno, unsigned int bitoff,
                                      unsigned int nbits, unsigned int basetype)
{
    if (used_cv_bitfields >= num_cv_bitfields)
    {
        num_cv_bitfields *= 2;
        if (cv_bitfields)
            cv_bitfields = HeapReAlloc(GetProcessHeap(), 0, cv_bitfields, 
                                       num_cv_bitfields * sizeof(struct codeview_bitfield));
        else
            cv_bitfields = HeapAlloc(GetProcessHeap(), 0, 
                                     num_cv_bitfields * sizeof(struct codeview_bitfield));
        if (!cv_bitfields) return 0;
    }
    
    cv_bitfields[used_cv_bitfields].symt.tag    = SymTagCVBitField;
    cv_bitfields[used_cv_bitfields].subtype     = basetype;
    cv_bitfields[used_cv_bitfields].bitposition = bitoff;
    cv_bitfields[used_cv_bitfields].bitlength   = nbits;

    return codeview_add_type(typeno, &cv_bitfields[used_cv_bitfields++].symt);
}

static int codeview_add_type_enum_field_list(struct module* module, 
                                             unsigned int typeno, 
                                             const unsigned char* list, int len)
{
    struct symt_enum*           symt;
    const unsigned char*        ptr = list;

    symt = symt_new_enum(module, NULL);
    while (ptr - list < len)
    {
        const union codeview_fieldtype* type = (const union codeview_fieldtype*)ptr;

        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr += *ptr & 0x0f;
            continue;
        }

        switch (type->generic.id)
        {
        case LF_ENUMERATE:
        {
            int value, vlen = numeric_leaf(&value, &type->enumerate.value);
            const unsigned char* name = (const unsigned char*)&type->enumerate.value + vlen;

            symt_add_enum_element(module, symt, terminate_string(name), value);
            ptr += 2 + 2 + vlen + (1 + name[0]);
            break;
        }

        default:
            FIXME("Unhandled type %04x in ENUM field list\n", type->generic.id);
            return FALSE;
        }
    }

    return codeview_add_type(typeno, &symt->symt);
}

static int codeview_add_type_struct_field_list(struct module* module, 
                                               unsigned int typeno, 
                                               const unsigned char* list, int len)
{
    struct symt_udt*            symt;
    const unsigned char*        ptr = list;

    symt = symt_new_udt(module, NULL, 0, UdtStruct /* don't care */);
    while (ptr - list < len)
    {
        const union codeview_fieldtype* type = (const union codeview_fieldtype*)ptr;

        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr +=* ptr & 0x0f;
            continue;
        }

        switch (type->generic.id)
        {
        case LF_BCLASS:
        {
            int offset, olen = numeric_leaf(&offset, &type->bclass.offset);

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 2 + olen;
            break;
        }

        case LF_BCLASS_32:
        {
            int offset, olen = numeric_leaf(&offset, &type->bclass32.offset);

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 4 + olen;
            break;
        }

        case LF_VBCLASS:
        case LF_IVBCLASS:
        {
            int vbpoff, vbplen = numeric_leaf(&vbpoff, &type->vbclass.vbpoff);
            const unsigned short int* p_vboff = (const unsigned short int*)((const char*)&type->vbclass.vbpoff + vbpoff);
            int vpoff, vplen = numeric_leaf(&vpoff, p_vboff);

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 2 + 2 + vbplen + vplen;
            break;
        }

        case LF_VBCLASS_32:
        case LF_IVBCLASS_32:
        {
            int vbpoff, vbplen = numeric_leaf(&vbpoff, &type->vbclass32.vbpoff);
            const unsigned short int* p_vboff = (const unsigned short int*)((const char*)&type->vbclass32.vbpoff + vbpoff);
            int vpoff, vplen = numeric_leaf(&vpoff, p_vboff);

            /* FIXME: ignored for now */

            ptr += 2 + 2 + 4 + 4 + vbplen + vplen;
            break;
        }

        case LF_MEMBER:
        {
            int offset, olen = numeric_leaf(&offset, &type->member.offset);
            const unsigned char* name = (const unsigned char*)&type->member.offset + olen;
            struct symt* subtype = codeview_get_type(type->member.type, TRUE);

            if (!subtype || subtype->tag != SymTagCVBitField)
            {
                DWORD elem_size = 0;
                if (subtype) symt_get_info(subtype, TI_GET_LENGTH, &elem_size);
                symt_add_udt_element(module, symt, terminate_string(name),
                                     subtype, offset << 3, elem_size << 3);
            }
            else
            {
                struct codeview_bitfield* cvbf = (struct codeview_bitfield*)subtype;
                symt_add_udt_element(module, symt, terminate_string(name),
                                     codeview_get_type(cvbf->subtype, FALSE),
                                     cvbf->bitposition, cvbf->bitlength);
            }

            ptr += 2 + 2 + 2 + olen + (1 + name[0]);
            break;
        }

        case LF_MEMBER_32:
        {
            int offset, olen = numeric_leaf(&offset, &type->member32.offset);
            const unsigned char* name = (const unsigned char*)&type->member32.offset + olen;
            struct symt* subtype = codeview_get_type(type->member32.type, TRUE);

            if (!subtype || subtype->tag != SymTagCVBitField)
            {
                DWORD elem_size = 0;
                if (subtype) symt_get_info(subtype, TI_GET_LENGTH, &elem_size);
                symt_add_udt_element(module, symt, terminate_string(name),
                                     subtype, offset << 3, elem_size << 3);
            }
            else
            {
                struct codeview_bitfield* cvbf = (struct codeview_bitfield*)subtype;
                symt_add_udt_element(module, symt, terminate_string(name),
                                     codeview_get_type(cvbf->subtype, FALSE),
                                     cvbf->bitposition, cvbf->bitlength);
            }

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
            switch ((type->onemethod.attribute >> 2) & 7)
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
            switch ((type->onemethod32.attribute >> 2) & 7)
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
            FIXME("Unhandled type %04x in STRUCT field list\n", type->generic.id);
            return FALSE;
        }
    }

    return codeview_add_type(typeno, &symt->symt);
}

static int codeview_add_type_enum(struct module* module, unsigned int typeno, 
                                  const char* name, unsigned int fieldlist)
{
    struct symt_enum*   symt = symt_new_enum(module, name);
    struct symt*        list = codeview_get_type(fieldlist, FALSE);

    /* FIXME: this is rather ugly !!! */
    if (list) symt->vchildren = ((struct symt_enum*)list)->vchildren;

    return codeview_add_type(typeno, &symt->symt);
}

static int codeview_add_type_struct(struct module* module, unsigned int typeno, 
                                    const char* name, int structlen, 
                                    unsigned int fieldlist, enum UdtKind kind)
{
    struct symt_udt*    symt = symt_new_udt(module, name, structlen, kind);
    struct symt*        list = codeview_get_type(fieldlist, FALSE);

    /* FIXME: this is rather ugly !!! */
    if (list) symt->vchildren = ((struct symt_udt*)list)->vchildren;

    return codeview_add_type(typeno, &symt->symt);
}

static int codeview_parse_type_table(struct module* module, const char* table, int len)
{
    unsigned int        curr_type = 0x1000;
    const char*         ptr = table;

    while (ptr - table < len)
    {
        const union codeview_type* type = (const union codeview_type*)ptr;
        int retv = TRUE;

        switch (type->generic.id)
        {
        case LF_POINTER:
            retv = codeview_add_type_pointer(module, curr_type, 
                                             type->pointer.datatype);
            break;
        case LF_POINTER_32:
            retv = codeview_add_type_pointer(module, curr_type, 
                                             type->pointer32.datatype);
            break;

        case LF_ARRAY:
        {
            int arrlen, alen = numeric_leaf(&arrlen, &type->array.arrlen);
            const unsigned char* name = (const unsigned char*)&type->array.arrlen + alen;

            retv = codeview_add_type_array(module, curr_type, terminate_string(name),
                                           type->array.elemtype, arrlen);
            break;
        }
        case LF_ARRAY_32:
        {
            int arrlen, alen = numeric_leaf(&arrlen, &type->array32.arrlen);
            const unsigned char* name = (const unsigned char*)&type->array32.arrlen + alen;

            retv = codeview_add_type_array(module, curr_type, terminate_string(name),
                                           type->array32.elemtype, 
                                           type->array32.arrlen);
            break;
        }

        /* a bitfields is a CodeView specific data type which represent a bitfield
         * in a structure or a class. For now, we store it in a SymTag-like type
         * (so that the rest of the process is seamless), but check at udt inclusion
         * type for its presence
         */
        case LF_BITFIELD:
            retv = codeview_add_type_bitfield(curr_type, type->bitfield.bitoff,
                                              type->bitfield.nbits,
                                              type->bitfield.type);
            break;
        case LF_BITFIELD_32:
            retv = codeview_add_type_bitfield(curr_type, type->bitfield32.bitoff,
                                              type->bitfield32.nbits,
                                              type->bitfield32.type);
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

            const char* list = type->fieldlist.list;
            int   len  = (ptr + type->generic.len + 2) - list;

            if (((union codeview_fieldtype*)list)->generic.id == LF_ENUMERATE)
                retv = codeview_add_type_enum_field_list(module, curr_type, list, len);
            else
                retv = codeview_add_type_struct_field_list(module, curr_type, list, len);
            break;
        }

        case LF_STRUCTURE:
        case LF_CLASS:
        {
            int structlen, slen = numeric_leaf(&structlen, &type->structure.structlen);
            const unsigned char* name = (const unsigned char*)&type->structure.structlen + slen;

            retv = codeview_add_type_struct(module, curr_type, terminate_string(name),
                                            structlen, type->structure.fieldlist,
                                            type->generic.id == LF_CLASS ? UdtClass : UdtStruct);
            break;
        }
        case LF_STRUCTURE_32:
        case LF_CLASS_32:
        {
            int structlen, slen = numeric_leaf(&structlen, &type->structure32.structlen);
            const unsigned char* name = (const unsigned char*)&type->structure32.structlen + slen;

            retv = codeview_add_type_struct(module, curr_type, terminate_string(name),
                                            structlen, type->structure32.fieldlist,
                                            type->generic.id == LF_CLASS ? UdtClass : UdtStruct);
            break;
        }

        case LF_UNION:
        {
            int un_len, ulen = numeric_leaf(&un_len, &type->t_union.un_len);
            const unsigned char* name = (const unsigned char*)&type->t_union.un_len + ulen;

            retv = codeview_add_type_struct(module, curr_type, terminate_string(name),
                                            un_len, type->t_union.fieldlist, UdtUnion);
            break;
        }
        case LF_UNION_32:
        {
            int un_len, ulen = numeric_leaf(&un_len, &type->t_union32.un_len);
            const unsigned char* name = (const unsigned char*)&type->t_union32.un_len + ulen;

            retv = codeview_add_type_struct(module, curr_type, terminate_string(name),
                                            un_len, type->t_union32.fieldlist, UdtUnion);
            break;
        }

        case LF_ENUM:
            retv = codeview_add_type_enum(module, curr_type, terminate_string(type->enumeration.name),
                                          type->enumeration.field);
            break;
        case LF_ENUM_32:
            retv = codeview_add_type_enum(module, curr_type, terminate_string(type->enumeration32.name),
                                          type->enumeration32.field);
            break;

        default:
            FIXME("Unhandled leaf %x\n", type->generic.id);
            break;
        }

        if (!retv)
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
    const char*           c;
    const short*          s;
    const int*            i;
    const unsigned int*   ui;
};

struct startend
{
    unsigned int	  start;
    unsigned int	  end;
};

struct codeview_linetab
{
    unsigned int		nline;
    unsigned int		segno;
    unsigned int		start;
    unsigned int		end;
    struct symt_compiland*      compiland;
    const unsigned short*       linetab;
    const unsigned int*         offtab;
};

static struct codeview_linetab* codeview_snarf_linetab(struct module* module, 
                                                       const char* linetab, int size)
{
    int				file_segcount;
    char			filename[PATH_MAX];
    const unsigned int*         filetab;
    const char*                 fn;
    int				i;
    int				k;
    struct codeview_linetab*    lt_hdr;
    const unsigned int*         lt_ptr;
    int				nfile;
    int				nseg;
    union any_size		pnt;
    union any_size		pnt2;
    const struct startend*      start;
    int				this_seg;
    struct symt_compiland*      compiland;

    /*
     * Now get the important bits.
     */
    pnt.c = linetab;
    nfile = *pnt.s++;
    nseg = *pnt.s++;

    filetab = (const unsigned int*) pnt.c;

    /*
     * Now count up the number of segments in the file.
     */
    nseg = 0;
    for (i = 0; i < nfile; i++)
    {
        pnt2.c = linetab + filetab[i];
        nseg += *pnt2.s;
    }

    /*
     * Next allocate the header we will be returning.
     * There is one header for each segment, so that we can reach in
     * and pull bits as required.
     */
    lt_hdr = (struct codeview_linetab*)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nseg + 1) * sizeof(*lt_hdr));
    if (lt_hdr == NULL)
    {
        goto leave;
    }

    /*
     * Now fill the header we will be returning, one for each segment.
     * Note that this will basically just contain pointers into the existing
     * line table, and we do not actually copy any additional information
     * or allocate any additional memory.
     */

    this_seg = 0;
    for (i = 0; i < nfile; i++)
    {
        /*
         * Get the pointer into the segment information.
         */
        pnt2.c = linetab + filetab[i];
        file_segcount = *pnt2.s;

        pnt2.ui++;
        lt_ptr = (const unsigned int*) pnt2.c;
        start = (const struct startend*)(lt_ptr + file_segcount);

        /*
         * Now snarf the filename for all of the segments for this file.
         */
        fn = (const unsigned char*)(start + file_segcount);
        memset(filename, 0, sizeof(filename));
        memcpy(filename, fn + 1, *fn);
        compiland = symt_new_compiland(module, filename);
        
        for (k = 0; k < file_segcount; k++, this_seg++)
	{
            pnt2.c = linetab + lt_ptr[k];
            lt_hdr[this_seg].start      = start[k].start;
            lt_hdr[this_seg].end        = start[k].end;
            lt_hdr[this_seg].compiland  = compiland;
            lt_hdr[this_seg].segno      = *pnt2.s++;
            lt_hdr[this_seg].nline      = *pnt2.s++;
            lt_hdr[this_seg].offtab     = pnt2.ui;
            lt_hdr[this_seg].linetab    = (const unsigned short*)(pnt2.ui + lt_hdr[this_seg].nline);
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
	short int	len;	        /* Total length of this entry */
	short int	id;		/* Always S_BPREL */
	unsigned int	offset;	        /* Stack offset relative to BP */
	unsigned short	symtype;
	unsigned char	namelen;
	unsigned char	name[1];
    } stack;

    struct
    {
	short int	len;	        /* Total length of this entry */
	short int	id;		/* Always S_BPREL_32 */
	unsigned int	offset;	        /* Stack offset relative to EBP */
	unsigned int	symtype;
	unsigned char	namelen;
	unsigned char	name[1];
    } stack32;

    struct
    {
	short int	len;	        /* Total length of this entry */
	short int	id;		/* Always S_REGISTER */
        unsigned short  type;
        unsigned short  reg;
	unsigned char	namelen;
	unsigned char	name[1];
        /* don't handle register tracking */
    } s_register;

    struct
    {
	short int	len;	        /* Total length of this entry */
	short int	id;		/* Always S_REGISTER_32 */
        unsigned int    type;           /* check whether type & reg are correct */
        unsigned int    reg;
	unsigned char	namelen;
	unsigned char	name[1];
        /* don't handle register tracking */
    } s_register32;

    struct
    {
        short int       len;
        short int       id;
        unsigned int    parent;
        unsigned int    end;
        unsigned int    length;
        unsigned int    offset;
        unsigned short  segment;
        unsigned char   namelen;
        unsigned char   name[1];
    } block;

    struct
    {
        short int       len;
        short int       id;
        unsigned int    offset;
        unsigned short  segment;
        unsigned char   flags;
        unsigned char   namelen;
        unsigned char   name[1];
    } label;

    struct
    {
        short int       len;
        short int       id;
        unsigned short  type;
        unsigned short  arrlen;         /* numeric leaf */
#if 0
        unsigned char   namelen;
        unsigned char   name[1];
#endif
    } constant;

    struct
    {
        short int       len;
        short int       id;
        unsigned        type;
        unsigned short  arrlen;         /* numeric leaf */
#if 0
        unsigned char   namelen;
        unsigned char   name[1];
#endif
    } constant32;

    struct
    {
        short int       len;
        short int       id;
        unsigned short  type;
        unsigned char   namelen;
        unsigned char   name[1];
    } udt;

    struct
    {
        short int       len;
        short int       id;
        unsigned        type;
        unsigned char   namelen;
        unsigned char   name[1];
    } udt32;
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

static unsigned int codeview_map_offset(const struct msc_debug_info* msc_dbg,
                                        unsigned int offset)
{
    int                 nomap = msc_dbg->nomap;
    const OMAP_DATA*    omapp = msc_dbg->omapp;
    int                 i;

    if (!nomap || !omapp) return offset;

    /* FIXME: use binary search */
    for (i = 0; i < nomap - 1; i++)
        if (omapp[i].from <= offset && omapp[i+1].from > offset)
            return !omapp[i].to ? 0 : omapp[i].to + (offset - omapp[i].from);

    return 0;
}

static const struct codeview_linetab*
codeview_get_linetab(const struct codeview_linetab* linetab,
                     unsigned seg, unsigned offset)
{
    /*
     * Check whether we have line number information
     */
    if (linetab)
    {
        for (; linetab->linetab; linetab++)
            if (linetab->segno == seg &&
                linetab->start <= offset && linetab->end   >  offset)
                break;
        if (!linetab->linetab) linetab = NULL;
    }
    return linetab;
}

static unsigned codeview_get_address(const struct msc_debug_info* msc_dbg, 
                                     unsigned seg, unsigned offset)
{
    int			        nsect = msc_dbg->nsect;
    const IMAGE_SECTION_HEADER* sectp = msc_dbg->sectp;

    if (!seg || seg > nsect) return 0;
    return msc_dbg->module->module.BaseOfImage +
        codeview_map_offset(msc_dbg, sectp[seg-1].VirtualAddress + offset);
}

static void codeview_add_func_linenum(struct module* module, 
                                      struct symt_function* func,
                                      const struct codeview_linetab* linetab,
                                      unsigned offset, unsigned size)
{
    unsigned int        i;

    if (!linetab) return;
    for (i = 0; i < linetab->nline; i++)
    {
        if (linetab->offtab[i] >= offset && linetab->offtab[i] < offset + size)
        {
            symt_add_func_line(module, func, linetab->compiland->source,
                               linetab->linetab[i], linetab->offtab[i] - offset);
        }
    }
}

static int codeview_snarf(const struct msc_debug_info* msc_dbg, const BYTE* root, 
                          int offset, int size,
                          struct codeview_linetab* linetab)
{
    struct symt_function*               curr_func = NULL;
    int                                 i, length;
    char                                symname[PATH_MAX];
    const struct codeview_linetab*      flt;
    struct symt_block*                  block = NULL;
    struct symt*                        symt;

    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    for (i = offset; i < size; i += length)
    {
        const union codeview_symbol* sym = (const union codeview_symbol*)(root + i);
        length = sym->generic.len + 2;

        switch (sym->generic.id)
        {
        /*
         * Global and local data symbols.  We don't associate these
         * with any given source file.
         */
	case S_GDATA:
	case S_LDATA:
            memcpy(symname, sym->data.name, sym->data.namelen);
            symname[sym->data.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->data.seg, sym->data.offset);
            symt_new_global_variable(msc_dbg->module, 
                                     flt ? flt->compiland : NULL,
                                     symname, sym->generic.id == S_LDATA,
                                     codeview_get_address(msc_dbg, sym->data.seg, sym->data.offset),
                                     0,
                                     codeview_get_type(sym->data.symtype, FALSE));
	    break;

	case S_GDATA_32:
	case S_LDATA_32:
            memcpy(symname, sym->data32.name, sym->data32.namelen);
            symname[sym->data32.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->data32.seg, sym->data32.offset);
            symt_new_global_variable(msc_dbg->module, flt ? flt->compiland : NULL,
                                     symname, sym->generic.id == S_LDATA_32,
                                     codeview_get_address(msc_dbg, sym->data32.seg, sym->data32.offset),
                                     0,
                                     codeview_get_type(sym->data32.symtype, FALSE));
	    break;

	case S_PUB: /* FIXME is this really a 'data' structure ?? */
            memcpy(symname, sym->data.name, sym->data.namelen);
            symname[sym->data.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->data.seg, sym->data.offset);
            symt_new_public(msc_dbg->module, flt ? flt->compiland : NULL,
                            symname, 
                            codeview_get_address(msc_dbg, sym->data.seg, sym->data.offset),
                            0, TRUE /* FIXME */, TRUE /* FIXME */);
            break;

	case S_PUB_32: /* FIXME is this really a 'data32' structure ?? */
            memcpy(symname, sym->data32.name, sym->data32.namelen);
            symname[sym->data32.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->data32.seg, sym->data32.offset);
            symt_new_public(msc_dbg->module, flt ? flt->compiland : NULL,
                            symname, 
                            codeview_get_address(msc_dbg, sym->data32.seg, sym->data32.offset),
                            0, TRUE /* FIXME */, TRUE /* FIXME */);
	    break;

        /*
         * Sort of like a global function, but it just points
         * to a thunk, which is a stupid name for what amounts to
         * a PLT slot in the normal jargon that everyone else uses.
         */
	case S_THUNK:
            memcpy(symname, sym->thunk.name, sym->thunk.namelen);
            symname[sym->thunk.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->thunk.segment, sym->thunk.offset);
            symt_new_thunk(msc_dbg->module, flt ? flt->compiland : NULL,
                           symname, sym->thunk.thtype,
                           codeview_get_address(msc_dbg, sym->thunk.segment, sym->thunk.offset),
                           sym->thunk.thunk_len);
	    break;

        /*
         * Global and static functions.
         */
	case S_GPROC:
	case S_LPROC:
            memcpy(symname, sym->proc.name, sym->proc.namelen);
            symname[sym->proc.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->proc.segment, sym->proc.offset);
            curr_func = symt_new_function(msc_dbg->module, 
                                          flt ? flt->compiland : NULL, symname, 
                                          codeview_get_address(msc_dbg, sym->proc.segment, sym->proc.offset),
                                          sym->proc.proc_len,
                                          codeview_get_type(sym->proc.proctype, FALSE));
            codeview_add_func_linenum(msc_dbg->module, curr_func, flt, 
                                      sym->proc.offset, sym->proc.proc_len);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, sym->proc.debug_start, NULL);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, sym->proc.debug_end, NULL);
	    break;
	case S_GPROC_32:
	case S_LPROC_32:
            memcpy(symname, sym->proc32.name, sym->proc32.namelen);
            symname[sym->proc32.namelen] = '\0';
            flt = codeview_get_linetab(linetab, sym->proc32.segment, sym->proc32.offset);
            curr_func = symt_new_function(msc_dbg->module,
                                          flt ? flt->compiland : NULL, symname, 
                                          codeview_get_address(msc_dbg, sym->proc32.segment, sym->proc32.offset),
                                          sym->proc32.proc_len,
                                          codeview_get_type(sym->proc32.proctype, FALSE));
            codeview_add_func_linenum(msc_dbg->module, curr_func, flt, 
                                      sym->proc32.offset, sym->proc32.proc_len);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, sym->proc32.debug_start, NULL);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, sym->proc32.debug_end, NULL);
	    break;

        /*
         * Function parameters and stack variables.
         */
	case S_BPREL:
            memcpy(symname, sym->stack.name, sym->stack.namelen);
            symname[sym->stack.namelen] = '\0';
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->stack.offset,
                                block, codeview_get_type(sym->stack.symtype, FALSE),
                                symname);
            break;
	case S_BPREL_32:
            memcpy(symname, sym->stack32.name, sym->stack32.namelen);
            symname[sym->stack32.namelen] = '\0';
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->stack32.offset,
                                block, codeview_get_type(sym->stack32.symtype, FALSE),
                                symname);
            break;

        case S_REGISTER:
            memcpy(symname, sym->s_register.name, sym->s_register.namelen);
            symname[sym->s_register.namelen] = '\0';
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->s_register.reg,
                                block, codeview_get_type(sym->s_register.type, FALSE),
                                symname);
            break;

        case S_REGISTER_32:
            memcpy(symname, sym->s_register32.name, sym->s_register32.namelen);
            symname[sym->s_register32.namelen] = '\0';
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->s_register32.reg,
                                block, codeview_get_type(sym->s_register32.type, FALSE),
                                symname);
            break;

        case S_BLOCK:
            block = symt_open_func_block(msc_dbg->module, curr_func, block, 
                                         codeview_get_address(msc_dbg, sym->block.segment, sym->block.offset),
                                         sym->block.length);
            break;

        case S_END:
            if (block)
            {
                block = symt_close_func_block(msc_dbg->module, curr_func, block, 0);
            }
            else if (curr_func)
            {
                symt_normalize_function(msc_dbg->module, curr_func);
                curr_func = NULL;
            }
            break;

        case S_COMPILE:
            TRACE("S-Compile %x %.*s\n", ((const BYTE*)sym)[4], ((const BYTE*)sym)[8], (const BYTE*)sym + 9);
            break;

        case S_OBJNAME:
            TRACE("S-ObjName %.*s\n", ((const BYTE*)sym)[8], (const BYTE*)sym + 9);
            break;

        case S_LABEL:
            memcpy(symname, sym->label.name, sym->label.namelen);
            symname[sym->label.namelen] = '\0';
            if (curr_func)
            {
                symt_add_function_point(msc_dbg->module, curr_func, SymTagLabel, 
                                        codeview_get_address(msc_dbg, sym->label.segment, sym->label.offset) - curr_func->address,
                                        symname);
            }
            else FIXME("No current function for label %s\n", symname);
            break;

#if 0
        case S_CONSTANT_32:
            {
                int             val, vlen;
                char*           ptr;
                const char*     x;
                struct symt*    se;

                vlen = numeric_leaf(&val, &sym->constant32.arrlen);
                ptr = (char*)&sym->constant32.arrlen + vlen;
                se = codeview_get_type(sym->constant32.type, FALSE);
                if (!se) x = "---";
                else if (se->tag == SymTagEnum) x = ((struct symt_enum*)se)->name;
                else x = "###";
                    
                FIXME("S-Constant %u %.*s %x (%s)\n", 
                      val, ptr[0], ptr + 1, sym->constant32.type, x);
            }
            break;
#endif

        case S_UDT:
            symt = codeview_get_type(sym->udt.type, FALSE);
            if (symt)
            {
                memcpy(symname, sym->udt.name, sym->udt.namelen);
                symname[sym->udt.namelen] = '\0';
                symt_new_typedef(msc_dbg->module, symt, symname);
            }
            else FIXME("S-Udt %.*s: couldn't find type 0x%x\n", 
                       sym->udt.namelen, sym->udt.name, sym->udt.type);
            break;

        case S_UDT_32:
            symt = codeview_get_type(sym->udt32.type, FALSE);
            if (symt)
            {
                memcpy(symname, sym->udt32.name, sym->udt32.namelen);
                symname[sym->udt32.namelen] = '\0';
                symt_new_typedef(msc_dbg->module, symt, symname);
            }
            else FIXME("S-Udt %.*s: couldn't find type 0x%x\n", 
                       sym->udt32.namelen, sym->udt32.name, sym->udt32.type);
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
                const BYTE* name = (const BYTE*)sym + length;
                length += (*name + 1 + 3) & ~3;
            }
            break;
        default:
            FIXME("Unsupported id %x\n", sym->generic.id);
        }
    }

    if (curr_func) symt_normalize_function(msc_dbg->module, curr_func);

    if (linetab) HeapFree(GetProcessHeap(), 0, linetab);
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
} PDB_FILE,* PPDB_FILE;

typedef struct _PDB_HEADER
{
    CHAR        ident[40];
    DWORD       signature;
    DWORD       blocksize;
    WORD        freelist;
    WORD        total_alloc;
    PDB_FILE    toc;
    WORD        toc_block[1];
} PDB_HEADER, *PPDB_HEADER;

typedef struct _PDB_TOC
{
    DWORD       nFiles;
    PDB_FILE    file[ 1 ];
} PDB_TOC, *PPDB_TOC;

typedef struct _PDB_ROOT
{
    DWORD       version;
    DWORD       TimeDateStamp;
    DWORD       unknown;
    DWORD       cbNames;
    CHAR        names[1];
} PDB_ROOT, *PPDB_ROOT;

typedef struct _PDB_TYPES_OLD
{
    DWORD       version;
    WORD        first_index;
    WORD        last_index;
    DWORD       type_size;
    WORD        file;
    WORD        pad;
} PDB_TYPES_OLD, *PPDB_TYPES_OLD;

typedef struct _PDB_TYPES
{
    DWORD       version;
    DWORD       type_offset;
    DWORD       first_index;
    DWORD       last_index;
    DWORD       type_size;
    WORD        file;
    WORD        pad;
    DWORD       hash_size;
    DWORD       hash_base;
    DWORD       hash_offset;
    DWORD       hash_len;
    DWORD       search_offset;
    DWORD       search_len;
    DWORD       unknown_offset;
    DWORD       unknown_len;
} PDB_TYPES, *PPDB_TYPES;

typedef struct _PDB_SYMBOL_RANGE
{
    WORD        segment;
    WORD        pad1;
    DWORD       offset;
    DWORD       size;
    DWORD       characteristics;
    WORD        index;
    WORD        pad2;
} PDB_SYMBOL_RANGE, *PPDB_SYMBOL_RANGE;

typedef struct _PDB_SYMBOL_RANGE_EX
{
    WORD        segment;
    WORD        pad1;
    DWORD       offset;
    DWORD       size;
    DWORD       characteristics;
    WORD        index;
    WORD        pad2;
    DWORD       timestamp;
    DWORD       unknown;
} PDB_SYMBOL_RANGE_EX, *PPDB_SYMBOL_RANGE_EX;

typedef struct _PDB_SYMBOL_FILE
{
    DWORD       unknown1;
    PDB_SYMBOL_RANGE range;
    WORD        flag;
    WORD        file;
    DWORD       symbol_size;
    DWORD       lineno_size;
    DWORD       unknown2;
    DWORD       nSrcFiles;
    DWORD       attribute;
    CHAR        filename[1];
} PDB_SYMBOL_FILE, *PPDB_SYMBOL_FILE;

typedef struct _PDB_SYMBOL_FILE_EX
{
    DWORD       unknown1;
    PDB_SYMBOL_RANGE_EX range;
    WORD        flag;
    WORD        file;
    DWORD       symbol_size;
    DWORD       lineno_size;
    DWORD       unknown2;
    DWORD       nSrcFiles;
    DWORD       attribute;
    DWORD       reserved[2];
    CHAR        filename[1];
} PDB_SYMBOL_FILE_EX, *PPDB_SYMBOL_FILE_EX;

typedef struct _PDB_SYMBOL_SOURCE
{
    WORD        nModules;
    WORD        nSrcFiles;
    WORD        table[1];
} PDB_SYMBOL_SOURCE, *PPDB_SYMBOL_SOURCE;

typedef struct _PDB_SYMBOL_IMPORT
{
    DWORD       unknown1;
    DWORD       unknown2;
    DWORD       TimeDateStamp;
    DWORD       nRequests;
    CHAR        filename[1];
} PDB_SYMBOL_IMPORT, *PPDB_SYMBOL_IMPORT;

typedef struct _PDB_SYMBOLS_OLD
{
    WORD        hash1_file;
    WORD        hash2_file;
    WORD        gsym_file;
    WORD        pad;
    DWORD       module_size;
    DWORD       offset_size;
    DWORD       hash_size;
    DWORD       srcmodule_size;
} PDB_SYMBOLS_OLD, *PPDB_SYMBOLS_OLD;

typedef struct _PDB_SYMBOLS
{
    DWORD       signature;
    DWORD       version;
    DWORD       unknown;
    DWORD       hash1_file;
    DWORD       hash2_file;
    DWORD       gsym_file;
    DWORD       module_size;
    DWORD       offset_size;
    DWORD       hash_size;
    DWORD       srcmodule_size;
    DWORD       pdbimport_size;
    DWORD       resvd[5];
} PDB_SYMBOLS, *PPDB_SYMBOLS;
#pragma pack()

static void* pdb_read(const BYTE* image, const WORD* block_list, int size)
{
    const PDB_HEADER*   pdb = (const PDB_HEADER*)image;
    int                 i, nBlocks;
    BYTE*               buffer;

    if (!size) return NULL;

    nBlocks = (size + pdb->blocksize - 1) / pdb->blocksize;
    buffer = HeapAlloc(GetProcessHeap(), 0, nBlocks * pdb->blocksize);

    for (i = 0; i < nBlocks; i++)
        memcpy(buffer + i * pdb->blocksize,
               image + block_list[i] * pdb->blocksize, pdb->blocksize);

    return buffer;
}

static void* pdb_read_file(const BYTE* image, const PDB_TOC* toc, DWORD fileNr)
{
    const PDB_HEADER*   pdb = (const PDB_HEADER*)image;
    const WORD*         block_list;
    DWORD               i;

    if (!toc || fileNr >= toc->nFiles) return NULL;

    block_list = (const WORD*) &toc->file[toc->nFiles];
    for (i = 0; i < fileNr; i++)
        block_list += (toc->file[i].size + pdb->blocksize - 1) / pdb->blocksize;

    return pdb_read(image, block_list, toc->file[fileNr].size);
}

static void pdb_free(void* buffer)
{
    HeapFree(GetProcessHeap(), 0, buffer);
}

static void pdb_convert_types_header(PDB_TYPES* types, const BYTE* image)
{
    memset(types, 0, sizeof(PDB_TYPES));
    if (!image) return;

    if (*(const DWORD*)image < 19960000)   /* FIXME: correct version? */
    {
        /* Old version of the types record header */
        const PDB_TYPES_OLD*    old = (const PDB_TYPES_OLD*)image;
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
        *types = *(const PDB_TYPES*)image;
    }
}

static void pdb_convert_symbols_header(PDB_SYMBOLS* symbols,
                                       int* header_size, const BYTE* image)
{
    memset(symbols, 0, sizeof(PDB_SYMBOLS));
    if (!image) return;

    if (*(const DWORD*)image != 0xffffffff)
    {
        /* Old version of the symbols record header */
        const PDB_SYMBOLS_OLD*  old = (const PDB_SYMBOLS_OLD*)image;
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
        *symbols = *(const PDB_SYMBOLS*)image;
        *header_size = sizeof(PDB_SYMBOLS);
    }
}

static BOOL CALLBACK pdb_match(char* file, void* user)
{
    /* accept first file */
    return FALSE;
}

static HANDLE open_pdb_file(const struct process* pcs, struct module* module,
                            const char* filename)
{
    HANDLE      h;
    char        dbg_file_path[MAX_PATH];

    h = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    /* FIXME: should give more bits on the file to look at */
    if (h == INVALID_HANDLE_VALUE &&
        SymFindFileInPath(pcs->handle, NULL, (char*)filename, NULL, 0, 0, 0,
                          dbg_file_path, pdb_match, NULL))
    {
        h = CreateFileA(dbg_file_path, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

static SYM_TYPE pdb_process_file(const struct process* pcs, 
                                 const struct msc_debug_info* msc_dbg,
                                 const char* filename, DWORD timestamp)
{
    SYM_TYPE    sym_type = -1;
    HANDLE      hFile, hMap = NULL;
    char*       image = NULL;
    PDB_HEADER* pdb = NULL;
    PDB_TOC*    toc = NULL;
    PDB_ROOT*   root = NULL;
    char*       types_image = NULL;
    char*       symbols_image = NULL;
    PDB_TYPES   types;
    PDB_SYMBOLS symbols;
    int         header_size = 0;
    char*       modimage;
    char*       file;

    TRACE("Processing PDB file %s\n", filename);

    /*
    *  Open and map() .PDB file
     */
    if ((hFile = open_pdb_file(pcs, msc_dbg->module, filename)) == NULL ||
        ((hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) == NULL) ||
        ((image = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) == NULL))
    {
        ERR("-Unable to peruse .PDB file %s\n", filename);
        goto leave;
    }

    /*
     * Read in TOC and well-known files
     */
    pdb = (PPDB_HEADER)image;
    toc = pdb_read(image, pdb->toc_block, pdb->toc.size);
    root = pdb_read_file(image, toc, 1);
    types_image = pdb_read_file(image, toc, 2);
    symbols_image = pdb_read_file(image, toc, 3);

    pdb_convert_types_header(&types, types_image);
    pdb_convert_symbols_header(&symbols, &header_size, symbols_image);

    if (!root)
    {
        ERR("-Unable to get root from .PDB file %s\n", filename);
        goto leave;
    }

    /*
     * Check for unknown versions
     */

    switch (root->version)
    {
        case 19950623:      /* VC 4.0 */
        case 19950814:
        case 19960307:      /* VC 5.0 */
        case 19970604:      /* VC 6.0 */
            break;
        default:
            ERR("-Unknown root block version %ld\n", root->version);
    }

    switch (types.version)
    {
        case 19950410:      /* VC 4.0 */
        case 19951122:
        case 19961031:      /* VC 5.0 / 6.0 */
            break;
        default:
            ERR("-Unknown type info version %ld\n", types.version);
    }

    switch (symbols.version)
    {
        case 0:            /* VC 4.0 */
        case 19960307:     /* VC 5.0 */
        case 19970606:     /* VC 6.0 */
            break;
        default:
            ERR("-Unknown symbol info version %ld\n", symbols.version);
    }


    /* Check .PDB time stamp */
    if (root->TimeDateStamp != timestamp)
    {
        ERR("-Wrong time stamp of .PDB file %s (0x%08lx, 0x%08lx)\n",
            filename, root->TimeDateStamp, timestamp);
    }

    /* Read type table */
    codeview_parse_type_table(msc_dbg->module, types_image + types.type_offset, types.type_size);

    /* Read type-server .PDB imports */
    if (symbols.pdbimport_size)
    {
        /* FIXME */
        ERR("-Type server .PDB imports ignored!\n");
    }

    /* Read global symbol table */
    modimage = pdb_read_file(image, toc, symbols.gsym_file);
    if (modimage)
    {
        codeview_snarf(msc_dbg, modimage, 0, toc->file[symbols.gsym_file].size, NULL);
        pdb_free(modimage);
    }

    /* Read per-module symbol / linenumber tables */
    file = symbols_image + header_size;
    while (file - symbols_image < header_size + symbols.module_size)
    {
        int file_nr, file_index, symbol_size, lineno_size;
        char* file_name;

        if (symbols.version < 19970000)
        {
            PDB_SYMBOL_FILE *sym_file = (PDB_SYMBOL_FILE*) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            file_index  = sym_file->range.index;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
        }
        else
        {
            PDB_SYMBOL_FILE_EX *sym_file = (PDB_SYMBOL_FILE_EX*) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            file_index  = sym_file->range.index;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
        }

        modimage = pdb_read_file(image, toc, file_nr);
        if (modimage)
        {
            struct codeview_linetab*    linetab = NULL;

            if (lineno_size)
                linetab = codeview_snarf_linetab(msc_dbg->module, modimage + symbol_size, lineno_size);

            if (symbol_size)
                codeview_snarf(msc_dbg, modimage, sizeof(DWORD), symbol_size, linetab);

            pdb_free(modimage);
        }

        file_name += strlen(file_name) + 1;
        file = (char*)((DWORD)(file_name + strlen(file_name) + 1 + 3) & ~3);
    }

    sym_type = SymCv;

 leave:

    /* Cleanup */
    codeview_clear_type_table();

    if (symbols_image) pdb_free(symbols_image);
    if (types_image) pdb_free(types_image);
    if (root) pdb_free(root);
    if (toc) pdb_free(toc);

    if (image) UnmapViewOfFile(image);
    if (hMap) CloseHandle(hMap);
    if (hFile) CloseHandle(hFile);

    return sym_type;
}

/*========================================================================
 * Process CodeView debug information.
 */

#define CODEVIEW_NB09_SIG  ('N' | ('B' << 8) | ('0' << 16) | ('9' << 24))
#define CODEVIEW_NB10_SIG  ('N' | ('B' << 8) | ('1' << 16) | ('0' << 24))
#define CODEVIEW_NB11_SIG  ('N' | ('B' << 8) | ('1' << 16) | ('1' << 24))

typedef struct _CODEVIEW_HEADER
{
    DWORD       dwSignature;
    DWORD       lfoDirectory;
} CODEVIEW_HEADER,* PCODEVIEW_HEADER;

typedef struct _CODEVIEW_PDB_DATA
{
    DWORD       timestamp;
    DWORD       unknown;
    CHAR        name[1];
} CODEVIEW_PDB_DATA, *PCODEVIEW_PDB_DATA;

typedef struct _CV_DIRECTORY_HEADER
{
    WORD        cbDirHeader;
    WORD        cbDirEntry;
    DWORD       cDir;
    DWORD       lfoNextDir;
    DWORD       flags;
} CV_DIRECTORY_HEADER, *PCV_DIRECTORY_HEADER;

typedef struct _CV_DIRECTORY_ENTRY
{
    WORD        subsection;
    WORD        iMod;
    DWORD       lfo;
    DWORD       cb;
} CV_DIRECTORY_ENTRY, *PCV_DIRECTORY_ENTRY;

#define	sstAlignSym		0x125
#define	sstSrcModule		0x127

static SYM_TYPE codeview_process_info(const struct process* pcs, 
                                      const struct msc_debug_info* msc_dbg)
{
    const CODEVIEW_HEADER*      cv = (const CODEVIEW_HEADER*)msc_dbg->root;
    SYM_TYPE                    sym_type = -1;

    switch (cv->dwSignature)
    {
    case CODEVIEW_NB09_SIG:
    case CODEVIEW_NB11_SIG:
    {
        const CV_DIRECTORY_HEADER*      hdr = (const CV_DIRECTORY_HEADER*)(msc_dbg->root + cv->lfoDirectory);
        const CV_DIRECTORY_ENTRY*       ent;
        const CV_DIRECTORY_ENTRY*       prev;
        const CV_DIRECTORY_ENTRY*       next;
        unsigned int                    i;

        codeview_init_basic_types(msc_dbg->module);
        ent = (const CV_DIRECTORY_ENTRY*)((const BYTE*)hdr + hdr->cbDirHeader);
        for (i = 0; i < hdr->cDir; i++, ent = next)
        {
            next = (i == hdr->cDir-1)? NULL :
                   (const CV_DIRECTORY_ENTRY*)((const BYTE*)ent + hdr->cbDirEntry);
            prev = (i == 0)? NULL :
                   (const CV_DIRECTORY_ENTRY*)((const BYTE*)ent - hdr->cbDirEntry);

            if (ent->subsection == sstAlignSym)
            {
                /*
                 * Check the next and previous entry.  If either is a
                 * sstSrcModule, it contains the line number info for
                 * this file.
                 *
                 * FIXME: This is not a general solution!
                 */
                struct codeview_linetab*        linetab = NULL;

                if (next && next->iMod == ent->iMod && next->subsection == sstSrcModule)
                    linetab = codeview_snarf_linetab(msc_dbg->module, msc_dbg->root + next->lfo, next->cb);

                if (prev && prev->iMod == ent->iMod && prev->subsection == sstSrcModule)
                    linetab = codeview_snarf_linetab(msc_dbg->module, msc_dbg->root + prev->lfo, prev->cb);

                codeview_snarf(msc_dbg, msc_dbg->root + ent->lfo, sizeof(DWORD),
                               ent->cb, linetab);
            }
        }

        sym_type = SymCv;
        break;
    }

    case CODEVIEW_NB10_SIG:
    {
        const CODEVIEW_PDB_DATA* pdb = (const CODEVIEW_PDB_DATA*)(cv + 1);

        codeview_init_basic_types(msc_dbg->module);
        sym_type = pdb_process_file(pcs, msc_dbg, pdb->name, pdb->timestamp);
        break;
    }
    default:
        ERR("Unknown CODEVIEW signature %08lX in module %s\n",
            cv->dwSignature, msc_dbg->module->module.ModuleName);
        break;
    }

    return sym_type;
}

/*========================================================================
 * Process debug directory.
 */
SYM_TYPE pe_load_debug_directory(const struct process* pcs, struct module* module, 
                                 const BYTE* mapping, const IMAGE_DEBUG_DIRECTORY* dbg, 
                                 int nDbg)
{
    SYM_TYPE                    sym_type;
    int                         i;
    struct msc_debug_info       msc_dbg;
    const IMAGE_NT_HEADERS*     nth = RtlImageNtHeader((void*)mapping);

    msc_dbg.module = module;
    msc_dbg.nsect  = nth->FileHeader.NumberOfSections;
    msc_dbg.sectp  = (const IMAGE_SECTION_HEADER*)((const char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
    msc_dbg.nomap  = 0;
    msc_dbg.omapp  = NULL;

    __TRY
    {
        sym_type = -1;

        /* First, watch out for OMAP data */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC)
            {
                msc_dbg.nomap = dbg[i].SizeOfData / sizeof(OMAP_DATA);
                msc_dbg.omapp = (const OMAP_DATA*)(mapping + dbg[i].PointerToRawData);
                break;
            }
        }
  
        /* Now, try to parse CodeView debug info */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                msc_dbg.root = mapping + dbg[i].PointerToRawData;
                sym_type = codeview_process_info(pcs, &msc_dbg);
                if (sym_type == SymCv) goto done;
            }
        }
    
        /* If not found, try to parse COFF debug info */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_COFF)
            {
                msc_dbg.root = mapping + dbg[i].PointerToRawData;
                sym_type = coff_process_info(&msc_dbg);
                if (sym_type == SymCoff) goto done;
            }
        }
    done:
#if 0
	 /* FIXME: this should be supported... this is the debug information for
	  * functions compiled without a frame pointer (FPO = frame pointer omission)
	  * the associated data helps finding out the relevant information
	  */
        for (i = 0; i < nDbg; i++)
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_FPO)
                DEBUG_Printf("This guy has FPO information\n");

#define FRAME_FPO   0
#define FRAME_TRAP  1
#define FRAME_TSS   2

typedef struct _FPO_DATA 
{
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
#else
;
#endif
    }
    __EXCEPT(page_fault)
    {
        ERR("Got a page fault while loading symbols\n");
        sym_type = -1;
    }
    __ENDTRY
    return sym_type;
}
