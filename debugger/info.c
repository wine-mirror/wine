/*
 * Wine debugger utility routines
 * Eric Youngdale
 * 9/93
 */

#include <stdio.h>
#include <stdlib.h>
#include "db_disasm.h"
#include "regpos.h"

extern int * regval;
extern unsigned int dbg_mask;
extern unsigned int dbg_mode;

void application_not_running()
{
  fprintf(stderr,"Application not running\n");
}

void print_address(unsigned int addr, FILE * outfile){
	char * name;
	extern char * find_nearest_symbol(unsigned int *);

	name = find_nearest_symbol((unsigned int *) addr);
	if(name)
		fprintf(outfile,"0x%8.8x(%s)", addr, name);
	else
		fprintf(outfile,"0x%8.8x", addr);

}

#ifdef GNU_DISASM
void print_address_info(bfd_vma addr, disassemble_info * info){
	print_address((unsigned int) addr, info->stream);
}

int print_insn(char *realmemaddr, char *memaddr, FILE *stream, int addrlen){
	static disassemble_info info;
	static int initialized = 0;

	if (!initialized) {
		INIT_DISASSEMBLE_INFO(info, stderr);
		info.print_address_func = print_address_info;
		initialized = 1;
	}
	info.stream = stream;
	info.buffer = memaddr;
	info.buffer_vma = (bfd_vma) realmemaddr;
	info.buffer_length = 1024;
	if (addrlen == 16)
		return print_insn_i286((bfd_vma) realmemaddr, &info);
	if (addrlen == 32)
		return print_insn_i386((bfd_vma) realmemaddr, &info);
	fprintf(stderr, "invalid address length %d.\n", addrlen);
	return 0;
}
#else
void db_task_printsym(unsigned int addr){
  print_address(addr, stderr);
}

int print_insn(char *realmemaddr, char *memaddr, FILE *stream, int addrlen)
{
	if (addrlen == 16)
		return db_disasm((unsigned int) realmemaddr, 0, 1) - 
			((unsigned int) realmemaddr);
	if (addrlen == 32)
	    return db_disasm((unsigned int) realmemaddr, 0, 0) -
	      ((unsigned int) realmemaddr);
	fprintf(stderr, "invalid address length %d.\n", addrlen);
	return 0;
}
#endif

void info_reg(){

	  if(!regval) {
	    application_not_running();
	    return;
	  }

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

	if(!regval) {
	  application_not_running();
	  return;
	}

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
				print_address((unsigned int) pnt, stderr);
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'b':
		pnt = (char *) addr;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %02x", (*pnt++) & 0xff);
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
"  break *<addr>                        bt",
"  disable bpnum                        enable bpnum",
"  help                                 quit",
"  x <expr>                             cont",
"  mode [16,32]                         print <expr>",
"  set <reg> = <expr>                   set *<expr> = <expr>",
"  info [reg,stack,break,segments]      symbolfile <filename>",
"  define <identifier> <expr>",
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


struct frame{
  union{
    struct {
      unsigned short saved_bp;
      unsigned short saved_ip;
      unsigned short saved_cs;
    } win16;
    struct {
      unsigned long saved_bp;
      unsigned long saved_ip;
      unsigned short saved_cs;
    } win32;
  } u;
};


void dbg_bt(){
  struct frame * frame;
  unsigned short cs;
  int frameno = 0;

  if(!regval) {
    application_not_running();
    return;
  }

  frame = (struct frame *) ((SC_EBP(dbg_mask) & ~1) | (SC_SS << 16));

  fprintf(stderr,"Backtrace:\n");
  fprintf(stderr,"%d ",frameno);
  print_address(frame->u.win32.saved_ip,stderr);
  cs = SC_CS;
  while((cs & 3) == 3) {
    /* See if in 32 bit mode or not.  Assume GDT means 32 bit. */
    if ((cs & 7) != 7) {
      void CallTo32();
      fprintf(stderr,"\n%d ",frameno++);
      print_address(frame->u.win32.saved_ip,stderr);
      if(frame->u.win32.saved_ip<((unsigned long)CallTo32+1000))break;
      frame = (struct frame *) frame->u.win32.saved_bp;
    } else {
      cs = frame->u.win16.saved_cs;
      fprintf(stderr,"%d %4.4x:%4.4x\n", frameno++, cs, 
	      frame->u.win16.saved_ip);
      frame = (struct frame *) ((frame->u.win16.saved_bp & ~1) |
				(SC_SS << 16));
    }
  }
  putchar('\n');
}

