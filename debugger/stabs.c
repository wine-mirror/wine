/*
 * File stabs.c - read stabs information from the wine executable itself.
 *
 * Copyright (C) 1996, Eric Youngdale.
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#include "win.h"
#include "debugger.h"
#include "xmalloc.h"

#ifdef __svr4__
#define __ELF__
#endif

#ifdef __ELF__
#include <elf.h>
#include <link.h>
#include <sys/mman.h>
#elif defined(__EMX__)
#include <a_out.h>
#else
#include <a.out.h>
#endif

#ifndef N_UNDF
#define N_UNDF		0x00
#endif

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
 * This is how we translate stab types into our internal representations
 * of datatypes.
 */
static struct datatype ** stab_types = NULL;
static int num_stab_types = 0;

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

/*
 * This is used to keep track of known datatypes so that we don't redefine
 * them over and over again.  It sucks up lots of memory otherwise.
 */
struct known_typedef
{
  struct known_typedef * next;
  char		       * name;
  int			 ndefs;
  struct datatype      * types[0];
};

#define NR_STAB_HASH 521

struct known_typedef * ktd_head[NR_STAB_HASH];

static unsigned int stab_hash( const char * name )
{
    unsigned int hash = 0;
    unsigned int tmp;
    const char * p;

    p = name;

    while (*p) 
      {
	hash = (hash << 4) + *p++;

	if( (tmp = (hash & 0xf0000000)) )
	  {
	    hash ^= tmp >> 24;
	  }
	hash &= ~tmp;
      }
    return hash % NR_STAB_HASH;
}


static void stab_strcpy(char * dest, const char * source)
{
  /*
   * A strcpy routine that stops when we hit the ':' character.
   * Faster than copying the whole thing, and then nuking the
   * ':'.
   */
  while(*source != '\0' && *source != ':')
    {
      *dest++ = *source++;
    }
  *dest++ = '\0';
}

#define MAX_TD_NESTING	128

static
int
DEBUG_RegisterTypedef(const char * name, struct datatype ** types, int ndef)
{
  int			 hash;
  struct known_typedef * ktd;

  if( ndef == 1 )
    {
      return TRUE;
    }

  ktd = (struct known_typedef *) malloc(sizeof(struct known_typedef) 
					+ ndef * sizeof(struct datatype *));
  
  hash = stab_hash(name);

  ktd->name = xstrdup(name);
  ktd->ndefs = ndef;
  memcpy(&ktd->types[0], types, ndef * sizeof(struct datatype *));
  ktd->next = ktd_head[hash];
  ktd_head[hash] = ktd;

  return TRUE;
}

static
int
DEBUG_HandlePreviousTypedef(const char * name, const char * stab)
{
  int			 count;
  enum debug_type	 expect;
  int			 hash;
  struct known_typedef * ktd;
  char		       * ptr;
  char		       * tc;
  int			 typenum;

  hash = stab_hash(name);

  for(ktd = ktd_head[hash]; ktd; ktd = ktd->next)
    {
      if(    (ktd->name[0] == name[0])
	  && (strcmp(name, ktd->name) == 0) )
	{
	  break;
	}
    }

  /*
   * Didn't find it.  This must be a new one.
   */
  if( ktd == NULL )
    {
      return FALSE;
    }

  /*
   * Examine the stab to make sure it has the same number of definitions.
   */
  count = 0;
  for(ptr = strchr(stab, '='); ptr; ptr = strchr(ptr+1, '='))
    {
      if( count >= ktd->ndefs )
	{
	  return FALSE;
	}

      /*
       * Make sure the types of all of the objects is consistent with
       * what we have already parsed.
       */
      switch(ptr[1])
	{
	case '*':
	  expect = POINTER;
	  break;
	case 's':
	case 'u':
	  expect = STRUCT;
	  break;
	case 'a':
	  expect = ARRAY;
	  break;
	case '1':
	case 'r':
	  expect = BASIC;
	  break;
	case 'x':
	  expect = STRUCT;
	  break;
	case 'e':
	  expect = ENUM;
	  break;
	case 'f':
	  expect = FUNC;
	  break;
	default:
	  fprintf(stderr, "Unknown type.\n");
          return FALSE;
	}
      if( expect != DEBUG_GetType(ktd->types[count]) )
	{
	  return FALSE;
	}
      count++;
    }

  if( ktd->ndefs != count )
    {
      return FALSE;
    }

  /*
   * OK, this one is safe.  Go through, dig out all of the type numbers,
   * and substitute the appropriate things.
   */
  count = 0;
  for(ptr = strchr(stab, '='); ptr; ptr = strchr(ptr+1, '='))
    {
      /*
       * Back up until we get to a non-numeric character.  This is the type
       * number.
       */
      tc = ptr - 1;
      while( *tc >= '0' && *tc <= '9' )
	{
	  tc--;
	}
      
      typenum = atol(tc + 1);
      if( num_stab_types <= typenum )
	{
	  num_stab_types = typenum + 32;
	  stab_types = (struct datatype **) xrealloc(stab_types, 
						     num_stab_types * sizeof(struct datatype *));
	  if( stab_types == NULL )
	    {
	      return FALSE;
	    }
	}

      stab_types[typenum] = ktd->types[count++];
    }

  return TRUE;
}

static int DEBUG_FreeRegisteredTypedefs()
{
  int			 count;
  int			 j;
  struct known_typedef * ktd;
  struct known_typedef * next;

  count = 0;
  for(j=0; j < NR_STAB_HASH; j++ )
    {
      for(ktd = ktd_head[j]; ktd; ktd = next)
	{
	  count++;
	  next = ktd->next;
	  free(ktd->name);
	  free(ktd);
	}  
      ktd_head[j] = NULL;
    }

  return TRUE;

}

static 
int
DEBUG_ParseTypedefStab(char * ptr, const char * typename)
{
  int		    arrmax;
  int		    arrmin;
  char		  * c;
  struct datatype * curr_type;
  struct datatype * datatype;
  struct datatype * curr_types[MAX_TD_NESTING];
  char	            element_name[1024];
  int		    ntypes = 0;
  int		    offset;
  const char	  * orig_typename;
  int		    rtn = FALSE;
  int		    size;
  char		  * tc;
  char		  * tc2;
  int		    typenum;

  orig_typename = typename;

  if( DEBUG_HandlePreviousTypedef(typename, ptr) == TRUE )
    {
      return TRUE;
    }

  /* 
   * Go from back to front.  First we go through and figure out what
   * type numbers we need, and register those types.  Then we go in
   * and fill the details.  
   */

  for( c = strchr(ptr, '='); c != NULL; c = strchr(c + 1, '=') )
    {
      /*
       * Back up until we get to a non-numeric character.  This is the type
       * number.
       */
      tc = c - 1;
      while( *tc >= '0' && *tc <= '9' )
	{
	  tc--;
	}
      typenum = atol(tc + 1);
      if( num_stab_types <= typenum )
	{
	  num_stab_types = typenum + 32;
	  stab_types = (struct datatype **) xrealloc(stab_types, 
						     num_stab_types * sizeof(struct datatype *));
	  if( stab_types == NULL )
	    {
	      goto leave;
	    }
	}

      if( ntypes >= MAX_TD_NESTING )
	{
	  /*
	   * If this ever happens, just bump the counter.
	   */
	  fprintf(stderr, "Typedef nesting overflow\n");
	  return FALSE;
	}

      switch(c[1])
	{
	case '*':
	  stab_types[typenum] = DEBUG_NewDataType(POINTER, NULL);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	case 's':
	case 'u':
	  stab_types[typenum] = DEBUG_NewDataType(STRUCT, typename);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	case 'a':
	  stab_types[typenum] = DEBUG_NewDataType(ARRAY, NULL);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	case '1':
	case 'r':
	  stab_types[typenum] = DEBUG_NewDataType(BASIC, typename);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	case 'x':
	  stab_strcpy(element_name, c + 3);
	  stab_types[typenum] = DEBUG_NewDataType(STRUCT, element_name);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	case 'e':
	  stab_types[typenum] = DEBUG_NewDataType(ENUM, NULL);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	case 'f':
	  stab_types[typenum] = DEBUG_NewDataType(FUNC, NULL);
	  curr_types[ntypes++] = stab_types[typenum];
	  break;
	default:
	  fprintf(stderr, "Unknown type.\n");
	}
      typename = NULL;
    }

      /*
      * Now register the type so that if we encounter it again, we will know
      * what to do.
      */
     DEBUG_RegisterTypedef(orig_typename, curr_types, ntypes);

     /* 
      * OK, now take a second sweep through.  Now we will be digging
      * out the definitions of the various components, and storing
      * them in the skeletons that we have already allocated.  We take
      * a right-to left search as this is much easier to parse.  
      */
     for( c = strrchr(ptr, '='); c != NULL; c = strrchr(ptr, '=') )
       {
	 /*
	  * Back up until we get to a non-numeric character.  This is the type
	  * number.
	  */
	 tc = c - 1;
	 while( *tc >= '0' && *tc <= '9' )
	   {
	     tc--;
	   }
	 typenum = atol(tc + 1);
	 curr_type = stab_types[typenum];
	 
	 switch(c[1])
	   {
	   case 'x':
	     tc = c + 3;
	     while( *tc != ':' )
	       {
		 tc ++;
	       }
	     tc++;
	     if( *tc == '\0' )
	       {
		 *c = '\0';
	       }
	     else
	       {
		 strcpy(c, tc);
	       }
	     
	     break;
	   case '*':
	   case 'f':
	     tc = c + 2;
	     datatype = stab_types[strtol(tc, &tc, 10)];
	     DEBUG_SetPointerType(curr_type, datatype);
	     if( *tc == '\0' )
	       {
		 *c = '\0';
	       }
	     else
	       {
		 strcpy(c, tc);
	       }
	     break;
	   case '1':
	   case 'r':
	     /*
	      * We have already handled these above.
	      */
	     *c = '\0';
	     break;
	   case 'a':
	     tc  = c + 5;
	     arrmin = strtol(tc, &tc, 10);
	     tc++;
	     arrmax = strtol(tc, &tc, 10);
	     tc++;
	     datatype = stab_types[strtol(tc, &tc, 10)];
	     if( *tc == '\0' )
	       {
		 *c = '\0';
	       }
	     else
	       {
		 strcpy(c, tc);
	       }
	     
	     DEBUG_SetArrayParams(curr_type, arrmin, arrmax, datatype);
	     break;
	   case 's':
	   case 'u':
	     tc = c + 2;
	     if( DEBUG_SetStructSize(curr_type, strtol(tc, &tc, 10)) == FALSE )
	       {
		 /*
		  * We have already filled out this structure.  Nothing to do,
		  * so just skip forward to the end of the definition.
		  */
		 while( tc[0] != ';' && tc[1] != ';' )
		   {
		     tc++;
		   }
		 
		 tc += 2;
		 
		 if( *tc == '\0' )
		   {
		     *c = '\0';
		   }
		 else
		   {
		     strcpy(c, tc + 1);
		   }
		 continue;
	       }

	     /*
	      * Now parse the individual elements of the structure/union.
	      */
	     while(*tc != ';')
	       {
		 tc2 = element_name;
		 while(*tc != ':')
		   {
		     *tc2++ = *tc++;
		   }
		 tc++;
		 *tc2++ = '\0';
		 datatype = stab_types[strtol(tc, &tc, 10)];
		 tc++;
		 offset  = strtol(tc, &tc, 10);
		 tc++;
		 size  = strtol(tc, &tc, 10);
		 tc++;
		 DEBUG_AddStructElement(curr_type, element_name, datatype, offset, size);
	       }
	     if( *tc == '\0' )
	       {
		 *c = '\0';
	       }
	     else
	       {
		 strcpy(c, tc + 1);
	       }
	     break;
	   case 'e':
	     tc = c + 2;
	     /*
	      * Now parse the individual elements of the structure/union.
	      */
	     while(*tc != ';')
	       {
		 tc2 = element_name;
		 while(*tc != ':')
		   {
		     *tc2++ = *tc++;
		   }
		 tc++;
		 *tc2++ = '\0';
		 offset  = strtol(tc, &tc, 10);
		 tc++;
		 DEBUG_AddStructElement(curr_type, element_name, NULL, offset, 0);
	       }
	     if( *tc == '\0' )
	       {
		 *c = '\0';
	       }
	     else
	       {
		 strcpy(c, tc + 1);
	       }
	     break;
	   default:
	     fprintf(stderr, "Unknown type.\n");
	     break;
	   }
       }
     
     rtn = TRUE;
     
leave:
     
     return rtn;

}

static struct datatype *
DEBUG_ParseStabType(const char * stab)
{
  char * c;
  int    typenum;

  /*
   * Look through the stab definition, and figure out what datatype
   * this represents.  If we have something we know about, assign the
   * type.
   */
  c = strchr(stab, ':');
  if( c == NULL )
    {
      return NULL;
    }

  c++;
  /*
   * The next character says more about the type (i.e. data, function, etc)
   * of symbol.  Skip it.
   */
  c++;

  typenum = atol(c);

  if( typenum < num_stab_types && stab_types[typenum] != NULL )
    {
      return stab_types[typenum];
    }

  return NULL;
}

static 
int
DEBUG_ParseStabs(char * addr, unsigned int load_offset,
		 unsigned int staboff, int stablen, 
		 unsigned int strtaboff, int strtablen)
{
  struct name_hash    * curr_func = NULL;
  struct wine_locals  * curr_loc = NULL;
  struct name_hash    * curr_sym = NULL;
  char			currpath[PATH_MAX];
  int			i;
  int			ignore = FALSE;
  int			last_nso = -1;
  int			len;
  DBG_ADDR		new_addr;
  int			nstab;
  char		      * ptr;
  char		      * stabbuff;
  int			stabbufflen;
  struct stab_nlist   * stab_ptr;
  char		      * strs;
  int			strtabinc;
  char		      * subpath = NULL;
  char			symname[4096];

  nstab = stablen / sizeof(struct stab_nlist);
  stab_ptr = (struct stab_nlist *) (addr + staboff);
  strs  = (char *) (addr + strtaboff);

  memset(currpath, 0, sizeof(currpath));

  /*
   * Allocate a buffer into which we can build stab strings for cases
   * where the stab is continued over multiple lines.
   */
  stabbufflen = 65536;
  stabbuff = (char *) xmalloc(stabbufflen);
  if( stabbuff == NULL )
    {
      goto leave;
    }

  strtabinc = 0;
  stabbuff[0] = '\0';
  for(i=0; i < nstab; i++, stab_ptr++ )
    {
      ptr = strs + (unsigned int) stab_ptr->n_un.n_name;
      if( ptr[strlen(ptr) - 1] == '\\' )
	{
	  /*
	   * Indicates continuation.  Append this to the buffer, and go onto the
	   * next record.  Repeat the process until we find a stab without the
	   * '/' character, as this indicates we have the whole thing.
	   */
	  len = strlen(ptr);
	  if( strlen(stabbuff) + len > stabbufflen )
	    {
	      stabbufflen += 65536;
	      stabbuff = (char *) xrealloc(stabbuff, stabbufflen);
	      if( stabbuff == NULL )
		{
		  goto leave;
		}
	    }
	  strncat(stabbuff, ptr, len - 1);
	  continue;
	}
      else if( stabbuff[0] != '\0' )
	{
	  strcat( stabbuff, ptr);
	  ptr = stabbuff;
	}

      if( strchr(ptr, '=') != NULL )
	{
	  /*
	   * The stabs aren't in writable memory, so copy it over so we are
	   * sure we can scribble on it.
	   */
	  if( ptr != stabbuff )
	    {
	      strcpy(stabbuff, ptr);
	      ptr = stabbuff;
	    }
	  stab_strcpy(symname, ptr);
	  DEBUG_ParseTypedefStab(ptr, symname);
	}

      switch(stab_ptr->n_type)
	{
	case N_GSYM:
	  /*
	   * These are useless with ELF.  They have no value, and you have to
	   * read the normal symbol table to get the address.  Thus we
	   * ignore them, and when we process the normal symbol table
	   * we should do the right thing.
	   *
	   * With a.out, they actually do make some amount of sense.
	   */
	  new_addr.seg = 0;
	  new_addr.type = DEBUG_ParseStabType(ptr);
	  new_addr.off = load_offset + stab_ptr->n_value;

	  stab_strcpy(symname, ptr);
#ifdef __ELF__
	  curr_sym = DEBUG_AddSymbol( symname, &new_addr, currpath,
				      SYM_WINE | SYM_DATA | SYM_INVALID);
#else
	  curr_sym = DEBUG_AddSymbol( symname, &new_addr, currpath, 
				      SYM_WINE | SYM_DATA );
#endif
	  break;
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
	  new_addr.type = DEBUG_ParseStabType(ptr);
	  new_addr.off = load_offset + stab_ptr->n_value;

	  stab_strcpy(symname, ptr);
	  curr_sym = DEBUG_AddSymbol( symname, &new_addr, currpath, 
				      SYM_WINE | SYM_DATA );
	  break;
	case N_PSYM:
	  /*
	   * These are function parameters.
	   */
	  if(     (curr_func != NULL)
	       && (stab_ptr->n_value != 0) )
	    {
	      stab_strcpy(symname, ptr);
	      curr_loc = DEBUG_AddLocal(curr_func, 0, 
					stab_ptr->n_value, 0, 0, symname);
	      DEBUG_SetLocalSymbolType( curr_loc, DEBUG_ParseStabType(ptr));
	    }
	  break;
	case N_RSYM:
	  if( curr_func != NULL )
	    {
	      stab_strcpy(symname, ptr);
	      curr_loc = DEBUG_AddLocal(curr_func, stab_ptr->n_value, 0, 0, 0, symname);
	      DEBUG_SetLocalSymbolType( curr_loc, DEBUG_ParseStabType(ptr));
	    }
	  break;
	case N_LSYM:
	  if(     (curr_func != NULL)
	       && (stab_ptr->n_value != 0) )
	    {
	      stab_strcpy(symname, ptr);
	      DEBUG_AddLocal(curr_func, 0, 
			     stab_ptr->n_value, 0, 0, symname);
	    }
	  else if (curr_func == NULL)
	    {
	      stab_strcpy(symname, ptr);
	    }
	  break;
	case N_SLINE:
	  /*
	   * This is a line number.  These are always relative to the start
	   * of the function (N_FUN), and this makes the lookup easier.
	   */
	  if( curr_func != NULL )
	    {
#ifdef __ELF__
	      DEBUG_AddLineNumber(curr_func, stab_ptr->n_desc, 
				  stab_ptr->n_value);
#else
#if 0
	      /*
	       * This isn't right.  The order of the stabs is different under
	       * a.out, and as a result we would end up attaching the line
	       * number to the wrong function.
	       */
	      DEBUG_AddLineNumber(curr_func, stab_ptr->n_desc, 
				  stab_ptr->n_value - curr_func->addr.off);
#endif
#endif
	    }
	  break;
	case N_FUN:
	  /*
	   * First, clean up the previous function we were working on.
	   */
	  DEBUG_Normalize(curr_func);

	  /*
	   * For now, just declare the various functions.  Later
	   * on, we will add the line number information and the
	   * local symbols.
	   */
	  if( !ignore )
	    {
	      new_addr.seg = 0;
	      new_addr.type = DEBUG_ParseStabType(ptr);
	      new_addr.off = load_offset + stab_ptr->n_value;
	      /*
	       * Copy the string to a temp buffer so we
	       * can kill everything after the ':'.  We do
	       * it this way because otherwise we end up dirtying
	       * all of the pages related to the stabs, and that
	       * sucks up swap space like crazy.
	       */
	      stab_strcpy(symname, ptr);
	      curr_func = DEBUG_AddSymbol( symname, &new_addr, currpath,
					   SYM_WINE | SYM_FUNC);
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
#ifndef __ELF__
	  /*
	   * With a.out, there is no NULL string N_SO entry at the end of
	   * the file.  Thus when we find non-consecutive entries,
	   * we consider that a new file is started.
	   */
	  if( last_nso < i-1 )
	    {
	      currpath[0] = '\0';
	      DEBUG_Normalize(curr_func);
	      curr_func = NULL;
	    }
#endif

	  if( *ptr == '\0' )
	    {
	      /*
	       * Nuke old path.
	       */
	      currpath[0] = '\0';
	      DEBUG_Normalize(curr_func);
	      curr_func = NULL;
	      /*
	       * The datatypes that we would need to use are reset when
	       * we start a new file.
	       */
	      memset(stab_types, 0, num_stab_types * sizeof(stab_types));
	    }
	  else
	    {
	      if (*ptr != '/')
	        strcat(currpath, ptr);
	      else
	        strcpy(currpath, ptr);
	      subpath = ptr;
	    }
	  last_nso = i;
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
	      DEBUG_Normalize(curr_func);
	      curr_func = NULL;
	    }
	  break;
	case N_UNDF:
	  strs += strtabinc;
	  strtabinc = stab_ptr->n_value;
 	  DEBUG_Normalize(curr_func);
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

      stabbuff[0] = '\0';

#if 0
      fprintf(stderr, "%d %x %s\n", stab_ptr->n_type, 
	      (unsigned int) stab_ptr->n_value,
	      strs + (unsigned int) stab_ptr->n_un.n_name);
#endif
    }

leave:

  if( stab_types != NULL )
    {
      free(stab_types);
      stab_types = NULL;
      num_stab_types = 0;
    }


  DEBUG_FreeRegisteredTypedefs();

  return TRUE;
}

#ifdef __ELF__

/*
 * Walk through the entire symbol table and add any symbols we find there.
 * This can be used in cases where we have stripped ELF shared libraries,
 * or it can be used in cases where we have data symbols for which the address
 * isn't encoded in the stabs.
 *
 * This is all really quite easy, since we don't have to worry about line
 * numbers or local data variables.
 */
static
int
DEBUG_ProcessElfSymtab(char * addr, unsigned int load_offset,
		       Elf32_Shdr * symtab, Elf32_Shdr * strtab)
{
  char		* curfile = NULL;
  struct name_hash * curr_sym = NULL;
  int		  flags;
  int		  i;
  DBG_ADDR        new_addr;
  int		  nsym;
  char		* strp;
  char		* symname;
  Elf32_Sym	* symp;


  symp = (Elf32_Sym *) (addr + symtab->sh_offset);
  nsym = symtab->sh_size / sizeof(*symp);
  strp = (char *) (addr + strtab->sh_offset);

  for(i=0; i < nsym; i++, symp++)
    {
      /*
       * Ignore certain types of entries which really aren't of that much
       * interest.
       */
      if( ELF32_ST_TYPE(symp->st_info) == STT_SECTION )
	{
	  continue;
	}

      symname = strp + symp->st_name;

      /*
       * Save the name of the current file, so we have a way of tracking
       * static functions/data.
       */
      if( ELF32_ST_TYPE(symp->st_info) == STT_FILE )
	{
	  curfile = symname;
	  continue;
	}


      /*
       * See if we already have something for this symbol.
       * If so, ignore this entry, because it would have come from the
       * stabs or from a previous symbol.  If the value is different,
       * we will have to keep the darned thing, because there can be
       * multiple local symbols by the same name.
       */
      if(    (DEBUG_GetSymbolValue(symname, -1, &new_addr, FALSE ) == TRUE)
	  && (new_addr.off == (load_offset + symp->st_value)) )
	{
	  continue;
	}

      new_addr.seg = 0;
      new_addr.type = NULL;
      new_addr.off = load_offset + symp->st_value;
      flags = SYM_WINE | (ELF32_ST_BIND(symp->st_info) == STT_FUNC 
			  ? SYM_FUNC : SYM_DATA);
      if( ELF32_ST_BIND(symp->st_info) == STB_GLOBAL )
	{
	  curr_sym = DEBUG_AddSymbol( symname, &new_addr, NULL, flags );
	}
      else
	{
	  curr_sym = DEBUG_AddSymbol( symname, &new_addr, curfile, flags );
	}

      /*
       * Record the size of the symbol.  This can come in handy in
       * some cases.  Not really used yet, however.
       */
      if(  symp->st_size != 0 )
	{
	  DEBUG_SetSymbolSize(curr_sym, symp->st_size);
	}
    }

  return TRUE;
}

static
int
DEBUG_ProcessElfObject(char * filename, unsigned int load_offset)
{
  int			rtn = FALSE;
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


  /*
   * Make sure we can stat and open this file.
   */
  if( filename == NULL )
    {
      goto leave;
    }

  status = stat(filename, &statbuf);
  if( status == -1 )
    {
      char *s,*t,*fn,*paths;
      if (strchr(filename,'/'))
      	goto leave;
      paths = xstrdup(getenv("PATH"));
      s = paths;
      while (s && *s) {
      	t = strchr(s,':');
	if (t) *t='\0';
	fn = (char*)xmalloc(strlen(filename)+1+strlen(s)+1);
	strcpy(fn,s);
	strcat(fn,"/");
	strcat(fn,filename);
	if ((rtn = DEBUG_ProcessElfObject(fn,load_offset))) {
      		free(paths);
		goto leave;
	}
	s = t+1;
      }
      free(paths);
      goto leave;
    }

  /*
   * Now open the file, so that we can mmap() it.
   */
  fd = open(filename, O_RDONLY);
  if( fd == -1 )
    {
      goto leave;
    }


  /*
   * Now mmap() the file.
   */
  addr = mmap(0, statbuf.st_size, PROT_READ, 
	      MAP_PRIVATE, fd, 0);
  if( addr == (char *) 0xffffffff )
    {
      goto leave;
    }

  /*
   * Give a nice status message here...
   * Well not, just print the name.
   */
  fprintf(stderr, " %s", filename);

  /*
   * Next, we need to find a few of the internal ELF headers within
   * this thing.  We need the main executable header, and the section
   * table.
   */
  ehptr = (Elf32_Ehdr *) addr;

  if( load_offset == NULL )
    {
      DEBUG_RegisterELFDebugInfo(ehptr->e_entry, statbuf.st_size, filename);
    }
  else
    {
      DEBUG_RegisterELFDebugInfo(load_offset, statbuf.st_size, filename);
    }

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
  rtn = DEBUG_ParseStabs(addr, load_offset, 
			 spnt[stabsect].sh_offset,
			 spnt[stabsect].sh_size,
			 spnt[stabstrsect].sh_offset,
			 spnt[stabstrsect].sh_size);

  if( rtn != TRUE )
    {
      goto leave;
    }

  for(i=0; i < nsect; i++)
    {
      if(    (strcmp(shstrtab + spnt[i].sh_name, ".symtab") == 0)
	  && (spnt[i].sh_type == SHT_SYMTAB) )
	{
	  DEBUG_ProcessElfSymtab(addr, load_offset, 
				 spnt + i, spnt + spnt[i].sh_link);
	}

      if(    (strcmp(shstrtab + spnt[i].sh_name, ".dynsym") == 0)
	  && (spnt[i].sh_type == SHT_DYNSYM) )
	{
	  DEBUG_ProcessElfSymtab(addr, load_offset, 
				 spnt + i, spnt + spnt[i].sh_link);
	}
    }

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

int
DEBUG_ReadExecutableDbgInfo(void)
{
  Elf32_Ehdr	      * ehdr;
  char		      * exe_name;
  Elf32_Dyn	      * dynpnt;
  struct r_debug      * dbg_hdr;
  struct link_map     * lpnt = NULL;
  extern Elf32_Dyn      _DYNAMIC[];
  int			rtn = FALSE;

  exe_name = DEBUG_argv0;

  /*
   * Make sure we can stat and open this file.
   */
  if( exe_name == NULL )
    {
      goto leave;
    }

  DEBUG_ProcessElfObject(exe_name, 0);

  /*
   * Finally walk the tables that the dynamic loader maintains to find all
   * of the other shared libraries which might be loaded.  Perform the
   * same step for all of these.
   */
  dynpnt = _DYNAMIC;
  if( dynpnt == NULL )
    {
      goto leave;
    }

  /*
   * Now walk the dynamic section (of the executable, looking for a DT_DEBUG
   * entry.
   */
  for(; dynpnt->d_tag != DT_NULL; dynpnt++)
    {
      if( dynpnt->d_tag == DT_DEBUG )
	{
	  break;
	}
    }

  if(    (dynpnt->d_tag != DT_DEBUG)
      || (dynpnt->d_un.d_ptr == NULL) )
    {
      goto leave;
    }

  /*
   * OK, now dig into the actual tables themselves.
   */
  dbg_hdr = (struct r_debug *) dynpnt->d_un.d_ptr;
  lpnt = dbg_hdr->r_map;

  /*
   * Now walk the linked list.  In all known ELF implementations,
   * the dynamic loader maintains this linked list for us.  In some
   * cases the first entry doesn't appear with a name, in other cases it
   * does.
   */
  for(; lpnt; lpnt = lpnt->l_next )
    {
      /*
       * We already got the stuff for the executable using the
       * argv[0] entry above.  Here we only need to concentrate on any
       * shared libraries which may be loaded.
       */
      ehdr = (Elf32_Ehdr *) lpnt->l_addr;
      if( (lpnt->l_addr == NULL) || (ehdr->e_type != ET_DYN) )
	{
	  continue;
	}

      if( lpnt->l_name != NULL )
	{
	  DEBUG_ProcessElfObject(lpnt->l_name, lpnt->l_addr);
	}
    }

  rtn = TRUE;

leave:

  return (rtn);

}

#else	/* !__ELF__ */

#ifdef linux
/*
 * a.out linux.
 */
int
DEBUG_ReadExecutableDbgInfo(void)
{
  char		      * addr = (char *) 0xffffffff;
  char		      * exe_name;
  struct exec	      * ahdr;
  int			fd = -1;
  int			rtn = FALSE;
  unsigned int		staboff;
  struct stat		statbuf;
  int			status;
  unsigned int		stroff;

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
  if( addr == (char *) 0xffffffff )
    {
      goto leave;
    }

  ahdr = (struct exec *) addr;

  staboff = N_SYMOFF(*ahdr);
  stroff = N_STROFF(*ahdr);
  rtn = DEBUG_ParseStabs(addr, 0, 
			 staboff, 
			 ahdr->a_syms,
			 stroff,
			 statbuf.st_size - stroff);

  /*
   * Give a nice status message here...
   */
  fprintf(stderr, " %s", exe_name);

  rtn = TRUE;

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
#else
/*
 * Non-linux, non-ELF platforms.
 */
int
DEBUG_ReadExecutableDbgInfo(void)
{
return FALSE;
}
#endif

#endif  /* __ELF__ */
