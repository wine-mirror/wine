/*
 * File stabs.c - read stabs information from the wine executable itself.
 *
 * Copyright (C) 1996, Eric Youngdale.
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include "win.h"
#include "debugger.h"

#ifdef __ELF__
#include <elf.h>
#endif

#define N_UNDF		0x00
#define N_GSYM		0x20
#define N_FUN		0x24
#define N_STSYM		0x26
#define N_LCSYM		0x28
#define N_MAIN		0x2a
#define N_ROSYM		0x2c
#define N_OPT		0x3c
#define N_RSYM		0x40
#define N_SLINE		0x44
#define N_SO		0x64
#define N_LSYM		0x80
#define N_BINCL		0x82
#define N_SOL		0x84
#define N_PSYM		0xa0
#define N_EINCL		0xa2
#define N_LBRAC		0xc0
#define N_RBRAC		0xe0


/*
 * Set so that we know the main executable name and path.
 */
char * DEBUG_argv0;

struct stab_nlist {
  union {
    char *n_name;
    struct stab_nlist *n_next;
    long n_strx;
  } n_un;
  unsigned char n_type;
  char n_other;
  short n_desc;
  unsigned long n_value;
};

#ifdef __ELF__

int
DEBUG_ParseStabs(char * addr, Elf32_Shdr * stabsect, Elf32_Shdr * stabstr)
{
  int i;
  int ignore = FALSE;
  int nstab;
  struct stab_nlist * stab_ptr;
  char * strs;
  char * ptr;
  char * xptr;
  char currpath[PATH_MAX];
  char symname[4096];
  char * subpath = NULL;
  DBG_ADDR     new_addr;
  struct name_hash * curr_func = NULL;
  int strtabinc;

  nstab = stabsect->sh_size / sizeof(struct stab_nlist);
  stab_ptr = (struct stab_nlist *) (addr + stabsect->sh_offset);
  strs  = (char *) (addr + stabstr->sh_offset);

  memset(currpath, 0, sizeof(currpath));

  strtabinc = 0;
  for(i=0; i < nstab; i++, stab_ptr++ )
    {
      ptr = strs + (unsigned int) stab_ptr->n_un.n_name;
      switch(stab_ptr->n_type)
	{
	case N_GSYM:
	  /*
	   * These are useless.  They have no value, and you have to
	   * read the normal symbol table to get the address.  Thus we
	   * ignore them, and when we process the normal symbol table
	   * we should do the right thing.
	   */
	case N_RBRAC:
	case N_LBRAC:
	  /*
	   * We need to keep track of these so we get symbol scoping
	   * right for local variables.  For now, we just ignore them.
	   * The hooks are already there for dealing with this however,
	   * so all we need to do is to keep count of the nesting level,
	   * and find the RBRAC for each matching LBRAC.
	   */
	  break;
	case N_LCSYM:
	case N_STSYM:
	  /*
	   * These are static symbols and BSS symbols.
	   */
	  new_addr.seg = 0;
	  new_addr.off = stab_ptr->n_value;

	  strcpy(symname, ptr);
	  xptr = strchr(symname, ':');
	  if( xptr != NULL )
	    {
	      *xptr = '\0';
	    }
	  DEBUG_AddSymbol( symname, &new_addr, currpath );
	  break;
	case N_PSYM:
	  /*
	   * These are function parameters.
	   */
	  if(     (curr_func != NULL)
	       && (stab_ptr->n_value != 0) )
	    {
	      strcpy(symname, ptr);
	      xptr = strchr(symname, ':');
	      if( xptr != NULL )
		{
		  *xptr = '\0';
		}
	      DEBUG_AddLocal(curr_func, 0, 
			     stab_ptr->n_value, 0, 0, symname);
	    }
	  break;
	case N_RSYM:
	  if( curr_func != NULL )
	    {
	      strcpy(symname, ptr);
	      xptr = strchr(symname, ':');
	      if( xptr != NULL )
		{
		  *xptr = '\0';
		}
	      DEBUG_AddLocal(curr_func, stab_ptr->n_value, 0, 0, 0, symname);
	    }
	  break;
	case N_LSYM:
	  if(     (curr_func != NULL)
	       && (stab_ptr->n_value != 0) )
	    {
	      strcpy(symname, ptr);
	      xptr = strchr(symname, ':');
	      if( xptr != NULL )
		{
		  *xptr = '\0';
		}
	      DEBUG_AddLocal(curr_func, 0, 
			     stab_ptr->n_value, 0, 0, symname);
	    }
	  break;
	case N_SLINE:
	  /*
	   * This is a line number.  These are always relative to the start
	   * of the function (N_FUN), and this makes the lookup easier.
	   */
	  if( curr_func != NULL )
	    {
	      DEBUG_AddLineNumber(curr_func, stab_ptr->n_desc, 
				  stab_ptr->n_value);
	    }
	  break;
	case N_FUN:
	  /*
	   * For now, just declare the various functions.  Later
	   * on, we will add the line number information and the
	   * local symbols.
	   */
	  if( !ignore )
	    {
	      new_addr.seg = 0;
	      new_addr.off = stab_ptr->n_value;
	      /*
	       * Copy the string to a temp buffer so we
	       * can kill everything after the ':'.  We do
	       * it this way because otherwise we end up dirtying
	       * all of the pages related to the stabs, and that
	       * sucks up swap space like crazy.
	       */
	      strcpy(symname, ptr);
	      xptr = strchr(symname, ':');
	      if( xptr != NULL )
		{
		  *xptr = '\0';
		}
	      curr_func = DEBUG_AddSymbol( symname, &new_addr, currpath );
	    }
	  else
	    {
	      /*
	       * Don't add line number information for this function
	       * any more.
	       */
	      curr_func = NULL;
	    }
	  break;
	case N_SO:
	  /*
	   * This indicates a new source file.  Append the records
	   * together, to build the correct path name.
	   */
	  if( *ptr == '\0' )
	    {
	      /*
	       * Nuke old path.
	       */
	      currpath[0] = '\0';
	      curr_func = NULL;
	    }
	  else
	    {
	      strcat(currpath, ptr);
	      subpath = ptr;
	    }
	  break;
	case N_SOL:
	  /*
	   * This indicates we are including stuff from an include file.
	   * If this is the main source, enable the debug stuff, otherwise
	   * ignore it.
	   */
	  if( subpath == NULL || strcmp(ptr, subpath) == 0 )
	    {
	      ignore = FALSE;
	    }
	  else
	    {
	      ignore = TRUE;
	      curr_func = NULL;
	    }
	  break;
	case N_UNDF:
	  strs += strtabinc;
	  strtabinc = stab_ptr->n_value;
	  curr_func = NULL;
	  break;
	case N_OPT:
	  /*
	   * Ignore this.  We don't care what it points to.
	   */
	  break;
	case N_BINCL:
	case N_EINCL:
	case N_MAIN:
	  /*
	   * Always ignore these.  GCC doesn't even generate them.
	   */
	  break;
	default:
	  break;
	}
#if 0
      fprintf(stderr, "%d %x %s\n", stab_ptr->n_type, 
	      (unsigned int) stab_ptr->n_value,
	      strs + (unsigned int) stab_ptr->n_un.n_name);
#endif
    }
  return TRUE;
}

int
DEBUG_ReadExecutableDbgInfo(void)
{
  int			rtn = FALSE;
  char		      * exe_name;
  struct stat		statbuf;
  int			fd = -1;
  int			status;
  char		      * addr = (char *) 0xffffffff;
  Elf32_Ehdr	      * ehptr;
  Elf32_Shdr	      * spnt;
  char		      * shstrtab;
  int			nsect;
  int			i;
  int			stabsect;
  int			stabstrsect;

  exe_name = DEBUG_argv0;

  /*
   * Make sure we can stat and open this file.
   */
  if( exe_name == NULL )
    {
      goto leave;
    }

  status = stat(exe_name, &statbuf);
  if( status == -1 )
    {
      goto leave;
    }

  /*
   * Now open the file, so that we can mmap() it.
   */
  fd = open(exe_name, O_RDONLY);
  if( fd == -1 )
    {
      goto leave;
    }


  /*
   * Now mmap() the file.
   */
  addr = mmap(0, statbuf.st_size, PROT_READ, 
	      MAP_PRIVATE, fd, 0);

  /*
   * Next, we need to find a few of the internal ELF headers within
   * this thing.  We need the main executable header, and the section
   * table.
   */
  ehptr = (Elf32_Ehdr *) addr;
  spnt = (Elf32_Shdr *) (addr + ehptr->e_shoff);
  nsect = ehptr->e_shnum;
  shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

  stabsect = stabstrsect = -1;

  for(i=0; i < nsect; i++)
    {
      if( strcmp(shstrtab + spnt[i].sh_name, ".stab") == 0 )
	{
	  stabsect = i;
	}

      if( strcmp(shstrtab + spnt[i].sh_name, ".stabstr") == 0 )
	{
	  stabstrsect = i;
	}
    }

  if( stabsect == -1 || stabstrsect == -1 )
    {
      goto leave;
    }

  /*
   * OK, now just parse all of the stabs.
   */
  rtn = DEBUG_ParseStabs(addr, spnt + stabsect, spnt + stabstrsect);

leave:

  if( addr != (char *) 0xffffffff )
    {
      munmap(addr, statbuf.st_size);
    }

  if( fd != -1 )
    {
      close(fd);
    }

  return (rtn);

}

#endif  /* __ELF__ */
