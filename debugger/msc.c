/*
 * File msc.c - read VC++ debug information from COFF and eventually
 * from PDB files.
 *
 * Copyright (C) 1996, Eric Youngdale.
 *
 * Note - this handles reading debug information for 32 bit applications
 * that run under Windows-NT for example.  I doubt that this would work well
 * for 16 bit applications, but I don't think it really matters since the
 * file format is different, and we should never get in here in such cases.
 */

#include <stdio.h>


#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <strings.h>
#include <unistd.h>
#include <malloc.h>

#include "win.h"
#include "pe_image.h"
#include "debugger.h"
#include "peexe.h"
#include "xmalloc.h"

/*
 * For the type CODEVIEW debug directory entries, the debug directory
 * points to a structure like this.  The cv_name field is the name
 * of an external .PDB file.
 */
struct CodeViewDebug
{
	char		    cv_nbtype[8];
	unsigned int	    cv_timestamp;
	char		    cv_unknown[4];
	char		    cv_name[1];
};

struct MiscDebug {
    unsigned int       DataType;
    unsigned int       Length;
    char	       Unicode;
    char	       Reserved[3];
    char	       Data[1];
};

/*
 * This is the header that the COFF variety of debug header points to.
 */
struct CoffDebug {
    unsigned int   N_Sym;
    unsigned int   SymbolOffset;
    unsigned int   N_Linenum;
    unsigned int   LinenumberOffset;
    unsigned int   Unused[4];
};

struct CoffLinenum {
	unsigned int	VirtualAddr;
	unsigned int	Linenum;
};

struct CoffFiles {
	unsigned int	startaddr;
	unsigned int	endaddr;
	char	      * filename;
};


struct CoffSymbol {
    union {
        char    ShortName[8];
        struct {
            unsigned int   NotLong;
            unsigned int   StrTaboff;
        } Name;
    } N;
    unsigned int   Value;
    short	   SectionNumber;
    short	   Type;
    char	   StorageClass;
    unsigned char  NumberOfAuxSymbols;
};

struct CoffAuxSection{
  unsigned int   Length;
  unsigned short NumberOfRelocations;
  unsigned short NumberOfLinenumbers;
  unsigned int   CheckSum;
  short          Number;
  char           Selection;
} Section;

struct deferred_debug_info
{
	struct deferred_debug_info	* next;
	char				* load_addr;
	char				* dbg_info;
	int				  dbg_size;
	struct PE_Debug_dir		* dbgdir;
	struct pe_data			* pe;
};

struct deferred_debug_info * dbglist = NULL;

/*
 * A simple macro that tells us whether a given COFF symbol is a
 * function or not.
 */
#define N_TMASK                             0x0030
#define IMAGE_SYM_DTYPE_FUNCTION            2
#define N_BTSHFT                            4
#define ISFCN(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT))


/*
 * This is what we are looking for in the COFF symbols.
 */
#define IMAGE_SYM_CLASS_EXTERNAL            0x2
#define IMAGE_SYM_CLASS_STATIC              0x3
#define IMAGE_SYM_CLASS_FILE                0x67

/*
 * In this function, we keep track of deferred debugging information
 * that we may need later if we were to need to use the internal debugger.
 * We don't fully process it here for performance reasons.
 */
int
DEBUG_RegisterDebugInfo(int fd, struct pe_data * pe, 
			int load_addr, u_long v_addr, u_long size)
{
  int			  rtn = FALSE;
  struct PE_Debug_dir	* dbgptr;
  struct deferred_debug_info * deefer;

  dbgptr = (struct PE_Debug_dir *) (load_addr + v_addr);
  for(; size > 0; size -= sizeof(*dbgptr), dbgptr++ )
    {
      switch(dbgptr->type)
	{
	case IMAGE_DEBUG_TYPE_COFF:
	case IMAGE_DEBUG_TYPE_CODEVIEW:
	case IMAGE_DEBUG_TYPE_MISC:
	  /*
	   * This is usually an indirection to a .DBG file.
	   * This is similar to (but a slightly older format) from the
	   * PDB file.
	   *
	   * First check to see if the image was 'stripped'.  If so, it
	   * means that this entry points to a .DBG file.  Otherwise,
	   * it just points to itself, and we can ignore this.
	   */
	  if(    (dbgptr->type == IMAGE_DEBUG_TYPE_MISC)
	      && (pe->pe_header->coff.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) == 0 )
	    {
	      break;
	    }

	  deefer = (struct deferred_debug_info *) xmalloc(sizeof(*deefer));
	  deefer->pe = pe;

	  deefer->dbg_info = NULL;
	  deefer->dbg_size = 0;

	  /*
	   * Read the important bits.  What we do after this depends
	   * upon the type, but this is always enough so we are able
	   * to proceed if we know what we need to do next.
	   */
	  deefer->dbg_size = dbgptr->dbgsize;
	  deefer->dbg_info = (char *) xmalloc(dbgptr->dbgsize);
	  lseek(fd, dbgptr->dbgoff, SEEK_SET);
	  read(fd, deefer->dbg_info, deefer->dbg_size);

	  deefer->load_addr = (char *) load_addr;
	  deefer->dbgdir = dbgptr;
	  deefer->next = dbglist;
	  dbglist = deefer;
	  break;
	default:
	}
    }

  return (rtn);

}

/*
 * Process COFF debugging information embedded in a Win32 application.
 *
 * FIXME - we need to process the source file information and the line
 * numbers.
 */
static
int
DEBUG_ProcessCoff(struct deferred_debug_info * deefer)
{
  struct CoffAuxSection * aux;
  struct CoffDebug   * coff;
  struct CoffSymbol  * coff_sym;
  struct CoffSymbol  * coff_symbol;
  struct CoffLinenum * coff_linetab;
  char		     * coff_strtab;
  int		       i;
  DBG_ADDR	       new_addr;
  int		       rtn = FALSE;
  int		       naux;
  char		       namebuff[9];
  char		     * nampnt;
  int		       nfiles = 0;
  int		       nfiles_alloc = 0;
  struct CoffFiles   * coff_files = NULL;
  struct CoffFiles   * curr_file = NULL;
  char		     * this_file;
  int		       j;

  coff = (struct CoffDebug *) deefer->dbg_info;

  coff_symbol = (struct CoffSymbol *) ((unsigned int) coff + coff->SymbolOffset);
  coff_linetab = (struct CoffLinenum *) ((unsigned int) coff + coff->LinenumberOffset);
  coff_strtab = (char *) ((unsigned int) coff_symbol + 18*coff->N_Sym);

  for(i=0; i < coff->N_Sym; i++ )
    {
      /*
       * We do this because some compilers (i.e. gcc) incorrectly
       * pad the structure up to a 4 byte boundary.  The structure
       * is really only 18 bytes long, so we have to manually make sure
       * we get it right.
       *
       * FIXME - there must be a way to have autoconf figure out the
       * correct compiler option for this.  If it is always gcc, that
       * makes life simpler, but I don't want to force this.
       */
      coff_sym = (struct CoffSymbol *) ((unsigned int) coff_symbol + 18*i);
      naux = coff_sym->NumberOfAuxSymbols;

      if( coff_sym->StorageClass == IMAGE_SYM_CLASS_FILE )
	{
	  if( nfiles + 1 >= nfiles_alloc )
	    {
	      nfiles_alloc += 10;
	      coff_files = (struct CoffFiles *) realloc( coff_files,
			nfiles_alloc * sizeof(struct CoffFiles));
	    }
	  curr_file = coff_files + nfiles;
	  nfiles++;
	  curr_file->startaddr = 0xffffffff;
	  curr_file->endaddr   = 0;
	  curr_file->filename =  ((char *) coff_sym) + 18;
	}

      /*
       * This guy marks the size and location of the text section
       * for the current file.  We need to keep track of this so
       * we can figure out what file the different global functions
       * go with.
       */
      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC)
	  && (naux != 0)
	  && (coff_sym->SectionNumber == 1) )
	{
	  aux = (struct CoffAuxSection *) ((unsigned int) coff_sym + 18);
	  if( curr_file->startaddr > coff_sym->Value )
	    {
	      curr_file->startaddr = coff_sym->Value;
	    }
	  
	  if( curr_file->startaddr > coff_sym->Value )
	    {
	      curr_file->startaddr = coff_sym->Value;
	    }
	  
	  if( curr_file->endaddr < coff_sym->Value + aux->Length )
	    {
	      curr_file->endaddr = coff_sym->Value + aux->Length;
	    }
	  
	}

      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC)
	  && (naux == 0)
	  && (coff_sym->SectionNumber == 1) )
	{
	  /*
	   * This is a normal static function when naux == 0.
	   * Just register it.  The current file is the correct
	   * one in this instance.
	   */
	  if( coff_sym->N.Name.NotLong )
	    {
	      memcpy(namebuff, coff_sym->N.ShortName, 8);
	      namebuff[8] = '\0';
	      nampnt = &namebuff[0];
	    }
	  else
	    {
	      nampnt = coff_strtab + coff_sym->N.Name.StrTaboff;
	    }

	  new_addr.seg = 0;
	  new_addr.off = (int) (deefer->load_addr + coff_sym->Value);
	  DEBUG_AddSymbol( nampnt, &new_addr, curr_file->filename );
	}

      if(    (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL)
	  && ISFCN(coff_sym->Type)
          && (coff_sym->SectionNumber > 0) )
	{
	  if( coff_sym->N.Name.NotLong )
	    {
	      memcpy(namebuff, coff_sym->N.ShortName, 8);
	      namebuff[8] = '\0';
	      nampnt = &namebuff[0];
	    }
	  else
	    {
	      nampnt = coff_strtab + coff_sym->N.Name.StrTaboff;
	    }

	  new_addr.seg = 0;
	  new_addr.off = (int) (deefer->load_addr + coff_sym->Value);

#if 0
	  fprintf(stderr, "%d: %x %s\n", i, new_addr.off, nampnt);
#endif

	  /*
	   * Now we need to figure out which file this guy belongs to.
	   */
	  this_file = NULL;
	  for(j=0; j < nfiles; j++)
	    {
	      if( coff_files[j].startaddr <= coff_sym->Value
		  && coff_files[j].endaddr > coff_sym->Value )
		{
		  this_file = coff_files[j].filename;
		  break;
		}
	    }
	  DEBUG_AddSymbol( nampnt, &new_addr, this_file );
	}
	  
      /*
       * For now, skip past the aux entries.
       */
      i += naux;
      
    }
    
  rtn = TRUE;

  if( coff_files != NULL )
    {
      free(coff_files);
    }

  return (rtn);

}

int
DEBUG_ProcessDeferredDebug()
{
  struct deferred_debug_info * deefer;
  struct CodeViewDebug	     * cvd;
  struct MiscDebug	     * misc;

  for(deefer = dbglist; deefer; deefer = deefer->next)
    {
      switch(deefer->dbgdir->type)
	{
	case IMAGE_DEBUG_TYPE_COFF:
	  /*
	   * Standard COFF debug information that VC++ adds when you
	   * use /debugtype:both with the linker.
	   */
#if 0
	  fprintf(stderr, "Processing COFF symbols...\n");
#endif
	  DEBUG_ProcessCoff(deefer);
	  break;
	case IMAGE_DEBUG_TYPE_CODEVIEW:
	  /*
	   * This is a pointer to a PDB file of some sort.
	   */
	  cvd = (struct CodeViewDebug *) deefer->dbg_info;
#if 0
	  fprintf(stderr, "Processing PDB file %s\n", cvd->cv_name);
#endif
	  break;
	case IMAGE_DEBUG_TYPE_MISC:
	  /*
	   * A pointer to a .DBG file of some sort.
	   */
	  misc = (struct MiscDebug *) deefer->dbg_info;
#if 0
	  fprintf(stderr, "Processing DBG file %s\n", misc->Data);
#endif
	  break;
	default:
	  /*
	   * We should never get here...
	   */
	  break;
	}
    }
  return TRUE;
}
