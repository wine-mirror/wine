/*
 * JBP (Jim Peterson <jspeter@birch.ee.vt.edu>): Lots of stubs needed for
 *      libwine.a.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dde_mem.h"
#include "windows.h"
#include "global.h"
#include "debug.h"

int CallTo32_LargeStack( int (*func)(), int nbargs, ...)
{
  va_list arglist;
  int i,a[32];

  va_start(arglist,nbargs);

  for(i=0; i<nbargs; i++) a[i]=va_arg(arglist,int);

  switch(nbargs) /* Ewww... Icky.  But what can I do? */
  {
  case 5: return func(a[0],a[1],a[2],a[3],a[4]);
  case 6: return func(a[0],a[1],a[2],a[3],a[4],a[5]);
  case 8: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
  case 10: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9]);
  case 11: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9],a[10]);
  case 14: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9],a[10],a[11],a[12],a[13]);
  default: fprintf(stderr,"JBP: CallTo32_LargeStack called with unsupported "
                          "number of arguments (%d).  Ignored.\n",nbargs);
           return 0;
  }
}

WORD CallTo16_word_ ( FARPROC func, WORD arg ) { return func(arg); }

/* typedef void* ATOM; */
/* ATOM GlobalAddAtom(char *n) */
/* { */
/*   return strdup(n); */
/* } */
/* GlobalDeleteAtom(ATOM n) */
/* { */
/*   free(n); */
/* } */
/* GlobalFindAtom(char*n) */
/* { */
/*   fprintf(stderr,"JBP: GlobalFindAtom() ignored.\n"); */
/*   return 0; */
/* } */
/* char *GlobalGetAtomName(ATOM a) */
/* { */
/*   return a; */
/* } */

void GlobalFreeAll(HANDLE owner)
{
  fprintf(stderr,"JBP: GlobalFreeAll() ignored.\n");
}

SEGPTR WIN16_GlobalLock(HGLOBAL h)
{
  return (SEGPTR)h;
}



#if 0
typedef WORD* HLOCAL;

int IsValidHLOCAL(HLOCAL handle)
{
  return *(handle-1) + *(handle-2) == 0; /* Valid HLOCAL's sum to 0 */
}

/***********************************************************************
 *           LOCAL_Free
 *
 */
HLOCAL LOCAL_Free( WORD ds, HLOCAL handle )
{
  if (!IsValidHLOCAL(handle)) return handle; /* couldn't free it */
  free(handle-2);
  return 0;
}


/***********************************************************************
 *           LOCAL_Alloc
 *
 */
HLOCAL LOCAL_Alloc( WORD ds, WORD flags, WORD size )
{
  HLOCAL handle;
    
  handle = malloc(size + 2*sizeof(WORD));
  handle += 2;
  *(handle-2) = size;
  *(handle-1) = -size;
  return handle;
}


/***********************************************************************
 *           LOCAL_ReAlloc
 *
 */
HLOCAL LOCAL_ReAlloc( WORD ds, HLOCAL handle, WORD size, WORD flags )
{
  HLOCAL newhandle;

  if(!IsValidHLOCAL(handle))return 0;
  newhandle = realloc(handle-2, size+2*sizeof(WORD));
  newhandle += 2;
  *(newhandle-2) = size;
  *(newhandle-1) = -size;
  return newhandle;
}


/***********************************************************************
 *           LOCAL_Lock
 */
WORD LOCAL_Lock( WORD ds, HLOCAL handle )
{
  if(!IsValidHLOCAL(handle))return 0;
  return handle;
}


/***********************************************************************
 *           LOCAL_Unlock
 */
BOOL LOCAL_Unlock( WORD ds, HLOCAL handle )
{
  return TRUE;
}


/***********************************************************************
 *           LOCAL_Size
 *
 */
WORD LOCAL_Size( WORD ds, HLOCAL handle )
{
  return *(handle-2);
}
#endif

HLOCAL LOCAL_Free( WORD ds, HLOCAL handle )
{
  fprintf(stderr,"JBP: LOCAL_*() ignored.\n");
  return 0;
}

HLOCAL LOCAL_Alloc( WORD ds, WORD flags, WORD size )
{
  fprintf(stderr,"JBP: LOCAL_*() ignored.\n");
  return 0;
}

HLOCAL LOCAL_ReAlloc( WORD ds, HLOCAL handle, WORD size, WORD flags )
{
  fprintf(stderr,"JBP: LOCAL_*() ignored.\n");
  return 0;
}

WORD LOCAL_Lock( WORD ds, HLOCAL handle )
{
  fprintf(stderr,"JBP: LOCAL_*() ignored.\n");
  return 0;
}

BOOL LOCAL_Unlock( WORD ds, HLOCAL handle )
{
  fprintf(stderr,"JBP: LOCAL_*() ignored.\n");
  return 1;
}

WORD LOCAL_Size( WORD ds, HLOCAL handle )
{
  fprintf(stderr,"JBP: LOCAL_*() ignored.\n");
  return 0;
}

void FarSetOwner(HANDLE a, WORD b)
{
  fprintf(stderr,"JBP: FarSetOwner() ignored.\n");
}

#define GLOBAL_MAX_ALLOC_SIZE 0x00ff0000  /* Largest allocation is 16M - 64K */

HGLOBAL GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL hOwner,
                      BOOL isCode, BOOL is32Bit, BOOL isReadOnly )
{
    void *ptr;
    HGLOBAL handle;
    SHMDATA shmdata;

    dprintf_global( stddeb, "GLOBAL_Alloc: %ld flags=%04x\n", size, flags );

      /* Fixup the size */

    if (size >= GLOBAL_MAX_ALLOC_SIZE - 0x1f) return 0;
    if (size == 0) size = 0x20;
    else size = (size + 0x1f) & ~0x1f;

      /* Allocate the linear memory */

#ifdef CONFIG_IPC
    if ((flags & GMEM_DDESHARE) && Options.ipc)
        ptr = DDE_malloc(flags, size, &shmdata);
    else 
#endif  /* CONFIG_IPC */
	ptr = malloc( size );
    if (!ptr) return 0;

      /* Allocate the selector(s) */

    handle = GLOBAL_CreateBlock( flags, ptr, size, hOwner,
				isCode, is32Bit, isReadOnly, &shmdata);
    if (!handle)
    {
        free( ptr );
        return 0;
    }

    if (flags & GMEM_ZEROINIT) memset( ptr, 0, size );
    return handle;
}

HGLOBAL GLOBAL_CreateBlock( WORD flags, void *ptr, DWORD size,
			    HGLOBAL hOwner, BOOL isCode,
			    BOOL is32Bit, BOOL isReadOnly,
			    SHMDATA *shmdata)
{
  fprintf(stderr,"JBP: GLOBAL_CreateBlock() faked.\n");
  return ptr;
}

BOOL GLOBAL_FreeBlock( HGLOBAL handle )
{
  fprintf(stderr,"JBP: GLOBAL_FreeBlock() ignored.\n");
  return 1;
}

DWORD GlobalHandle(WORD a)
{
  fprintf(stderr,"JBP: GlobalHandle() ignored.\n");
  return 0;
}

void *RELAY32_GetEntryPoint(char *dll_name, char *item, int hint)
{
  fprintf(stderr,"JBP: RELAY32_GetEntryPoint() ignored.\n");
  return NULL;
}

extern LRESULT AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ButtonWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT CARET_Callback(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ColorDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ComboBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ComboLBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT DesktopWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT EditWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT FileOpenDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT FileSaveDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT FindTextDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ListBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT MDIClientWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT PopupMenuWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT PrintDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT PrintSetupDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ReplaceTextDlgProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT ScrollBarWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT StaticWndProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT SystemMessageBoxProc(HWND,UINT,WPARAM,LPARAM);
extern LRESULT TASK_Reschedule(HWND,UINT,WPARAM,LPARAM);

LRESULT ErrorProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  fprintf(stderr,"ERROR: ErrorProc() called!\n");
  return 0;
}

/***********************************************************************
 *           GetWndProcEntry16 (not a Windows API function)
 *
 * Return an entry point from the WINPROCS dll.
 */
WNDPROC GetWndProcEntry16( char *name )
{
#define MAP_STR_TO_PROC(str,proc) if(!strcmp(name,str))return proc
  MAP_STR_TO_PROC("AboutDlgProc",AboutDlgProc);
  MAP_STR_TO_PROC("ButtonWndProc",ButtonWndProc);
  MAP_STR_TO_PROC("CARET_Callback",CARET_Callback);
  MAP_STR_TO_PROC("ColorDlgProc",ColorDlgProc);
  MAP_STR_TO_PROC("ComboBoxWndProc",ComboBoxWndProc);
  MAP_STR_TO_PROC("ComboLBoxWndProc",ComboLBoxWndProc);
  MAP_STR_TO_PROC("DefDlgProc",DefDlgProc);
  MAP_STR_TO_PROC("DesktopWndProc",DesktopWndProc);
  MAP_STR_TO_PROC("EditWndProc",EditWndProc);
  MAP_STR_TO_PROC("FileOpenDlgProc",FileOpenDlgProc);
  MAP_STR_TO_PROC("FileSaveDlgProc",FileSaveDlgProc);
  MAP_STR_TO_PROC("FindTextDlgProc",FindTextDlgProc);
  MAP_STR_TO_PROC("ListBoxWndProc",ListBoxWndProc);
  MAP_STR_TO_PROC("MDIClientWndProc",MDIClientWndProc);
  MAP_STR_TO_PROC("PopupMenuWndProc",PopupMenuWndProc);
  MAP_STR_TO_PROC("PrintDlgProc",PrintDlgProc);
  MAP_STR_TO_PROC("PrintSetupDlgProc",PrintSetupDlgProc);
  MAP_STR_TO_PROC("ReplaceTextDlgProc",ReplaceTextDlgProc);
  MAP_STR_TO_PROC("ScrollBarWndProc",ScrollBarWndProc);
  MAP_STR_TO_PROC("StaticWndProc",StaticWndProc);
  MAP_STR_TO_PROC("SystemMessageBoxProc",SystemMessageBoxProc);
  MAP_STR_TO_PROC("TASK_Reschedule",TASK_Reschedule);
  return ErrorProc;
}
