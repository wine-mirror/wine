/*
 * Debugger memory handling
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "debugger.h"


/***********************************************************************
 *           DEBUG_ReadMemory
 *
 * Read a memory value.
 */
int DEBUG_ReadMemory( const DBG_ADDR *address )
{
    DBG_ADDR addr = *address;

    DBG_FIX_ADDR_SEG( &addr, DS_reg(DEBUG_context) );
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
        fprintf(stderr,":  ");
    }

    pnt = DBG_ADDR_TO_LIN( &addr );

    switch(format)
    {
	case 's':
		if (count == 1) count = 256;
	        while(*pnt && count--) fputc( *pnt++, stderr );
		fprintf(stderr,"\n");
		return;

	case 'i':
		while (count--)
                {
                    DEBUG_PrintAddress( &addr, dbg_mode );
                    fprintf(stderr,":  ");
                    DEBUG_Disasm( &addr );
                    fprintf(stderr,"\n");
		}
		return;
	case 'x':
		dump = (unsigned int *)pnt;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %8.8x", *dump++);
                        addr.off += 4;
			if ((i % 8) == 7) {
				fprintf(stderr,"\n");
				DEBUG_PrintAddress( &addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'd':
		dump = (unsigned int *)pnt;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %d", *dump++);
                        addr.off += 4;
			if ((i % 8) == 7) {
				fprintf(stderr,"\n");
				DEBUG_PrintAddress( &addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'w':
		wdump = (unsigned short *)pnt;
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %04x", *wdump++);
                        addr.off += 2;
			if ((i % 10) == 7) {
				fprintf(stderr,"\n");
				DEBUG_PrintAddress( &addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'c':
		for(i=0; i<count; i++) 
		{
			if(*pnt < 0x20) {
				fprintf(stderr,"  ");
				pnt++;
			} else
				fprintf(stderr," %c", *pnt++);
                        addr.off++;
			if ((i % 32) == 7) {
				fprintf(stderr,"\n");
				DEBUG_PrintAddress( &addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	
	case 'b':
		for(i=0; i<count; i++) 
		{
			fprintf(stderr," %02x", (*pnt++) & 0xff);
                        addr.off++;
			if ((i % 32) == 7) {
				fprintf(stderr,"\n");
				DEBUG_PrintAddress( &addr, dbg_mode );
				fprintf(stderr,":  ");
			};
		}
		fprintf(stderr,"\n");
		return;
	}
}
