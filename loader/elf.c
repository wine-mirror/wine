/*
 *	UNIX dynamic loader
 *
 * Currently only supports stuff using the dl* API.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME: 	Small reentrancy problem.
 * IDEA(s):	could be used to split up shell32,comctl32...
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "snoop.h"
#include "file.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);

typedef struct {
	WORD	popl	WINE_PACKED;	/* 0x8f 0x05 */
	DWORD	addr_popped WINE_PACKED;/* ...  */
	BYTE	pushl1	WINE_PACKED;	/* 0x68 */
	DWORD	newret WINE_PACKED;	/* ...  */
	BYTE	pushl2 	WINE_PACKED;	/* 0x68 */
	DWORD	origfun WINE_PACKED;	/* original function */
	BYTE	ret1	WINE_PACKED;	/* 0xc3 */
	WORD	addesp 	WINE_PACKED;	/* 0x83 0xc4 */
	BYTE	nrofargs WINE_PACKED;	/* nr of arguments to add esp, */
	BYTE	pushl3	WINE_PACKED;	/* 0x68 */
	DWORD	oldret	WINE_PACKED;	/* Filled out from popl above  */
	BYTE	ret2	WINE_PACKED;	/* 0xc3 */
} ELF_STDCALL_STUB;

#define UNIX_DLL_ENDING		"so"

#define	STUBSIZE		4095
#define STUBOFFSET  (sizeof(IMAGE_DOS_HEADER) + \
                     sizeof(IMAGE_NT_HEADERS) + \
                     sizeof(IMAGE_SECTION_HEADER))

static FARPROC ELF_FindExportedFunction( WINE_MODREF *wm, LPCSTR funcName, int hint, BOOL snoop );

static HMODULE ELF_CreateDummyModule( LPCSTR libname, LPCSTR modname )
{
	PIMAGE_DOS_HEADER	dh;
	PIMAGE_NT_HEADERS	nth;
	PIMAGE_SECTION_HEADER	sh;
	HMODULE hmod;

	hmod = (HMODULE)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     sizeof(IMAGE_DOS_HEADER) +
                                     sizeof(IMAGE_NT_HEADERS) +
                                     sizeof(IMAGE_SECTION_HEADER) + STUBSIZE );
	dh = (PIMAGE_DOS_HEADER)hmod;
	dh->e_magic = IMAGE_DOS_SIGNATURE;
	dh->e_lfanew = sizeof(IMAGE_DOS_HEADER);
	nth = (IMAGE_NT_HEADERS *)(dh + 1);
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
	sh->Misc.VirtualSize	= STUBSIZE;
	sh->VirtualAddress	= STUBOFFSET; /* so snoop can use it ... */
	sh->SizeOfRawData	= STUBSIZE;
	sh->PointerToRawData	= 0;
	sh->Characteristics	= IMAGE_SCN_CNT_CODE|IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ;
        return hmod;
}

WINE_MODREF *ELF_LoadLibraryExA( LPCSTR libname, DWORD flags)
{
	WINE_MODREF	*wm;
        HMODULE		hmod;
	char		*modname,*s,*t,*x;
	LPVOID		*dlhandle;
    char      error[1024];

	t = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       strlen(libname) + strlen("lib.so") + 1 );
	*t = '\0';
	/* copy path to tempvar ... */
	s=strrchr(libname,'/');
	if (!s)
		s=strrchr(libname,'\\');
	if (s) {
		s++; /* skip / or \ */
		/* copy everything up to s-1 */
		memcpy(t,libname,s-libname);
		t[s-libname]= '\0';
	} else
		s = (LPSTR)libname;
	modname = s;
	/* append "lib" foo ".so" */
	strcat(t,"lib");
	x = t+strlen(t);
	strcat(t,s);
	s = strchr(x,'.');
	if (s) {
            while (s) {
		if (!FILE_strcasecmp(s,".dll")) {
                    strcpy(s+1,UNIX_DLL_ENDING);
                    break;
		}
		s=strchr(s+1,'.');
            }
	} else {
	    	strcat(x,"."UNIX_DLL_ENDING);
	}

        /* grab just the last piece of the path/filename
           which should be the name of the library we are
           looking to load. increment by 1 to skip the DOS slash */
        s = strrchr(t,'\\');
        s++;

       /* ... and open the library pointed by s, while t points
         points to the ENTIRE DOS filename of the library
         t is returned by HeapAlloc() above and so is also used
         with HeapFree() below */
	dlhandle = wine_dlopen(s,RTLD_NOW,error,sizeof(error));
	if (!dlhandle) {
                WARN("failed to load %s: %s\n", s, error);
		HeapFree( GetProcessHeap(), 0, t );
		SetLastError( ERROR_FILE_NOT_FOUND );
		return NULL;
	}

	hmod = ELF_CreateDummyModule( t, modname );

	SNOOP_RegisterDLL(hmod,libname,0,STUBSIZE/sizeof(ELF_STDCALL_STUB));

        wm = PE_CreateModule( hmod, libname, 0, 0, FALSE );
        wm->find_export = ELF_FindExportedFunction;
	wm->dlhandle = dlhandle;
	return wm;
}

static FARPROC ELF_FindExportedFunction( WINE_MODREF *wm, LPCSTR funcName, int hint, BOOL snoop )
{
	LPVOID			fun;
	int			i,nrofargs = 0;
	ELF_STDCALL_STUB	*stub, *first_stub;
	char			error[256];

	if (!HIWORD(funcName)) {
		ERR("Can't import from UNIX dynamic libs by ordinal, sorry.\n");
		return (FARPROC)0;
	}
	fun = wine_dlsym(wm->dlhandle,funcName,error,sizeof(error));
        if (!fun)
        {
            /* we sometimes have an excess '_' at the beginning of the name */
            if (funcName[0]=='_')
            {
		funcName++ ;
		fun = wine_dlsym(wm->dlhandle,funcName,error,sizeof(error));
            }
        }
	if (!fun) {
		/* Function@nrofargs usually marks a stdcall function
		 * with nrofargs bytes that are popped at the end
		 */
            LPCSTR t;
            if ((t = strchr(funcName,'@')))
            {
                LPSTR fn = HeapAlloc( GetProcessHeap(), 0, t - funcName + 1 );
                memcpy( fn, funcName, t - funcName );
                fn[t - funcName] = 0;
                nrofargs = 0;
                sscanf(t+1,"%d",&nrofargs);
                fun = wine_dlsym(wm->dlhandle,fn,error,sizeof(error));
                HeapFree( GetProcessHeap(), 0, fn );
            }
	}
	/* We sometimes have Win32 dlls implemented using stdcall but UNIX
	 * dlls using cdecl. If we find out the number of args the function
	 * uses, we remove them from the stack using two small stubs.
	 */
        stub = first_stub = (ELF_STDCALL_STUB *)((char *)wm->module + STUBOFFSET);
	for (i=0;i<STUBSIZE/sizeof(ELF_STDCALL_STUB);i++) {
		if (!stub->origfun)
			break;
		if (stub->origfun == (DWORD)fun)
			break;
		stub++;
	}
	if (i==STUBSIZE/sizeof(ELF_STDCALL_STUB)) {
		ERR("please report, that there are not enough slots for stdcall stubs in the ELF loader.\n");
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
		fun=(FARPROC)stub;
	}
	if (!fun) {
		FIXME("function %s not found: %s\n",funcName,error);
		return fun;
	}
	fun = SNOOP_GetProcAddress(wm->module,funcName,stub-first_stub,fun);
	return (FARPROC)fun;
}
