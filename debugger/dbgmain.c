/*
 * File dbgmain.c - main wrapper for internal debugger test bed.
 *
 * Copyright (C) 1997, Eric Youngdale.
 */
#include <signal.h>

#include <ldt.h>
#include "windows.h"
#include "toolhelp.h"
#include "module.h"
#include "debugger.h"
#include "class.h"
#include <X11/Xlib.h>

#include "debugger.h"
#include "peexe.h"
#include "pe_image.h"

ldt_copy_entry ldt_copy[LDT_SIZE];
unsigned char ldt_flags_copy[LDT_SIZE];

Display * display;

int
XUngrabPointer( Display * d, Time t)
{
  return(0);
}

int
XUngrabServer( Display * d )
{
  return(0);
}

int
XFlush(Display * d )
{
  return(0);
}

HTASK16    GetCurrentTask()
{
  exit(0);
}

HMODULE16  GetExePtr(HANDLE16 h)
{
  exit(0);
}

int PROFILE_GetWineIniString( const char *section, const char *key_name,
                                     const char *def, char *buffer, int len )
{
  exit(0);
}


void CLASS_DumpClass( CLASS *class )
{
  exit(0);
}

void MODULE_DumpModule( HMODULE16 hmodule )
{
  exit(0);
}

void QUEUE_DumpQueue( HQUEUE16 hQueue )
{
  exit(0);
}

void WIN_DumpWindow( HWND32 hwnd )
{
  exit(0);
}


void CLASS_WalkClasses()
{
  exit(0);
}

void MODULE_WalkModules()
{
  exit(0);
}

void QUEUE_WalkQueues()
{
  exit(0);
}

void WIN_WalkWindows( HWND32 hwnd, int indent )
{
  exit(0);
}

NE_MODULE *NE_GetPtr( HMODULE16 hModule )
{
  exit(0);
}

FARPROC16 NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal )
{
  exit(0);
}

void LDT_Print( int start, int length )
{
  exit(0);
}

LPVOID     GlobalLock16(HGLOBAL16 h)
{
  exit(0);
}

BOOL16 ModuleFirst(MODULEENTRY *lpModule)
{
  return 0;
}

BOOL16 ModuleNext(MODULEENTRY *lpModule)
{
  return 0;
}

BOOL16     IsBadReadPtr16(SEGPTR s,UINT16 o)
{
  exit(0);
}

BOOL32     IsBadReadPtr32(LPCVOID s,UINT32 o)
{
  exit(0);
}

struct qwert
{
  unsigned flag1:1;
  unsigned flag3:3;
  signed remain:11;
  unsigned whatsup:17;
} vvv;

int xyzzy(const char * qwe, int ijk)
{
  return strlen(qwe) + ijk;
}

unsigned int * xxx;
unsigned int * yyy;

int xxx3[10];

char vdv[100];

struct deferred_debug_info
{
	struct deferred_debug_info	* next;
	char				* load_addr;
	char				* module_name;
	char				* dbg_info;
	int				  dbg_size;
	LPIMAGE_DEBUG_DIRECTORY		  dbgdir;
	struct pe_data			* pe;
        LPIMAGE_SECTION_HEADER            sectp;
	int				  nsect;
	short int			  dbg_index;			
	char				  loaded;
};

struct CodeViewDebug
{
	char		    cv_nbtype[8];
	unsigned int	    cv_timestamp;
	char		    cv_unknown[4];
	char		    cv_name[1];
};

test_pdbstuff()
{
  struct deferred_debug_info deefer;
  IMAGE_DEBUG_DIRECTORY dinfo;
  struct CodeViewDebug cdebug;
  IMAGE_SECTION_HEADER  sects[10];

  memset(&deefer, 0, sizeof(deefer));
  memset(&dinfo, 0, sizeof(dinfo));
  memset(&cdebug, 0, sizeof(cdebug));
  memset(&sects, 0, sizeof(sects));

  deefer.dbg_info = (char *) &cdebug;
  dinfo.TimeStamp = 812932395;
  cdebug.cv_timestamp = 833392137  /* 841951397 */;
  deefer.dbgdir = &dinfo;
  deefer.sectp = sects;
  deefer.nsect = 10;

  DEBUG_InitTypes();
  DEBUG_ProcessPDBFile(&deefer, "../dumpexe.pdb");
}

int
main(int argc, char * argv[])
{
  extern char * DEBUG_argv0;
  SIGCONTEXT  reg;
  
  strcpy(vdv, "This is a test");
  memset(&vvv, 0xff, sizeof(vvv));
  vvv.whatsup = 0;
  vvv.flag3 = 0;
  vvv.remain = 0x401;
  DEBUG_argv0 = argv[0];
  xxx = (unsigned int*) &vvv;
  yyy = xxx + 5;
  xxx3[5] = 7;

  test_pdbstuff();

  memset(&reg, 0, sizeof(reg));
  wine_debug(SIGSEGV, &reg);
  return 0;
}
