
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef linux
#include <sys/utsname.h>
#endif
#include <windows.h>
#include "db_disasm.h"

#define N_BP 25

extern int dbg_mode;

struct wine_bp{
  unsigned long addr;
  unsigned long next_addr;
  char in_use;
  char enabled;
  unsigned char databyte;
};

static struct wine_bp wbp[N_BP] = {{0,},};

static int current_bp = -1;
static int cont_mode=0;	/* 0 - continuous execution
			   1 - advancing after breakpoint
			   2 - single step - not implemented
			*/

void info_break(void)
{
  int j;
  fprintf(stderr,"Breakpoint status\n");
  for(j=0; j<N_BP; j++)
    if(wbp[j].in_use)
      fprintf(stderr,"%d: %c %8lx\n", j, (wbp[j].enabled ? 'y' : 'n'),
	      wbp[j].addr);
}

void disable_break(int bpnum)
{
  if(bpnum >= N_BP || bpnum < 0)
    fprintf(stderr,"Breakpoint number out of range\n");

  wbp[bpnum].enabled = 0;
}

void enable_break(int bpnum)
{
  if(bpnum >= N_BP || bpnum < 0)
    fprintf(stderr,"Breakpoint number out of range\n");

  wbp[bpnum].enabled = 1;
}

void add_break(unsigned long addr)
{
  int j;
  for(j=0; j<N_BP; j++)
    if(!wbp[j].in_use)
      {
	wbp[j].in_use = 1;
	wbp[j].enabled = 1;
	wbp[j].addr = addr;
	wbp[j].next_addr = 0;
	return;
      }
  fprintf(stderr,"No more breakpoints\n");
}

static void bark()
{
  static int barked=0;
  if(barked)return;
  barked=1;
  perror("Sorry, can't set break point");
#ifdef linux
  {struct utsname buf;
  uname(&buf);
  if(strcmp(buf.sysname,"Linux")==0)
  {	if(strcmp(buf.release,"1.1.62")<0)
	  fprintf(stderr,"Your current Linux release is %s. "
		"You should upgrade to 1.1.62 or higher\n"
		"Alternatively, in /usr/src/linux/fs/exec.c,"
		" change MAP_SHARED to MAP_PRIVATE.\n", buf.release);
  } else
  fprintf(stderr,"Why did you compile for Linux, while your system is"
   " actually %s?\n",buf.sysname);
  }
#endif
}
  
void insert_break(int flag)
{
  unsigned char * pnt;
  int j;

  for(j=0; j<N_BP; j++)
    if(wbp[j].enabled)
      {
        /* There are a couple of problems with this. On Linux prior to
           1.1.62, this call fails (ENOACCESS) due to a bug in fs/exec.c.
           This code is currently not tested at all on BSD.
           How do I determine the page size in a more symbolic manner?
           And why does mprotect need that start address of the page
           in the first place?
           Not that portability matters, this code is i386 only anyways...
           How do I get the old protection in order to restore it later on?
        */
	if(mprotect((caddr_t)(wbp[j].addr & (~4095)), 4096, 
	  PROT_READ|PROT_WRITE|PROT_EXEC) == -1){
	    bark();
            return;
	}
	pnt = (unsigned char *) wbp[j].addr;
	if(flag) {
	  wbp[j].databyte = *pnt;
	  *pnt = 0xcc;  /* Change to an int 3 instruction */
	} else {
	  *pnt = wbp[j].databyte;
	}
	mprotect((caddr_t)(wbp[j].addr & ~4095), 4096, PROT_READ|PROT_EXEC);
      }
}

/* Get the breakpoint number that we broke upon */
int get_bpnum(unsigned int addr)
{
  int j;

  for(j=0; j<N_BP; j++)
    if(wbp[j].enabled)
      if(wbp[j].addr == addr) return j;

  return -1;
}

void toggle_next(int num)
{
   unsigned int addr;
   addr=wbp[num].addr;
   if(wbp[num].next_addr == 0)
	wbp[num].next_addr = db_disasm( 0, addr, (dbg_mode == 16) );
   wbp[num].addr=wbp[num].next_addr;
   wbp[num].next_addr=addr;
}

int should_continue(int bpnum)
{
    if(bpnum<0)return 0;
    toggle_next(bpnum);
    if(bpnum==current_bp){
        current_bp=-1;
	cont_mode=0;
	return 1;
    }
    cont_mode=1;
    current_bp=bpnum;
    return 0;
}
