/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "win.h"
#include "windows.h"
#include "stddebug.h"
#include "debug.h"
#include "module.h"
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


/*********************************************************************
 *                  _GetMainArgs  (CRTDLL.022)
 */
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

/*********************************************************************
 *                  _initterm     (CRTDLL.135)
 */
DWORD CRTDLL__initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
	_INITTERMFUN	*current;

	dprintf_crtdll(stddeb,"_initterm(%p,%p)\n",start,end);
	current=start;
	while (current<end) {
		if (*current) (*current)();
		current++;
	}
	return 0;
}

/*********************************************************************
 *                  srand         (CRTDLL.460)
 */
void CRTDLL_srand(DWORD seed)
{
	/* FIXME: should of course be thread? process? local */
	srand(seed);
}

/*********************************************************************
 *                  fprintf       (CRTDLL.373)
 */
int CRTDLL_fprintf(DWORD *args)
{
	/* FIXME: use args[0] */
	return vfprintf(stderr,(LPSTR)(args[1]),args+2);
}

/*********************************************************************
 *                  printf        (CRTDLL.440)
 */
int CRTDLL_printf(DWORD *args)
{
	return vfprintf(stdout,(LPSTR)(args[0]),args+1);
}

/*********************************************************************
 *                  time          (CRTDLL.488)
 */
time_t CRTDLL_time(time_t *timeptr)
{
	time_t	curtime = time(NULL);

	if (timeptr)
		*timeptr = curtime;
	return curtime;
}

/*********************************************************************
 *                  _isatty       (CRTDLL.137)
 */
BOOL32 CRTDLL__isatty(DWORD x)
{
	dprintf_crtdll(stderr,"CRTDLL__isatty(%ld)\n",x);
	return TRUE;
}

/*********************************************************************
 *                  _write        (CRTDLL.332)
 */
INT32 CRTDLL__write(DWORD x,LPVOID buf,DWORD len)
{
	if (x<=2)
		return write(x,buf,len);
	/* hmm ... */
	dprintf_crtdll(stderr,"CRTDLL__write(%ld,%p,%ld)\n",x,buf,len);
	return len;
}


/*********************************************************************
 *                  exit          (CRTDLL.359)
 */
void CRTDLL_exit(DWORD ret)
{
        dprintf_crtdll(stderr,"CRTDLL_exit(%ld)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  fflush        (CRTDLL.365)
 */
void CRTDLL_fflush(DWORD x)
{
    dprintf_crtdll(stderr,"CRTDLL_fflush(%ld)\n",x);
}


/*********************************************************************
 *                  gets          (CRTDLL.391)
 */
LPSTR CRTDLL_gets(LPSTR buf)
{
  /* BAD, for the whole WINE process blocks... just done this way to test
   * windows95's ftp.exe.
   */
    return gets(buf);
}


/*********************************************************************
 *                  abs           (CRTDLL.339)
 */
INT32 CRTDLL_abs(INT32 x)
{
    return abs(x);
}


/*********************************************************************
 *                  acos          (CRTDLL.340)
 */
float CRTDLL_acos(float x)
{
    return acos(x);
}


/*********************************************************************
 *                  asin          (CRTDLL.342)
 */
float CRTDLL_asin(float x)
{
    return asin(x);
}


/*********************************************************************
 *                  atan          (CRTDLL.343)
 */
float CRTDLL_atan(float x)
{
    return atan(x);
}


/*********************************************************************
 *                  atan2         (CRTDLL.344)
 */
float CRTDLL_atan2(float x, float y)
{
    return atan2(x,y);
}


/*********************************************************************
 *                  atof          (CRTDLL.346)
 */
float CRTDLL_atof(LPCSTR x)
{
    return atof(x);
}


/*********************************************************************
 *                  atoi          (CRTDLL.347)
 */
INT32 CRTDLL_atoi(LPCSTR x)
{
    return atoi(x);
}


/*********************************************************************
 *                  atol          (CRTDLL.348)
 */
LONG CRTDLL_atol(LPCSTR x)
{
    return atol(x);
}


/*********************************************************************
 *                  cos           (CRTDLL.354)
 */
float CRTDLL_cos(float x)
{
    return cos(x);
}


/*********************************************************************
 *                  cosh          (CRTDLL.355)
 */
float CRTDLL_cosh(float x)
{
    return cosh(x);
}


/*********************************************************************
 *                  exp           (CRTDLL.360)
 */
float CRTDLL_exp(float x)
{
    return exp(x);
}


/*********************************************************************
 *                  fabs          (CRTDLL.361)
 */
float CRTDLL_fabs(float x)
{
    return fabs(x);
}


/*********************************************************************
 *                  isalnum       (CRTDLL.394)
 */
CHAR CRTDLL_isalnum(CHAR x)
{
    return isalnum(x);
}


/*********************************************************************
 *                  isalpha       (CRTDLL.395)
 */
CHAR CRTDLL_isalpha(CHAR x)
{
    return isalpha(x);
}


/*********************************************************************
 *                  iscntrl       (CRTDLL.396)
 */
CHAR CRTDLL_iscntrl(CHAR x)
{
    return iscntrl(x);
}


/*********************************************************************
 *                  isdigit       (CRTDLL.397)
 */
CHAR CRTDLL_isdigit(CHAR x)
{
    return isdigit(x);
}


/*********************************************************************
 *                  isgraph       (CRTDLL.398)
 */
CHAR CRTDLL_isgraph(CHAR x)
{
    return isgraph(x);
}


/*********************************************************************
 *                  islower       (CRTDLL.400)
 */
CHAR CRTDLL_islower(CHAR x)
{
    return islower(x);
}


/*********************************************************************
 *                  isprint       (CRTDLL.401)
 */
CHAR CRTDLL_isprint(CHAR x)
{
    return isprint(x);
}


/*********************************************************************
 *                  ispunct       (CRTDLL.402)
 */
CHAR CRTDLL_ispunct(CHAR x)
{
    return ispunct(x);
}


/*********************************************************************
 *                  isspace       (CRTDLL.403)
 */
CHAR CRTDLL_isspace(CHAR x)
{
    return isspace(x);
}


/*********************************************************************
 *                  isupper       (CRTDLL.404)
 */
CHAR CRTDLL_isupper(CHAR x)
{
    return isupper(x);
}


/*********************************************************************
 *                  isxdigit      (CRTDLL.418)
 */
CHAR CRTDLL_isxdigit(CHAR x)
{
    return isxdigit(x);
}


/*********************************************************************
 *                  labs          (CRTDLL.419)
 */
LONG CRTDLL_labs(LONG x)
{
    return labs(x);
}


/*********************************************************************
 *                  log           (CRTDLL.424)
 */
float CRTDLL_log(float x)
{
    return log(x);
}


/*********************************************************************
 *                  log10         (CRTDLL.425)
 */
float CRTDLL_log10(float x)
{
    return log10(x);
}


/*********************************************************************
 *                  pow           (CRTDLL.439)
 */
float CRTDLL_pow(float x, float y)
{
    return pow(x,y);
}


/*********************************************************************
 *                  rand          (CRTDLL.446)
 */
INT32 CRTDLL_rand()
{
    return rand();
}


/*********************************************************************
 *                  sin           (CRTDLL.456)
 */
float CRTDLL_sin(float x)
{
    return sin(x);
}


/*********************************************************************
 *                  sinh          (CRTDLL.457)
 */
float CRTDLL_sinh(float x)
{
    return sinh(x);
}


/*********************************************************************
 *                  sqrt          (CRTDLL.459)
 */
float CRTDLL_sqrt(float x)
{
    return sqrt(x);
}


/*********************************************************************
 *                  tan           (CRTDLL.486)
 */
float CRTDLL_tan(float x)
{
    return tan(x);
}


/*********************************************************************
 *                  tanh          (CRTDLL.487)
 */
float CRTDLL_tanh(float x)
{
    return tanh(x);
}


/*********************************************************************
 *                  tolower       (CRTDLL.491)
 */
CHAR CRTDLL_tolower(CHAR x)
{
    return tolower(x);
}


/*********************************************************************
 *                  toupper       (CRTDLL.492)
 */
CHAR CRTDLL_toupper(CHAR x)
{
    return toupper(x);
}


/*********************************************************************
 *                  putchar       (CRTDLL.442)
 */
void CRTDLL_putchar(INT32 x)
{
    putchar(x);
}


/*********************************************************************
 *                  _mbsicmp      (CRTDLL.204)
 */
int CRTDLL__mbsicmp(unsigned char *x,unsigned char *y)
{
    do {
	if (!*x)
	    return !!*y;
	if (!*y)
	    return !!*x;
	/* FIXME: MBCS handling... */
	if (*x!=*y)
	    return 1;
        x++;
        y++;
    } while (1);
}


/*********************************************************************
 *                  _mbsinc       (CRTDLL.205)
 */
unsigned char* CRTDLL__mbsinc(unsigned char *x)
{
    /* FIXME: mbcs */
    return x++;
}


/*********************************************************************
 *                  vsprintf      (CRTDLL.500)
 */
int CRTDLL_vsprintf(DWORD *args)
{
    return vsprintf((char *)args[0],(char *)args[1],args+2);
}


/*********************************************************************
 *                  _mbscpy       (CRTDLL.200)
 */
unsigned char* CRTDLL__mbscpy(unsigned char *x,unsigned char *y)
{
    return strcpy(x,y);
}


/*********************************************************************
 *                  _mbscat       (CRTDLL.197)
 */
unsigned char* CRTDLL__mbscat(unsigned char *x,unsigned char *y)
{
    return strcat(x,y);
}

/*********************************************************************
 *                  _strupr       (CRTDLL.300)
 */
CHAR* CRTDLL__strupr(CHAR *x)
{
	CHAR	*y=x;

	while (*y) {
		*y=toupper(*y);
		y++;
	}
	return x;
}

/*********************************************************************
 *                  malloc        (CRTDLL.427)
 */
VOID* CRTDLL_malloc(DWORD size)
{
    return HeapAlloc(GetProcessHeap(),0,size);
}

/*********************************************************************
 *                  free          (CRTDLL.427)
 */
VOID CRTDLL_free(LPVOID ptr)
{
    HeapFree(GetProcessHeap(),0,ptr);
}
