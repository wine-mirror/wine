/*
 * 386-specific Win16 dll<->dll snooping functions
 *
 * Copyright 1998 Marcus Meissner
 */

#include <assert.h>
#include <string.h>
#include "winbase.h"
#include "winnt.h"
#include "heap.h"
#include "global.h"
#include "selectors.h"
#include "stackframe.h"
#include "snoop.h"
#include "debugstr.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(snoop)

#ifdef __i386__

#include "pshpack1.h"

void WINAPI SNOOP16_Entry(CONTEXT86 *context);
void WINAPI SNOOP16_Return(CONTEXT86 *context);
extern void KERNEL_CallFrom16_p_regs_();

/* Generic callfrom16_p_regs function entry.
 *      pushw %bp			0x55
 *      pushl $DOS3Call			DWORD fun32
 *      .byte 0x9a			0x9a
 *      .long CallFrom16_p_regs_	DWORD addr
 *      .long 0x90900023		WORD seg;nop;nop
 */

typedef	struct tagSNOOP16_FUN {
	/* code part */
	BYTE		lcall;		/* 0x9a call absolute with segment */
	DWORD		snr;
	/* unreached */
	int		nrofargs;
	FARPROC16	origfun;
	char		*name;
} SNOOP16_FUN;

typedef struct tagSNOOP16_DLL {
	HMODULE16	hmod;
	HANDLE16	funhandle;
	SNOOP16_FUN	*funs;
	LPCSTR		name;
	struct tagSNOOP16_DLL	*next;
} SNOOP16_DLL;

typedef struct tagSNOOP16_RETURNENTRY {
	/* code part */
	BYTE		lcall;		/* 0x9a call absolute with segment */
	DWORD		snr;
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

typedef struct tagSNOOP16_RELAY {
	/* code part */
	BYTE		prefix;		/* 0x66 , 32bit prefix */
	BYTE		pushbp;		/* 0x55 */
	BYTE		pushl;		/* 0x68 */
	DWORD		realfun;	/* SNOOP16_Return */
	BYTE		lcall;		/* 0x9a call absolute with segment */
	DWORD		callfromregs;
	WORD		seg;
	/* unreached */
} SNOOP16_RELAY;

#include "poppack.h"

static	SNOOP16_DLL		*firstdll = NULL;
static	SNOOP16_RETURNENTRIES 	*firstrets = NULL;
static	SNOOP16_RELAY		*snr;
static	HANDLE16		xsnr = 0;

void
SNOOP16_RegisterDLL(NE_MODULE *pModule,LPCSTR name) {
	SNOOP16_DLL	**dll = &(firstdll);
	char		*s;

	if (!TRACE_ON(snoop)) return;
	if (!snr) {
		xsnr=GLOBAL_Alloc(GMEM_ZEROINIT,2*sizeof(*snr),0,TRUE,TRUE,FALSE);
		snr = GlobalLock16(xsnr);
		snr[0].prefix	= 0x66;
		snr[0].pushbp	= 0x55;
		snr[0].pushl	= 0x68;
		snr[0].realfun	= (DWORD)SNOOP16_Entry;
		snr[0].lcall 	= 0x9a;
		snr[0].callfromregs = (DWORD)KERNEL_CallFrom16_p_regs_;
		GET_CS(snr[0].seg);
		snr[1].prefix	= 0x66;
		snr[1].pushbp	= 0x55;
		snr[1].pushl	= 0x68;
		snr[1].realfun	= (DWORD)SNOOP16_Return;
		snr[1].lcall 	= 0x9a;
		snr[1].callfromregs = (DWORD)KERNEL_CallFrom16_p_regs_;
		GET_CS(snr[1].seg);
	}
	while (*dll) {
		if ((*dll)->hmod == pModule->self)
			return; /* already registered */
		dll = &((*dll)->next);
	}
	*dll = (SNOOP16_DLL*)HeapAlloc(SystemHeap,HEAP_ZERO_MEMORY,sizeof(SNOOP16_DLL));
	(*dll)->next	= NULL;
	(*dll)->hmod	= pModule->self;
	if ((s=strrchr(name,'\\')))
		name = s+1;
	(*dll)->name	= HEAP_strdupA(SystemHeap,0,name);
	if ((s=strrchr((*dll)->name,'.')))
		*s='\0';
	(*dll)->funhandle = GlobalHandleToSel16(GLOBAL_Alloc(GMEM_ZEROINIT,65535,0,TRUE,FALSE,FALSE));
	(*dll)->funs = GlobalLock16((*dll)->funhandle);
	if (!(*dll)->funs) {
		HeapFree(SystemHeap,0,*dll);
		FIXME("out of memory\n");
		return;
	}
	memset((*dll)->funs,0,65535);
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
	if (!*(LPBYTE)PTR_SEG_TO_LIN(origfun)) /* 0x00 is an imposs. opcode, poss. dataref. */
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
	fun->snr	= MAKELONG(0,xsnr);
	fun->origfun	= origfun;
	if (fun->name)
		return (FARPROC16)(SEGPTR)MAKELONG(((char*)fun-(char*)dll->funs),dll->funhandle);
	cpnt = (unsigned char *)pModule + pModule->name_table;
	while (*cpnt) {
		cpnt += *cpnt + 1 + sizeof(WORD);
		if (*(WORD*)(cpnt+*cpnt+1) == ordinal) {
			sprintf(name,"%.*s",*cpnt,cpnt+1);
			break;
		}
	}
	/* Now search the non-resident names table */

	if (!*cpnt && pModule->nrname_handle) {
		cpnt = (char *)GlobalLock16( pModule->nrname_handle );
		while (*cpnt) {
			cpnt += *cpnt + 1 + sizeof(WORD);
			if (*(WORD*)(cpnt+*cpnt+1) == ordinal) {
				    sprintf(name,"%.*s",*cpnt,cpnt+1);
				    break;
			}
		}
	}
	if (*cpnt)
		fun->name = HEAP_strdupA(SystemHeap,0,name);
	else
		fun->name = HEAP_strdupA(SystemHeap,0,"");
	if (!SNOOP_ShowDebugmsgSnoop(dll->name, ordinal, fun->name))
		return origfun;

	/* more magic. do not try to snoop thunk data entries (MMSYSTEM) */
	if (strchr(fun->name,'_')) {
		char *s=strchr(fun->name,'_');

		if (!strncasecmp(s,"_thunkdata",10)) {
			HeapFree(SystemHeap,0,fun->name);
			fun->name = NULL;
			return origfun;
		}
	}
	fun->lcall 	= 0x9a;
	fun->snr	= MAKELONG(0,xsnr);
	fun->origfun	= origfun;
	fun->nrofargs	= -1;
	return (FARPROC16)(SEGPTR)MAKELONG(((char*)fun-(char*)dll->funs),dll->funhandle);
}

#define CALLER1REF (*(DWORD*)(PTR_SEG_OFF_TO_LIN(SS_reg(context),SP_reg(context)+4)))
void WINAPI SNOOP16_Entry(CONTEXT86 *context) {
	DWORD		ordinal=0;
	DWORD		entry=(DWORD)PTR_SEG_OFF_TO_LIN(CS_reg(context),IP_reg(context))-5;
	WORD		xcs = CS_reg(context);
	SNOOP16_DLL	*dll = firstdll;
	SNOOP16_FUN	*fun = NULL;
	SNOOP16_RETURNENTRIES	**rets = &firstrets;
	SNOOP16_RETURNENTRY	*ret;
	int		i,max;

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
		for (i=0;i<sizeof((*rets)->entry)/sizeof((*rets)->entry[0]);i++)
			if (!(*rets)->entry[i].origreturn)
				break;
		if (i!=sizeof((*rets)->entry)/sizeof((*rets)->entry[0]))
			break;
		rets = &((*rets)->next);
	}
	if (!*rets) {
		HANDLE16	hand = GlobalHandleToSel16(GLOBAL_Alloc(GMEM_ZEROINIT,65535,0,TRUE,FALSE,FALSE));
		*rets = GlobalLock16(hand);
		memset(*rets,0,65535);
		(*rets)->rethandle = hand;
		i = 0;	/* entry 0 is free */
	}
	ret = &((*rets)->entry[i]);
	ret->lcall 	= 0x9a;
	ret->snr	= MAKELONG(sizeof(SNOOP16_RELAY),xsnr);
	ret->origreturn	= (FARPROC16)CALLER1REF;
	CALLER1REF	= MAKELONG((char*)&(ret->lcall)-(char*)((*rets)->entry),(*rets)->rethandle);
	ret->dll	= dll;
	ret->args	= NULL;
	ret->ordinal	= ordinal;
	ret->origSP	= SP_reg(context);

	IP_reg(context)= LOWORD(fun->origfun);
	CS_reg(context)= HIWORD(fun->origfun);


	DPRINTF("Call %s.%ld: %s(",dll->name,ordinal,fun->name);
	if (fun->nrofargs>0) {
		max = fun->nrofargs;
		if (max>16) max=16;
		for (i=max;i--;)
			DPRINTF("%04x%s",*(WORD*)((char *) PTR_SEG_OFF_TO_LIN(SS_reg(context),SP_reg(context))+8+sizeof(WORD)*i),i?",":"");
		if (max!=fun->nrofargs)
			DPRINTF(" ...");
	} else if (fun->nrofargs<0) {
		DPRINTF("<unknown, check return>");
		ret->args = HeapAlloc(SystemHeap,0,16*sizeof(WORD));
		memcpy(ret->args,(LPBYTE)((char *) PTR_SEG_OFF_TO_LIN(SS_reg(context),SP_reg(context))+8),sizeof(WORD)*16);
	}
	DPRINTF(") ret=%04x:%04x\n",HIWORD(ret->origreturn),LOWORD(ret->origreturn));
}

void WINAPI SNOOP16_Return(CONTEXT86 *context) {
	SNOOP16_RETURNENTRY	*ret = (SNOOP16_RETURNENTRY*)((char *) PTR_SEG_OFF_TO_LIN(CS_reg(context),IP_reg(context))-5);

	/* We haven't found out the nrofargs yet. If we called a cdecl
	 * function it is too late anyway and we can just set '0' (which 
	 * will be the difference between orig and current SP
	 * If pascal -> everything ok.
	 */
	if (ret->dll->funs[ret->ordinal].nrofargs<0) {
		ret->dll->funs[ret->ordinal].nrofargs=(SP_reg(context)-ret->origSP-4)/2;
	}
	IP_reg(context) = LOWORD(ret->origreturn);
	CS_reg(context) = HIWORD(ret->origreturn);
	if (ret->args) {
		int	i,max;

		DPRINTF("Ret  %s.%ld: %s(",ret->dll->name,ret->ordinal,ret->dll->funs[ret->ordinal].name);
		max = ret->dll->funs[ret->ordinal].nrofargs;
		if (max>16)
			max=16;
		if (max<0) 
			max=0;

		for (i=max;i--;)
			DPRINTF("%04x%s",ret->args[i],i?",":"");
		if (max!=ret->dll->funs[ret->ordinal].nrofargs)
			DPRINTF(" ...");
		DPRINTF(") retval = %04x:%04x ret=%04x:%04x\n",
			DX_reg(context),AX_reg(context),HIWORD(ret->origreturn),LOWORD(ret->origreturn)
		);
		HeapFree(SystemHeap,0,ret->args);
		ret->args = NULL;
	} else
		DPRINTF("Ret  %s.%ld: %s() retval = %04x:%04x ret=%04x:%04x\n",
			ret->dll->name,ret->ordinal,ret->dll->funs[ret->ordinal].name,
			DX_reg(context),AX_reg(context),HIWORD(ret->origreturn),LOWORD(ret->origreturn)
		);
	ret->origreturn = NULL; /* mark as empty */
}
#else	/* !__i386__ */
void SNOOP16_RegisterDLL(NE_MODULE *pModule,LPCSTR name) {
	FIXME("snooping works only on i386 for now.\n");
	return;
}

FARPROC16 SNOOP16_GetProcAddress16(HMODULE16 hmod,DWORD ordinal,FARPROC16 origfun) {
	return origfun;
}
#endif	/* !__i386__ */

void
SNOOP16_Init() {
	fnSNOOP16_GetProcAddress16=SNOOP16_GetProcAddress16;
	fnSNOOP16_RegisterDLL=SNOOP16_RegisterDLL;
}
