/*
 * File hash.c - generate hash tables for Wine debugger symbols
 *
 * Copyright (C) 1993, Eric Youngdale.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <neexe.h>
#include "module.h"
#include "selectors.h"
#include "debugger.h"
#include "toolhelp.h"
#include "xmalloc.h"

#define NR_NAME_HASH 128

static struct name_hash * name_hash_table[NR_NAME_HASH];

static unsigned int name_hash( const char * name )
{
    unsigned int hash = 0;
    const char * p;

    p = name;

    while (*p) hash = (hash << 15) + (hash << 3) + (hash >> 3) + *p++;
    return hash % NR_NAME_HASH;
}


/***********************************************************************
 *           DEBUG_AddSymbol
 *
 * Add a symbol to the table.
 */
struct name_hash *
DEBUG_AddSymbol( const char * name, const DBG_ADDR *addr, const char * source )
{
    struct name_hash  * new;
    int hash;

    new = (struct name_hash *) xmalloc(sizeof(struct name_hash));
    new->addr = *addr;
    new->name = xstrdup(name);

    if( source != NULL )
      {
	new->sourcefile = xstrdup(source);
      }
    else
      {
	new->sourcefile = NULL;
      }

    new->n_lines = 0;
    new->lines_alloc = 0;
    new->linetab = NULL;

    new->n_locals = 0;
    new->locals_alloc = 0;
    new->local_vars = NULL;

    new->next = NULL;
    hash = name_hash(name);

    /* Now insert into the hash table */
    new->next = name_hash_table[hash];
    name_hash_table[hash] = new;

    return new;
}


/***********************************************************************
 *           DEBUG_GetSymbolValue
 *
 * Get the address of a named symbol.
 */
BOOL32 DEBUG_GetSymbolValue( const char * name, const int lineno, 
			     DBG_ADDR *addr )
{
    char buffer[256];
    int i;
    struct name_hash *nh;

    for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
        if (!strcmp(nh->name, name)) break;

    if (!nh && (name[0] != '_'))
    {
        buffer[0] = '_';
        strcpy(buffer+1, name);
        for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
            if (!strcmp(nh->name, buffer)) break;
    }

    /*
     * If we don't have anything here, then try and see if this
     * is a local symbol to the current stack frame.  No matter
     * what, we have nothing more to do, so we let that function
     * decide what we ultimately return.
     */
    if (!nh) return DEBUG_GetStackSymbolValue(name, addr);


    if( lineno == -1 )
      {
	*addr = nh->addr;
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
 *           DEBUG_SetSymbolValue
 *
 * Set the address of a named symbol.
 */
BOOL32 DEBUG_SetSymbolValue( const char * name, const DBG_ADDR *addr )
{
    char buffer[256];
    struct name_hash *nh;

    for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
        if (!strcmp(nh->name, name)) break;

    if (!nh && (name[0] != '_'))
    {
        buffer[0] = '_';
        strcpy(buffer+1, name);
        for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
            if (!strcmp(nh->name, buffer)) break;
    }

    if (!nh) return FALSE;
    nh->addr = *addr;
    DBG_FIX_ADDR_SEG( &nh->addr, DS_reg(&DEBUG_context) );
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
				      unsigned int ebp)
{
    static char name_buffer[MAX_PATH + 256];
    static char arglist[1024];
    static char argtmp[256];
    struct name_hash * nearest = NULL;
    struct name_hash * nh;
    unsigned int nearest_address = 0;
    unsigned	int * ptr;
    int lineno;
    char * lineinfo, *sourcefile;
    int i;
    char linebuff[16];

    *rtn = NULL;

    for(i=0; i<NR_NAME_HASH; i++)
    {
        for (nh = name_hash_table[i]; nh; nh = nh->next)
            if (nh->addr.seg == addr->seg &&
                nh->addr.off <= addr->off &&
                nh->addr.off >= nearest_address)
            {
                nearest_address = nh->addr.off;
                nearest = nh;
            }
    }
    if (!nearest) return NULL;

    *rtn = nearest;
    lineinfo = "";
    lineno = -1;

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

	    sprintf(argtmp, "%s=0x%x", nearest->local_vars[i].name,
		    *ptr);
	    strcat(arglist, argtmp);
	  }
	if( arglist[0] == '(' )
	  {
	    strcat(arglist, ")");
	  }
      }

    if( (nearest->sourcefile != NULL) && (flag == TRUE)
	&& (addr->off - nearest->addr.off < 0x100000) )
      {

	/*
	 * Try and find the nearest line number to the current offset.
	 */
	if( nearest->linetab != NULL )
	  {
	    /*
	     * FIXME - this is an inefficient linear search.  A binary
	     * search would be better if this gets to be a performance
	     * bottleneck.
	     */
	    for(i=0; i < nearest->n_lines; i++)
	      {
		if( addr->off < nearest->linetab[i].pc_offset.off )
		{
		  break;
		}
		lineno = nearest->linetab[i].line_number;
	      }
	  }

	if( lineno != -1 )
	  {
	    sprintf(linebuff, ":%d", lineno);
	    lineinfo = linebuff;
	  }

        /* Remove the path from the file name */
        sourcefile = strrchr( nearest->sourcefile, '/' );
        if (!sourcefile) sourcefile = nearest->sourcefile;
        else sourcefile++;

	if (addr->off == nearest->addr.off)
	  sprintf( name_buffer, "%s%s [%s%s]", nearest->name, 
		   arglist, sourcefile, lineinfo);
	else
	  sprintf( name_buffer, "%s+0x%lx%s [%s%s]", nearest->name,
		   addr->off - nearest->addr.off, 
		   arglist, sourcefile, lineinfo );
      }
    else
      {
	if (addr->off == nearest->addr.off)
	  sprintf( name_buffer, "%s%s", nearest->name, arglist);
	else
	  sprintf( name_buffer, "%s+0x%lx%s", nearest->name,
		   addr->off - nearest->addr.off, arglist);
      }
    return name_buffer;
}


/***********************************************************************
 *           DEBUG_ReadSymbolTable
 *
 * Read a symbol file into the hash table.
 */
void DEBUG_ReadSymbolTable( const char * filename )
{
    FILE * symbolfile;
    DBG_ADDR addr = { 0, 0 };
    int nargs;
    char type;
    char * cpnt;
    char buffer[256];
    char name[256];

    if (!(symbolfile = fopen(filename, "r")))
    {
        fprintf( stderr, "Unable to open symbol table %s\n", filename );
        return;
    }

    fprintf( stderr, "Reading symbols from file %s\n", filename );

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
		
        nargs = sscanf(buffer, "%lx %c %s", &addr.off, &type, name);
        DEBUG_AddSymbol( name, &addr, NULL );
    }
    fclose(symbolfile);
}


/***********************************************************************
 *           DEBUG_LoadEntryPoints
 *
 * Load the entry points of all the modules into the hash table.
 */
void DEBUG_LoadEntryPoints(void)
{
    MODULEENTRY entry;
    NE_MODULE *pModule;
    DBG_ADDR addr;
    char buffer[256];
    unsigned char *cpnt, *name;
    FARPROC16 address;
    BOOL32 ok;

    for (ok = ModuleFirst(&entry); ok; ok = ModuleNext(&entry))
    {
        if (!(pModule = MODULE_GetPtr( entry.hModule ))) continue;

        name = (unsigned char *)pModule + pModule->name_table;

        /* First search the resident names */

        cpnt = (unsigned char *)pModule + pModule->name_table;
        while (*cpnt)
        {
            cpnt += *cpnt + 1 + sizeof(WORD);
            sprintf( buffer, "%*.*s.%*.*s", *name, *name, name + 1,
                     *cpnt, *cpnt, cpnt + 1 );
            if ((address = MODULE_GetEntryPoint( entry.hModule,
                                            *(WORD *)(cpnt + *cpnt + 1) )))
            {
                addr.seg = HIWORD(address);
                addr.off = LOWORD(address);
                DEBUG_AddSymbol( buffer, &addr, NULL );
            }
        }

        /* Now search the non-resident names table */

        if (!pModule->nrname_handle) continue;  /* No non-resident table */
        cpnt = (char *)GlobalLock16( pModule->nrname_handle );
        while (*cpnt)
        {
            cpnt += *cpnt + 1 + sizeof(WORD);
            sprintf( buffer, "%*.*s.%*.*s", *name, *name, name + 1,
                     *cpnt, *cpnt, cpnt + 1 );
            if ((address = MODULE_GetEntryPoint( entry.hModule,
                                                *(WORD *)(cpnt + *cpnt + 1) )))
            {
                addr.seg = HIWORD(address);
                addr.off = LOWORD(address);
                DEBUG_AddSymbol( buffer, &addr, NULL);
            }
        }
    }
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
      func->lines_alloc += 32;
      func->linetab = realloc(func->linetab,
			      func->lines_alloc * sizeof(WineLineNo));
    }

  func->linetab[func->n_lines].line_number = line_num;
  func->linetab[func->n_lines].pc_offset.seg = func->addr.seg;
  func->linetab[func->n_lines].pc_offset.off = func->addr.off + offset;
  func->n_lines++;
}


void
DEBUG_AddLocal( struct name_hash * func, int regno, 
		int offset,
		int pc_start,
		int pc_end,
		char * name)
{
  if( func == NULL )
    {
      return;
    }

  if( func->n_locals + 1 >= func->locals_alloc )
    {
      func->locals_alloc += 32;
      func->local_vars = realloc(func->local_vars,
			      func->locals_alloc * sizeof(WineLocals));
    }

  func->local_vars[func->n_locals].regno = regno;
  func->local_vars[func->n_locals].offset = offset;
  func->local_vars[func->n_locals].pc_start = pc_start;
  func->local_vars[func->n_locals].pc_end = pc_end;
  func->local_vars[func->n_locals].name = xstrdup(name);
  func->n_locals++;
}

