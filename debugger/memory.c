/*
 * Debugger memory handling
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "debugger.h"


/***********************************************************************
 *           DEBUG_IsBadReadPtr
 *
 * Check if we are allowed to read memory at 'address'.
 */
BOOL32 DEBUG_IsBadReadPtr( const DBG_ADDR *address, int size )
{
    if (address->seg)  /* segmented addr */
        return IsBadReadPtr16( (SEGPTR)MAKELONG( (WORD)address->off,
                                                 (WORD)address->seg ), size );
        /* FIXME: should check if resulting linear addr is readable */
    else  /* linear address */
        return FALSE;  /* FIXME: should do some checks here */
}


/***********************************************************************
 *           DEBUG_IsBadWritePtr
 *
 * Check if we are allowed to write memory at 'address'.
 */
BOOL32 DEBUG_IsBadWritePtr( const DBG_ADDR *address, int size )
{
    if (address->seg)  /* segmented addr */
        /* Note: we use IsBadReadPtr here because we are */
        /* always allowed to write to read-only segments */
        return IsBadReadPtr16( (SEGPTR)MAKELONG( (WORD)address->off,
                                                 (WORD)address->seg ), size );
        /* FIXME: should check if resulting linear addr is writable */
    else  /* linear address */
        return FALSE;  /* FIXME: should do some checks here */
}


/***********************************************************************
 *           DEBUG_ReadMemory
 *
 * Read a memory value.
 */
int DEBUG_ReadMemory( const DBG_ADDR *address )
{
    DBG_ADDR addr = *address;

    DBG_FIX_ADDR_SEG( &addr, DS_reg(DEBUG_context) );
    if (!DBG_CHECK_READ_PTR( &addr, sizeof(int) )) return 0;
    return *(int *)DBG_ADDR_TO_LIN( &addr );
}


/***********************************************************************
 *           DEBUG_WriteMemory
 *
 * Store a value in memory.
 */
void DEBUG_WriteMemory( const DBG_ADDR *address, int value )
{
    DBG_ADDR addr = *address;

    DBG_FIX_ADDR_SEG( &addr, DS_reg(DEBUG_context) );
    if (!DBG_CHECK_WRITE_PTR( &addr, sizeof(int) )) return;
    *(int *)DBG_ADDR_TO_LIN( &addr ) = value;
}


/***********************************************************************
 *           DEBUG_ExamineMemory
 *
 * Implementation of the 'x' command.
 */
void DEBUG_ExamineMemory( const DBG_ADDR *address, int count, char format )
{
    DBG_ADDR addr = *address;
    unsigned char * pnt;
    unsigned int * dump;
    unsigned short int * wdump;
    int i;

    DBG_FIX_ADDR_SEG( &addr, (format == 'i') ?
                             CS_reg(DEBUG_context) : DS_reg(DEBUG_context) );

    if (format != 'i' && count > 1)
    {
        DEBUG_PrintAddress( &addr, dbg_mode );
        fprintf(stderr,": ");
    }

    pnt = DBG_ADDR_TO_LIN( &addr );

    switch(format)
    {
	case 's':
		if (count == 1) count = 256;
                while (count--)
                {
                    if (!DBG_CHECK_READ_PTR( &addr, sizeof(char) )) return;
                    if (!*pnt) break;
                    addr.off++;
                    fputc( *pnt++, stderr );
                }
		fprintf(stderr,"\n");
		return;

	case 'i':
		while (count--)
                {
                    DEBUG_PrintAddress( &addr, dbg_mode );
                    fprintf(stderr,": ");
                    if (!DBG_CHECK_READ_PTR( &addr, 1 )) return;
                    DEBUG_Disasm( &addr );
                    fprintf(stderr,"\n");
		}
		return;
	case 'x':
		dump = (unsigned int *)pnt;
		for(i=0; i<count; i++) 
		{
                    if (!DBG_CHECK_READ_PTR( &addr, sizeof(int) )) return;
                    fprintf(stderr," %8.8x", *dump++);
                    addr.off += sizeof(int);
                    if ((i % 8) == 7)
                    {
                        fprintf(stderr,"\n");
                        DEBUG_PrintAddress( &addr, dbg_mode );
                        fprintf(stderr,": ");
                    }
		}
		fprintf(stderr,"\n");
		return;
	
	case 'd':
		dump = (unsigned int *)pnt;
		for(i=0; i<count; i++) 
		{
                    if (!DBG_CHECK_READ_PTR( &addr, sizeof(int) )) return;
                    fprintf(stderr," %d", *dump++);
                    addr.off += sizeof(int);
                    if ((i % 8) == 7)
                    {
                        fprintf(stderr,"\n");
                        DEBUG_PrintAddress( &addr, dbg_mode );
                        fprintf(stderr,": ");
                    }
		}
		fprintf(stderr,"\n");
		return;
	
	case 'w':
		wdump = (unsigned short *)pnt;
		for(i=0; i<count; i++) 
		{
                    if (!DBG_CHECK_READ_PTR( &addr, sizeof(short) )) return;
                    fprintf(stderr," %04x", *wdump++);
                    addr.off += sizeof(short);
                    if ((i % 8) == 7)
                    {
                        fprintf(stderr,"\n");
                        DEBUG_PrintAddress( &addr, dbg_mode );
                        fprintf(stderr,": ");
                    }
		}
		fprintf(stderr,"\n");
		return;
	
	case 'c':
		for(i=0; i<count; i++) 
		{
                    if (!DBG_CHECK_READ_PTR( &addr, sizeof(char) )) return;
                    if(*pnt < 0x20)
                    {
                        fprintf(stderr,"  ");
                        pnt++;
                    }
                    else fprintf(stderr," %c", *pnt++);
                    addr.off++;
                    if ((i % 32) == 31)
                    {
                        fprintf(stderr,"\n");
                        DEBUG_PrintAddress( &addr, dbg_mode );
                        fprintf(stderr,": ");
                    }
		}
		fprintf(stderr,"\n");
		return;
	
	case 'b':
		for(i=0; i<count; i++) 
		{
                    if (!DBG_CHECK_READ_PTR( &addr, sizeof(char) )) return;
                    fprintf(stderr," %02x", (*pnt++) & 0xff);
                    addr.off++;
                    if ((i % 16) == 15)
                    {
                        fprintf(stderr,"\n");
                        DEBUG_PrintAddress( &addr, dbg_mode );
                        fprintf(stderr,": ");
                    }
		}
		fprintf(stderr,"\n");
		return;
	}
}
