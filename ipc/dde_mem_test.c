/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_mem_test.c
 * Purpose :  test shared DDE memory functionality for DDE
 * Usage:     Look for assertion failures
 ***************************************************************************
 */
#include <stdio.h>
#include <assert.h>
#include <win.h>
#include "dde_mem.h"
/* stub */

void ATOM_GlobalInit()
{
  printf("ATOM_GlobalInit\n");
}


int main()
{
  HWND h1,h2,h3;
  int ret;
  void *p1,*p2,*p3,*p;
  SHMDATA shmdata;
  
  /* alloc h1, h2, h3 */

  setbuf(stdout,NULL);
  p1=DDE_malloc(GMEM_DDESHARE, 0x6000, &shmdata);
  h1= shmdata.handle;
  assert(p1 != NULL);
  assert(h1 != 0);
  p2=DDE_malloc(GMEM_DDESHARE, 0xff00, &shmdata);
  h2= shmdata.handle;
  assert(p2 != NULL);
  assert(h2 != 0);
  p3=DDE_malloc(GMEM_DDESHARE, 0x6000, &shmdata);
  h3= shmdata.handle;
  assert(p3 != 0);
  assert(h3 != 0);

  /* lock h1, h2, h3 */
  p=DDE_AttachHandle(h1,NULL);
  assert(p1==p);
  p=DDE_AttachHandle(h2,NULL);
  assert(p2==p);
  p=DDE_AttachHandle(h3,NULL);
  assert(p3==p);


  
  ret=DDE_GlobalFree(h1);
  assert(ret==0);
  /* do some implementation dependant tests */
  p=DDE_malloc(GMEM_DDESHARE, 0x6000, &shmdata);
  assert(p!=NULL);
  assert(shmdata.handle==h1);
  p=DDE_AttachHandle(h1,NULL);
  assert(p1==p);

  /* check freeing */
  ret=DDE_GlobalFree(h1);
  assert(ret==0);
  ret=DDE_GlobalFree(h2);
  assert(ret==0);
  ret=DDE_GlobalFree(h3);
  assert(ret==0);
  return 0;
}
