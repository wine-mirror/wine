/* 
 *	UNIX dynamic loader
 * 
 * Currently only supports stuff using the dl* API.
 *
 * Copyright 1998 Marcus Meissner
 *
 * FIXME: 	Small reentrancy problem.
 * IDEA(s):	could be used to split up shell32,comctl32... 
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "snoop.h"
#include "process.h"
#include "neexe.h"
#include "peexe.h"
#include "heap.h"
#include "pe_image.h"
#include "module.h"
#include "debug.h"

WINE_MODREF *
ELF_CreateDummyModule( LPCSTR libname, LPCSTR modname, PDB32 *process )
{
	PIMAGE_DOS_HEADER	dh;
	PIMAGE_NT_HEADERS	nth;
	PIMAGE_SECTION_HEADER	sh;
	WINE_MODREF *wm;
	HMODULE32 hmod;

	wm=(WINE_MODREF*)HeapAlloc(process->heap,HEAP_ZERO_MEMORY,sizeof(*wm));
	wm->type = MODULE32_ELF;

	/* FIXME: hmm, order? */
	wm->next = process->modref_list;
	process->modref_list = wm;

	wm->modname = HEAP_strdupA(process->heap,0,modname);
	wm->longname = HEAP_strdupA(process->heap,0,libname);

	hmod = (HMODULE32)HeapAlloc(process->heap,HEAP_ZERO_MEMORY,sizeof(IMAGE_DOS_HEADER)+sizeof(IMAGE_NT_HEADERS)+sizeof(IMAGE_SECTION_HEADER)+100);
	dh = (PIMAGE_DOS_HEADER)hmod;
	dh->e_magic = IMAGE_DOS_SIGNATURE;
	dh->e_lfanew = sizeof(IMAGE_DOS_HEADER);
	nth = PE_HEADER(hmod);
	nth->Signature = IMAGE_NT_SIGNATURE; 
	nth->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
	nth->FileHeader.NumberOfSections = 1;
	nth->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
	nth->FileHeader.Characteristics = 
		IMAGE_FILE_RELOCS_STRIPPED|IMAGE_FILE_LINE_NUMS_STRIPPED|
		IMAGE_FILE_LOCAL_SYMS_STRIPPED|IMAGE_FILE_32BIT_MACHINE|
		IMAGE_FILE_DLL|IMAGE_FILE_DEBUG_STRIPPED;
	nth->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
	nth->OptionalHeader.SizeOfCode = 0;
	nth->OptionalHeader.SizeOfInitializedData = 0;
	nth->OptionalHeader.SizeOfUninitializedData = 0;
	nth->OptionalHeader.AddressOfEntryPoint	= 0;
	nth->OptionalHeader.BaseOfCode		= 0;
	nth->OptionalHeader.MajorOperatingSystemVersion = 4;
	nth->OptionalHeader.MajorImageVersion	= 4;
	nth->OptionalHeader.SizeOfImage		= 0;
	nth->OptionalHeader.SizeOfHeaders	= 0;
	nth->OptionalHeader.Subsystem		= IMAGE_SUBSYSTEM_NATIVE;
	nth->OptionalHeader.DllCharacteristics	= 0;
	nth->OptionalHeader.NumberOfRvaAndSizes	= 0;

	/* allocate one code section that crosses the whole process range
	 * (we could find out from internal tables ... hmm )
	 */
	sh=(PIMAGE_SECTION_HEADER)(nth+1);
	strcpy(sh->Name,".text");
	sh->Misc.VirtualSize	= 0x7fffffff;
	sh->VirtualAddress	= 0x42; /* so snoop can use it ... */
	sh->SizeOfRawData	= 0x7fffffff;
	sh->PointerToRawData	= 0;
	sh->Characteristics	= IMAGE_SCN_CNT_CODE|IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ;
	wm->module = hmod;
	return wm;
}


#if defined(HAVE_LIBDL) && defined(HAVE_DLFCN_H)

#define UNIX_DLL_ENDING		"so"

#define	STUBSIZE		4095

#include <dlfcn.h>

HMODULE32
ELF_LoadLibraryEx32A(LPCSTR libname,PDB32 *process,HANDLE32 hf,DWORD flags) {
	WINE_MODREF	*wm;
	char		*modname,*s,*t,*x;
	LPVOID		*dlhandle;

	t = HeapAlloc(process->heap,HEAP_ZERO_MEMORY,strlen(libname)+strlen("lib.so")+1);
	*t = '\0';
	/* copy path to tempvar ... */
	s=strrchr(libname,'/');
	if (!s)
		s=strrchr(libname,'\\');
	if (s) {
		strncpy(t,libname,s-libname+1);
		t[s-libname+1]= '\0';
	} else
		s = (LPSTR)libname;
	modname = s;
	/* append "lib" foo ".so" */
	strcat(t,"lib");
	x = t+strlen(t);
	strcat(t,s);
	s = strchr(x,'.');
	while (s) {
		if (!strcasecmp(s,".dll")) {
			strcpy(s+1,UNIX_DLL_ENDING);
			break;
		}
		s=strchr(s+1,'.');
	}

	/* FIXME: make UNIX filename from DOS fn? */

	/* ... and open it */
	dlhandle = dlopen(t,RTLD_NOW);
	if (!dlhandle) {
		HeapFree(process->heap,0,t);
		return 0;
	}

	wm = ELF_CreateDummyModule( t, modname, process );
	wm->binfmt.elf.dlhandle = dlhandle;

	SNOOP_RegisterDLL(wm->module,libname,STUBSIZE/sizeof(ELF_STDCALL_STUB));
	return wm->module;
}

FARPROC32
ELF_FindExportedFunction( PDB32 *process,WINE_MODREF *wm, LPCSTR funcName) {
	LPVOID			fun;
	int			i,nrofargs = 0;
	ELF_STDCALL_STUB	*stub;

	assert(wm->type == MODULE32_ELF);
	if (!HIWORD(funcName)) {
		ERR(win32,"Can't import from UNIX dynamic libs by ordinal, sorry.\n");
		return (FARPROC32)0;
	}
	fun = dlsym(wm->binfmt.elf.dlhandle,funcName);
	/* we sometimes have an excess '_' at the beginning of the name */
	if (!fun && (funcName[0]=='_')) {
		funcName++ ;
		fun = dlsym(wm->binfmt.elf.dlhandle,funcName);
	}
	if (!fun) {
		/* Function@nrofargs usually marks a stdcall function 
		 * with nrofargs bytes that are popped at the end
		 */
		if (strchr(funcName,'@')) {
			LPSTR	t,fn = HEAP_strdupA(process->heap,0,funcName);

			t = strchr(fn,'@');
			*t = '\0';
			nrofargs = 0;
			sscanf(t+1,"%d",&nrofargs);
			fun = dlsym(wm->binfmt.elf.dlhandle,fn);
			HeapFree(process->heap,0,fn);
		}
	}
	/* We sometimes have Win32 dlls implemented using stdcall but UNIX 
	 * dlls using cdecl. If we find out the number of args the function
	 * uses, we remove them from the stack using two small stubs.
	 */
	if (!wm->binfmt.elf.stubs) {
		/* one page should suffice */
		wm->binfmt.elf.stubs = VirtualAlloc(NULL,STUBSIZE,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		memset(wm->binfmt.elf.stubs,0,STUBSIZE);
	}
	stub = wm->binfmt.elf.stubs;
	for (i=0;i<STUBSIZE/sizeof(ELF_STDCALL_STUB);i++) {
		if (!stub->origfun)
			break;
		if (stub->origfun == (DWORD)fun)
			break;
		stub++;
	}
	if (i==STUBSIZE/sizeof(ELF_STDCALL_STUB)) {
		ERR(win32,"please report, that there are not enough slots for stdcall stubs in the ELF loader.\n");
		assert(i<STUBSIZE/sizeof(ELF_STDCALL_STUB));
	}
	if (!stub->origfun)
		stub->origfun=(DWORD)fun; /* just a marker */

	if (fun && nrofargs) { /* we don't need it for 0 args */
		/* Selfmodifying entry/return stub for stdcall -> cdecl 
		 * conversion.
		 *  - Pop returnaddress directly into our return code
		 * 		popl <into code below>
		 *  - Replace it by pointer to start of our returncode
		 * 		push $newret
		 *  - And call the original function
		 * 		jmp $orgfun
		 *  - Remove the arguments no longer needed
		 * newret: 	add esp, <nrofargs>
		 *  - Push the original returnvalue on the stack
		 *		pushl <poppedvalue>
		 *  - And return to it.
		 *		ret
		 */

		/* FIXME: The function stub is not reentrant. */

		((LPBYTE)&(stub->popl))[0]	  = 0x8f;
		((LPBYTE)&(stub->popl))[1]	  = 0x05;
		stub->addr_popped = (DWORD)&(stub->oldret);
		stub->pushl1	  = 0x68;
		stub->newret	  = (DWORD)&(stub->addesp);
		stub->pushl2	  = 0x68;
		stub->origfun	  = (DWORD)fun;
		stub->ret1	  = 0xc3;
		((LPBYTE)&(stub->addesp))[0]=0x83;
		((LPBYTE)&(stub->addesp))[1]=0xc4;
		stub->nrofargs	  = nrofargs;
		stub->pushl3	  = 0x68;
			/* filled out by entrycode */
		stub->oldret	  = 0xdeadbeef;
		stub->ret2	  = 0xc3;
		fun=(FARPROC32)stub;
	}
	if (!fun) {
		FIXME(win32,"function %s not found: %s\n",funcName,dlerror());
		return fun;
	}
	fun = SNOOP_GetProcAddress32(wm->module,funcName,stub-wm->binfmt.elf.stubs,fun);
	return (FARPROC32)fun;
}
#else

HMODULE32
ELF_LoadLibraryEx32A(LPCSTR libname,PDB32 *process,HANDLE32 hf,DWORD flags) {
	return 0;
}
FARPROC32
ELF_FindExportedFunction( PDB32 *process,WINE_MODREF *wm, LPCSTR funcName) {
	return (FARPROC32)0;
}

#endif
