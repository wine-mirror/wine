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

struct name_hash
{
    struct name_hash * next;
    char *             name;
    DBG_ADDR           addr;
};

#define NR_NAME_HASH 128

static struct name_hash * name_hash_table[NR_NAME_HASH] = {0,};

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
void DEBUG_AddSymbol( const char * name, const DBG_ADDR *addr )
{
    struct name_hash  * new;
    int hash;

    new = (struct name_hash *) xmalloc(sizeof(struct name_hash));
    new->addr = *addr;
    new->name = strdup(name);
    new->next = NULL;
    hash = name_hash(name);

    /* Now insert into the hash table */
    new->next = name_hash_table[hash];
    name_hash_table[hash] = new;
}


/***********************************************************************
 *           DEBUG_GetSymbolValue
 *
 * Get the address of a named symbol.
 */
BOOL DEBUG_GetSymbolValue( const char * name, DBG_ADDR *addr )
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
    *addr = nh->addr;
    return TRUE;
}


/***********************************************************************
 *           DEBUG_SetSymbolValue
 *
 * Set the address of a named symbol.
 */
BOOL DEBUG_SetSymbolValue( const char * name, const DBG_ADDR *addr )
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
    DBG_FIX_ADDR_SEG( &nh->addr, DS_reg(DEBUG_context) );
    return TRUE;
}


/***********************************************************************
 *           DEBUG_FindNearestSymbol
 *
 * Find the symbol nearest to a given address.
 */
const char * DEBUG_FindNearestSymbol( const DBG_ADDR *addr )
{
    static char name_buffer[256];
    struct name_hash * nearest = NULL;
    struct name_hash * nh;
    unsigned int nearest_address = 0;
    int i;

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

    if (addr->off == nearest->addr.off)
        sprintf( name_buffer, "%s", nearest->name );
    else
        sprintf( name_buffer, "%s+0x%lx", nearest->name,
                addr->off - nearest->addr.off );
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
        DEBUG_AddSymbol( name, &addr );
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
    unsigned int address;
    BOOL ok;

    fprintf( stderr, "Adding symbols from loaded modules\n" );
    for (ok = ModuleFirst(&entry); ok; ok = ModuleNext(&entry))
    {
        if (!(pModule = (NE_MODULE *)GlobalLock( entry.hModule ))) continue;

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
                DEBUG_AddSymbol( buffer, &addr );
            }
        }

        /* Now search the non-resident names table */

        if (!pModule->nrname_handle) continue;  /* No non-resident table */
        cpnt = (char *)GlobalLock( pModule->nrname_handle );
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
                DEBUG_AddSymbol( buffer, &addr );
            }
        }
    }
}
