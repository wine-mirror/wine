/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include "win.h"
#include "windows.h"
#include "stddebug.h"
#include "debug.h"
#include "module.h"
#include "callback.h"
#include "xmalloc.h"

UINT32 CRTDLL_argc_dll;         /* CRTDLL.23 */
LPSTR *CRTDLL_argv_dll;         /* CRTDLL.24 */
LPSTR  CRTDLL_acmdln_dll;       /* CRTDLL.38 */
UINT32 CRTDLL_basemajor_dll;    /* CRTDLL.42 */
UINT32 CRTDLL_baseminor_dll;    /* CRTDLL.43 */
UINT32 CRTDLL_baseversion_dll;  /* CRTDLL.44 */
LPSTR  CRTDLL_environ_dll;      /* CRTDLL.75 */
UINT32 CRTDLL_osmajor_dll;      /* CRTDLL.241 */
UINT32 CRTDLL_osminor_dll;      /* CRTDLL.242 */
UINT32 CRTDLL_osver_dll;        /* CRTDLL.244 */
UINT32 CRTDLL_osversion_dll;    /* CRTDLL.245 */
UINT32 CRTDLL_winmajor_dll;     /* CRTDLL.329 */
UINT32 CRTDLL_winminor_dll;     /* CRTDLL.330 */
UINT32 CRTDLL_winver_dll;       /* CRTDLL.331 */

DWORD
CRTDLL__GetMainArgs(LPDWORD argc,LPSTR **argv,LPSTR *environ,DWORD flag)
{
        char *cmdline;
        char  **xargv;
	int	xargc,i,afterlastspace;
	DWORD	version;

	dprintf_crtdll(stderr,"__GetMainArgs(%p,%p,%p,%ld).\n",
		argc,argv,environ,flag
	);
	CRTDLL_acmdln_dll = cmdline = xstrdup( GetCommandLine32A() );

	version	= GetVersion32();
	CRTDLL_osver_dll       = version >> 16;
	CRTDLL_winminor_dll    = version & 0xFF;
	CRTDLL_winmajor_dll    = (version>>8) & 0xFF;
	CRTDLL_baseversion_dll = version >> 16;
	CRTDLL_winver_dll      = ((version >> 8) & 0xFF) + ((version & 0xFF) << 8);
	CRTDLL_baseminor_dll   = (version >> 16) & 0xFF;
	CRTDLL_basemajor_dll   = (version >> 24) & 0xFF;
	CRTDLL_osversion_dll   = version & 0xFFFF;
	CRTDLL_osminor_dll     = version & 0xFF;
	CRTDLL_osmajor_dll     = (version>>8) & 0xFF;

	/* missing heapinit */
	/* missing threading init */

	i=0;xargv=NULL;xargc=0;afterlastspace=0;
	while (cmdline[i]) {
		if (cmdline[i]==' ') {
			xargv=(char**)xrealloc(xargv,sizeof(char*)*(++xargc));
			cmdline[i]='\0';
			xargv[xargc-1] = xstrdup(cmdline+afterlastspace);
			i++;
			while (cmdline[i]==' ')
				i++;
			if (cmdline[i])
				afterlastspace=i;
		} else
			i++;
	}
	xargv=(char**)xrealloc(xargv,sizeof(char*)*(++xargc));
	cmdline[i]='\0';
	xargv[xargc-1] = xstrdup(cmdline+afterlastspace);
	CRTDLL_argc_dll	= xargc;
	*argc		= xargc;
	CRTDLL_argv_dll	= xargv;
	*argv		= xargv;

	/* FIXME ... use real environment */
	*environ	= xmalloc(sizeof(LPSTR));
	CRTDLL_environ_dll = *environ;
	(*environ)[0] = NULL;
	return 0;
}

typedef void (*_INITTERMFUN)();
DWORD
CRTDLL__initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
	_INITTERMFUN	*current;

	dprintf_crtdll(stddeb,"_initterm(%p,%p)\n",start,end);
	current=start;
	while (current<end) {
		if (*current)
			_InitTermProc(*current);
		current++;
	}
	return 0;
}

void
CRTDLL_srand(DWORD seed) {
	/* FIXME: should of course be thread? process? local */
	srand(seed);
}

int
CRTDLL_fprintf(DWORD *args) {
	/* FIXME: use args[0] */
	return vfprintf(stderr,(LPSTR)(args[1]),args+2);
}

int
CRTDLL_printf(DWORD *args) {
	return vfprintf(stdout,(LPSTR)(args[0]),args+1);
}

time_t
CRTDLL_time(time_t *timeptr) {
	time_t	curtime = time(NULL);

	if (timeptr)
		*timeptr = curtime;
	return curtime;
}

BOOL32
CRTDLL__isatty(DWORD x) {
	dprintf_crtdll(stderr,"CRTDLL__isatty(%ld)\n",x);
	return TRUE;
}

INT32
CRTDLL__write(DWORD x,LPVOID buf,DWORD len) {
	if (x<=2)
		return write(x,buf,len);
	/* hmm ... */
	dprintf_crtdll(stderr,"CRTDLL__write(%ld,%p,%ld)\n",x,buf,len);
	return len;
}

void
CRTDLL_exit(DWORD ret) {
	dprintf_crtdll(stderr,"CRTDLL_exit(%ld)\n",ret);
	ExitProcess(ret);
}

void
CRTDLL_fflush(DWORD x) {
	dprintf_crtdll(stderr,"CRTDLL_fflush(%ld)\n",x);
}

/* BAD, for the whole WINE process blocks... just done this way to test
 * windows95's ftp.exe.
 */
LPSTR 
CRTDLL_gets(LPSTR buf) {
	return gets(buf);
}

CHAR
CRTDLL_toupper(CHAR x) {
	return toupper(x);
}

void
CRTDLL_putchar(INT32 x) {
	putchar(x);
}
