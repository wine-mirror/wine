/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "winbase.h"
#include "handle32.h"
#include "stddebug.h"
#include "debug.h"

#ifndef PROT_NONE  /* FreeBSD doesn't define PROT_NONE */
#define PROT_NONE 0
#endif

typedef struct {
    caddr_t	ptr;
    long	size;
} virtual_mem_t;

virtual_mem_t *mem = 0;
int mem_count = 0;
int mem_used = 0;

/*******************************************************************
 *                   VRANGE
 * A VRANGE denotes a contiguous part of the address space. It is used
 * for house keeping, and will be obtained by higher-level memory allocation
 * functions (VirtualAlloc, MapViewOfFile)
 * There can be at most one VRANGE object covering any address at any time.
 * Currently, all VRANGE objects are stored in a sorted list. Wine does not
 * attempt to give a complete list of in-use address ranges, only those
 * allocated via Win32.
 * An exception is IsVrangeFree, which should test the OS specific 
 * mappings, too. As a default, an range not known to be allocated is 
 * considered free.
 *******************************************************************/

VRANGE_OBJECT *MEMORY_ranges=0;

VRANGE_OBJECT *MEMORY_FindVrange(DWORD start)
{
	VRANGE_OBJECT *range;
	for(range=MEMORY_ranges;range && range->start<start;range=range->next)
	{
		if(range->start<start && start<range->start+range->size)
			return range;
	}
	return 0;
}

static int MEMORY_IsVrangeFree(DWORD start,DWORD size)
{
	DWORD end;
	VRANGE_OBJECT *range;
	if(!size)
		return 1;
	/* First, check our lists*/
	end=start+size;
	for(range=MEMORY_ranges;range && range->start<start;range=range->next)
	{
		if((range->start<start && start<range->start+range->size) ||
			(range->start<end && end<range->start+range->size))
		return 0;
	}
	/* Now, check the maps that are not under our control */
#ifdef linux
	{
	FILE *f=fopen("/proc/self/maps","r");
	char line[80];
	int found=0;
	while(1)
	{
		char *it;
		int lower,upper;
		if(!fgets(line,sizeof(line),f))
			break;
		it=line;
		lower=strtoul(it,&it,16);
		if(*it++!='-')
			fprintf(stderr,"Format of /proc/self/maps changed\n");
		upper=strtoul(it,&it,16);
		if((lower<start && start<upper) || (lower<start+size && start+size<upper))
		{
			found=1;
			break;
		}
	}
	fclose(f);
	return !found;
	}
#else
	{
	static int warned=0;
	if(!warned)
	{
		fprintf(stdnimp, "Don't know how to perform MEMORY_IsVrangeFree on "
			"this system.\n Please fix\n");
		warned=0;
	}
	return 1;
	}
#endif
}

/* FIXME: might need to consolidate ranges */
void MEMORY_InsertVrange(VRANGE_OBJECT *r)
{
	VRANGE_OBJECT *it,*last;
	if(!MEMORY_ranges || r->start<MEMORY_ranges->start)
	{
		r->next=MEMORY_ranges;
		MEMORY_ranges=r;
	}
	for(it=MEMORY_ranges,last=0;it && it->start<r->start;it=it->next)
		last=it;
	r->next=last->next;
	last->next=r;
}
	

VRANGE_OBJECT *MEMORY_AllocVrange(int start,int size)
{
	VRANGE_OBJECT *ret=CreateKernelObject(sizeof(VRANGE_OBJECT));
	ret->common.magic=KERNEL_OBJECT_VRANGE;
	MEMORY_InsertVrange(ret);
	return ret;
}

void MEMORY_ReleaseVrange(VRANGE_OBJECT *r)
{
	VRANGE_OBJECT *it;
	if(MEMORY_ranges==r)
	{
		MEMORY_ranges=r->next;
		ReleaseKernelObject(r);
		return;
	}
	for(it=MEMORY_ranges;it;it=it->next)
		if(it->next==r)break;
	if(!it)
	{
		fprintf(stderr,"VRANGE not found\n");
		return;
	}
	it->next=r->next;
	ReleaseKernelObject(r);
}

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
    static int fdzero = -1;

    if (fdzero == -1)
    {
        if ((fdzero = open( "/dev/zero", O_RDONLY )) == -1)
        {
            perror( "/dev/zero: open" );
            return (LPVOID)NULL;
        }
    }

    dprintf_win32(stddeb, "VirtualAlloc: size = %ld, address=%p\n", cbSize, lpvAddress);
    if (fdwAllocationType & MEM_RESERVE || !lpvAddress) {
        ptr = mmap((void *)((((unsigned long)lpvAddress-1) & 0xFFFF0000L) 
	                + 0x00010000L),
		   cbSize, PROT_NONE, MAP_PRIVATE, fdzero, 0 );
	if (ptr == (caddr_t) -1) {
	    dprintf_win32(stddeb, "VirtualAlloc: returning NULL");
	    return (LPVOID) NULL;
	}
        if (lpvAddress && ((unsigned long)ptr & 0xFFFF0000L)) {
	    munmap(ptr, cbSize);
	    cbSize += 65535;
	    ptr =  mmap(lpvAddress, cbSize, 
	                PROT_NONE, MAP_PRIVATE, fdzero, 0 );
	    if (ptr == (caddr_t) -1) {
		dprintf_win32(stddeb, "VirtualAlloc: returning NULL");
		return (LPVOID) NULL;
	    }
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
BOOL32 VirtualFree(LPVOID lpvAddress, DWORD cbSize, DWORD fdwFreeType)
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


/******************************************************************
 *                   IsBadReadPtr
 */
BOOL WIN32_IsBadReadPtr(void* ptr, unsigned int bytes)
{
	dprintf_global(stddeb,"IsBadReadPtr(%x,%x)\n",ptr,bytes);
	/* FIXME: Should make check based on actual mappings, here */
	return FALSE;
}

/******************************************************************
 *                   IsBadWritePtr
 */
BOOL WIN32_IsBadWritePtr(void* ptr, unsigned int bytes)
{
	dprintf_global(stddeb,"IsBadWritePtr(%x,%x)\n",ptr,bytes);
	/* FIXME: Should make check based on actual mappings, here */
	return FALSE;
}
/******************************************************************
 *                   IsBadWritePtr
 */
BOOL WIN32_IsBadCodePtr(void* ptr, unsigned int bytes)
{
	dprintf_global(stddeb,"IsBadCodePtr(%x,%x)\n",ptr,bytes);
	/* FIXME: Should make check based on actual mappings, here */
	return FALSE;
}
