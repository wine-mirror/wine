/*
 * Wine debugger utility routines
 * Eric Youngdale
 * 9/93
 */

#include <stdio.h>
#include <stdlib.h>
#include "debugger.h"

extern char * find_nearest_symbol( unsigned int seg, unsigned int addr );

void application_not_running()
{
  fprintf(stderr,"Application not running\n");
}

void print_address( unsigned int segment, unsigned int addr, int addrlen )
{
    char *name = find_nearest_symbol( segment, addr );

    if (segment) fprintf( stderr, "0x%04x:", segment );
    if (addrlen == 16) fprintf( stderr, "0x%04x", addr );
    else fprintf( stderr, "0x%08x", addr );
    if (name) fprintf( stderr, " (%s)", name );
}


void examine_memory( unsigned int segment, unsigned int addr,
                     int count, char format )
{
    char * pnt;
    unsigned int * dump;
    unsigned short int * wdump;
    int i;

    if (segment == 0xffffffff) segment = (format == 'i' ? CS : DS);
    if ((segment == WINE_CODE_SELECTOR) || (segment == WINE_DATA_SELECTOR))
        segment = 0;

    if (format != 'i' && count > 1)
    {
        print_address( segment, addr, dbg_mode );
        fprintf(stderr,":  ");
    }

    pnt = segment ? (char *)PTR_SEG_OFF_TO_LIN(segment,addr) : (char *)addr;

    switch(format)
    {
	case 's':
		if (count == 1) count = 256;
	        while(*pnt && count) {
			fputc( *pnt++, stderr);
			count--;
		};
		fprintf(stderr,"\n");
		return;

	case 'i':
		for(i=0; i<count; i++) {
			print_address( segment, addr, dbg_mode );
			fprintf(stderr,":  ");
			addr = db_disasm( segment, addr );
			fprintf(stderr,"\n");
		};
		return;
	case 'x':
		dump = (unsigned int *)pnt;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %8.8x", *dump++);
                        addr += 4;
			if ((i % 8) == 7) {
				fprintf(stderr,"\n");
				print_address( segment, addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'd':
		dump = (unsigned int *)pnt;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %d", *dump++);
                        addr += 4;
			if ((i % 8) == 7) {
				fprintf(stderr,"\n");
				print_address( segment, addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'w':
		wdump = (unsigned short *)pnt;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %04x", *wdump++);
                        addr += 2;
			if ((i % 10) == 7) {
				fprintf(stderr,"\n");
				print_address( segment, addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'c':
		for(i=0; i<count; i++) 
		{
			if(*pnt < 0x20) {
				fprintf(stderr,"  ");
				pnt++;
			} else
				fprintf(stderr," %c", *pnt++);
                        addr++;
			if ((i % 32) == 7) {
				fprintf(stderr,"\n");
				print_address( segment, addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'b':
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %02x", (*pnt++) & 0xff);
                        addr++;
			if ((i % 32) == 7) {
				fprintf(stderr,"\n");
				print_address( segment, addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	};
	
	/* The rest are fairly straightforward */

	fprintf(stderr,"examine mem: %x %d %c\n", addr, count, format);
}

char * helptext[] = {
"The commands accepted by the Wine debugger are a small subset",
"of the commands that gdb would accept.  The commands currently",
"are:\n",
"  break [*<addr>]                      delete break bpnum",
"  disable bpnum                        enable bpnum",
"  help                                 quit",
"  x <expr>                             cont",
"  step                                 next",
"  mode [16,32]                         print <expr>",
"  set <reg> = <expr>                   set *<expr> = <expr>",
"  info [reg,stack,break,segments]      bt",
"  symbolfile <filename>                define <identifier> <expr>",
"",
"The 'x' command accepts repeat counts and formats (including 'i') in the",
"same way that gdb does.",
"",
" The following are examples of legal expressions:",
" $eax     $eax+0x3   0x1000   ($eip + 256)  *$eax   *($esp + 3)",
" Also, a nm format symbol table can be read from a file using the",
" symbolfile command.  Symbols can also be defined individually with",
" the define command.",
"",
NULL};

void dbg_help(){
	int i;
	i = 0;
	while(helptext[i]) fprintf(stderr,"%s\n", helptext[i++]);
}
