
#include <stdio.h>

#define N_BP 25

struct wine_bp{
  unsigned long addr;
  char in_use;
  char enabled;
  unsigned char databyte;
};

static struct wine_bp wbp[N_BP] = {{0,},};

void info_break()
{
  int j;
  fprintf(stderr,"Breakpoint status\n");
  for(j=0; j<N_BP; j++)
    if(wbp[j].in_use)
      fprintf(stderr,"%d: %c %8.8x\n", j, (wbp[j].enabled ? 'y' : 'n'),
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
	return;
      }
  fprintf(stderr,"No more breakpoints\n");
}

void insert_break(int flag)
{
  unsigned char * pnt;
  int j;

  for(j=0; j<N_BP; j++)
    if(wbp[j].enabled)
      {
	pnt = (unsigned char *) wbp[j].addr;
	if(flag) {
	  wbp[j].databyte = *pnt;
	  *pnt = 0xcc;  /* Change to an int 3 instruction */
	} else {
	  *pnt = wbp[j].databyte;
	}
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
