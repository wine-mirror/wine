/*
 * 386-specific Win32 dll<->dll snooping functions
 *
 * Copyright 1998 Marcus Meissner
 */


#include <assert.h>
#include "windows.h"
#include "winbase.h"
#include "winnt.h"
#include "heap.h"
#include "builtin32.h"
#include "snoop.h"
#include "peexe.h"
#include "selectors.h"
#include "stackframe.h"
#include "debugstr.h"
#include "debug.h"

#ifdef __i386__

char **debug_snoop_excludelist = NULL, **debug_snoop_includelist = NULL;

#ifdef NEED_UNDERSCORE_PREFIX
# define PREFIX "_"
#else
# define PREFIX
#endif

/* Well, not exactly extern since they are in the same file (in the lines
 * below). But the C Compiler doesn't see them there, so we have to help a bit.
 */
extern void SNOOP_Return();
extern void SNOOP_Entry();
__asm__(".align 4\n\t"
        ".globl "PREFIX"SNOOP_Entry\n\t"
        ".type "PREFIX"SNOOP_Entry,@function\n\t"
        PREFIX"SNOOP_Entry:\n\t"
        "pushl $"PREFIX"__regs_SNOOP_Entry\n\t"
        "pushl $"PREFIX"CALL32_Regs\n\t"
        "ret\n\t"
	".align 4\n\t"
        ".globl "PREFIX"SNOOP_Return\n\t"
        ".type "PREFIX"SNOOP_Return,@function\n\t"
        PREFIX"SNOOP_Return:\n\t"
        "pushl $"PREFIX"__regs_SNOOP_Return\n\t"
        "pushl $"PREFIX"CALL32_Regs\n\t"
        "ret"
);

#pragma pack(1)

typedef	struct tagSNOOP_FUN {
	/* code part */
	BYTE		lcall;		/* 0xe8 call snoopentry (relative) */
	/* NOTE: If you move snoopentry OR nrofargs fix the relative offset
	 * calculation!
	 */
	DWORD		snoopentry;	/* SNOOP_Entry relative */
	/* unreached */
	int		nrofargs;
	FARPROC32	origfun;
	char		*name;
} SNOOP_FUN;

typedef struct tagSNOOP_DLL {
	HMODULE32	hmod;
	SNOOP_FUN	*funs;
	LPCSTR		name;
	int		nrofordinals;
	struct tagSNOOP_DLL	*next;
} SNOOP_DLL;
typedef struct tagSNOOP_RETURNENTRY {
	/* code part */
	BYTE		lcall;		/* 0xe8 call snoopret relative*/
	/* NOTE: If you move snoopret OR origreturn fix the relative offset
	 * calculation!
	 */
	DWORD		snoopret;	/* SNOOP_Ret relative */
	/* unreached */
	FARPROC32	origreturn;
	SNOOP_DLL	*dll;
	DWORD		ordinal;
	DWORD		origESP;
	DWORD		*args;		/* saved args across a stdcall */
	BYTE		show;
} SNOOP_RETURNENTRY;

typedef struct tagSNOOP_RETURNENTRIES {
	SNOOP_RETURNENTRY entry[4092/sizeof(SNOOP_RETURNENTRY)];
	struct tagSNOOP_RETURNENTRIES	*next;
} SNOOP_RETURNENTRIES;

#pragma pack(4)

static	SNOOP_DLL		*firstdll = NULL;
static	SNOOP_RETURNENTRIES 	*firstrets = NULL;

/***********************************************************************
 *          SNOOP_ShowDebugmsgSnoop
 *
 * Simple function to decide if a particular debugging message is
 * wanted.
 */
int SNOOP_ShowDebugmsgSnoop(const char *dll, int ord, const char *fname) {

  if(debug_snoop_excludelist || debug_snoop_includelist) {
    char **listitem;
    char buf[80];
    int len, len2, itemlen, show;

    if(debug_snoop_excludelist) {
      show = 1;
      listitem = debug_snoop_excludelist;
    } else {
      show = 0;
      listitem = debug_snoop_includelist;
    }
    len = strlen(dll);
    assert(len < 64);
    sprintf(buf, "%s.%d", dll, ord);
    len2 = strlen(buf);
    for(; *listitem; listitem++) {
      itemlen = strlen(*listitem);
      if((itemlen == len && !strncmp(*listitem, buf, len)) ||
         (itemlen == len2 && !strncmp(*listitem, buf, len2)) ||
         !strcmp(*listitem, fname)) {
        show = !show;
       break;
      }
    }
    return show;
  }
  return 1;
}

void
SNOOP_RegisterDLL(HMODULE32 hmod,LPCSTR name,DWORD nrofordinals) {
	SNOOP_DLL	**dll = &(firstdll);
	char		*s;

	if (!TRACE_ON(snoop)) return;
	while (*dll) {
		if ((*dll)->hmod == hmod)
			return; /* already registered */
		dll = &((*dll)->next);
	}
	*dll = (SNOOP_DLL*)HeapAlloc(SystemHeap,HEAP_ZERO_MEMORY,sizeof(SNOOP_DLL));
	(*dll)->next	= NULL;
	(*dll)->hmod	= hmod;
	(*dll)->nrofordinals = nrofordinals;
	(*dll)->name	= HEAP_strdupA(SystemHeap,0,name);
	if ((s=strrchr((*dll)->name,'.')))
		*s='\0';
	(*dll)->funs = VirtualAlloc(NULL,nrofordinals*sizeof(SNOOP_FUN),MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	memset((*dll)->funs,0,nrofordinals*sizeof(SNOOP_FUN));
	if (!(*dll)->funs) {
		HeapFree(SystemHeap,0,*dll);
		FIXME(snoop,"out of memory\n");
		return;
	}
}

FARPROC32
SNOOP_GetProcAddress32(HMODULE32 hmod,LPCSTR name,DWORD ordinal,FARPROC32 origfun) {
	SNOOP_DLL			*dll = firstdll;
	SNOOP_FUN			*fun;
	int				j;
	IMAGE_SECTION_HEADER		*pe_seg = PE_SECTIONS(hmod);

	if (!TRACE_ON(snoop)) return origfun;
	if (!*(LPBYTE)origfun) /* 0x00 is an imposs. opcode, poss. dataref. */
		return origfun;
	for (j=0;j<PE_HEADER(hmod)->FileHeader.NumberOfSections;j++)
		if (((DWORD)origfun-hmod>=pe_seg[j].VirtualAddress)&&
		    ((DWORD)origfun-hmod <pe_seg[j].VirtualAddress+
		    		   pe_seg[j].SizeOfRawData)
		)
			break;
	/* If we looked through all sections (and didn't find one)
	 * or if the sectionname contains "data", we return the
	 * original function since it is most likely a datareference.
	 */
	if (	(j==PE_HEADER(hmod)->FileHeader.NumberOfSections)	||
		(strstr(pe_seg[j].Name,"data"))				||
		!(pe_seg[j].Characteristics & IMAGE_SCN_CNT_CODE)
	)
		return origfun;

	while (dll) {
		if (hmod == dll->hmod)
			break;
		dll=dll->next;
	}
	if (!dll)	/* probably internal */
		return origfun;
	assert(ordinal<dll->nrofordinals);
	fun = dll->funs+ordinal;
	if (!fun->name) fun->name = HEAP_strdupA(SystemHeap,0,name);
	fun->lcall	= 0xe8;
	/* NOTE: origreturn struct member MUST come directly after snoopentry */
	fun->snoopentry	= (char*)SNOOP_Entry-((char*)(&fun->nrofargs));
	fun->origfun	= origfun;
	fun->nrofargs	= -1;
	return (FARPROC32)&(fun->lcall);
}

static char*
SNOOP_PrintArg(DWORD x) {
	static char	buf[200];
	int		i,nostring;
	MEMORY_BASIC_INFORMATION	mbi;

	if (	!HIWORD(x)					||
		!VirtualQuery((LPVOID)x,&mbi,sizeof(mbi))	||
		!mbi.Type
	) {
		sprintf(buf,"%08lx",x);
		return buf;
	}
	i=0;nostring=0;
	if (!IsBadStringPtr32A((LPSTR)x,80)) {
		while (i<80) {
			LPBYTE	s=(LPBYTE)x;

			if (s[i]==0) break;
			if (s[i]<0x20) {nostring=1;break;}
			if (s[i]>=0x80) {nostring=1;break;}
			i++;
		}
		if (!nostring) {
			if (i>5) {
				sprintf(buf,"%08lx \"",x);
				strncat(buf,(LPSTR)x,198-strlen(buf)-2);
				strcat(buf,"\"");
				return buf;
			}
		}
	}
	i=0;nostring=0;
	if (!IsBadStringPtr32W((LPWSTR)x,80)) {
		while (i<80) {
			LPWSTR	s=(LPWSTR)x;

			if (s[i]==0) break;
			if (s[i]<0x20) {nostring=1;break;}
			if (s[i]>0x100) {nostring=1;break;}
			i++;
		}
		if (!nostring) {
			if (i>5) {
				sprintf(buf,"%08lx L",x);
				strcat(buf,debugstr_wn((LPWSTR)x,198-strlen(buf)-2));
				return buf;
			}
		}
	}
	sprintf(buf,"%08lx",x);
	return buf;
}

#define CALLER1REF (*(DWORD*)(ESP_reg(context)+4))
REGS_ENTRYPOINT(SNOOP_Entry) {
	DWORD		ordinal=0,entry = EIP_reg(context)-5;
	SNOOP_DLL	*dll = firstdll;
	SNOOP_FUN	*fun = NULL;
	SNOOP_RETURNENTRIES	**rets = &firstrets;
	SNOOP_RETURNENTRY	*ret;
	int		i,max,show;

	while (dll) {
		if (	((char*)entry>=(char*)dll->funs)	&&
			((char*)entry<=(char*)(dll->funs+dll->nrofordinals))
		) {
			fun = (SNOOP_FUN*)entry;
			ordinal = fun-dll->funs;
			break;
		}
		dll=dll->next;
	}
	if (!dll) {
		FIXME(snoop,"entrypoint 0x%08lx not found\n",entry);
		return; /* oops */
	}
	if (!SNOOP_ShowDebugmsgSnoop(dll->name, ordinal, fun->name)) {
		/* we don't display, so we don't need returndisplay either */
		EIP_reg(context)= (DWORD)fun->origfun;
		return;
	}
	/* guess cdecl ... */
	if (fun->nrofargs<0) {
		/* Typical cdecl return frame is:
		 * 	add esp, xxxxxxxx
		 * which has (for xxxxxxxx up to 255 the opcode "83 C4 xx".
		 */
		LPBYTE	reteip = (LPBYTE)CALLER1REF;

		if (reteip) {
			if ((reteip[0]==0x83)&&(reteip[1]==0xc4))
				fun->nrofargs=reteip[2]/4;
		}
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
		*rets = VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		memset(*rets,0,4096);
		i = 0;	/* entry 0 is free */
	}
	ret = &((*rets)->entry[i]);
	ret->lcall	= 0xe8;
	/* NOTE: origreturn struct member MUST come directly after snoopret */
	ret->snoopret	= ((char*)SNOOP_Return)-(char*)(&ret->origreturn);
	ret->origreturn	= (FARPROC32)CALLER1REF;
	CALLER1REF	= (DWORD)&ret->lcall;
	ret->dll	= dll;
	ret->args	= NULL;
	ret->ordinal	= ordinal;
	ret->origESP	= ESP_reg(context);

	EIP_reg(context)= (DWORD)fun->origfun;

	if(!ret->show) return;
	DPRINTF("Call %s.%ld: %s(",dll->name,ordinal,fun->name);
	if (fun->nrofargs>0) {
		max = fun->nrofargs; if (max>16) max=16;
		for (i=0;i<max;i++)
			DPRINTF("%s%s",SNOOP_PrintArg(*(DWORD*)(ESP_reg(context)+8+sizeof(DWORD)*i)),(i<fun->nrofargs-1)?",":"");
		if (max!=fun->nrofargs)
			DPRINTF(" ...");
	} else if (fun->nrofargs<0) {
		DPRINTF("<unknown, check return>");
		ret->args = HeapAlloc(SystemHeap,0,16*sizeof(DWORD));
		memcpy(ret->args,(LPBYTE)(ESP_reg(context)+8),sizeof(DWORD)*16);
	}
	DPRINTF(") ret=%08lx fs=%04lx\n",ret->origreturn,FS_reg(context));
}

REGS_ENTRYPOINT(SNOOP_Return) {
	SNOOP_RETURNENTRY	*ret = (SNOOP_RETURNENTRY*)(EIP_reg(context)-5);

	/* We haven't found out the nrofargs yet. If we called a cdecl
	 * function it is too late anyway and we can just set '0' (which
	 * will be the difference between orig and current ESP
	 * If stdcall -> everything ok.
	 */
	if (ret->dll->funs[ret->ordinal].nrofargs<0)
		ret->dll->funs[ret->ordinal].nrofargs=(ESP_reg(context)-ret->origESP-4)/4;
	EIP_reg(context) = (DWORD)ret->origreturn;
	if(!ret->show) {
		;
	} else if (ret->args) {
		int	i,max;

		DPRINTF("Ret  %s.%ld: %s(",ret->dll->name,ret->ordinal,ret->dll->funs[ret->ordinal].name);
		max = ret->dll->funs[ret->ordinal].nrofargs;
		if (max>16) max=16;

		for (i=0;i<max;i++)
			DPRINTF("%s%s",SNOOP_PrintArg(ret->args[i]),(i<max-1)?",":"");
		DPRINTF(") retval = %08lx ret=%08lx fs=%04lx\n",
			EAX_reg(context),(DWORD)ret->origreturn,FS_reg(context)
		);
		HeapFree(SystemHeap,0,ret->args);
		ret->args = NULL;
	} else
		DPRINTF("Ret  %s.%ld: %s() retval = %08lx ret=%08lx fs=%04lx\n",
			ret->dll->name,ret->ordinal,ret->dll->funs[ret->ordinal].name,
			EAX_reg(context),(DWORD)ret->origreturn,FS_reg(context)
		);
	ret->origreturn = NULL; /* mark as empty */
}
#else	/* !__i386__ */
void SNOOP_RegisterDLL(HMODULE32 hmod,LPCSTR name,DWORD nrofordinals) {
	FIXME(snoop,"snooping works only on i386 for now.\n");
	return;
}

FARPROC32 SNOOP_GetProcAddress32(HMODULE32 hmod,LPCSTR name,DWORD ordinal,FARPROC32 origfun) {
	return origfun;
}
#endif	/* !__i386__ */
