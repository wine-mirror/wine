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
#include <segmem.h>
#include <prototypes.h>
#include <wine.h>
#include <dlls.h>

struct  name_hash{
	struct name_hash * next;
	unsigned int * address;
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


void add_hash(char * name, unsigned int * address){
	struct name_hash  * new;
	int hash;

	new = (struct  name_hash *) malloc(sizeof(struct name_hash));
	new->address = address;
	new->name = strdup(name);
	new->next = NULL;
	hash = name_hash(name);

	/* Now insert into the hash table */
	new->next = name_hash_table[hash];
	name_hash_table[hash] = new;
}

unsigned int * find_hash(char * name){
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


	return (unsigned int *) 0xffffffff;
}


static char name_buffer[256];

char * find_nearest_symbol(unsigned int * address){
	struct name_hash * nearest;
	struct name_hash start;
	struct name_hash  * nh;
	int i;
	
	nearest = &start;
	start.address = (unsigned int *) 0;
	
	for(i=0; i<NR_NAME_HASH; i++) {
		for(nh = name_hash_table[i]; nh; nh = nh->next)
			if(nh->address <= address && nh->address > nearest->address)
				nearest = nh;
	};
	if((unsigned int) nearest->address == 0) return NULL;

	sprintf(name_buffer, "%s+0x%x", nearest->name, ((unsigned int) address) - 
		((unsigned int) nearest->address));
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
		add_hash(name, (unsigned int *) addr);
      };
      fclose(symbolfile);
}


/* Load the entry points from the dynamic linking into the hash tables. 
 * This does not work yet - something needs to be added before it scans the
 * tables correctly 
 */

void
load_entrypoints(){
	char buffer[256];
	char * cpnt;
	int j, ordinal, len;
	unsigned int address;

	struct w_files * wpnt;
	for(wpnt = wine_files; wpnt; wpnt = wpnt->next){
		cpnt  = wpnt->nrname_table;
		while(1==1){
			if( ((int) cpnt)  - ((int)wpnt->nrname_table) >  
			   wpnt->ne_header->nrname_tab_length)  break;
			len = *cpnt++;
			strncpy(buffer, cpnt, len);
			buffer[len] = 0;
			ordinal =  *((unsigned short *)  (cpnt +  len));
			j = GetEntryPointFromOrdinal(wpnt, ordinal);		
			address  = j & 0xffff;
			j = j >> 16;
			address |= (wpnt->selector_table[j].selector) << 16;
			fprintf(stderr,"%s -> %x\n", buffer, address);
			add_hash(buffer, (unsigned int *) address);
			cpnt += len + 2;
		};
	};
	return;
}
