/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1997 Marcus Meissner
 */

#include <stdio.h>
#include "windows.h"
#include "callback.h"
#include "resource.h"
#include "task.h"
#include "user.h"
#include "heap.h"
#include "module.h"
#include "stackframe.h"
#include "selectors.h"
#include "task.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *                                                                     *
 *                 Win95 internal thunks                               *
 *                                                                     *
 ***********************************************************************/

/***********************************************************************
 * Generates a FT_Prolog call.
 *	
 *  0FB6D1                  movzbl edx,cl
 *  8B1495xxxxxxxx	    mov edx,[4*edx + xxxxxxxx]
 *  68xxxxxxxx		    push FT_Prolog
 *  C3			    lret
 */
static void _write_ftprolog(LPBYTE thunk,DWORD thunkstart) {
	LPBYTE	x;

	x	= thunk;
	*x++	= 0x0f;*x++=0xb6;*x++=0xd1; /* movzbl edx,cl */
	*x++	= 0x8B;*x++=0x14;*x++=0x95;*(DWORD*)x= thunkstart;
	x+=4;	/* mov edx, [4*edx + thunkstart] */
	*x++	= 0x68; *(DWORD*)x = (DWORD)GetProcAddress32(GetModuleHandle32A("KERNEL32"),"FT_Prolog");
	x+=4; 	/* push FT_Prolog */
	*x++	= 0xC3;		/* lret */
	/* fill rest with 0xCC / int 3 */
}

/***********************************************************************
 *			FT_PrologPrime			(KERNEL32.89)
 */
void WINAPI FT_PrologPrime(DWORD startind,LPBYTE thunk) {
	_write_ftprolog(thunk,*(DWORD*)(startind+thunk));
}

/***********************************************************************
 * Generates a QT_Thunk style call.
 *	
 *  33C9                    xor ecx, ecx
 *  8A4DFC                  mov cl , [ebp-04]
 *  8B148Dxxxxxxxx          mov edx, [4*ecx + (EAX+EDX)]
 *  B8yyyyyyyy              mov eax, QT_Thunk
 *  FFE0                    jmp eax
 */
static void _write_qtthunk(LPBYTE start,DWORD thunkstart) {
	LPBYTE	x;

	x	= start;
	*x++	= 0x33;*x++=0xC9; /* xor ecx,ecx */
	*x++	= 0x8A;*x++=0x4D;*x++=0xFC; /* movb cl,[ebp-04] */
	*x++	= 0x8B;*x++=0x14;*x++=0x8D;*(DWORD*)x= thunkstart;
	x+=4;	/* mov edx, [4*ecx + (EAX+EDX) */
	*x++	= 0xB8; *(DWORD*)x = (DWORD)GetProcAddress32(GetModuleHandle32A("KERNEL32"),"QT_Thunk");
	x+=4; 	/* mov eax , QT_Thunk */
	*x++	= 0xFF; *x++ = 0xE0;	/* jmp eax */
	/* should fill the rest of the 32 bytes with 0xCC */
}

/***********************************************************************
 *		ThunkConnect32		(KERNEL32)
 * Connects a 32bit and a 16bit thunkbuffer.
 */
struct thunkstruct
{
	char	magic[4];
	DWORD	length;
	DWORD	ptr;
	DWORD	x0C;

	DWORD	x10;
	DWORD	x14;
	DWORD	x18;
	DWORD	x1C;
	DWORD	x20;
};

UINT32 WINAPI ThunkConnect32( struct thunkstruct *ths, LPSTR thunkfun16,
                              LPSTR module16, LPSTR module32, HMODULE32 hmod32,
                              DWORD dllinitarg1 )
{
	HINSTANCE16	hmm;
	SEGPTR		thkbuf;
	struct	thunkstruct	*ths16;

	dprintf_thunk(stddeb,"ThunkConnect32(<struct>,%s,%s,%s,%x,%lx)\n",
		thunkfun16,module32,module16,hmod32,dllinitarg1
	);
	dprintf_thunk(stddeb,"	magic = %c%c%c%c\n",
		ths->magic[0],
		ths->magic[1],
		ths->magic[2],
		ths->magic[3]
	);
	dprintf_thunk(stddeb,"	length = %lx\n",ths->length);
	if (lstrncmp32A(ths->magic,"SL01",4)&&lstrncmp32A(ths->magic,"LS01",4))
		return 0;
	hmm=LoadModule16(module16,NULL);
	if (hmm<=32)
		return 0;
	thkbuf=(SEGPTR)WIN32_GetProcAddress16(hmm,thunkfun16);
	if (!thkbuf)
		return 0;
	ths16=(struct thunkstruct*)PTR_SEG_TO_LIN(thkbuf);
	if (lstrncmp32A(ths16->magic,ths->magic,4))
		return 0;

	if (!lstrncmp32A(ths->magic,"SL01",4))  {
		if (ths16->length != ths->length)
			return 0;
		ths->x0C = (DWORD)ths16;

		dprintf_thunk(stddeb,"	ths16 magic is 0x%08lx\n",*(DWORD*)ths16->magic);
		if (*((DWORD*)ths16->magic) != 0x0000304C)
			return 0;
		if (!*(WORD*)(((LPBYTE)ths16)+0x12))
			return 0;
		
	}
	if (!lstrncmp32A(ths->magic,"LS01",4))  {
		if (ths16->length != ths->length)
			return 0;
		ths->ptr = (DWORD)PTR_SEG_TO_LIN(ths16->ptr);
		/* code offset for QT_Thunk is at 0x1C...  */
		_write_qtthunk (((LPBYTE)ths) + ths->x1C,ths->ptr);
		/* code offset for FT_Prolog is at 0x20...  */
		_write_ftprolog(((LPBYTE)ths) + ths->x20,ths->ptr);
		return 1;
	}
	return TRUE;
}


/**********************************************************************
 * 		QT_Thunk			(KERNEL32)
 *
 * The target address is in EDX.
 * The 16 bit arguments start at ESP+4.
 * The number of 16bit argumentbytes is EBP-ESP-0x44 (68 Byte thunksetup).
 * [ok]
 */
VOID WINAPI QT_Thunk(CONTEXT *context)
{
	CONTEXT	context16;
	DWORD	argsize;

	memcpy(&context16,context,sizeof(context16));

	CS_reg(&context16)	 = HIWORD(EDX_reg(context));
	IP_reg(&context16)	 = LOWORD(EDX_reg(context));

	argsize = EBP_reg(context)-ESP_reg(context)-0x44;

	/* additional 4 bytes used by the relaycode for storing the stackptr */
	memcpy(	((LPBYTE)CURRENT_STACK16)-argsize-4,
		(LPBYTE)ESP_reg(context)+4,
		argsize
	);
	EAX_reg(context) = Callbacks->CallRegisterShortProc(&context16,
                                                            -argsize);
}


/**********************************************************************
 *           WOWCallback16 (KERNEL32.62)
 */
DWORD WINAPI WOWCallback16(FARPROC16 fproc,DWORD arg)
{
	DWORD	ret;
	dprintf_thunk(stddeb,"WOWCallback16(%p,0x%08lx) ",fproc,arg);
	ret =  Callbacks->CallWOWCallbackProc(fproc,arg);
	dprintf_thunk(stddeb,"... returns %ld\n",ret);
	return ret;
}

/***********************************************************************
 *           _KERNEL32_52    (KERNEL32.52)
 * Returns a pointer to ThkBuf in the 16bit library SYSTHUNK.DLL.
 * [ok probably]
 */
LPVOID WINAPI _KERNEL32_52()
{
	HMODULE32	hmod = LoadLibrary16("systhunk.dll");

	dprintf_thunk(stddeb, "_KERNEL32_52: systhunk.dll module %d\n", hmod);
	
	if (hmod<=32)
		return 0;
	return PTR_SEG_TO_LIN(WIN32_GetProcAddress16(hmod,"ThkBuf"));
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
 * [ok probably]
 */
DWORD WINAPI _KERNEL32_43(LPDWORD thunk,LPCSTR thkbuf,DWORD len,
                           LPCSTR dll16,LPCSTR dll32)
{
	HINSTANCE16	hmod;
	LPDWORD		addr;
	SEGPTR		segaddr;

	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		fprintf(stderr,"KERNEL32_43->failed to load 16bit DLL %s, error %d\n",dll16,hmod);
		return 0;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		fprintf(stderr,"KERNEL32_43->no %s exported from %s!\n",thkbuf,dll16);
		return 0;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"KERNEL32_43->thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return 0;
	}
	if (!addr[1])
		return 0;
	*(DWORD*)thunk = addr[1];

	dprintf_thunk(stddeb, "_KERNEL32_43: loaded module %d, func %s (%d) @ %p (%p), returning %p\n",
		      hmod, HIWORD(thkbuf)==0 ? "<ordinal>" : thkbuf, (int)thkbuf, (void*)segaddr, addr, (void*)addr[1]);

	return addr[1];
}

/***********************************************************************
 * 		_KERNEL32_45 	(KERNEL32.44)
 * Another 32->16 thunk, the difference to QT_Thunk is, that the called routine
 * uses 0x66 lret, and that we have to pass CX in DI.
 * (there seems to be some kind of BL/BX return magic too...)
 *
 * [doesn't crash anymore]
 */
VOID WINAPI _KERNEL32_45(CONTEXT *context)
{
	CONTEXT	context16;
	LPBYTE	curstack;
	DWORD	ret,stacksize;

	dprintf_thunk(stddeb,"KERNEL32_45(%%eax=0x%08lx(%%cx=0x%04lx,%%edx=0x%08lx))\n",
		(DWORD)EAX_reg(context),(DWORD)CX_reg(context),(DWORD)EDX_reg(context)
	);
	stacksize = EBP_reg(context)-ESP_reg(context);
	dprintf_thunk(stddeb,"	stacksize = %ld\n",stacksize);

	memcpy(&context16,context,sizeof(context16));

	DI_reg(&context16)	 = CX_reg(context);
	CS_reg(&context16)	 = HIWORD(EAX_reg(context));
	IP_reg(&context16)	 = LOWORD(EAX_reg(context));

	curstack = PTR_SEG_TO_LIN(STACK16_PUSH(stacksize));
	memcpy(curstack-stacksize,(LPBYTE)ESP_reg(context),stacksize);
	ret = Callbacks->CallRegisterLongProc(&context16,0);
	STACK16_POP(stacksize);

	dprintf_thunk(stddeb,". returned %08lx\n",ret);
	EAX_reg(context) 	 = ret;
}

/***********************************************************************
 * 		_KERNEL32_40 	(KERNEL32.40)
 * YET Another 32->16 thunk, the difference to the others is still mysterious
 * target address is EDX
 *
 * [crashes]
 */
VOID WINAPI _KERNEL32_40(CONTEXT *context)
{
	CONTEXT	context16;
	LPBYTE	curstack;
	DWORD	ret,stacksize;

	dprintf_thunk(stddeb,"_KERNEL32_40(EDX=0x%08lx)\n",
		EDX_reg(context)
	);
	stacksize = EBP_reg(context)-ESP_reg(context);
	dprintf_thunk(stddeb,"	stacksize = %ld\n",stacksize);
	dprintf_thunk(stddeb,"on top of stack: 0x%04x\n",*(WORD*)ESP_reg(context));

	memcpy(&context16,context,sizeof(context16));

	CS_reg(&context16)	 = HIWORD(EDX_reg(context));
	IP_reg(&context16)	 = LOWORD(EDX_reg(context));

	curstack = PTR_SEG_TO_LIN(STACK16_PUSH(stacksize));
	memcpy(curstack-stacksize,(LPBYTE)ESP_reg(context),stacksize);
	ret = Callbacks->CallRegisterShortProc(&context16,0);
	STACK16_POP(stacksize);

	dprintf_thunk(stddeb,". returned %08lx\n",ret);
	EAX_reg(context) 	 = ret;
}


/***********************************************************************
 *		(KERNEL32.41)
 * A thunk setup routine.
 * Expects a pointer to a preinitialized thunkbuffer in the first argument
 * looking like:
 *	00..03:		unknown	(pointer, check _41, _43, _46)
 *	04: EB1E		jmp +0x20
 *
 *	06..23:		unknown (space for replacement code, check .90)
 *
 *	24:>E800000000		call offset 29
 *	29:>58			pop eax		   ( target of call )
 *	2A: 2D25000000		sub eax,0x00000025 ( now points to offset 4 )
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
 * [ok probably]
 */

LPVOID WINAPI _KERNEL32_41(LPBYTE thunk,LPCSTR thkbuf,DWORD len,LPCSTR dll16,
                           LPCSTR dll32)
{
	HMODULE32	hkrnl32 = GetModuleHandle32A("KERNEL32");
	HMODULE16	hmod;
	LPDWORD		addr,addr2;
	DWORD		segaddr;

	/* FIXME: add checks for valid code ... */
	/* write pointers to kernel32.89 and kernel32.90 (+ordinal base of 1) */
	*(DWORD*)(thunk+0x35) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)90);
	*(DWORD*)(thunk+0x6D) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)89);

	
	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		fprintf(stderr,"KERNEL32_41->failed to load 16bit DLL %s, error %d\n",dll16,hmod);
		return NULL;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		fprintf(stderr,"KERNEL32_41->no %s exported from %s!\n",thkbuf,dll16);
		return NULL;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"KERNEL32_41->thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return NULL;
	}
	addr2 = PTR_SEG_TO_LIN(addr[1]);
	if (HIWORD(addr2))
		*(DWORD*)thunk = (DWORD)addr2;

	dprintf_thunk(stddeb, "_KERNEL32_41: loaded module %d, func %s(%d) @ %p (%p), returning %p\n",
		      hmod, HIWORD(thkbuf)==0 ? "<ordinal>" : thkbuf, (int)thkbuf, (void*)segaddr, addr, addr2);

	return addr2;
}

/***********************************************************************
 *							(KERNEL32.90)
 * QT Thunk priming function
 * Rewrites the first part of the thunk to use the QT_Thunk interface
 * and jumps to the start of that code.
 * [ok]
 */
VOID WINAPI _KERNEL32_90(CONTEXT *context)
{
	dprintf_thunk(stddeb, "_KERNEL32_90: QT Thunk priming; context %p\n", context);
	
	_write_qtthunk((LPBYTE)EAX_reg(context),*(DWORD*)(EAX_reg(context)+EDX_reg(context)));
	/* we just call the real QT_Thunk right now 
	 * we can bypass the relaycode, for we already have the registercontext
	 */
	EDX_reg(context) = *(DWORD*)((*(DWORD*)(EAX_reg(context)+EDX_reg(context)))+4*(((BYTE*)EBP_reg(context))[-4]));
	return QT_Thunk(context);
}

/***********************************************************************
 *							(KERNEL32.45)
 * Another thunkbuf link routine.
 * The start of the thunkbuf looks like this:
 * 	00: DWORD	length
 *	04: SEGPTR	address for thunkbuffer pointer
 * [ok probably]
 */
VOID WINAPI _KERNEL32_46(LPBYTE thunk,LPSTR thkbuf,DWORD len,LPSTR dll16,
                         LPSTR dll32)
{
	LPDWORD		addr;
	HMODULE16	hmod;
	SEGPTR		segaddr;

	hmod = LoadLibrary16(dll16);
	if (hmod < 32) {
		fprintf(stderr,"KERNEL32_46->couldn't load %s, error %d\n",dll16,hmod);
		return;
	}
	segaddr = (SEGPTR)WIN32_GetProcAddress16(hmod,thkbuf);
	if (!segaddr) {
		fprintf(stderr,"KERNEL32_46-> haven't found %s in %s!\n",thkbuf,dll16);
		return;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"KERNEL32_46-> length of thkbuf differs from expected length! (%ld vs %ld)\n",addr[0],len);
		return;
	}
	*(DWORD*)PTR_SEG_TO_LIN(addr[1]) = (DWORD)thunk;

	dprintf_thunk(stddeb, "_KERNEL32_46: loaded module %d, func %s(%d) @ %p (%p)\n",
		      hmod, HIWORD(thkbuf)==0 ? "<ordinal>" : thkbuf, (int)thkbuf, (void*)segaddr, addr);
}

/**********************************************************************
 *           _KERNEL32_87
 * Check if thunking is initialized (ss selector set up etc.)
 * We do that differently, so just return TRUE.
 * [ok]
 */
BOOL32 WINAPI _KERNEL32_87()
{
    dprintf_thunk(stddeb, "_KERNEL32_87: Yes, thunking is initialized\n");
    return TRUE;
}

/**********************************************************************
 *           _KERNEL32_88
 * One of the real thunking functions. This one seems to be for 32<->32
 * thunks. It should probably be capable of crossing processboundaries.
 *
 * And YES, I've seen nr=48 (somewhere in the Win95 32<->16 OLE coupling)
 * [ok]
 */
DWORD WINAPIV _KERNEL32_88( DWORD nr, DWORD flags, FARPROC32 fun, ... )
{
    DWORD i,ret;
    DWORD *args = ((DWORD *)&fun) + 1;

    dprintf_thunk(stddeb,"KERNEL32_88(%ld,0x%08lx,%p,[ ",nr,flags,fun);
    for (i=0;i<nr/4;i++) { dprintf_thunk(stddeb,"0x%08lx,",args[i]); }
    dprintf_thunk(stddeb,"])\n");
    switch (nr) {
    case 0:	ret = fun();
		break;
    case 4:	ret = fun(args[0]);
		break;
    case 8:	ret = fun(args[0],args[1]);
		break;
    case 12:	ret = fun(args[0],args[1],args[2]);
		break;
    case 16:	ret = fun(args[0],args[1],args[2],args[3]);
		break;
    case 20:	ret = fun(args[0],args[1],args[2],args[3],args[4]);
		break;
    case 24:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5]);
		break;
    case 28:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
		break;
    case 32:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
		break;
    case 36:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
		break;
    case 40:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
		break;
    case 44:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
		break;
    case 48:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
		break;
    default:
	fprintf(stderr,"_KERNEL32_88: unsupported nr of arguments, %ld\n",nr);
	ret = 0;
	break;

    }
    dprintf_thunk(stddeb," returning %ld ...\n",ret);
    return ret;
}

/**********************************************************************
 * 		KERNEL_619		(KERNEL)
 * Seems to store y and z depending on x in some internal lists...
 */
WORD WINAPI _KERNEL_619(WORD x,DWORD y,DWORD z)
{
    fprintf(stderr,"KERNEL_619(0x%04x,0x%08lx,0x%08lx)\n",x,y,z);
    return x;
}

/**********************************************************************
 *			AllocSLCallback		(KERNEL32)
 *
 * Win95 uses some structchains for callbacks. It allocates them
 * in blocks of 100 entries, size 32 bytes each, layout:
 * blockstart:
 * 	0:	PTR	nextblockstart
 *	4:	entry	*first;
 *	8:	WORD	sel ( start points to blockstart)
 *	A:	WORD	unknown
 * 100xentry:
 *	00..17:		Code
 *	18:	PDB	*owning_process;
 *	1C:	PTR	blockstart
 *
 * We ignore this for now. (Just a note for further developers)
 * FIXME: use this method, so we don't waste selectors...
 *
 * Following code is then generated by AllocSLCallback. The code is 16 bit, so
 * the 0x66 prefix switches from word->long registers.
 *
 *	665A		pop	edx 
 *	6668x arg2 x 	pushl	<arg2>
 *	6652		push	edx
 *	EAx arg1 x	jmpf	<arg1>
 *
 * returns the startaddress of this thunk.
 *
 * Note, that they look very similair to the ones allocates by THUNK_Alloc.
 */
DWORD WINAPI
AllocSLCallback(DWORD finalizer,DWORD callback) {
	LPBYTE	x,thunk = HeapAlloc( GetProcessHeap(), 0, 32 );
	WORD	sel;

	x=thunk;
	*x++=0x66;*x++=0x5a;				/* popl edx */
	*x++=0x66;*x++=0x68;*(DWORD*)x=finalizer;x+=4;	/* pushl finalizer */
	*x++=0x66;*x++=0x52;				/* pushl edx */
	*x++=0xea;*(DWORD*)x=callback;x+=4;		/* jmpf callback */

	*(DWORD*)(thunk+18) = GetCurrentProcessId();

	sel = SELECTOR_AllocBlock( thunk , 32, SEGMENT_CODE, FALSE, FALSE );
	return (sel<<16)|0;
}

void WINAPI
FreeSLCallback(DWORD x) {
	fprintf(stderr,"FreeSLCallback(0x%08lx)\n",x);
}

/**********************************************************************
 * 		KERNEL_358		(KERNEL)
 * Allocates a code segment which starts at the address passed in x. limit
 * 0xfffff, and returns the pointer to the start.
 */
DWORD WINAPI
_KERNEL_358(DWORD x) {
	WORD	sel;

	fprintf(stderr,"_KERNEL_358(0x%08lx),stub\n",x);
	if (!HIWORD(x))
		return x;

	sel = SELECTOR_AllocBlock( PTR_SEG_TO_LIN(x) , 0xffff, SEGMENT_CODE, FALSE, FALSE );
	return (sel<<16)|(0x0000);
}

/**********************************************************************
 * 		KERNEL_359		(KERNEL)
 * Frees the code segment of the passed linear pointer (This has usually
 * been allocated by _KERNEL_358).
 */
VOID WINAPI
_KERNEL_359(DWORD x) {
	DWORD	savedsssp;

	fprintf(stderr,"_KERNEL_359(0x%08lx),stub\n",x);
	if ((HIWORD(x) & 7)!=7)
		return;
	savedsssp = IF1632_Saved16_ss_sp;IF1632_Saved16_ss_sp = 0;
	SELECTOR_FreeBlock(x>>16,1);
	IF1632_Saved16_ss_sp = savedsssp;
	return;
}

/**********************************************************************
 * 		KERNEL_472		(KERNEL)
 * something like GetCurrenthInstance.
 */
VOID WINAPI
_KERNEL_472(CONTEXT *context) {
	fprintf(stderr,"_KERNEL_472(0x%08lx),stub\n",EAX_reg(context));
	if (!EAX_reg(context)) {
		TDB *pTask = (TDB*)GlobalLock16(GetCurrentTask());
		AX_reg(context)=pTask->hInstance;
		return;
	}
	if (!HIWORD(EAX_reg(context)))
		return; /* returns the passed value */
	/* hmm ... fixme */
}

/**********************************************************************
 * 		KERNEL_431		(KERNEL.431)
 * IsPeFile
 */
BOOL16 WINAPI KERNEL_431(LPSTR fn,WORD x) {
	IMAGE_DOS_HEADER	mzh;
	HFILE32			hf;
	OFSTRUCT		ofs;
	DWORD			xmagic;

	hf = OpenFile32(fn,&ofs,OF_READ);
	if (hf==HFILE_ERROR32)
		return FALSE;
	if (sizeof(mzh)!=_lread32(hf,&mzh,sizeof(mzh))) {
		_lclose32(hf);
		return FALSE;
	}
	if (mzh.e_magic!=IMAGE_DOS_SIGNATURE) {
		fprintf(stderr,"file has not got dos signature!\n");
		_lclose32(hf);
		return FALSE;
	}
	_llseek32(hf,mzh.e_lfanew,SEEK_SET);
	if (sizeof(DWORD)!=_lread32(hf,&xmagic,sizeof(DWORD))) {
		_lclose32(hf);
		return FALSE;
	}
	_lclose32(hf);
	return (xmagic == IMAGE_NT_SIGNATURE);
}
