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
#include "wine.h"

struct  name_hash{
	struct name_hash * next;
        unsigned int segment;
	unsigned int address;
	char * name;
};

#define NR_NAME_HASH 128

static struct name_hash * name_hash_table[NR_NAME_HASH] = {0,};

static  unsigned int name_hash(const char * name){
	unsigned int hash = 0;
	const char * p;

	p = name;

	while (*p) hash = (hash << 15) + (hash << 3) + (hash >> 3) + *p++;
	return hash % NR_NAME_HASH;

}


void add_hash(char * name, unsigned int segment, unsigned int address)
{
	struct name_hash  * new;
	int hash;

	new = (struct  name_hash *) malloc(sizeof(struct name_hash));
        new->segment = segment;
	new->address = address;
	new->name = strdup(name);
	new->next = NULL;
	hash = name_hash(name);

	/* Now insert into the hash table */
	new->next = name_hash_table[hash];
	name_hash_table[hash] = new;
}

unsigned int find_hash(char * name)
{
	char buffer[256];
	struct name_hash  * nh;

	for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
		if(strcmp(nh->name, name) == 0) return nh->address;

	if(name[0] != '_'){
		buffer[0] = '_';
		strcpy(buffer+1, name);
		for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
			if(strcmp(nh->name, buffer) == 0) return nh->address;
	};


	return 0xffffffff;
}


static char name_buffer[256];

char * find_nearest_symbol(unsigned int segment, unsigned int address)
{
	struct name_hash * nearest;
	struct name_hash  * nh;
        unsigned int nearest_address;
	int i;
	
	nearest = NULL;
        nearest_address = 0;
	
	for(i=0; i<NR_NAME_HASH; i++) {
		for(nh = name_hash_table[i]; nh; nh = nh->next)
			if (nh->segment == segment &&
                            nh->address <= address &&
                            nh->address >= nearest_address)
                        {
                            nearest_address = nh->address;
                            nearest = nh;
                        }
	}
        if (!nearest) return NULL;

        if (address == nearest->address)
            sprintf( name_buffer, "%s", nearest->name );
	else
            sprintf( name_buffer, "%s+0x%x", nearest->name,
                     address - nearest->address );
	return name_buffer;
}


void
read_symboltable(char * filename){
	FILE * symbolfile;
	unsigned int addr;
	int nargs;
	char type;
	char * cpnt;
	char buffer[256];
	char name[256];

	symbolfile = fopen(filename, "r");
	if(!symbolfile) {
		fprintf(stderr,"Unable to open symbol table %s\n", filename);
		return;
	};

	fprintf(stderr,"Reading symbols from file %s\n", filename);


	while (1)
	{
		fgets(buffer, sizeof(buffer),  symbolfile);
		if (feof(symbolfile)) break;
		
		/* Strip any text after a # sign (i.e. comments) */
		cpnt = buffer;
		while(*cpnt){
			if(*cpnt == '#') {*cpnt = 0; break; };
			cpnt++;
		};
		
		/* Quietly ignore any lines that have just whitespace */
		cpnt = buffer;
		while(*cpnt){
			if(*cpnt != ' ' && *cpnt != '\t') break;
			cpnt++;
		};
		if (!(*cpnt) || *cpnt == '\n') {
			continue;
		};
		
		nargs = sscanf(buffer, "%x %c %s", &addr, &type, name);
		add_hash(name, 0, addr);
      };
      fclose(symbolfile);
}


/* Load the entry points from the dynamic linking into the hash tables. 
 * This does not work yet - something needs to be added before it scans the
 * tables correctly 
 */

void load_entrypoints( HMODULE hModule )
{
    char buffer[256];
    unsigned char *cpnt, *name;
    NE_MODULE *pModule;
    unsigned int address;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return;
    name = (unsigned char *)pModule + pModule->name_table;

      /* First search the resident names */

    cpnt = (unsigned char *)pModule + pModule->name_table;
    while (*cpnt)
    {
        cpnt += *cpnt + 1 + sizeof(WORD);
        sprintf( buffer, "%*.*s.%*.*s", *name, *name, name + 1,
                 *cpnt, *cpnt, cpnt + 1 );
        address = MODULE_GetEntryPoint( hModule, *(WORD *)(cpnt + *cpnt + 1) );
        if (address) add_hash( buffer, HIWORD(address), LOWORD(address) );
    }

      /* Now search the non-resident names table */

    if (!pModule->nrname_handle) return;  /* No non-resident table */
    cpnt = (char *)GlobalLock( pModule->nrname_handle );
    while (*cpnt)
    {
        cpnt += *cpnt + 1 + sizeof(WORD);
        sprintf( buffer, "%*.*s.%*.*s", *name, *name, name + 1,
                 *cpnt, *cpnt, cpnt + 1 );
        address = MODULE_GetEntryPoint( hModule, *(WORD *)(cpnt + *cpnt + 1) );
        if (address) add_hash( buffer, HIWORD(address), LOWORD(address) );
    }
}
