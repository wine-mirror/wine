/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "winbase.h"
#include "stddebug.h"
#include "debug.h"

#ifndef PROT_NONE  /* FreeBSD doesn't define PROT_NONE */
#define PROT_NONE 0
#endif
#ifndef MAP_ANON
#define MAP_ANON 0
#endif

typedef struct {
    caddr_t	ptr;
    long	size;
} virtual_mem_t;

virtual_mem_t *mem = 0;
int mem_count = 0;
int mem_used = 0;

/***********************************************************************
 *           VirtualAlloc             (KERNEL32.548)
 */
int TranslateProtectionFlags(DWORD);
LPVOID VirtualAlloc(LPVOID lpvAddress, DWORD cbSize,
                   DWORD fdwAllocationType, DWORD fdwProtect)
{
    caddr_t	ptr;
    int	i;
    virtual_mem_t *tmp_mem;
    int	prot;

    dprintf_win32(stddeb, "VirtualAlloc: size = %ld, address=%p\n", cbSize, lpvAddress);
    if (fdwAllocationType & MEM_RESERVE || !lpvAddress) {
        ptr = mmap((void *)((((unsigned long)lpvAddress-1) & 0xFFFF0000L) 
	                + 0x00010000L),
		   cbSize, PROT_NONE, MAP_ANON|MAP_PRIVATE,0,0);
        if (lpvAddress && ((unsigned long)ptr & 0xFFFF0000L)) {
	    munmap(ptr, cbSize);
	    cbSize += 65535;
	    ptr =  mmap(lpvAddress, cbSize, 
	                PROT_NONE, MAP_ANON|MAP_PRIVATE,0,0);
	    ptr = (void *)((((unsigned long)ptr-1) & 0xFFFF0000L)+0x00010000L);
	}
	/* remember the size for VirtualFree since it's going to be handed
	   a zero len */
	if (ptr) {
	    if (mem_count == mem_used) {
	        tmp_mem = realloc(mem,(mem_count+10)*sizeof(virtual_mem_t));
		if (!tmp_mem) return 0;
		mem = tmp_mem;
		memset(mem+mem_count, 0, 10*sizeof(virtual_mem_t));
		mem_count += 10;
	    }
	    for (i=0; i<mem_count; i++) {
	        if (!(mem+i)->ptr) {
		    (mem+i)->ptr = ptr;
		    (mem+i)->size = cbSize;
		    mem_used++;
		    break;
		}
	    }
	}
    } else {
        ptr = lpvAddress;
    }
    if (fdwAllocationType & MEM_COMMIT) {
        prot = TranslateProtectionFlags(fdwProtect & 
                                          ~(PAGE_GUARD | PAGE_NOCACHE));
	mprotect(ptr, cbSize, prot);
    }
    return ptr;
#if 0
/* kludge for gnu-win32 */
    if (fdwAllocationType & MEM_RESERVE) return sbrk(0);
    ptr = malloc(cbSize + 65536);
    if(ptr)
    {
        /* Round it up to the next 64K boundary and zero it.
         */
        ptr = (void *)(((unsigned long)ptr & 0xFFFF0000L) + 0x00010000L);
        memset(ptr, 0, cbSize);
    }
#endif
    dprintf_win32(stddeb, "VirtualAlloc: got pointer %p\n", ptr);
    return ptr;
}

/***********************************************************************
 *           VirtualFree               (KERNEL32.550)
 */
BOOL VirtualFree(LPVOID lpvAddress, DWORD cbSize, DWORD fdwFreeType)
{
    int i;

    if (fdwFreeType & MEM_RELEASE) {
        for (i=0; i<mem_count; i++) {
            if ((mem+i)->ptr == lpvAddress) {
    	         munmap(lpvAddress, (mem+i)->size);
    	         (mem+i)->ptr = 0;
    	         mem_used--;
    	         break;
	    }
    	}
    } else {
        mprotect(lpvAddress, cbSize, PROT_NONE);
    }
#if 0
    if(lpvAddress)
        free(lpvAddress);
#endif
    return 1;
}

int TranslateProtectionFlags(DWORD protection_flags)
{
    int prot;

        switch(protection_flags) {
	    case PAGE_READONLY:
	        prot=PROT_READ;
		break;
	    case PAGE_READWRITE:
	        prot=PROT_READ|PROT_WRITE;
		break;
	    case PAGE_WRITECOPY:
	        prot=PROT_WRITE;
		break;
	    case PAGE_EXECUTE:
	        prot=PROT_EXEC;
		break;
	    case PAGE_EXECUTE_READ:
	        prot=PROT_EXEC|PROT_READ;
		break;
	    case PAGE_EXECUTE_READWRITE:
	        prot=PROT_EXEC|PROT_READ|PROT_WRITE;
		break;
	    case PAGE_EXECUTE_WRITECOPY:
	        prot=PROT_EXEC|PROT_WRITE;
		break;
	    case PAGE_NOACCESS:
	    default:
	        prot=PROT_NONE;
		break;
	}
   return prot;
}
