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


/************************************************************
 *   
 *  Check if linear pointer in [addr, addr+size[
 *     read  (rwflag == 1)
 *   or
 *     write (rwflag == 0)
 ************************************************************/

#ifdef linux
BOOL32 DEBUG_checkmap_bad( const char *addr, size_t size, int rwflag)
{
  FILE *fp;
  char buf[80];      /* temporary line buffer */
  char prot[5];      /* protection string */
  char *start, *end;
  int ret = TRUE;

  /* 
     The entries in /proc/self/maps are of the form:
     08000000-08002000 r-xp 00000000 03:41 2361
     08002000-08003000 rw-p 00001000 03:41 2361
     08003000-08005000 rwxp 00000000 00:00 0
     40000000-40005000 r-xp 00000000 03:41 67219
     40005000-40006000 rw-p 00004000 03:41 67219
     40006000-40007000 rw-p 00000000 00:00 0
     ...
      start    end     perm   ???    major:minor inode

     Only permissions start and end are used here
     */
  
  if (!(fp = fopen("/proc/self/maps", "r")))
    return FALSE; 

  while (fgets( buf, 79, fp)) {
    sscanf(buf, "%x-%x %3s", (int *) &start, (int *) &end, prot);
    if ( end <= addr)
      continue;
    if (start <= addr && addr+size < end) {
      if (rwflag) 
	ret = (prot[0] != 'r'); /* test for reading */
      else
	ret = (prot[1] != 'w'); /* test for writing */
    }
    break;
  }
  fclose( fp);
  return ret;
}
#else  /* linux */
/* FIXME: code needed for BSD et al. */
BOOL32 DEBUG_checkmap_bad(char *addr, size_t size, int rwflag)
{
    return FALSE;
}
#endif  /* linux */


/***********************************************************************
 *           DEBUG_IsBadReadPtr
 *
 * Check if we are allowed to read memory at 'address'.
 */
BOOL32 DEBUG_IsBadReadPtr( const DBG_ADDR *address, int size )
{
    if (address->seg)  /* segmented addr */
    {
        if (IsBadReadPtr16( (SEGPTR)MAKELONG( (WORD)address->off,
                                              (WORD)address->seg ), size ))
            return TRUE;
    }
    return DEBUG_checkmap_bad( DBG_ADDR_TO_LIN(address), size, 1);
}


/***********************************************************************
 *           DEBUG_IsBadWritePtr
 *
 * Check if we are allowed to write memory at 'address'.
 */
BOOL32 DEBUG_IsBadWritePtr( const DBG_ADDR *address, int size )
{
    if (address->seg)  /* segmented addr */
    {
        /* Note: we use IsBadReadPtr here because we are */
        /* always allowed to write to read-only segments */
        if (IsBadReadPtr16( (SEGPTR)MAKELONG( (WORD)address->off,
                                              (WORD)address->seg ), size ))
            return TRUE;
    }
    return DEBUG_checkmap_bad( DBG_ADDR_TO_LIN(address), size, 0);
}


/***********************************************************************
 *           DEBUG_ReadMemory
 *
 * Read a memory value.
 */
int DEBUG_ReadMemory( const DBG_ADDR *address )
{
    DBG_ADDR addr = *address;

    DBG_FIX_ADDR_SEG( &addr, DS_reg(&DEBUG_context) );
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

    DBG_FIX_ADDR_SEG( &addr, DS_reg(&DEBUG_context) );
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
    DBG_ADDR addr =	* address;
    unsigned int	* dump;
    int			  i;
    unsigned char	* pnt;
    unsigned int	  seg2;
    struct datatype	* testtype;
    unsigned short int	* wdump;

    DBG_FIX_ADDR_SEG( &addr, (format == 'i') ?
                             CS_reg(&DEBUG_context) : DS_reg(&DEBUG_context) );

    /*
     * Dereference pointer to get actual memory address we need to be
     * reading.  We will use the same segment as what we have already,
     * and hope that this is a sensible thing to do.
     */
    if( addr.type != NULL )
      {
	if( addr.type == DEBUG_TypeIntConst )
	  {
	    /*
	     * We know that we have the actual offset stored somewhere
	     * else in 32-bit space.  Grab it, and we
	     * should be all set.
	     */
	    seg2 = addr.seg;
	    addr.seg = 0;
	    addr.off = DEBUG_GetExprValue(&addr, NULL);
	    addr.seg = seg2;
	  }
	else
	  {
	    if (!DBG_CHECK_READ_PTR( &addr, 1 )) return;
	    DEBUG_TypeDerefPointer(&addr, &testtype);
	    if( testtype != NULL || addr.type == DEBUG_TypeIntConst )
	      {
		addr.off = DEBUG_GetExprValue(&addr, NULL);
	      }
	  }
      }
    else if (!addr.seg && !addr.off)
    {
	fprintf(stderr,"Invalid expression\n");
	return;
    }

    if (format != 'i' && count > 1)
    {
        DEBUG_PrintAddress( &addr, dbg_mode, FALSE );
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
                    DEBUG_PrintAddress( &addr, dbg_mode, TRUE );
                    fprintf(stderr,": ");
                    if (!DBG_CHECK_READ_PTR( &addr, 1 )) return;
                    DEBUG_Disasm( &addr, TRUE );
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
                        DEBUG_PrintAddress( &addr, dbg_mode, FALSE );
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
                        DEBUG_PrintAddress( &addr, dbg_mode, FALSE );
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
                        DEBUG_PrintAddress( &addr, dbg_mode, FALSE );
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
                        DEBUG_PrintAddress( &addr, dbg_mode, FALSE );
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
                        DEBUG_PrintAddress( &addr, dbg_mode, FALSE );
                        fprintf(stderr,": ");
                    }
		}
		fprintf(stderr,"\n");
		return;
	}
}
