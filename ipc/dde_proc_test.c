/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_proc.c
 * Purpose :  test DDE signals and processes functionality for DDE
 * Usage:     run two independant processes, one with an argument another
 *            without (with the argument is the server).
 ***************************************************************************
 */
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/syscall.h>
#include <sys/param.h>
#else
#include <syscall.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <win.h>
#include "dde.h"
#include "dde_proc.h"
#include "shm_main_blk.h"

#if !defined(BSD4_4) || defined(linux) || defined(__FreeBSD__)
char * cstack[4096];
#endif
#ifdef linux
extern void ___sig_restore();
extern void ___masksig_restore();

/* Similar to the sigaction function in libc, except it leaves alone the
   restorer field */

static int
wine_sigaction(int sig,struct sigaction * new, struct sigaction * old)
{
	__asm__("int $0x80":"=a" (sig)
		:"0" (SYS_sigaction),"b" (sig),"c" (new),"d" (old));
	if (sig>=0)
		return 0;
	errno = -sig;
	return -1;
}
#endif

struct sigaction usr2_act;


void init_signals()
{
#ifdef linux
  usr2_act.sa_handler = (__sighandler_t) stop_wait;
  usr2_act.sa_flags = 0;
  usr2_act.sa_restorer = 
    (void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
  wine_sigaction(SIGUSR2,&usr2_act,NULL);
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
  usr2_act.sa_hadnler = (void (*)) stop_wait;
  usr2_act.sa_flags = SA_ONSTACK;
  usr2_act.sa_mask = sig_mask;
  usr2_act.sa_restorer = 
    (void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
  if (sigaction(SIGUSR2,&usr2_act,NULL) <0) {
     perror("sigaction: SIGUSR2");
     exit(1);
  }
#endif  
}
void ATOM_GlobalInit()
{
  printf("ATOM_GlobalInit\n");
}


void idle_loop()
{
  int timeout;
  for(timeout=500; timeout ; timeout--) {
     if (DDE_GetRemoteMessage())
         exit(0); ;
     usleep(1000);
  }
  exit(-1);
}

void client()
{
  MSG msg;
  msg.hwnd=(HWND)-1;
  msg.message= WM_DDE_INITIATE;
  msg.wParam= 3;
  msg.lParam= 4;
  if (!DDE_SendMessage(&msg))
       exit(-1);
  idle_loop();
}
void server()
{
  DDE_IPC_init();
  idle_loop();
}

int main(int argc, char *argv[])
{
  printf("Kill when done one message\n");
  init_signals();
  if (argc>1)
     server();
  else
     client();
  return 0;
}
