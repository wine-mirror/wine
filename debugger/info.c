/*
 * Wine debugger utility routines
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "toolhelp.h"
#include "tlhelp32.h"
#include "debugger.h"
#include "expr.h"

/***********************************************************************
 *           DEBUG_PrintBasic
 *
 * Implementation of the 'print' command.
 */
void DEBUG_PrintBasic( const DBG_VALUE* value, int count, char format )
{
  char        * default_format;
  long long int res;

  assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);
  if( value->type == NULL ) 
    {
      DEBUG_Printf(DBG_CHN_MESG, "Unable to evaluate expression\n");
      return;
    }
  
  default_format = NULL;
  res = DEBUG_GetExprValue(value, &default_format);

  switch(format)
    {
    case 'x':
      if (value->addr.seg) 
	{
	  DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, "0x%04lx", (long unsigned int) res );
	}
      else 
	{
	  DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, "0x%08lx", (long unsigned int) res );
	}
      break;
      
    case 'd':
      DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, "%ld\n", (long int) res );
      break;
      
    case 'c':
      DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, "%d = '%c'",
				   (char)(res & 0xff), (char)(res & 0xff) );
      break;
      
    case 'i':
    case 's':
    case 'w':
    case 'b':
      DEBUG_Printf( DBG_CHN_MESG, "Format specifier '%c' is meaningless in 'print' command\n", format );
    case 0:
      if( default_format != NULL )
	{
	  if (strstr(default_format, "%S") == NULL)
	    {
	       DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, default_format, res );
	    } 
	  else
	    {
	       char* 	ptr;
	       int	state = 0;

	       /* FIXME: simplistic implementation for default_format being
		* foo%Sbar => will print foo, then string then bar
		*/
	       for (ptr = default_format; *ptr; ptr++) 
	       {
		  if (*ptr == '%') state++;
		  else if (state == 1) 
		    {
		       if (*ptr == 'S') 
			 {
			    char 	ch;
			    char*	str = (char*)(long)res;

			    for (; DEBUG_READ_MEM(str, &ch, 1) && ch; str++) {
			       DEBUG_Output(DBG_CHN_MESG, &ch, 1);
			       DEBUG_nchar++;
			    }
			 }
		       else 
			 {
			    /* shouldn't happen */
			    DEBUG_Printf(DBG_CHN_MESG, "%%%c", *ptr);
			    DEBUG_nchar += 2;
			 }
		       state = 0;
		    }
		  else
		    {
		       DEBUG_Output(DBG_CHN_MESG, ptr, 1);
		       DEBUG_nchar++;
		    }
	       }
	    } 
	}
      break;
    }
}


/***********************************************************************
 *           DEBUG_PrintAddress
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
struct symbol_info
DEBUG_PrintAddress( const DBG_ADDR *addr, int addrlen, int flag )
{
    struct symbol_info rtn;

    const char *name = DEBUG_FindNearestSymbol( addr, flag, &rtn.sym, 0, 
						&rtn.list );

    if (addr->seg) DEBUG_Printf( DBG_CHN_MESG, "0x%04lx:", addr->seg&0xFFFF );
    if (addrlen == 16) DEBUG_Printf( DBG_CHN_MESG, "0x%04lx", addr->off );
    else DEBUG_Printf( DBG_CHN_MESG, "0x%08lx", addr->off );
    if (name) DEBUG_Printf( DBG_CHN_MESG, " (%s)", name );
    return rtn;
}
/***********************************************************************
 *           DEBUG_PrintAddressAndArgs
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 * Similar to DEBUG_PrintAddress, but we print the arguments to
 * each function (if known).  This is useful in a backtrace.
 */
struct symbol_info
DEBUG_PrintAddressAndArgs( const DBG_ADDR *addr, int addrlen, 
			   unsigned int ebp, int flag )
{
    struct symbol_info rtn;

    const char *name = DEBUG_FindNearestSymbol( addr, flag, &rtn.sym, ebp, 
						&rtn.list );

    if (addr->seg) DEBUG_Printf( DBG_CHN_MESG, "0x%04lx:", addr->seg );
    if (addrlen == 16) DEBUG_Printf( DBG_CHN_MESG, "0x%04lx", addr->off );
    else DEBUG_Printf( DBG_CHN_MESG, "0x%08lx", addr->off );
    if (name) DEBUG_Printf( DBG_CHN_MESG, " (%s)", name );

    return rtn;
}


/***********************************************************************
 *           DEBUG_Help
 *
 * Implementation of the 'help' command.
 */
void DEBUG_Help(void)
{
    int i = 0;
    static const char * const helptext[] =
{
"The commands accepted by the Wine debugger are a reasonable",
"subset of the commands that gdb accepts.",
"The commands currently are:",
"  help                                   quit",
"  break [*<addr>]                        delete break bpnum",
"  disable bpnum                          enable bpnum",
"  condition <bpnum> [<expr>]             pass",
"  bt                                     cont [N]",
"  step [N]                               next [N]",
"  stepi [N]                              nexti [N]",
"  x <addr>                               print <expr>",
"  set <reg> = <expr>                     set *<addr> = <expr>",
"  up                                     down",
"  list <lines>                           disassemble [<addr>][,<addr>]",
"  frame <n>                              finish",
"  show dir                               dir <path>",
"  display <expr>                         undisplay <disnum>",
"  delete display <disnum>                debugmsg <class>[-+]<type>\n",
"  mode [16,32]                           walk [wnd,class,queue,module,",
"  whatis                                       process,modref <pid>]",
"  info (see 'help info' for options)\n",

"The 'x' command accepts repeat counts and formats (including 'i') in the",
"same way that gdb does.\n",

" The following are examples of legal expressions:",
" $eax     $eax+0x3   0x1000   ($eip + 256)  *$eax   *($esp + 3)",
" Also, a nm format symbol table can be read from a file using the",
" symbolfile command.  Symbols can also be defined individually with",
" the define command.",
"",
NULL
};

    while(helptext[i]) DEBUG_Printf(DBG_CHN_MESG,"%s\n", helptext[i++]);
}


/***********************************************************************
 *           DEBUG_HelpInfo
 *
 * Implementation of the 'help info' command.
 */
void DEBUG_HelpInfo(void)
{
    int i = 0;
    static const char * const infotext[] =
{
"The info commands allow you to get assorted bits of interesting stuff",
"to be displayed.  The options are:",
"  info break           Dumps information about breakpoints",
"  info display         Shows auto-display expressions in use",
"  info locals          Displays values of all local vars for current frame",
"  info maps            Dumps all virtual memory mappings",
"  info module <handle> Displays internal module state",
"  info queue <handle>  Displays internal queue state",
"  info reg             Displays values in all registers at top of stack",
"  info segments        Dumps information about all known segments",
"  info share           Dumps information about shared libraries",
"  info stack           Dumps information about top of stack",
"  info wnd <handle>    Displays internal window state",
"",
NULL
};

    while(infotext[i]) DEBUG_Printf(DBG_CHN_MESG,"%s\n", infotext[i++]);
}

/* FIXME: merge InfoClass and InfoClass2 */
void DEBUG_InfoClass(const char* name)
{
   WNDCLASSEXA	wca;

   if (!GetClassInfoEx(0, name, &wca)) {
      DEBUG_Printf(DBG_CHN_MESG, "Cannot find class '%s'\n", name);
      return;
   }

   DEBUG_Printf(DBG_CHN_MESG,  "Class '%s':\n", name);
   DEBUG_Printf(DBG_CHN_MESG,  
		"style=%08x  wndProc=%08lx\n"
		"inst=%04x  icon=%04x  cursor=%04x  bkgnd=%04x\n"
		"clsExtra=%d  winExtra=%d\n",
		wca.style, (DWORD)wca.lpfnWndProc, wca.hInstance,
		wca.hIcon, wca.hCursor, wca.hbrBackground,
		wca.cbClsExtra, wca.cbWndExtra);

   /* FIXME: 
    * + print #windows (or even list of windows...)
    * + print extra bytes => this requires a window handle on this very class...
    */
}

static	void DEBUG_InfoClass2(HWND hWnd, const char* name)
{
   WNDCLASSEXA	wca;

   if (!GetClassInfoEx(GetWindowLong(hWnd, GWL_HINSTANCE), name, &wca)) {
      DEBUG_Printf(DBG_CHN_MESG, "Cannot find class '%s'\n", name);
      return;
   }

   DEBUG_Printf(DBG_CHN_MESG,  "Class '%s':\n", name);
   DEBUG_Printf(DBG_CHN_MESG,  
		"style=%08x  wndProc=%08lx\n"
		"inst=%04x  icon=%04x  cursor=%04x  bkgnd=%04x\n"
		"clsExtra=%d  winExtra=%d\n",
		wca.style, (DWORD)wca.lpfnWndProc, wca.hInstance,
		wca.hIcon, wca.hCursor, wca.hbrBackground,
		wca.cbClsExtra, wca.cbWndExtra);

   if (wca.cbClsExtra) {
      int		i;
      WORD		w;

      DEBUG_Printf(DBG_CHN_MESG,  "Extra bytes:" );
      for (i = 0; i < wca.cbClsExtra / 2; i++) {
	 w = GetClassWord(hWnd, i * 2);
	 /* FIXME: depends on i386 endian-ity */
	 DEBUG_Printf(DBG_CHN_MESG,  " %02x", HIBYTE(w));
	 DEBUG_Printf(DBG_CHN_MESG,  " %02x", LOBYTE(w));
      }
      DEBUG_Printf(DBG_CHN_MESG,  "\n" );
    }
    DEBUG_Printf(DBG_CHN_MESG,  "\n" );
}

struct class_walker {
   ATOM*	table;
   int		used;
   int		alloc;
};

static	void DEBUG_WalkClassesHelper(HWND hWnd, struct class_walker* cw)
{
   char	clsName[128];
   int	i;
   ATOM	atom;
   HWND	child;

   if (!GetClassName(hWnd, clsName, sizeof(clsName)))
      return;
   if ((atom = FindAtom(clsName)) == 0)
      return;

   for (i = 0; i < cw->used; i++) {
      if (cw->table[i] == atom)
	 break;
   }
   if (i == cw->used) {
      if (cw->used >= cw->alloc) {
	 cw->alloc += 16;
	 cw->table = DBG_realloc(cw->table, cw->alloc * sizeof(ATOM));
      }
      cw->table[cw->used++] = atom;
      DEBUG_InfoClass2(hWnd, clsName);
   }
   do {
      if ((child = GetWindow(hWnd, GW_CHILD)) != 0)
	 DEBUG_WalkClassesHelper(child, cw);
   } while ((hWnd = GetWindow(hWnd, GW_HWNDNEXT)) != 0);
}

void DEBUG_WalkClasses(void)
{
   struct class_walker cw;

   cw.table = NULL;
   cw.used = cw.alloc = 0;
   DEBUG_WalkClassesHelper(GetDesktopWindow(), &cw);
   DBG_free(cw.table);
}

void DEBUG_DumpQueue(DWORD q)
{
   DEBUG_Printf(DBG_CHN_MESG, "No longer doing info queue '0x%08lx'\n", q);
}

void DEBUG_WalkQueues(void)
{
   DEBUG_Printf(DBG_CHN_MESG, "No longer walking queues list\n");
}

void DEBUG_InfoWindow(HWND hWnd)
{
   char	clsName[128];
   char	wndName[128];
   RECT	clientRect;
   RECT	windowRect;
   int	i;
   WORD	w;

   if (!GetClassName(hWnd, clsName, sizeof(clsName)))
       strcpy(clsName, "-- Unknown --");
   if (!GetWindowText(hWnd, wndName, sizeof(wndName)))
      strcpy(wndName, "-- Empty --");
   if (!GetClientRect(hWnd, &clientRect))
      SetRectEmpty(&clientRect);
   if (!GetWindowRect(hWnd, &windowRect))
      SetRectEmpty(&windowRect);

   /* FIXME missing fields: hmemTaskQ, hrgnUpdate, dce, flags, pProp, scroll */
   DEBUG_Printf(DBG_CHN_MESG,
		"next=0x%04x  child=0x%04x  parent=0x%04x  owner=0x%04x  class='%s'\n"
		"inst=%08lx  active=%04x  idmenu=%08lx\n"
		"style=%08lx  exstyle=%08lx  wndproc=%08lx  text='%s'\n"
		"client=%d,%d-%d,%d  window=%d,%d-%d,%d sysmenu=%04x\n",
		GetWindow(hWnd, GW_HWNDNEXT), 
		GetWindow(hWnd, GW_CHILD),
		GetParent(hWnd), 
		GetWindow(hWnd, GW_OWNER),
		clsName,
		GetWindowLong(hWnd, GWL_HINSTANCE), 
		GetLastActivePopup(hWnd),
		GetWindowLong(hWnd, GWL_ID),
		GetWindowLong(hWnd, GWL_STYLE),
		GetWindowLong(hWnd, GWL_EXSTYLE),
		GetWindowLong(hWnd, GWL_WNDPROC),
		wndName, 
		clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, 
		windowRect.left, windowRect.top, windowRect.right, windowRect.bottom, 
		GetSystemMenu(hWnd, FALSE));

    if (GetClassLong(hWnd, GCL_CBWNDEXTRA)) {
        DEBUG_Printf(DBG_CHN_MESG,  "Extra bytes:" );
        for (i = 0; i < GetClassLong(hWnd, GCL_CBWNDEXTRA) / 2; i++) {
	   w = GetWindowWord(hWnd, i * 2);
	   /* FIXME: depends on i386 endian-ity */
	   DEBUG_Printf(DBG_CHN_MESG,  " %02x", HIBYTE(w));
	   DEBUG_Printf(DBG_CHN_MESG,  " %02x", LOBYTE(w));
	}
        DEBUG_Printf(DBG_CHN_MESG, "\n");
    }
    DEBUG_Printf(DBG_CHN_MESG, "\n");
}

void DEBUG_WalkWindows(HWND hWnd, int indent)
{
   char	clsName[128];
   char	wndName[128];
   HWND	child;

   if (!IsWindow(hWnd))
      hWnd = GetDesktopWindow();

    if (!indent)  /* first time around */
       DEBUG_Printf(DBG_CHN_MESG,  
		    "%-16.16s %-17.17s %-8.8s %s\n",
		    "hwnd", "Class Name", " Style", " WndProc Text");

    do {
       if (!GetClassName(hWnd, clsName, sizeof(clsName)))
	  strcpy(clsName, "-- Unknown --");
       if (!GetWindowText(hWnd, wndName, sizeof(wndName)))
	  strcpy(wndName, "-- Empty --");
       
       /* FIXME: missing hmemTaskQ */
       DEBUG_Printf(DBG_CHN_MESG, "%*s%04x%*s", indent, "", hWnd, 13-indent,"");
       DEBUG_Printf(DBG_CHN_MESG, "%-17.17s %08lx %08lx %.14s\n",
		    clsName, GetWindowLong(hWnd, GWL_STYLE),
		    GetWindowLong(hWnd, GWL_WNDPROC), wndName);

       if ((child = GetWindow(hWnd, GW_CHILD)) != 0)
	  DEBUG_WalkWindows(child, indent + 1 );
    } while ((hWnd = GetWindow(hWnd, GW_HWNDNEXT)) != 0);
}

void DEBUG_WalkProcess(void)
{
    HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if (snap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 entry;
        DWORD current = DEBUG_CurrProcess ? DEBUG_CurrProcess->pid : 0;
        BOOL ok;

		  entry.dwSize = sizeof(entry);
		  ok = Process32First( snap, &entry );

        DEBUG_Printf(DBG_CHN_MESG, "%-8.8s %-8.8s %-8.8s %s\n",
                     "pid", "threads", "parent", "exe" );
        while (ok)
        {
            if (entry.th32ProcessID != GetCurrentProcessId())
                DEBUG_Printf(DBG_CHN_MESG, "%08lx %8ld %08lx '%s'%s\n",
                             entry.th32ProcessID, entry.cntThreads,
                             entry.th32ParentProcessID, entry.szExeFile,
                             (entry.th32ProcessID == current) ? " <==" : "" );
            ok = Process32Next( snap, &entry );
        }
        CloseHandle( snap );
    }
}

void DEBUG_WalkThreads(void)
{
    HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
    if (snap != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 entry;
        DWORD current = DEBUG_CurrThread ? DEBUG_CurrThread->tid : 0;
        BOOL ok;

		  entry.dwSize = sizeof(entry);
		  ok = Thread32First( snap, &entry );

        DEBUG_Printf(DBG_CHN_MESG, "%-8.8s %-8.8s %s\n",
                     "tid", "process", "prio" );
        while (ok)
        {
            if (entry.th32OwnerProcessID != GetCurrentProcessId())
                DEBUG_Printf(DBG_CHN_MESG, "%08lx %08lx %4ld%s\n",
                             entry.th32ThreadID, entry.th32OwnerProcessID,
                             entry.tbBasePri, (entry.th32ThreadID == current) ? " <==" : "" );
            ok = Thread32Next( snap, &entry );
        }
        CloseHandle( snap );
    }
}

void DEBUG_WalkModref(DWORD p)
{
   DEBUG_Printf(DBG_CHN_MESG, "No longer walking module references list\n");
}

void DEBUG_InfoSegments(DWORD start, int length)
{
    char 	flags[3];
    DWORD 	i;
    LDT_ENTRY	le;

    if (length == -1) length = (8192 - start);

    for (i = start; i < start + length; i++)
    {
       if (!GetThreadSelectorEntry(DEBUG_CurrThread->handle, (i << 3)|7, &le))
	  continue;

        if (le.HighWord.Bits.Type & 0x08) 
        {
            flags[0] = (le.HighWord.Bits.Type & 0x2) ? 'r' : '-';
            flags[1] = '-';
            flags[2] = 'x';
        }
        else
        {
            flags[0] = 'r';
            flags[1] = (le.HighWord.Bits.Type & 0x2) ? 'w' : '-';
            flags[2] = '-';
        }
        DEBUG_Printf(DBG_CHN_MESG, 
		     "%04lx: sel=%04lx base=%08x limit=%08x %d-bit %c%c%c\n",
		     i, (i<<3)|7, 
		     (le.HighWord.Bits.BaseHi << 24) + 
		     (le.HighWord.Bits.BaseMid << 16) + le.BaseLow,
		     ((le.HighWord.Bits.LimitHi << 8) + le.LimitLow) << 
		     (le.HighWord.Bits.Granularity ? 12 : 0),
		     le.HighWord.Bits.Default_Big ? 32 : 16,
		     flags[0], flags[1], flags[2] );
    }
}

void DEBUG_InfoVirtual(void)
{
   DEBUG_Printf(DBG_CHN_MESG, "No longer providing virtual mapping information\n");
}
