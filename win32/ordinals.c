/*
 * Win32 ordinal only exported functions
 *
 * Copyright 1997 Marcus Meissner
 */

#include <stdio.h>
#include "thread.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "miscemu.h"
#include "winnt.h"
#include "module.h"
#include "callback.h"
#include "debug.h"
#include "stddebug.h"

extern THDB	*pCurrentThread;
extern PDB32	*pCurrentProcess;

/**********************************************************************
 *           _KERNEL32_88
 */
DWORD
WOW32_1(DWORD x,DWORD y) {
    fprintf(stderr,"WOW32_1(0x%08lx,0x%08lx), stub!\n",x,y);
    return 0;
}

/**********************************************************************
 *           WOWGetVDMPointer	(KERNEL32.55)
 * Get linear from segmented pointer. (MSDN lib)
 */
LPVOID
WOWGetVDMPointer(DWORD vp,DWORD nrofbytes,BOOL32 protected) {
    /* FIXME: add size check too */
    fprintf(stdnimp,"WOWGetVDMPointer(%08lx,%ld,%d)\n",vp,nrofbytes,protected);
    if (protected)
	return PTR_SEG_TO_LIN(vp);
    else
	return DOSMEM_MapRealToLinear(vp);
}

/**********************************************************************
 *           WOWGetVDMPointerFix (KERNEL32.55)
 * Dito, but fix heapsegment (MSDN lib)
 */
LPVOID
WOWGetVDMPointerFix(DWORD vp,DWORD nrofbytes,BOOL32 protected) {
    /* FIXME: fix heapsegment */
    fprintf(stdnimp,"WOWGetVDMPointerFix(%08lx,%ld,%d)\n",vp,nrofbytes,protected);
    return WOWGetVDMPointer(vp,nrofbytes,protected);
}

/**********************************************************************
 *           WOWGetVDMPointerUnFix (KERNEL32.56)
 */
void
WOWGetVDMPointerUnfix(DWORD vp) {
    fprintf(stdnimp,"WOWGetVDMPointerUnfix(%08lx), STUB\n",vp);
    /* FIXME: unfix heapsegment */
}


/***********************************************************************
 *           _KERNEL32_18    (KERNEL32.18)
 * 'Of course you cannot directly access Windows internal structures'
 */

DWORD
_KERNEL32_18(DWORD processid,DWORD action) {
	PDB32	*process;
	TDB	*pTask;

	action+=56;
	fprintf(stderr,"KERNEL32_18(%ld,%ld+0x38)\n",processid,action);
	if (action>56)
		return 0;
	if (!processid) {
		process=pCurrentProcess;
		/* check if valid process */
	} else
		process=(PDB32*)pCurrentProcess; /* decrypt too, if needed */
	switch (action) {
	case 0:	/* return app compat flags */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->compat_flags;
	case 4:	/* returns offset 0xb8 of process struct... dunno what it is */
		return 0;
	case 8:	/* return hinstance16 */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->hInstance;
	case 12:/* return expected windows version */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->version;
	case 16:/* return uncrypted pointer to current thread */
		return (DWORD)pCurrentThread;
	case 20:/* return uncrypted pointer to process */
		return (DWORD)process;
	case 24:/* return stdoutput handle from startupinfo */
		return (DWORD)(process->env_db->startup_info->hStdOutput);
	case 28:/* return stdinput handle from startupinfo */
		return (DWORD)(process->env_db->startup_info->hStdInput);
	case 32:/* get showwindow flag from startupinfo */
		return (DWORD)(process->env_db->startup_info->wShowWindow);
	case 36:{/* return startup x and y sizes */
		LPSTARTUPINFO32A si = process->env_db->startup_info;
		DWORD x,y;

		x=si->dwXSize;if (x==0x80000000) x=0x8000;
		y=si->dwYSize;if (y==0x80000000) y=0x8000;
		return (y<<16)+x;
	}
	case 40:{/* return startup x and y */
		LPSTARTUPINFO32A si = process->env_db->startup_info;
		DWORD x,y;

		x=si->dwX;if (x==0x80000000) x=0x8000;
		y=si->dwY;if (y==0x80000000) y=0x8000;
		return (y<<16)+x;
	}
	case 44:/* return startup flags */
		return process->env_db->startup_info->dwFlags;
	case 48:/* return uncrypted pointer to parent process (if any) */
		return (DWORD)process->parent;
	case 52:/* return process flags */
		return process->flags;
	case 56:/* unexplored */
		return 0;
	default:
		fprintf(stderr,"_KERNEL32_18:unknown offset (%ld)\n",action);
		return 0;
	}
	/* shouldn't come here */
}

/***********************************************************************
 *           _KERNEL32_52    (KERNEL32.52)
 * FIXME: what does it really do?
 */
VOID
_KERNEL32_52(DWORD arg1,CONTEXT *regs) {
	fprintf(stderr,"_KERNE32_52(arg1=%08lx,%08lx)\n",arg1,EDI_reg(regs));

	EAX_reg(regs) = (DWORD)WIN32_GetProcAddress16(EDI_reg(regs),"ThkBuf");

	fprintf(stderr,"	GetProcAddress16(\"ThkBuf\") returns %08lx\n",
			EAX_reg(regs)
	);
}

/***********************************************************************
 *           GetPWinLock    (KERNEL32) FIXME
 * get mutex? critical section for 16 bit mode?
 */
VOID
GetPWinLock(CRITICAL_SECTION **lock) {
	static CRITICAL_SECTION plock;
	fprintf(stderr,"GetPWinlock(%p)\n",lock);
	*lock = &plock;
}

/***********************************************************************
 * 		_KERNEL32_43 	(KERNEL32.42)
 * A thunkbuffer link routine 
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The pointer ptr is written into the first DWORD of 'thunk'.
 * (probably correct implemented)
 */
BOOL32
_KERNEL32_43(LPDWORD thunk,LPCSTR thkbuf,DWORD len,LPCSTR dll16,LPCSTR dll32) {
	HINSTANCE16	hmod;
	LPDWORD		addr;
	SEGPTR		segaddr;

	fprintf(stderr,"_KERNEL32_43(%p,%s,0x%08lx,%s,%s)\n",thunk,thkbuf,len,dll16,dll32);

	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		fprintf(stderr,"->failed to load 16bit DLL %s, error %d\n",dll16,hmod);
		return NULL;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		fprintf(stderr,"->no %s exported from %s!\n",thkbuf,dll16);
		return NULL;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"->thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return NULL;
	}
	if (!addr[1])
		return 0;
	*(DWORD*)thunk = addr[1];
	return addr[1];
}

/***********************************************************************
 * 		_KERNEL32_45 	(KERNEL32.44)
 * FIXME: not sure what it does
 */
VOID
_KERNEL32_45(DWORD x,CONTEXT *context) {
	fprintf(stderr,"_KERNEL32_45(0x%08lx,%%eax=0x%08lx,%%cx=0x%04lx,%%edx=0x%08lx)\n",
		x,(DWORD)EAX_reg(context),(DWORD)CX_reg(context),(DWORD)EDX_reg(context)
	);
}

/***********************************************************************
 *		(KERNEL32.40)
 * A thunk setup routine.
 * Expects a pointer to a preinitialized thunkbuffer in the first argument
 * looking like:
 *	00..03:		unknown	(some pointer to the 16bit struct?)
 *	04: EB1E		jmp +0x20
 *
 *	06..23:		unknown (space for replacement code, check .90)
 *
 *	24:>E8xxxxxxxx		call <32bitoffset xxxxxxxx>
 *	29: 58			pop eax
 *	2A: 2D2500xxxx		and eax,0x2500xxxx
 *	2F: BAxxxxxxxx		mov edx,xxxxxxxx
 *	34: 68yyyyyyyy		push KERNEL32.90
 *	39: C3			ret
 *
 *	3A: EB1E		jmp +0x20
 *	3E ... 59:	unknown (space for replacement code?)
 *	5A: E8xxxxxxxx		call <32bitoffset xxxxxxxx>
 *	5F: 5A			pop edx
 *	60: 81EA25xxxxxx	sub edx, 0x25xxxxxx
 *	66: 52			push edx
 *	67: 68xxxxxxxx		push xxxxxxxx
 *	6C: 68yyyyyyyy		push KERNEL32.89
 *	71: C3			ret
 *	72: end?
 * This function checks if the code is there, and replaces the yyyyyyyy entries
 * by the functionpointers.
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The segpointer ptr is written into the first DWORD of 'thunk'.
 * (probably correct implemented)
 */

LPVOID
_KERNEL32_41(LPBYTE thunk,LPCSTR thkbuf,DWORD len,LPCSTR dll16,LPCSTR dll32) {
	HMODULE32	hkrnl32 = WIN32_GetModuleHandleA("KERNEL32");
	HMODULE16	hmod;
	LPDWORD		addr,addr2;
	DWORD		segaddr;

	fprintf(stderr,"KERNEL32_41(%p,%s,%ld,%s,%s)\n",
		thunk,thkbuf,len,dll16,dll32
	);

	/* FIXME: add checks for valid code ... */
	/* write pointers to kernel32.89 and kernel32.90 (+ordinal base of 1) */
	*(DWORD*)(thunk+0x35) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)91);
	*(DWORD*)(thunk+0x6D) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)90);

	
	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		fprintf(stderr,"->failed to load 16bit DLL %s, error %d\n",dll16,hmod);
		return NULL;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		fprintf(stderr,"->no %s exported from %s!\n",thkbuf,dll16);
		return NULL;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"->thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return NULL;
	}
	addr2 = PTR_SEG_TO_LIN(addr[1]);
	if (HIWORD(addr2))
		*(DWORD*)thunk = (DWORD)addr2;
	return addr2;
}

/***********************************************************************
 *							(KERNEL32.33)
 * Returns some internal value.... probably the default environment database?
 */
DWORD
_KERNEL32_34() {
	fprintf(stderr,"KERNEL32_34(), STUB returning 0\n");
	return 0;
}

/***********************************************************************
 *							(KERNEL32.90)
 * Thunk priming? function
 * Rewrites the first part of the thunk to use the QT_Thunk interface.
 * Replaces offset 4 ... 24 by:
 *	
 *  33C9                    xor ecx, ecx
 *  8A4DFC                  mov cl , [ebp-04]
 *  8B148Dxxxxxxxx          mov edx, [4*ecx + (EAX+EDX)]
 *  B8yyyyyyyy              mov eax, QT_Thunk
 *  FFE0                    jmp eax
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 *  CC                      int 03
 * and jumps to the start of that code.
 * (ok)
 */
VOID
_KERNEL32_91(CONTEXT *context) {
	LPBYTE	x;

	fprintf(stderr,"_KERNEL32_91(eax=0x%08lx,edx=0x%08lx)\n",
		EAX_reg(context),EDX_reg(context)
	);
	x = (LPBYTE)EAX_reg(context);
	*x++ = 0x33;*x++=0xC9; /* xor ecx,ecx */
	*x++ = 0x8A;*x++=0x4D;*x++=0xFC; /* mov cl,[ebp-04] */
	*x++ = 0x8B;*x++=0x14;*x++=0x8D;*(DWORD*)x= EAX_reg(context)+EDX_reg(context);
	x+=4;	/* mov edx, [4*ecx + (EAX+EDX) */
	*x++ = 0xB8; *(DWORD*)x = (DWORD)GetProcAddress32(WIN32_GetModuleHandleA("KERNEL32"),"QT_Thunk");
	x+=4; 	/* mov eax , QT_Thunk */
	*x++ = 0xFF; *x++ = 0xE0;	/* jmp eax */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	*x++ = 0xCC;	/* int 03 */
	EIP_reg(context) = EAX_reg(context);
}

/***********************************************************************
 *							(KERNEL32.45)
 * Another thunkbuf link routine.
 * The start of the thunkbuf looks like this:
 * 	00: DWORD	length
 *	04: SEGPTR	address for thunkbuffer pointer
 */
VOID
_KERNEL32_46(LPBYTE thunk,LPSTR thkbuf,DWORD len,LPSTR dll16,LPSTR dll32) {
	LPDWORD		addr;
	HMODULE16	hmod;
	SEGPTR		segaddr;

	fprintf(stderr,"KERNEL32_46(%p,%s,%lx,%s,%s)\n",
		thunk,thkbuf,len,dll16,dll32
	);
	hmod = LoadLibrary16(dll16);
	if (hmod < 32) {
		fprintf(stderr,"->couldn't load %s, error %d\n",dll16,hmod);
		return;
	}
	segaddr = (SEGPTR)WIN32_GetProcAddress16(hmod,thkbuf);
	if (!segaddr) {
		fprintf(stderr,"-> haven't found %s in %s!\n",thkbuf,dll16);
		return;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"-> length of thkbuf differs from expected length! (%ld vs %ld)\n",addr[0],len);
		return;
	}
	*(DWORD*)PTR_SEG_TO_LIN(addr[1]) = (DWORD)thunk;
}

/**********************************************************************
 *           _KERNEL32_87
 * Check if thunking is initialized (ss selector set up etc.)
 */
BOOL32 _KERNEL32_87() {
    fprintf(stderr,"KERNEL32_87 stub, returning TRUE\n");
    return TRUE;
}

/**********************************************************************
 *           _KERNEL32_88
 * One of the real thunking functions
 * Probably implemented totally wrong.
 */
BOOL32 _KERNEL32_88(DWORD nr,DWORD flags,LPVOID fun,DWORD *hmm) {
    fprintf(stderr,"KERNEL32_88(%ld,0x%08lx,%p,%p) stub, returning TRUE\n",
	nr,flags,fun,hmm
    );
#ifndef WINELIB
    switch (nr) {
    case 0: CallTo32_0(fun);
	break;
    case 4: CallTo32_1(fun,hmm[0]);
	break;
    case 8: CallTo32_2(fun,hmm[0],hmm[1]);
	break;
    default:
	fprintf(stderr,"    unsupported nr of arguments, %ld\n",nr);
	break;

    }
#endif
    return TRUE;
}
