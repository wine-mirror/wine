/*
 * 386-specific Win16 dll<->dll snooping functions
 *
 * Copyright 1998 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wine/winbase16.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(snoop);

#pragma pack(push,1)

typedef	struct tagSNOOP16_FUN {
	/* code part */
	BYTE		lcall;		/* 0x9a call absolute with segment */
	FARPROC16 snoop_entry;
	/* unreached */
	int		nrofargs;
	FARPROC16	origfun;
	char		*name;
} SNOOP16_FUN;

typedef struct tagSNOOP16_DLL {
	HMODULE16	hmod;
	HANDLE16	funhandle;
	SNOOP16_FUN	*funs;
	struct tagSNOOP16_DLL	*next;
	char name[1];
} SNOOP16_DLL;

typedef struct tagSNOOP16_RETURNENTRY {
	/* code part */
	BYTE		lcall;		/* 0x9a call absolute with segment */
	FARPROC16 snoop_return;
	/* unreached */
	FARPROC16	origreturn;
	SNOOP16_DLL	*dll;
	DWORD		ordinal;
	WORD		origSP;
	WORD		*args;		/* saved args across a stdcall */
} SNOOP16_RETURNENTRY;

typedef struct tagSNOOP16_RETURNENTRIES {
	SNOOP16_RETURNENTRY entry[65500/sizeof(SNOOP16_RETURNENTRY)];
	HANDLE16	rethandle;
	struct tagSNOOP16_RETURNENTRIES	*next;
} SNOOP16_RETURNENTRIES;

#pragma pack(pop)

static	SNOOP16_DLL		*firstdll = NULL;
static	SNOOP16_RETURNENTRIES 	*firstrets = NULL;

void
SNOOP16_RegisterDLL(HMODULE16 hModule,LPCSTR name) {
	SNOOP16_DLL	**dll = &(firstdll);
	const char	*p;
	char		*q;

	if (!TRACE_ON(snoop)) return;

        TRACE("hmod=%x, name=%s\n", hModule, name);

	while (*dll) {
		if ((*dll)->hmod == hModule)
                {
                    /* another dll, loaded at the same address */
                    GlobalUnlock16((*dll)->funhandle);
                    GlobalFree16((*dll)->funhandle);
                    break;
                }
		dll = &((*dll)->next);
	}

	if (*dll)
		*dll = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *dll, sizeof(SNOOP16_DLL)+strlen(name));
	else
		*dll = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SNOOP16_DLL)+strlen(name));	

	(*dll)->next	= NULL;
	(*dll)->hmod	= hModule;
	if ((p=strrchr(name,'\\')))
		name = p+1;
	strcpy( (*dll)->name, name );
	if ((q=strrchr((*dll)->name,'.')))
		*q='\0';
	(*dll)->funhandle = GlobalHandleToSel16(GLOBAL_Alloc(GMEM_ZEROINIT,65535,0,LDT_FLAGS_CODE));
	(*dll)->funs = GlobalLock16((*dll)->funhandle);
	if (!(*dll)->funs) {
		HeapFree(GetProcessHeap(),0,*dll);
		FIXME("out of memory\n");
		return;
	}
}

FARPROC16
SNOOP16_GetProcAddress16(HMODULE16 hmod,DWORD ordinal,FARPROC16 origfun) {
	SNOOP16_DLL			*dll = firstdll;
	SNOOP16_FUN			*fun;
	NE_MODULE			*pModule = NE_GetPtr(hmod);
	unsigned char			*cpnt;
	char				name[200];

	if (!TRACE_ON(snoop) || !pModule || !HIWORD(origfun))
		return origfun;
	if (!*(LPBYTE)MapSL((SEGPTR)origfun)) /* 0x00 is an impossible opcode, possible dataref. */
		return origfun;
	while (dll) {
		if (hmod == dll->hmod)
			break;
		dll=dll->next;
	}
	if (!dll)	/* probably internal */
		return origfun;
	if (ordinal>65535/sizeof(SNOOP16_FUN))
		return origfun;
	fun = dll->funs+ordinal;
	/* already done? */
	fun->lcall 	= 0x9a;
	fun->snoop_entry = GetProcAddress16( GetModuleHandle16( "KERNEL" ), "__wine_snoop_entry" );
	fun->origfun	= origfun;
	if (fun->name)
		return (FARPROC16)(SEGPTR)MAKELONG(((char*)fun-(char*)dll->funs),dll->funhandle);
	cpnt = (unsigned char *)pModule + pModule->ne_restab;
	while (*cpnt) {
		cpnt += *cpnt + 1 + sizeof(WORD);
		if (*(WORD*)(cpnt+*cpnt+1) == ordinal) {
			sprintf(name,"%.*s",*cpnt,cpnt+1);
			break;
		}
	}
	/* Now search the non-resident names table */

	if (!*cpnt && pModule->nrname_handle) {
		cpnt = GlobalLock16( pModule->nrname_handle );
		while (*cpnt) {
			cpnt += *cpnt + 1 + sizeof(WORD);
			if (*(WORD*)(cpnt+*cpnt+1) == ordinal) {
				    sprintf(name,"%.*s",*cpnt,cpnt+1);
				    break;
			}
		}
	}
	if (*cpnt)
        {
            fun->name = HeapAlloc(GetProcessHeap(),0,strlen(name)+1);
            strcpy( fun->name, name );
        }
	else
            fun->name = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,1); /* empty string */

	if (!SNOOP16_ShowDebugmsgSnoop(dll->name, ordinal, fun->name))
		return origfun;

	/* more magic. do not try to snoop thunk data entries (MMSYSTEM) */
	if (strchr(fun->name,'_')) {
		char *s=strchr(fun->name,'_');

		if (!_strnicmp(s,"_thunkdata",10)) {
			HeapFree(GetProcessHeap(),0,fun->name);
			fun->name = NULL;
			return origfun;
		}
	}
	fun->lcall 	= 0x9a;
	fun->snoop_entry = GetProcAddress16( GetModuleHandle16( "KERNEL" ), "__wine_snoop_entry" );
	fun->origfun	= origfun;
	fun->nrofargs	= -1;
	return (FARPROC16)(SEGPTR)MAKELONG(((char*)fun-(char*)dll->funs),dll->funhandle);
}

#define CALLER1REF (*(DWORD*)(MapSL( MAKESEGPTR(context->SegSs,LOWORD(context->Esp)+4))))
void WINAPI __wine_snoop_entry( CONTEXT *context )
{
	DWORD		ordinal=0;
	DWORD		entry=(DWORD)MapSL( MAKESEGPTR(context->SegCs,LOWORD(context->Eip)) )-5;
	WORD		xcs = context->SegCs;
	SNOOP16_DLL	*dll = firstdll;
	SNOOP16_FUN	*fun = NULL;
	SNOOP16_RETURNENTRIES	**rets = &firstrets;
	SNOOP16_RETURNENTRY	*ret;
	unsigned	i=0;
	int		max;

	while (dll) {
		if (xcs == dll->funhandle) {
			fun = (SNOOP16_FUN*)entry;
			ordinal = fun-dll->funs;
			break;
		}
		dll=dll->next;
	}
	if (!dll) {
		FIXME("entrypoint 0x%08lx not found\n",entry);
		return; /* oops */
	}
	while (*rets) {
		for (i=0;i<ARRAY_SIZE((*rets)->entry);i++)
			if (!(*rets)->entry[i].origreturn)
				break;
		if (i!=ARRAY_SIZE((*rets)->entry))
			break;
		rets = &((*rets)->next);
	}
	if (!*rets) {
		HANDLE16 hand = GlobalHandleToSel16(GLOBAL_Alloc(GMEM_ZEROINIT,65535,0,LDT_FLAGS_CODE));
		*rets = GlobalLock16(hand);
		(*rets)->rethandle = hand;
		i = 0;	/* entry 0 is free */
	}
	ret = &((*rets)->entry[i]);
	ret->lcall 	= 0x9a;
	ret->snoop_return = GetProcAddress16( GetModuleHandle16( "KERNEL" ), "__wine_snoop_return" );
	ret->origreturn	= (FARPROC16)CALLER1REF;
	CALLER1REF	= MAKELONG((char*)&(ret->lcall)-(char*)((*rets)->entry),(*rets)->rethandle);
	ret->dll	= dll;
	ret->args	= NULL;
	ret->ordinal	= ordinal;
	ret->origSP	= LOWORD(context->Esp);

	context->Eip= LOWORD(fun->origfun);
	context->SegCs = HIWORD(fun->origfun);


	TRACE("\1CALL %s.%ld: %s(", dll->name, ordinal, fun->name);
	if (fun->nrofargs>0) {
		max = fun->nrofargs;
		if (max>16) max=16;
		for (i=max;i--;)
			TRACE("%04x%s",*(WORD*)((char *) MapSL( MAKESEGPTR(context->SegSs,LOWORD(context->Esp)) )+8+sizeof(WORD)*i),i?",":"");
		if (max!=fun->nrofargs)
			TRACE(" ...");
	} else if (fun->nrofargs<0) {
		TRACE("<unknown, check return>");
		ret->args = HeapAlloc(GetProcessHeap(),0,16*sizeof(WORD));
		memcpy(ret->args,(LPBYTE)((char *) MapSL( MAKESEGPTR(context->SegSs,LOWORD(context->Esp)) )+8),sizeof(WORD)*16);
	}
	TRACE(") ret=%04x:%04x\n",HIWORD(ret->origreturn),LOWORD(ret->origreturn));
}

void WINAPI __wine_snoop_return( CONTEXT *context )
{
	SNOOP16_RETURNENTRY	*ret = (SNOOP16_RETURNENTRY*)((char *) MapSL( MAKESEGPTR(context->SegCs,LOWORD(context->Eip)) )-5);

	/* We haven't found out the nrofargs yet. If we called a cdecl
	 * function it is too late anyway and we can just set '0' (which
	 * will be the difference between orig and current SP
	 * If pascal -> everything ok.
	 */
	if (ret->dll->funs[ret->ordinal].nrofargs<0) {
		ret->dll->funs[ret->ordinal].nrofargs=(LOWORD(context->Esp)-ret->origSP-4)/2;
	}
	context->Eip = LOWORD(ret->origreturn);
	context->SegCs  = HIWORD(ret->origreturn);
        TRACE("\1RET  %s.%ld: %s(", ret->dll->name, ret->ordinal, ret->dll->funs[ret->ordinal].name);
	if (ret->args) {
		int	i,max;

		max = ret->dll->funs[ret->ordinal].nrofargs;
		if (max>16)
			max=16;
		if (max<0)
			max=0;

		for (i=max;i--;)
			TRACE("%04x%s",ret->args[i],i?",":"");
		if (max!=ret->dll->funs[ret->ordinal].nrofargs)
			TRACE(" ...");
		HeapFree(GetProcessHeap(),0,ret->args);
		ret->args = NULL;
	}
        TRACE(") retval = %04x:%04x ret=%04x:%04x\n",
              (WORD)context->Edx,(WORD)context->Eax,
              HIWORD(ret->origreturn),LOWORD(ret->origreturn));
	ret->origreturn = NULL; /* mark as empty */
}
