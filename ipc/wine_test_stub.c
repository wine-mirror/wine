#include <stdlib.h>
#include "dde.h"
#include <wintypes.h>
#include "global.h"
#include <win.h>
#define DEBUG_DEFINE_VARIABLES
#define DEBUG_ALL
#include <stddebug.h>
#include <debug.h>

#define DDE_PROC2WIN(proc_idx)   (  (HWND) ~( (proc_idx)+1)  )
#define DDE_WIN2PROC(win)        (  (int) ~(short) ((win)+1) )
#define DDE_IsRemoteWindow(win)  (  (win)<0xffff && (win)>=(0xffff-DDE_PROCS))


char *MessageTypeNames[0x400]={NULL};
char *dummy_store_for_debug_msg_name;

ldt_copy_entry ldt_copy[LDT_SIZE];

int LDT_GetEntry( int entry, ldt_entry *content )
{
    return 0;
}

int LDT_SetEntry( int entry, ldt_entry const *content )
{
    return 0;
}

void dummy_usage_of_debug_msg_name()
{
  dummy_store_for_debug_msg_name=debug_msg_name[0];
}

/* stub */
HWND32 GetDesktopWindow32()
{
  printf("GetDesktopWindow\n");
  return 0;
}
/* stub */
/* smart stub */
LONG SendMessage(HWND a,WORD b,WORD c,LONG d)
{
  MSG msg;
  printf("SendMessage(%04x,%04x,%04x,%04lx)\n",a,b,c,d);
  if (DDE_IsRemoteWindow(a) || a==(HWND)-1)
     return 0;
  if (b!=WM_DDE_INITIATE)
     return 0;
  msg.hwnd=c;
  msg.message= WM_DDE_ACK;
  msg.lParam= 0;
  msg.wParam= 0;
  return DDE_SendMessage(&msg);
}
/* stub */
BOOL PostMessage(HWND a,WORD b,WORD c,LONG d)
{
  printf("PostMessage(%04x,%04x,%04x,%04lx)\n",a,b,c,d);
  return 0;
}
/* stub */
HWND GetTopWindow(HWND a)
{
  printf("GetTopWindow(%04x)\n",a);
  return 1;
}
/* stub */
WORD FreeSelector(WORD a)
{
  printf("FreeSelector(%04x)\n",a);
  return 0;
}

/* stub that partially emulates the true GLOBAL_CreateBlock function */
HGLOBAL16 GLOBAL_CreateBlock( WORD flags, void *ptr, DWORD size,
                            HGLOBAL16 hOwner, BOOL isCode,
                            BOOL is32Bit, BOOL isReadOnly,
			    SHMDATA *shmdata  )
{
  
  printf("GLOBAL_CreateBlock(flags=0x%x,ptr=0x%08lx, size=0x%x,hOwner=0x%x\n",
	 (int)flags, (long)ptr, (int)size, (int)hOwner);
  printf("isCode=%d, is32Bit=%d, isReadOnly=%d, \n", isCode, is32Bit,
	 isReadOnly);
  printf("*shmdata={handle=0x%x,sel=0x%x, shmid=%d})\n",
	 shmdata->handle, shmdata->sel, shmdata->shmid);
  return 1;
}

/* stub */
WND *WIN_FindWndPtr(HWND hwnd)
{
  static WND win;
  printf("WIN_FindWndPtr(%d)\n",hwnd);
  if (hwnd==0)
     return NULL;
  win.next=NULL;
  win.dwStyle=WS_POPUP;
  
  return &win;
}

/* stub */
WORD GetCurrentPDB(void)
{
  printf("GetCurrentPDB()\n");
  
  return 0;
}

/* stub */
void Yield(void)
{
}
