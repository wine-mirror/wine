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
#include "module.h"
#include "neexe.h"
#include "pe_image.h"
#include "peexe.h"
#include "relay32.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

WIN32_builtin	*WIN32_builtin_list;

/* Functions are in generated code */
void ADVAPI32_Init();
void COMCTL32_Init();
void COMDLG32_Init();
void OLE32_Init();
void GDI32_Init();
void KERNEL32_Init();
void SHELL32_Init();
void USER32_Init();
void WINPROCS32_Init();
void WINSPOOL_Init();

int RELAY32_Init(void)
{
#ifndef WINELIB
	/* Add a call for each DLL */
	ADVAPI32_Init();
	COMCTL32_Init();
	COMDLG32_Init();
	GDI32_Init();
	KERNEL32_Init();
	OLE32_Init();
	SHELL32_Init();
	USER32_Init();
	WINPROCS32_Init();
	WINSPOOL_Init();
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

void *RELAY32_GetEntryPoint(WIN32_builtin *dll, char *item, int hint)
{
	int i;

	dprintf_module(stddeb, "Looking for %s in %s, hint %x\n",
		item ? item: "(no name)", dll->name, hint);
	if(!dll)
		return 0;
	/* import by ordinal */
	if(!item){
		if(hint && hint<dll->size)
			return dll->functions[hint-dll->base].definition;
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
			dll->name,hint);
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

void RELAY32_MakeFakeModule(WIN32_builtin*dll)
{
	NE_MODULE *pModule;
	struct w_files *wpnt;
	int size;
	HMODULE hModule;
	OFSTRUCT *pFileInfo;
	char *pStr;
	wpnt=xmalloc(sizeof(struct w_files));
	wpnt->hinstance=0;
	wpnt->hModule=0;
	wpnt->initialised=1;
	wpnt->mz_header=wpnt->pe=0;
	size=sizeof(NE_MODULE) +
		/* loaded file info */
		sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName) +
                strlen(dll->name) + 1 +
		/* name table */
		12 +
		/* several empty tables */
		8;
	hModule = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
	wpnt->hModule = hModule;
	FarSetOwner( hModule, hModule );
	pModule = (NE_MODULE*)GlobalLock(hModule);
	/* Set all used entries */
	pModule->magic=PE_SIGNATURE;
	pModule->count=1;
	pModule->next=0;
	pModule->flags=0;
	pModule->dgroup=0;
	pModule->ss=0;
	pModule->cs=0;
	pModule->heap_size=0;
	pModule->stack_size=0;
	pModule->seg_count=0;
	pModule->modref_count=0;
	pModule->nrname_size=0;
	pModule->seg_table=0;
	pModule->fileinfo=sizeof(NE_MODULE);
	pModule->os_flags=NE_OSFLAGS_WINDOWS;
	pModule->expected_version=0x30A;
	pFileInfo=(OFSTRUCT *)(pModule + 1);
	pFileInfo->cBytes = sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName)
                            + strlen(dll->name);
	strcpy( pFileInfo->szPathName, dll->name );
	pStr = ((char*)pFileInfo) + pFileInfo->cBytes + 1;
	pModule->name_table=(int)pStr-(int)pModule;
	*pStr=strlen(dll->name);
	strcpy(pStr+1,dll->name);
	pStr += *pStr+1;
	pModule->res_table=pModule->import_table=pModule->entry_table=
		(int)pStr-(int)pModule;
	MODULE_RegisterModule(hModule);
	wpnt->builtin=dll;
	wpnt->next=wine_files;
	wine_files=wpnt;
}



