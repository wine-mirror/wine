/*
 * File hash.c - generate hash tables for Wine debugger symbols
 *
 * Copyright (C) 1993, Eric Youngdale.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include "debugger.h"

#define NR_NAME_HASH 16384
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#ifdef __i386__
static char * reg_name[] =
{
  "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};

static unsigned reg_ofs[] =
{
  FIELD_OFFSET(CONTEXT, Eax), FIELD_OFFSET(CONTEXT, Ecx),
  FIELD_OFFSET(CONTEXT, Edx), FIELD_OFFSET(CONTEXT, Ebx),
  FIELD_OFFSET(CONTEXT, Esp), FIELD_OFFSET(CONTEXT, Ebp),
  FIELD_OFFSET(CONTEXT, Esi), FIELD_OFFSET(CONTEXT, Edi)
};
#else
static char * reg_name[] = { NULL };   /* FIXME */
static unsigned reg_ofs[] = { 0 };
#endif


struct name_hash
{
    struct name_hash * next;		/* Used to look up within name hash */
    char *             name;
    char *             sourcefile;

    int		       n_locals;
    int		       locals_alloc;
    WineLocals       * local_vars;

    int		       n_lines;
    int		       lines_alloc;
    WineLineNo       * linetab;

    DBG_VALUE          value;
    unsigned short     flags;
    unsigned short     breakpoint_offset;
    unsigned int       symbol_size;
};


static BOOL DEBUG_GetStackSymbolValue( const char * name, DBG_VALUE *value );
static int sortlist_valid = FALSE;

static int sorttab_nsym;
static struct name_hash ** addr_sorttab = NULL;

static struct name_hash * name_hash_table[NR_NAME_HASH];

static unsigned int name_hash( const char * name )
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
    return hash % NR_NAME_HASH;
}

int
DEBUG_cmp_sym(const void * p1, const void * p2)
{
  struct name_hash ** name1 = (struct name_hash **) p1;
  struct name_hash ** name2 = (struct name_hash **) p2;

  if( ((*name1)->flags & SYM_INVALID) != 0 )
    {
      return -1;
    }

  if( ((*name2)->flags & SYM_INVALID) != 0 )
    {
      return 1;
    }

  if( (*name1)->value.addr.seg > (*name2)->value.addr.seg )
    {
      return 1;
    }

  if( (*name1)->value.addr.seg < (*name2)->value.addr.seg )
    {
      return -1;
    }

  if( (*name1)->value.addr.off > (*name2)->value.addr.off )
    {
      return 1;
    }

  if( (*name1)->value.addr.off < (*name2)->value.addr.off )
    {
      return -1;
    }

  return 0;
}

/***********************************************************************
 *           DEBUG_ResortSymbols
 *
 * Rebuild sorted list of symbols.
 */
static
void
DEBUG_ResortSymbols(void)
{
    struct name_hash *nh;
    int		nsym = 0;
    int		i;

    for(i=0; i<NR_NAME_HASH; i++)
    {
        for (nh = name_hash_table[i]; nh; nh = nh->next)
	  {
	    if( (nh->flags & SYM_INVALID) == 0 )
	       nsym++;
	    else
	       DEBUG_Printf( DBG_CHN_MESG, "Symbol %s (%04lx:%08lx) is invalid\n", 
                             nh->name, nh->value.addr.seg, nh->value.addr.off );
          }
    }

    sorttab_nsym = nsym;
    if( nsym == 0 )
      {
	return;
      }

    addr_sorttab = (struct name_hash **) DBG_realloc(addr_sorttab,
					 nsym * sizeof(struct name_hash *));

    nsym = 0;
    for(i=0; i<NR_NAME_HASH; i++)
    {
        for (nh = name_hash_table[i]; nh; nh = nh->next)
	  {
	    if( (nh->flags & SYM_INVALID) == 0 )
	       addr_sorttab[nsym++] = nh;
	  }
    }

    qsort(addr_sorttab, nsym,
	  sizeof(struct name_hash *), DEBUG_cmp_sym);
    sortlist_valid = TRUE;

}

/***********************************************************************
 *           DEBUG_AddSymbol
 *
 * Add a symbol to the table.
 */
struct name_hash *
DEBUG_AddSymbol( const char * name, const DBG_VALUE *value,
		 const char * source, int flags)
{
    struct name_hash  * new;
    struct name_hash *nh;
    static char  prev_source[PATH_MAX] = {'\0', };
    static char * prev_duped_source = NULL;
    int hash;

    assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);

    hash = name_hash(name);
    for (nh = name_hash_table[hash]; nh; nh = nh->next)
    {
        if (name[0] == nh->name[0] && strcmp(name, nh->name) == 0)
        {
            int c = memcmp(&nh->value.addr, &value->addr, sizeof(value->addr));

            if ((nh->flags & SYM_INVALID) != 0)
            {
                /* this case happens in ELF files, where we don't get the
                 * address of a symbol for the stabs part, but rather from
                 * the ELF sections. in this case, we'll call this function
                 * twice: 
                 * - a first time while parsing the stabs, with a NULL
                 * address
                 * - a second time with the correct address
                 * SYM_INVALID is set for the first pass, and cleared in the second
                 * the code below gets most of information for both passes
                 *
                 * some GCC versions may provide the correct address in the first pass
                 * but it does not seem to be sensible to rely on that.
                 */
                if (nh->value.addr.seg == 0 && nh->value.addr.off == 0 && c != 0)
                {
#if 0
                    DEBUG_Printf(DBG_CHN_MESG, "Changing address for symbol %s (%04lx:%08lx => %04lx:%08lx)\n",
                                 name, nh->value.addr.seg, nh->value.addr.off, value->addr.seg, value->addr.off);
#endif
                    nh->value.addr = value->addr;
                }
                if (nh->value.type == NULL && value->type != NULL)
                {
                    nh->value.type = value->type;
                    nh->value.cookie = value->cookie;
                }
                /* it may happen that the same symbol is defined in several compilation
                 * units, but the linker decides to merge it into a single instance.
                 * in that case, we don't clear the invalid flag for all the compilation
                 * units (N_GSYM), and wait to get the symbol from the symtab
                 */
                if ((flags & SYM_INVALID) == 0)
                    nh->flags &= ~SYM_INVALID;
                return nh;
            }
            /* don't define a symbol twice */
            if (c == 0 && (flags & SYM_INVALID) == 0) return nh;
        }
    }

#if 0
    DEBUG_Printf(DBG_CHN_TRACE, "adding %s symbol (%s) from file '%s' at %04lx:%08lx\n",
		 (flags & SYM_INVALID) ? "invalid" : "  valid", name, source, value->addr.seg, value->addr.off);
#endif

    /*
     * First see if we already have an entry for this symbol.  If so
     * return it, so we don't end up with duplicates.
     */

    new = (struct name_hash *) DBG_alloc(sizeof(struct name_hash));
    new->value = *value;
    new->name = DBG_strdup(name);

    if( source != NULL )
      {
	/*
	 * This is an enhancement to reduce memory consumption.  The idea
	 * is that we duplicate a given string only once.  This is a big
	 * win if there are lots of symbols defined in a given source file.
	 */
	if( strcmp(source, prev_source) == 0 )
	  {
	    new->sourcefile = prev_duped_source;
	  }
	else
	  {
	    strcpy(prev_source, source);
	    prev_duped_source = new->sourcefile = DBG_strdup(source);
	  }
      }
    else
      {
	new->sourcefile = NULL;
      }

    new->n_lines	= 0;
    new->lines_alloc	= 0;
    new->linetab	= NULL;

    new->n_locals	= 0;
    new->locals_alloc	= 0;
    new->local_vars	= NULL;

    new->flags		= flags;
    new->next		= NULL;

    /* Now insert into the hash table */
    new->next = name_hash_table[hash];
    name_hash_table[hash] = new;

    /*
     * Check some heuristics based upon the file name to see whether
     * we want to step through this guy or not.  These are machine generated
     * assembly files that are used to translate between the MS way of
     * calling things and the GCC way of calling things.  In general we
     * always want to step through.
     */
    if ( source != NULL ) {
        int	len = strlen(source);

	if (len > 2 && source[len-2] == '.' && source[len-1] == 's') {
	    char* c = strrchr(source - 2, '/');
	    if (c != NULL) {
		if (strcmp(c + 1, "asmrelay.s") == 0)
		    new->flags |= SYM_TRAMPOLINE;
	    }
	}
    }

    sortlist_valid = FALSE;
    return new;
}

BOOL DEBUG_Normalize(struct name_hash * nh )
{

  /*
   * We aren't adding any more locals or linenumbers to this function.
   * Free any spare memory that we might have allocated.
   */
  if( nh == NULL )
    {
      return TRUE;
    }

  if( nh->n_locals != nh->locals_alloc )
    {
      nh->locals_alloc = nh->n_locals;
      nh->local_vars = DBG_realloc(nh->local_vars,
				  nh->locals_alloc * sizeof(WineLocals));
    }

  if( nh->n_lines != nh->lines_alloc )
    {
      nh->lines_alloc = nh->n_lines;
      nh->linetab = DBG_realloc(nh->linetab,
			      nh->lines_alloc * sizeof(WineLineNo));
    }

  return TRUE;
}

/***********************************************************************
 *           DEBUG_GetSymbolValue
 *
 * Get the address of a named symbol.
 * Return values:
 *      gsv_found:   if the symbol is found
 *      gsv_unknown: if the symbol isn't found
 *      gsv_aborted: some error occurred (likely, many symbols of same name exist,
 *          and user didn't pick one of them)
 */
static int    DEBUG_GSV_Helper(const char* name, const int lineno,
			       DBG_VALUE* value, int num, int bp_flag)
{
   struct name_hash*	nh;
   int			i = 0;
   DBG_ADDR		addr;

   for (nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
   {
      if ((nh->flags & SYM_INVALID) != 0) continue;
      if (!strcmp(nh->name, name) && DEBUG_GetLineNumberAddr( nh, lineno, &addr, bp_flag ))
      {
	 if (i >= num) return num + 1;
	 value[i].addr = addr;
	 value[i].type = nh->value.type;
	 value[i].cookie = nh->value.cookie;
	 i++;
      }
   }
   return i;
}

enum get_sym_val DEBUG_GetSymbolValue( const char * name, 
                                       const int lineno,
                                       DBG_VALUE *rtn, int bp_flag )
{
#define NUMDBGV 10
   /* FIXME: NUMDBGV should be made variable */
   DBG_VALUE 	value[NUMDBGV];
   DBG_VALUE	vtmp;
   int		num, i, local = -1;

   num = DEBUG_GSV_Helper(name, lineno, value, NUMDBGV, bp_flag);
   if (!num && (name[0] != '_'))
   {
      char buffer[512];

      if (strlen(name) < sizeof(buffer) - 2) /* one for '_', one for '\0' */
      {
          buffer[0] = '_';
          strcpy(buffer + 1, name);
          num = DEBUG_GSV_Helper(buffer, lineno, value, NUMDBGV, bp_flag);
      }
      else DEBUG_Printf(DBG_CHN_WARN, "Way too long symbol (%s)\n", name);
   }

   /* now get the local symbols if any */
   if (DEBUG_GetStackSymbolValue(name, &vtmp) && num < NUMDBGV)
   {
      value[num] = vtmp;
      local = num;
      num++;
   }

   if (num == 0) {
      return gsv_unknown;
   } else if (!DEBUG_InteractiveP || num == 1) {
      i = 0;
   } else {
      char	buffer[256];

      if (num == NUMDBGV+1) {
	 DEBUG_Printf(DBG_CHN_MESG, "Too many addresses for symbol '%s', limiting the first %d\n", name, NUMDBGV);
	 num = NUMDBGV;
      }
      DEBUG_Printf(DBG_CHN_MESG, "Many symbols with name '%s', choose the one you want (<cr> to abort):\n", name);
      for (i = 0; i < num; i++) {
	 DEBUG_Printf(DBG_CHN_MESG, "[%d]: ", i + 1);
         if (i == local) {
             struct name_hash*func;
             unsigned int     ebp;
             unsigned int     eip;

             if (DEBUG_GetCurrentFrame(&func, &eip, &ebp))
                 DEBUG_Printf(DBG_CHN_MESG, "local variable of %s in %s\n", func->name, func->sourcefile);
             else
                 DEBUG_Printf(DBG_CHN_MESG, "local variable\n");
         } else {
             DEBUG_PrintAddress( &value[i].addr, DEBUG_GetSelectorType(value[i].addr.seg), TRUE);
             DEBUG_Printf(DBG_CHN_MESG, "\n");
         }
      }
      do {
	  i = 0;
	  if (DEBUG_ReadLine("=> ", buffer, sizeof(buffer)))
	  {
              if (buffer[0] == '\0') return gsv_aborted;
	      i = atoi(buffer);
	      if (i < 1 || i > num)
		  DEBUG_Printf(DBG_CHN_MESG, "Invalid choice %d\n", i);
	  }
      } while (i < 1 || i > num);

      /* The array is 0-based, but the choices are 1..n, so we have to subtract one before returning. */
      i--;
   }
   *rtn = value[i];
   return gsv_found;
}

/***********************************************************************
 *           DEBUG_GetLineNumberAddr
 *
 * Get the address of a named symbol.
 */
BOOL DEBUG_GetLineNumberAddr( const struct name_hash * nh, const int lineno,
			      DBG_ADDR *addr, int bp_flag )
{
    int i;

    if( lineno == -1 )
      {
	*addr = nh->value.addr;
	if( bp_flag )
	  {
	    addr->off += nh->breakpoint_offset;
	  }
      }
    else
      {
	/*
	 * Search for the specific line number.  If we don't find it,
	 * then return FALSE.
	 */
	if( nh->linetab == NULL )
	  {
	    return FALSE;
	  }

	for(i=0; i < nh->n_lines; i++ )
	  {
	    if( nh->linetab[i].line_number == lineno )
	      {
		*addr = nh->linetab[i].pc_offset;
		return TRUE;
	      }
	  }

	/*
	 * This specific line number not found.
	 */
	return FALSE;
      }

    return TRUE;
}

/***********************************************************************
 *           DEBUG_FindNearestSymbol
 *
 * Find the symbol nearest to a given address.
 * If ebp is specified as non-zero, it means we should dump the argument
 * list into the string we return as well.
 */
const char * DEBUG_FindNearestSymbol( const DBG_ADDR *addr, int flag,
				      struct name_hash ** rtn,
				      unsigned int ebp,
				      struct list_id * source)
{
    static char name_buffer[MAX_PATH + 256];
    static char arglist[1024];
    static char argtmp[256];
    struct name_hash * nearest = NULL;
    int mid, high, low;
    unsigned	int * ptr;
    int lineno;
    char * lineinfo, *sourcefile;
    int i;
    char linebuff[16];
    unsigned val;
    DBG_MODULE*	module;
    char modbuf[256];

    if( rtn != NULL )
      {
	*rtn = NULL;
      }

    if( source != NULL )
      {
 	source->sourcefile = NULL;
	source->line = -1;
      }

    if( sortlist_valid == FALSE )
      {
	DEBUG_ResortSymbols();
      }

    if( sortlist_valid == FALSE )
      {
	return NULL;
      }

    /*
     * FIXME  - use the binary search that we added to
     * the function DEBUG_CheckLinenoStatus.  Better yet, we should
     * probably keep some notion of the current function so we don't
     * have to search every time.
     */
    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = sorttab_nsym;
    if( addr_sorttab[0]->value.addr.seg > addr->seg
	|| (   addr_sorttab[0]->value.addr.seg == addr->seg
	    && addr_sorttab[0]->value.addr.off > addr->off) )
      {
	nearest = NULL;
      }
    else if( addr_sorttab[high - 1]->value.addr.seg < addr->seg
	|| (   addr_sorttab[high - 1]->value.addr.seg == addr->seg
	    && addr_sorttab[high - 1]->value.addr.off < addr->off) )
      {
	nearest = addr_sorttab[high - 1];
      }
    else
      {
	while(1==1)
	  {
	    mid = (high + low)/2;
	    if( mid == low )
	      {
		/*
		 * See if there are any other entries that might also
		 * have the same address, and would also have a line
		 * number table.
		 */
		if( mid > 0 && addr_sorttab[mid]->linetab == NULL )
		  {
		    if(    (addr_sorttab[mid - 1]->value.addr.seg ==
			    addr_sorttab[mid]->value.addr.seg)
			&& (addr_sorttab[mid - 1]->value.addr.off ==
			    addr_sorttab[mid]->value.addr.off)
		        && (addr_sorttab[mid - 1]->linetab != NULL) )
		      {
			mid--;
		      }
		  }

		if(    (mid < sorttab_nsym - 1)
		    && (addr_sorttab[mid]->linetab == NULL) )
		  {
		    if(    (addr_sorttab[mid + 1]->value.addr.seg ==
			    addr_sorttab[mid]->value.addr.seg)
			&& (addr_sorttab[mid + 1]->value.addr.off ==
			    addr_sorttab[mid]->value.addr.off)
		        && (addr_sorttab[mid + 1]->linetab != NULL) )
		      {
			mid++;
		      }
		  }
		nearest = addr_sorttab[mid];
#if 0
		DEBUG_Printf(DBG_CHN_MESG, "Found %x:%x when looking for %x:%x %x %s\n",
			     addr_sorttab[mid ]->value.addr.seg,
			     addr_sorttab[mid ]->value.addr.off,
			     addr->seg, addr->off,
			     addr_sorttab[mid ]->linetab,
			     addr_sorttab[mid ]->name);
#endif
		break;
	      }
	    if(    (addr_sorttab[mid]->value.addr.seg < addr->seg)
		|| (   addr_sorttab[mid]->value.addr.seg == addr->seg
		    && addr_sorttab[mid]->value.addr.off <= addr->off) )
	      {
		low = mid;
	      }
	    else
	      {
		high = mid;
	      }
	  }
      }

    if (!nearest) return NULL;

    if( rtn != NULL )
      {
	*rtn = nearest;
      }

    /*
     * Fill in the relevant bits to the structure so that we can
     * locate the source and line for this bit of code.
     */
    if( source != NULL )
      {
	source->sourcefile = nearest->sourcefile;
	if( nearest->linetab == NULL )
	  {
	    source->line = -1;
	  }
	else
	  {
	    source->line = nearest->linetab[0].line_number;
	  }
      }

    lineinfo = "";
    lineno = -1;

    /*
     * Prepare to display the argument list.  If ebp is specified, it is
     * the framepointer for the function in question.  If not specified,
     * we don't want the arglist.
     */
    memset(arglist, '\0', sizeof(arglist));
    if( ebp != 0 )
      {
	for(i=0; i < nearest->n_locals; i++ )
	  {
	    /*
	     * If this is a register (offset == 0) or a local
	     * variable, we don't want to know about it.
	     */
	    if( nearest->local_vars[i].offset <= 0 )
	      {
		continue;
	      }

	    ptr = (unsigned int *) (ebp + nearest->local_vars[i].offset);
	    if( arglist[0] == '\0' )
	      {
		arglist[0] = '(';
	      }
	    else
	      {
		strcat(arglist, ", ");
	      }
	    DEBUG_READ_MEM_VERBOSE(ptr, &val, sizeof(val));
	    snprintf(argtmp, sizeof(argtmp), "%s=0x%x", nearest->local_vars[i].name, val);

	    strcat(arglist, argtmp);
	  }
	if( arglist[0] == '(' )
	  {
	    strcat(arglist, ")");
	  }
      }

    module = DEBUG_FindModuleByAddr((void*)DEBUG_ToLinear(addr), DMT_UNKNOWN);
    if (module) {
        char *p, *name = module->module_name;
        if ((p = strrchr(name, '/'))) name = p + 1;
        if ((p = strrchr(name, '\\'))) name = p + 1;
        snprintf( modbuf, sizeof(modbuf), " in %s", name);
    }
    else
       modbuf[0] = '\0';

    if( (nearest->sourcefile != NULL) && (flag == TRUE)
	&& (addr->off - nearest->value.addr.off < 0x100000) )
      {

	/*
	 * Try and find the nearest line number to the current offset.
	 */
	if( nearest->linetab != NULL )
        {
            low = 0;
            high = nearest->n_lines;
            while ((high - low) > 1)
            {
                mid = (high + low) / 2;
                if (addr->off < nearest->linetab[mid].pc_offset.off)
                    high = mid;
                else
                    low = mid;
            }
            lineno = nearest->linetab[low].line_number;
        }

	if( lineno != -1 )
	  {
	    snprintf(linebuff, sizeof(linebuff), ":%d", lineno);
	    lineinfo = linebuff;
	    if( source != NULL )
	      {
		source->line = lineno;
	      }
	  }

        /* Remove the path from the file name */
        sourcefile = strrchr( nearest->sourcefile, '/' );
        if (!sourcefile) sourcefile = nearest->sourcefile;
        else sourcefile++;

	if (addr->off == nearest->value.addr.off)
	  snprintf( name_buffer, sizeof(name_buffer), "%s%s [%s%s]%s", nearest->name,
		   arglist, sourcefile, lineinfo, modbuf);
	else
	  snprintf( name_buffer, sizeof(name_buffer), "%s+0x%lx%s [%s%s]%s", nearest->name,
		   addr->off - nearest->value.addr.off,
		   arglist, sourcefile, lineinfo, modbuf );
      }
    else
      {
	if (addr->off == nearest->value.addr.off)
	  snprintf( name_buffer, sizeof(name_buffer), "%s%s%s", nearest->name, arglist, modbuf);
	else {
	  if (addr->seg && (nearest->value.addr.seg!=addr->seg))
	      return NULL;
	  else
	      snprintf( name_buffer, sizeof(name_buffer), "%s+0x%lx%s%s", nearest->name,
		       addr->off - nearest->value.addr.off, arglist, modbuf);
 	}
      }
    return name_buffer;
}


/***********************************************************************
 *           DEBUG_ReadSymbolTable
 *
 * Read a symbol file into the hash table.
 */
void DEBUG_ReadSymbolTable( const char* filename, unsigned long offset )
{
    FILE * symbolfile;
    DBG_VALUE value;
    char type;
    char * cpnt;
    char buffer[256];
    char name[256];

    if (!(symbolfile = fopen(filename, "r")))
    {
        DEBUG_Printf( DBG_CHN_WARN, "Unable to open symbol table %s\n", filename );
        return;
    }

    DEBUG_Printf( DBG_CHN_MESG, "Reading symbols from file %s\n", filename );

    value.type = NULL;
    value.addr.seg = 0;
    value.addr.off = 0;
    value.cookie = DV_TARGET;

    while (1)
    {
        fgets( buffer, sizeof(buffer), symbolfile );
        if (feof(symbolfile)) break;

        /* Strip any text after a # sign (i.e. comments) */
        cpnt = buffer;
        while (*cpnt)
            if(*cpnt++ == '#') { *cpnt = 0; break; }

        /* Quietly ignore any lines that have just whitespace */
        cpnt = buffer;
        while(*cpnt)
        {
            if(*cpnt != ' ' && *cpnt != '\t') break;
            cpnt++;
        }
        if (!(*cpnt) || *cpnt == '\n') continue;

        if (sscanf(buffer, "%lx %c %s", &value.addr.off, &type, name) == 3)
        {
           if (value.addr.off + offset < value.addr.off)
              DEBUG_Printf( DBG_CHN_WARN, "Address wrap around\n");
           value.addr.off += offset;
	   DEBUG_AddSymbol( name, &value, NULL, SYM_WINE );
        }
    }
    fclose(symbolfile);
}


void
DEBUG_AddLineNumber( struct name_hash * func, int line_num,
		     unsigned long offset )
{
  if( func == NULL )
    {
      return;
    }

  if( func->n_lines + 1 >= func->lines_alloc )
    {
      func->lines_alloc += 64;
      func->linetab = DBG_realloc(func->linetab,
			      func->lines_alloc * sizeof(WineLineNo));
    }

  func->linetab[func->n_lines].line_number = line_num;
  func->linetab[func->n_lines].pc_offset.seg = func->value.addr.seg;
  func->linetab[func->n_lines].pc_offset.off = func->value.addr.off + offset;
  func->n_lines++;
}


struct wine_locals *
DEBUG_AddLocal( struct name_hash * func, int regno,
		int offset,
		int pc_start,
		int pc_end,
		char * name)
{
  if( func == NULL )
    {
      return NULL;
    }

  if( func->n_locals + 1 >= func->locals_alloc )
    {
      func->locals_alloc += 32;
      func->local_vars = DBG_realloc(func->local_vars,
			      func->locals_alloc * sizeof(WineLocals));
    }

  func->local_vars[func->n_locals].regno = regno;
  func->local_vars[func->n_locals].offset = offset;
  func->local_vars[func->n_locals].pc_start = pc_start;
  func->local_vars[func->n_locals].pc_end = pc_end;
  func->local_vars[func->n_locals].name = DBG_strdup(name);
  func->local_vars[func->n_locals].type = NULL;
  func->n_locals++;

  return &func->local_vars[func->n_locals - 1];
}

void
DEBUG_DumpHashInfo(void)
{
  int i;
  int depth;
  struct name_hash *nh;

  /*
   * Utility function to dump stats about the hash table.
   */
    for(i=0; i<NR_NAME_HASH; i++)
    {
      depth = 0;
      for (nh = name_hash_table[i]; nh; nh = nh->next)
	{
	  depth++;
	}
      DEBUG_Printf(DBG_CHN_MESG, "Bucket %d: %d\n", i, depth);
    }
}

/***********************************************************************
 *           DEBUG_CheckLinenoStatus
 *
 * Find the symbol nearest to a given address.
 * If ebp is specified as non-zero, it means we should dump the argument
 * list into the string we return as well.
 */
int DEBUG_CheckLinenoStatus( const DBG_ADDR *addr)
{
    struct name_hash * nearest = NULL;
    int mid, high, low;

    if( sortlist_valid == FALSE )
      {
	DEBUG_ResortSymbols();
      }

    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = sorttab_nsym;
    if( addr_sorttab[0]->value.addr.seg > addr->seg
	|| (   addr_sorttab[0]->value.addr.seg == addr->seg
	    && addr_sorttab[0]->value.addr.off > addr->off) )
      {
	nearest = NULL;
      }
    else if( addr_sorttab[high - 1]->value.addr.seg < addr->seg
	|| (   addr_sorttab[high - 1]->value.addr.seg == addr->seg
	    && addr_sorttab[high - 1]->value.addr.off < addr->off) )
      {
	nearest = addr_sorttab[high - 1];
      }
    else
      {
	while(1==1)
	  {
	    mid = (high + low)/2;
	    if( mid == low )
	      {
		/*
		 * See if there are any other entries that might also
		 * have the same address, and would also have a line
		 * number table.
		 */
		if( mid > 0 && addr_sorttab[mid]->linetab == NULL )
		  {
		    if(    (addr_sorttab[mid - 1]->value.addr.seg ==
			    addr_sorttab[mid]->value.addr.seg)
			&& (addr_sorttab[mid - 1]->value.addr.off ==
			    addr_sorttab[mid]->value.addr.off)
		        && (addr_sorttab[mid - 1]->linetab != NULL) )
		      {
			mid--;
		      }
		  }

		if(    (mid < sorttab_nsym - 1)
		    && (addr_sorttab[mid]->linetab == NULL) )
		  {
		    if(    (addr_sorttab[mid + 1]->value.addr.seg ==
			    addr_sorttab[mid]->value.addr.seg)
			&& (addr_sorttab[mid + 1]->value.addr.off ==
			    addr_sorttab[mid]->value.addr.off)
		        && (addr_sorttab[mid + 1]->linetab != NULL) )
		      {
			mid++;
		      }
		  }
		nearest = addr_sorttab[mid];
#if 0
		DEBUG_Printf(DBG_CHN_MESG, "Found %x:%x when looking for %x:%x %x %s\n",
			     addr_sorttab[mid ]->value.addr.seg,
			     addr_sorttab[mid ]->value.addr.off,
			     addr->seg, addr->off,
			     addr_sorttab[mid ]->linetab,
			     addr_sorttab[mid ]->name);
#endif
		break;
	      }
	    if(    (addr_sorttab[mid]->value.addr.seg < addr->seg)
		|| (   addr_sorttab[mid]->value.addr.seg == addr->seg
		    && addr_sorttab[mid]->value.addr.off <= addr->off) )
	      {
		low = mid;
	      }
	    else
	      {
		high = mid;
	      }
	  }
      }

    if (!nearest) return FUNC_HAS_NO_LINES;

    if( nearest->flags & SYM_STEP_THROUGH )
      {
	/*
	 * This will cause us to keep single stepping until
	 * we get to the other side somewhere.
	 */
	return NOT_ON_LINENUMBER;
      }

    if( (nearest->flags & SYM_TRAMPOLINE) )
      {
	/*
	 * This will cause us to keep single stepping until
	 * we get to the other side somewhere.
	 */
	return FUNC_IS_TRAMPOLINE;
      }

    if( nearest->linetab == NULL )
      {
	return FUNC_HAS_NO_LINES;
      }


    /*
     * We never want to stop on the first instruction of a function
     * even if it has it's own linenumber.  Let the thing keep running
     * until it gets past the function prologue.  We only do this if there
     * is more than one line number for the function, of course.
     */
    if( nearest->value.addr.off == addr->off && nearest->n_lines > 1 )
      {
	return NOT_ON_LINENUMBER;
      }

    if( (nearest->sourcefile != NULL)
	&& (addr->off - nearest->value.addr.off < 0x100000) )
      {
          low = 0;
          high = nearest->n_lines;
          while ((high - low) > 1)
          {
              mid = (high + low) / 2;
              if (addr->off < nearest->linetab[mid].pc_offset.off) high = mid;
              else low = mid;
          }
          if (addr->off == nearest->linetab[low].pc_offset.off)
              return AT_LINENUMBER;
          else
              return NOT_ON_LINENUMBER;
      }

    return FUNC_HAS_NO_LINES;
}

/***********************************************************************
 *           DEBUG_GetFuncInfo
 *
 * Find the symbol nearest to a given address.
 * Returns sourcefile name and line number in a format that the listing
 * handler can deal with.
 */
void
DEBUG_GetFuncInfo( struct list_id * ret, const char * filename,
		   const char * name)
{
    char buffer[256];
    char * pnt;
    struct name_hash *nh;

    for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
      {
	if( filename != NULL )
	  {

	    if( nh->sourcefile == NULL )
	      {
		continue;
	      }

	    pnt = strrchr(nh->sourcefile, '/');
	    if( strcmp(nh->sourcefile, filename) != 0
		&& (pnt == NULL || strcmp(pnt + 1, filename) != 0) )
	      {
		continue;
	      }
	  }
        if (!strcmp(nh->name, name)) break;
      }

    if (!nh && (name[0] != '_'))
    {
        buffer[0] = '_';
        strcpy(buffer+1, name);
        for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
	  {
	    if( filename != NULL )
	      {
		if( nh->sourcefile == NULL )
		  {
		    continue;
		  }

		pnt = strrchr(nh->sourcefile, '/');
		if( strcmp(nh->sourcefile, filename) != 0
		    && (pnt == NULL || strcmp(pnt + 1, filename) != 0) )
		  {
		    continue;
		  }
	      }
            if (!strcmp(nh->name, buffer)) break;
	  }
    }

    if( !nh )
      {
	if( filename != NULL )
	  {
	    DEBUG_Printf(DBG_CHN_MESG, "No such function %s in %s\n", name, filename);
	  }
	else
	  {
	    DEBUG_Printf(DBG_CHN_MESG, "No such function %s\n", name);
	  }
	ret->sourcefile = NULL;
	ret->line = -1;
	return;
      }

    ret->sourcefile = nh->sourcefile;

    /*
     * Search for the specific line number.  If we don't find it,
     * then return FALSE.
     */
    if( nh->linetab == NULL )
      {
	ret->line = -1;
      }
    else
      {
	ret->line = nh->linetab[0].line_number;
      }
}

/***********************************************************************
 *           DEBUG_GetStackSymbolValue
 *
 * Get the address of a named symbol from the current stack frame.
 */
static
BOOL DEBUG_GetStackSymbolValue( const char * name, DBG_VALUE *value )
{
  struct name_hash * curr_func;
  unsigned int	     ebp;
  unsigned int	     eip;
  int		     i;

  if( DEBUG_GetCurrentFrame(&curr_func, &eip, &ebp) == FALSE )
    {
      return FALSE;
    }

  for(i=0; i < curr_func->n_locals; i++ )
    {
      /*
       * Test the range of validity of the local variable.  This
       * comes up with RBRAC/LBRAC stabs in particular.
       */
      if(    (curr_func->local_vars[i].pc_start != 0)
	  && ((eip - curr_func->value.addr.off)
	      < curr_func->local_vars[i].pc_start) )
	{
	  continue;
	}

      if(    (curr_func->local_vars[i].pc_end != 0)
	  && ((eip - curr_func->value.addr.off)
	      > curr_func->local_vars[i].pc_end) )
	{
	  continue;
	}

      if( strcmp(name, curr_func->local_vars[i].name) == 0 )
	{
	  /*
	   * OK, we found it.  Now figure out what to do with this.
	   */
	  if( curr_func->local_vars[i].regno != 0 )
	    {
	      /*
	       * Register variable.  Point to DEBUG_context field.
	       */
	      assert(curr_func->local_vars[i].regno - 1 < sizeof(reg_ofs)/sizeof(reg_ofs[0]));
	      value->addr.off = ((DWORD)&DEBUG_context) +
		 reg_ofs[curr_func->local_vars[i].regno - 1];
	      value->cookie = DV_HOST;
	    }
	  else
	    {
	      value->addr.off = ebp + curr_func->local_vars[i].offset;
	      value->cookie = DV_TARGET;
	    }
	  value->addr.seg = 0;
	  value->type = curr_func->local_vars[i].type;

	  return TRUE;
	}
    }

  return FALSE;
}

int
DEBUG_InfoLocals(void)
{
  struct name_hash  * curr_func;
  unsigned int	      ebp;
  unsigned int	      eip;
  int		      i;
  unsigned int      * ptr;
  unsigned int	      val;

  if( DEBUG_GetCurrentFrame(&curr_func, &eip, &ebp) == FALSE )
    {
      return FALSE;
    }

  DEBUG_Printf(DBG_CHN_MESG, "%s:\n", curr_func->name);

  for(i=0; i < curr_func->n_locals; i++ )
    {
      /*
       * Test the range of validity of the local variable.  This
       * comes up with RBRAC/LBRAC stabs in particular.
       */
      if(    (curr_func->local_vars[i].pc_start != 0)
	  && ((eip - curr_func->value.addr.off)
	      < curr_func->local_vars[i].pc_start) )
	{
	  continue;
	}

      if(    (curr_func->local_vars[i].pc_end != 0)
	  && ((eip - curr_func->value.addr.off)
	      > curr_func->local_vars[i].pc_end) )
	{
	  continue;
	}

      DEBUG_PrintTypeCast(curr_func->local_vars[i].type);

      if( curr_func->local_vars[i].regno != 0 )
	{
	  ptr = (unsigned int *)(((DWORD)&DEBUG_context)
				 + reg_ofs[curr_func->local_vars[i].regno - 1]);
	  DEBUG_Printf(DBG_CHN_MESG, " %s (optimized into register $%s) == 0x%8.8x\n",
		       curr_func->local_vars[i].name,
		       reg_name[curr_func->local_vars[i].regno - 1],
		       *ptr);
	}
      else
	{
	  DEBUG_READ_MEM_VERBOSE((void*)(ebp + curr_func->local_vars[i].offset),
				 &val, sizeof(val));
	  DEBUG_Printf(DBG_CHN_MESG, " %s == 0x%8.8x\n",
		       curr_func->local_vars[i].name, val);
	}
    }

  return TRUE;
}

int
DEBUG_SetSymbolSize(struct name_hash * sym, unsigned int len)
{
  sym->symbol_size = len;

  return TRUE;
}

int
DEBUG_SetSymbolBPOff(struct name_hash * sym, unsigned int off)
{
  sym->breakpoint_offset = off;

  return TRUE;
}

int
DEBUG_GetSymbolAddr(struct name_hash * sym, DBG_ADDR * addr)
{

  *addr = sym->value.addr;

  return TRUE;
}

int DEBUG_SetLocalSymbolType(struct wine_locals * sym, struct datatype * type)
{
  sym->type = type;

  return TRUE;
}

#ifdef HAVE_REGEX_H

static int cmp_sym_by_name(const void * p1, const void * p2)
{
  struct name_hash ** name1 = (struct name_hash **) p1;
  struct name_hash ** name2 = (struct name_hash **) p2;

  return strcmp( (*name1)->name, (*name2)->name );
}

#include <regex.h>

void DEBUG_InfoSymbols(const char* str)
{
    int                 i;
    struct name_hash*   nh;
    struct name_hash**  array = NULL;
    unsigned            num_used_array = 0;
    unsigned            num_alloc_array = 0;
    const char*         name;
    enum dbg_mode       mode;
    regex_t             preg;

    regcomp(&preg, str, REG_NOSUB);

    /* grab all symbols */
    for (i = 0; i < NR_NAME_HASH; i++)
    {
        for (nh = name_hash_table[i]; nh; nh = nh->next)
	{
            if (regexec(&preg, nh->name, 0, NULL, 0) == 0)
            {
                if (num_used_array == num_alloc_array)
                {
                    array = HeapReAlloc(GetProcessHeap(), 0, array, sizeof(*array) * (num_alloc_array += 32));
                    if (!array) return;
                }
                array[num_used_array++] = nh;
            }
	}
    }
    regfree(&preg);

    /* now sort them by alphabetical order */
    qsort(array, num_used_array, sizeof(*array), cmp_sym_by_name);

    /* and display them */
    for (i = 0; i < num_used_array; i++)
    {
        mode = DEBUG_GetSelectorType(array[i]->value.addr.seg);
        name = DEBUG_FindNearestSymbol( &array[i]->value.addr, TRUE, 
                                        NULL, 0, NULL );

        if (mode != MODE_32) 
            DEBUG_Printf( DBG_CHN_MESG, "%04lx:%04lx :", 
                          array[i]->value.addr.seg & 0xFFFF,
                          array[i]->value.addr.off );
        else
            DEBUG_Printf( DBG_CHN_MESG, "%08lx  :", array[i]->value.addr.off );
        if (array[i]->value.type)
        {
            DEBUG_Printf( DBG_CHN_MESG, " (");
            DEBUG_PrintTypeCast(array[i]->value.type);
            DEBUG_Printf( DBG_CHN_MESG, ")");
        }
        if (name) DEBUG_Printf( DBG_CHN_MESG, " %s\n", name );
    }
    HeapFree(GetProcessHeap(), 0, array);
}

#else /* HAVE_REGEX_H */

void DEBUG_InfoSymbols(const char* str)
{
    DEBUG_Printf( DBG_CHN_MESG, "FIXME: needs regex support\n" );
}

#endif /* HAVE_REGEX_H */
