/*
 * Win32 kernel functions
 *
 * Copyright 1995 Thomas Sandford <t.d.g.sandford@prds-grn.demon.co.uk>
 * Copyright 1995 Martin von Löwis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "handle32.h"
#include "winbase.h"
#include "stddebug.h"
#include "debug.h"

#define HEAP_ZERO_MEMORY	0x8

/* FIXME: these functions do *not* implement the win32 API properly. They
are here merely as "get you going" aids */
/***********************************************************************
 *           HeapAlloc			(KERNEL32.222)
 *
 */
LPVOID SIMPLE_HeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes)

{
	void *result;

	result = malloc(dwBytes);
	if(result && (dwFlags & HEAP_ZERO_MEMORY))
		memset(result, 0, dwBytes);
	return result;
}

HANDLE32 HeapCreate(DWORD flOptions, DWORD dwInitialSize, DWORD dwMaximumSize)
{
	LPVOID start;
	HEAP_OBJECT *ret;
	HEAPITEM_OBJECT *item;
	if(dwInitialSize<4096)
		return 0;
	start = VirtualAlloc(0,dwMaximumSize, MEM_RESERVE, PAGE_READWRITE);
	if(!start)
		return 0;
	if(!VirtualAlloc(start,dwInitialSize, MEM_COMMIT, PAGE_READWRITE))
	{
		VirtualFree(start,dwMaximumSize,MEM_RELEASE);
		return 0;
	}
	ret=CreateKernelObject(sizeof(HEAP_OBJECT));
	ret->common.magic=KERNEL_OBJECT_HEAP;
	ret->start=start;
	ret->size=dwInitialSize;
	ret->maximum=dwMaximumSize;
	ret->flags=flOptions;
	item=start;
	item->common.magic=KERNEL_OBJECT_HEAPITEM;
	item->next=item->prev=0;
	item->heap=ret;

	ret->first=ret->last=item;

	return (HANDLE32)ret;
}

BOOL HeapDestroy(HEAP_OBJECT *h)
{
	/* FIXME: last error */
	if(h->common.magic!=KERNEL_OBJECT_HEAP)
		return 0;
	h->common.magic=0;
	VirtualFree(h->start,h->size,MEM_RELEASE);
	ReleaseKernelObject(h);
	return 1;
}


static BOOL HEAP_GrowHeap(HEAP_OBJECT *h,DWORD size)
{
	HEAPITEM_OBJECT *last;
	/* FIXME: last error */
	if(size > h->maximum-h->size)
		return 0;
	if(!VirtualAlloc(h->start+h->size,size,MEM_COMMIT,PAGE_READWRITE))
		return 0;
	/* FIXME: consolidate with previous item */
	last=h->start+size;
	h->size+=size;
	h->last->next=last;
	last->prev=h->last;
	last->next=0;
	h->last=last;
	last->common.magic=KERNEL_OBJECT_HEAPITEM;
	last->heap=h;
	return 1;
}


LPVOID HeapAlloc(HEAP_OBJECT *h,DWORD dwFlags,DWORD dwBytes)
{
	HEAPITEM_OBJECT *it,*next,*prev;
	/* FIXME: last error */
	if(!h)
		return SIMPLE_HeapAlloc(h,dwFlags,dwBytes);
	if(h->common.magic!=KERNEL_OBJECT_HEAP)
		return 0;
	/* align to DWORD */
	dwBytes = (dwBytes + 3) & ~3;
	for(it=h->first;it;it=it->next)
	{
		if(it->size>dwBytes+sizeof(HEAPITEM_OBJECT))
			break;
	}
	if(!it)
	{
		if(!HEAP_GrowHeap(h,dwBytes))
			return 0;
		it=h->last;
	}
	next = it->next;
	prev = it->prev;
	if(it->size > dwBytes+2*sizeof(HEAPITEM_OBJECT))
	{	/* split item */
		HEAPITEM_OBJECT *next=(HEAPITEM_OBJECT*)((char*)(it+1)+dwBytes);
		next->common.magic=KERNEL_OBJECT_HEAPITEM;
		next->size=it->size-sizeof(HEAPITEM_OBJECT)-dwBytes;
		next->next = it->next;
	}
	if(next)
		next->prev=prev;
	else
		h->last=prev;
	if(prev)
		prev->next=next;
	else
		h->first=next;
	/* Allocated item is denoted by self-referencing */
	it->next=it->prev=it;
	return (LPVOID)(it+1);
}

BOOL HeapFree(HEAP_OBJECT *h,DWORD dwFlags,LPVOID lpMem)
{
	HEAPITEM_OBJECT *item,*prev,*next;
	item = (HEAPITEM_OBJECT*)lpMem - 1;
	if(item->common.magic != KERNEL_OBJECT_HEAP)
		return 0;
	for(next=item;(caddr_t)next<(caddr_t)h->start+h->size;
		next=(HEAPITEM_OBJECT*)((char*)next+next->size))
		if(next!=next->next)
			break;
	if((caddr_t)next>=(caddr_t)h->start+h->size)
		next=0;
	if(next)
		prev=next->prev;
	else
		prev=h->last;
	/* consolidate with previous */
	if(prev && (char*)prev+prev->size==(char*)item)
	{
		prev->size+=item->size;
		item=prev;
		prev=item->prev;
	}
	/* consolidate with next */
	if(next && (char*)item+item->size==(char*)next)
	{
		item->size+=next->size;
		next=next->next;
	}
	if(prev)
	{
		item->prev=prev;
		prev->next=item;
	} else 
		h->first=item;
	if(next)
	{
		item->next=next;
		next->prev=item;
	}else
		h->last=item;
	return 1;
}

