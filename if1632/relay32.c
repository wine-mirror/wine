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
#include "peexe.h"
#include "relay32.h"
#include "xmalloc.h"
#include "stddebug.h"
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
	size_t len;
	char *cp;

	len = (cp=strchr(name,'.')) ? (cp-name) : strlen(name);
	for(it=WIN32_builtin_list;it;it=it->next)
	if(lstrncmpi(name,it->name,len)==0)
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
  	u_short * ordinal;
  	u_long * function;
  	u_char ** name;
	struct PE_Export_Directory * pe_exports;
	unsigned int load_addr;

	dprintf_module(stddeb, "Looking for %s in %s, hint %x\n",
		item ? item: "(no name)", dll_name, hint);
	dll=RELAY32_GetBuiltinDLL(dll_name);
	/* This should deal with built-in DLLs only. See pe_module on loading
	   PE DLLs */
#if 0
	if(!dll) {
		if(!wine_files || !wine_files->name ||
		   lstrcmpi(dll_name, wine_files->name)) {
			LoadModule(dll_name, (LPVOID) -1);
			if(!wine_files || !wine_files->name ||
		   	   lstrcmpi(dll_name, wine_files->name)) 
				return 0;
		}
		load_addr = wine_files->load_addr;
		pe_exports = wine_files->pe->pe_export;
  		ordinal = (u_short *) (((char *) load_addr) + (int) pe_exports->Address_Of_Name_Ordinals);
  		function = (u_long *)  (((char *) load_addr) + (int) pe_exports->AddressOfFunctions);
  		name = (u_char **)  (((char *) load_addr) + (int) pe_exports->AddressOfNames);
		/* import by ordinal */
		if(!item){
			return 0;
		}
		/* hint is correct */
		#if 0
		if(hint && hint<dll->size && 
			dll->functions[hint].name &&
			strcmp(item,dll->functions[hint].name)==0)
			return dll->functions[hint].definition;
		#endif
		/* hint is incorrect, search for name */
		for(i=0;i<pe_exports->Number_Of_Functions;i++)
	            if (name[i] && !strcmp(item,name[i]+load_addr))
	                return function[i]+(char *)load_addr;
	
		/* function at hint has no name (unimplemented) */
		#if 0
		if(hint && hint<dll->size && !dll->functions[hint].name)
		{
			dll->functions[hint].name=xstrdup(item);
			dprintf_module(stddeb, "Returning unimplemented function %s.%d\n",
				dll_name,hint);
			return dll->functions[hint].definition;
		}
		#endif
		printf("Not found\n");
		return 0;
	}
#endif
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
		dll->functions[hint].name=xstrdup(item);
		dprintf_module(stddeb, "Returning unimplemented function %s.%d\n",
			dll_name,hint);
		return dll->functions[hint].definition;
	}
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
