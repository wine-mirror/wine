/*
 * Wine debugger utility routines
 * Eric Youngdale
 * 9/93
 */

#include <stdio.h>
#include "regpos.h"

extern int * regval;
extern unsigned int dbg_mask;
extern unsigned int dbg_mode;

extern int print_insn(char * memaddr, char * realaddr, FILE * stream, int addrlen);

/* THese three helper functions eliminate the need for patching the
module from gdb for disassembly of code */

void read_memory(char * memaddr, char * buffer, int len){
	memcpy(buffer, memaddr, len);
}

void fputs_filtered(char * buffer, FILE * outfile){
	fputs(buffer, outfile);
}

void print_address(unsigned int addr, FILE * outfile){
	char * name;
	name = find_nearest_symbol(addr);
	if(name)
		fprintf(outfile,"0x%8.8x(%s)", addr, name);
	else
		fprintf(outfile,"0x%8.8x", addr);

}


void info_reg(){
	fprintf(stderr,"Register dump:\n");
	/* First get the segment registers out of the way */
	fprintf(stderr," CS:%4.4x SS:%4.4x DS:%4.4x ES:%4.4x GS:%4.4x FS:%4.4x\n", 
		SC_CS, SC_SS, SC_DS, SC_ES, SC_GS, SC_FS);

	/* Now dump the main registers */
	fprintf(stderr," EIP:%8.8x ESP:%8.8x EBP:%8.8x EFLAGS:%8.8x\n", 
		SC_EIP(dbg_mask), SC_ESP(dbg_mask), SC_EBP(dbg_mask), SC_EFLAGS);

	/* And dump the regular registers */

	fprintf(stderr," EAX:%8.8x EBX:%8.8x ECX:%8.8x EDX:%8.8x\n", 
		SC_EAX(dbg_mask), SC_EBX(dbg_mask), SC_ECX(dbg_mask), SC_EDX(dbg_mask));

	/* Finally dump these main registers */
	fprintf(stderr," EDI:%8.8x ESI:%8.8x\n", 
		SC_EDI(dbg_mask), SC_ESI(dbg_mask));

}

void info_stack(){
	unsigned int * dump;
	int i;


	fprintf(stderr,"Stack dump:\n");
	dump = (int*) SC_EIP(dbg_mask);
	for(i=0; i<22; i++) 
	{
	    fprintf(stderr," %8.8x", *dump++);
	    if ((i % 8) == 7)
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"\n");
}


void examine_memory(int addr, int count, char format){
	char * pnt;
	unsigned int * dump;
	unsigned short int * wdump;
	int i;

	if((addr & 0xffff0000) == 0 && dbg_mode == 16)
	        addr |= (format == 'i' ? SC_CS : SC_DS) << 16;


	if(format != 'i' && count > 1) {
		print_address(addr, stderr);
		fprintf(stderr,":  ");
	};


	switch(format){
	case 's':
		pnt = (char *) addr;
		if (count == 1) count = 256;
	        while(*pnt && count) {
			fputc( *pnt++, stderr);
			count--;
		};
		fprintf(stderr,"\n");
		return;

	case 'i':
		for(i=0; i<count; i++) {
			print_address(addr, stderr);
			fprintf(stderr,":  ");
			addr += print_insn((char *) addr, (char *) addr, stderr, dbg_mode);
			fprintf(stderr,"\n");
		};
		return;
	case 'x':
		dump = (unsigned int *) addr;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %8.8x", *dump++);
			if ((i % 8) == 7) {
				fprintf(stderr,"\n");
				print_address((unsigned int) dump, stderr);
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'd':
		dump = (unsigned int *) addr;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %d", *dump++);
			if ((i % 8) == 7) {
				fprintf(stderr,"\n");
				print_address((unsigned int) dump, stderr);
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'w':
		wdump = (unsigned short int *) addr;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %x", *wdump++);
			if ((i % 10) == 7) {
				fprintf(stderr,"\n");
				print_address((unsigned int) wdump, stderr);
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'c':
		pnt = (char *) addr;
		for(i=0; i<count; i++) 
		{
			if(*pnt < 0x20) {
				fprintf(stderr,"  ");
				pnt++;
			} else
				fprintf(stderr," %c", *pnt++);
			if ((i % 32) == 7) {
				fprintf(stderr,"\n");
				print_address((unsigned int) dump, stderr);
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'b':
		pnt = (char *) addr;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %02.2x", (*pnt++) & 0xff);
			if ((i % 32) == 7) {
				fprintf(stderr,"\n");
				print_address((unsigned int) pnt, stderr);
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
"  info reg",
"  info stack",
"  help",
"  quit",
"  print <expr>",
"  symbolfile <filename>",
"  define <identifier> <expr>",
"  x <expr>",
"  cont",
"  set <reg> = <expr>",
"  set *<expr> = <expr>",
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
"The disassembly code seems to work most of the time, but it does get",
"a little confused at times.  The 16 bit mode probably has not been used",
"much so there are probably bugs.  I snagged the file from the gdb-4.7",
"source tree, which is what was on my latest cdrom.  I should check to see",
"if newer versions of gdb have anything substanitally different for the",
"disassembler.",
"",
NULL};

void dbg_help(){
	int i;
	i = 0;
	while(helptext[i]) fprintf(stderr,"%s\n", helptext[i++]);
}

