/* $Id: heap.h,v 1.2 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef HEAP_H
#define HEAP_H

typedef struct heap_mem_desc_s
{
    struct heap_mem_desc_s *prev, *next;
    int   length;
} MDESC;

extern void HEAP_Init(MDESC **free_list, void *start, int length);
extern void *HEAP_Alloc(MDESC **free_list, int flags, int bytes);
extern void HEAP_Free(MDESC **free_list, void *block);

#endif /* HEAP_H */
