/*
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "windows.h"
#include "dlls.h"
#include "pe_image.h"
#include "stddebug.h"
/* #define DEBUG_RELAY */
#include "debug.h"

WIN32_builtin	*WIN32_builtin_list;

/* Functions are in generated code */
void ADVAPI32_Init();
void COMDLG32_Init();
void GDI32_Init();
void KERNEL32_Init();
void SHELL32_Init();
void USER32_Init();
void WINPROCS32_Init();

int RELAY32_Init(void)
{
#ifndef WINELIB
	/* Add a call for each DLL */
	ADVAPI32_Init();
	COMDLG32_Init();
	GDI32_Init();
	KERNEL32_Init();
	SHELL32_Init();
	USER32_Init();
	WINPROCS32_Init();
#endif
	/* Why should it fail, anyways? */
	return 1;
}

WIN32_builtin *RELAY32_GetBuiltinDLL(char *name)
{
	WIN32_builtin *it;
	for(it=WIN32_builtin_list;it;it=it->next)
	if(strcasecmp(name,it->name)==0)
		return it;
	return NULL;
}

void RELAY32_Unimplemented(char *dll, int item)
{
	WIN32_builtin *Dll;
	fprintf( stderr, "No handler for routine %s.%d", dll, item);
	Dll=RELAY32_GetBuiltinDLL(dll);
	if(Dll && Dll->functions[item].name)
		fprintf(stderr, "(%s?)\n", Dll->functions[item].name);
	else
		fprintf(stderr, "\n");
	fflush(stderr);
	exit(1);
}

void *RELAY32_GetEntryPoint(char *dll_name, char *item, int hint)
{
	WIN32_builtin *dll;
	int i;
	dprintf_module(stddeb, "Looking for %s in %s, hint %x\n",
		item ? item: "(no name)", dll_name, hint);
	dll=RELAY32_GetBuiltinDLL(dll_name);
	if(!dll)return 0;
	/* import by ordinal */
	if(!item){
		if(hint && hint<dll->size)return dll->functions[hint].definition;
		return 0;
	}
	/* hint is correct */
	if(hint && hint<dll->size && 
		dll->functions[hint].name &&
		strcmp(item,dll->functions[hint].name)==0)
		return dll->functions[hint].definition;
	/* hint is incorrect, search for name */
	for(i=0;i<dll->size;i++)
            if (dll->functions[i].name && !strcmp(item,dll->functions[i].name))
                return dll->functions[i].definition;

	/* function at hint has no name (unimplemented) */
	if(hint && hint<dll->size && !dll->functions[hint].name)
	{
		dll->functions[hint].name=strdup(item);
		dprintf_module(stddeb, "Returning unimplemented function %s.%d\n",
			dll_name,hint);
		return dll->functions[hint].definition;
	}
	printf("Not found\n");
	return 0;
}

void RELAY32_DebugEnter(char *dll,char *name)
{
	dprintf_relay(stddeb, "Entering %s.%s\n",dll,name);
}

LONG RELAY32_CallWindowProc( WNDPROC func, int hwnd, int message,
             int wParam, int lParam )
{
	int ret;
	SpyMessage(hwnd, message, wParam, lParam);
	__asm__ (
		"push %1;"
		"push %2;"
		"push %3;"
		"push %4;"
		"call %5;"
		: "=a" (ret)
		: "g" (lParam), "g" (wParam), "g" (message), "g" (hwnd), "g" (func)
	);
	return ret;
}
