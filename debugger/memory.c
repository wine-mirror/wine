/*
 * Debugger memory handling
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 * Copyright 2000 Eric Pouech
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugger.h"
#include "miscemu.h"
#include "winbase.h"

#ifdef __i386__
#include "wine/winbase16.h"

#define DBG_V86_MODULE(seg) ((seg)>>16)
#define IS_SELECTOR_V86(seg) DBG_V86_MODULE(seg)

static	void	DEBUG_Die(const char* msg)
{
   DEBUG_Printf(DBG_CHN_MESG, msg);
   exit(1);
}

void*	DEBUG_XMalloc(size_t size)
{
   void *res = malloc(size ? size : 1);
   if (res == NULL)
      DEBUG_Die("Memory exhausted.\n");
   memset(res, 0, size);
   return res;
}

void* DEBUG_XReAlloc(void *ptr, size_t size)
{
   void* res = realloc(ptr, size);
   if ((res == NULL) && size)
      DEBUG_Die("Memory exhausted.\n");
   return res;
}

char*	DEBUG_XStrDup(const char *str)
{
   char *res = strdup(str);
   if (!res)
      DEBUG_Die("Memory exhausted.\n");
   return res;
}

void DEBUG_FixAddress( DBG_ADDR *addr, DWORD def) 
{
   if (addr->seg == 0xffffffff) addr->seg = def;
   if (!IS_SELECTOR_V86(addr->seg) && DEBUG_IsSelectorSystem(addr->seg)) addr->seg = 0;
}

BOOL  DEBUG_FixSegment( DBG_ADDR* addr )
{
   /* V86 mode ? */
   if (DEBUG_context.EFlags & V86_FLAG) {
      addr->seg |= (DWORD)(GetExePtr(GetCurrentTask())) << 16;
      return TRUE;
   }
   return FALSE;
}

DWORD DEBUG_ToLinear( const DBG_ADDR *addr )
{
   LDT_ENTRY	le;
   
   if (IS_SELECTOR_V86(addr->seg))
      return (DWORD) DOSMEM_MemoryBase(DBG_V86_MODULE(addr->seg)) + (((addr->seg)&0xFFFF)<<4) + addr->off;
   if (DEBUG_IsSelectorSystem(addr->seg))
      return addr->off;
   
   if (GetThreadSelectorEntry( DEBUG_CurrThread->handle, addr->seg, &le)) {
      return (le.HighWord.Bits.BaseHi << 24) + (le.HighWord.Bits.BaseMid << 16) + le.BaseLow + addr->off;
   }
   return 0;
}

int	DEBUG_GetSelectorType( WORD sel )
{
    LDT_ENTRY	le;

    if (sel == 0)
        return 32;
    if (IS_SELECTOR_V86(sel))
        return 16;
    if (GetThreadSelectorEntry( DEBUG_CurrThread->handle, sel, &le)) 
        return le.HighWord.Bits.Default_Big ? 32 : 16;
	 /* selector doesn't exist */
    return 0;
}

/* Determine if sel is a system selector (i.e. not managed by Wine) */
BOOL	DEBUG_IsSelectorSystem(WORD sel)
{
   return !(sel & 4) || (((sel & 0xFFFF) >> 3) < 17);
}
#endif /* __i386__ */

void DEBUG_GetCurrentAddress( DBG_ADDR *addr )
{
#ifdef __i386__
    addr->seg  = DEBUG_context.SegCs;

    if (!DEBUG_FixSegment( addr ) && DEBUG_IsSelectorSystem(addr->seg)) 
       addr->seg = 0;
    addr->off  = DEBUG_context.Eip;
#else
    addr->seg  = 0;
    addr->off  = 0;
#endif
}

void	DEBUG_InvalAddr( const DBG_ADDR* addr )
{
   DEBUG_Printf(DBG_CHN_MESG,"*** Invalid address ");
   DEBUG_PrintAddress(addr, DEBUG_CurrThread->dbg_mode, FALSE);
   DEBUG_Printf(DBG_CHN_MESG,"\n");
   if (DBG_IVAR(ExtDbgOnInvalidAddress)) DEBUG_ExternalDebugger();
}

void	DEBUG_InvalLinAddr( void* addr )
{
   DBG_ADDR address;

   address.seg = 0;
   address.off = (unsigned long)addr;
   DEBUG_InvalAddr( &address );
}

/***********************************************************************
 *           DEBUG_ReadMemory
 *
 * Read a memory value.
 */
int DEBUG_ReadMemory( const DBG_ADDR *address )
{
    DBG_ADDR	addr = *address;
    void*	lin;
    int		value;
	
    DEBUG_FixAddress( &addr, DEBUG_context.SegDs );
    lin = (void*)DEBUG_ToLinear( &addr );
    if (!DEBUG_READ_MEM_VERBOSE(lin, &value, sizeof(value)))
       value = 0;
    return value;
}


/***********************************************************************
 *           DEBUG_WriteMemory
 *
 * Store a value in memory.
 */
void DEBUG_WriteMemory( const DBG_ADDR *address, int value )
{
    DBG_ADDR 	addr = *address;
    void*	lin;

    DEBUG_FixAddress( &addr, DEBUG_context.SegDs );
    lin = (void*)DEBUG_ToLinear( &addr );
    DEBUG_WRITE_MEM_VERBOSE(lin, &value, sizeof(value));
}


/***********************************************************************
 *           DEBUG_ExamineMemory
 *
 * Implementation of the 'x' command.
 */
void DEBUG_ExamineMemory( const DBG_VALUE *_value, int count, char format )
{
    DBG_VALUE		  value = *_value;
    int			  i;
    unsigned char	* pnt;
    struct datatype	* testtype;

    assert(_value->cookie == DV_TARGET || _value->cookie == DV_HOST);

    DEBUG_FixAddress( &value.addr, 
		      (format == 'i') ?
		      DEBUG_context.SegCs : 
		      DEBUG_context.SegDs );

    /*
     * Dereference pointer to get actual memory address we need to be
     * reading.  We will use the same segment as what we have already,
     * and hope that this is a sensible thing to do.
     */
    if( value.type != NULL )
      {
	if( value.type == DEBUG_TypeIntConst )
	  {
	    /*
	     * We know that we have the actual offset stored somewhere
	     * else in 32-bit space.  Grab it, and we
	     * should be all set.
	     */
	    unsigned int  seg2 = value.addr.seg;
	    value.addr.seg = 0;
	    value.addr.off = DEBUG_GetExprValue(&value, NULL);
	    value.addr.seg = seg2;
	  }
	else
	  {
	    if (DEBUG_TypeDerefPointer(&value, &testtype) == 0)
	      return;
	    if( testtype != NULL || value.type == DEBUG_TypeIntConst )
	      {
		value.addr.off = DEBUG_GetExprValue(&value, NULL);
	      }
	  }
      }
    else if (!value.addr.seg && !value.addr.off)
    {
	DEBUG_Printf(DBG_CHN_MESG,"Invalid expression\n");
	return;
    }

    if (format != 'i' && count > 1)
    {
        DEBUG_PrintAddress( &value.addr, DEBUG_CurrThread->dbg_mode, FALSE );
        DEBUG_Printf(DBG_CHN_MESG,": ");
    }

    pnt = (void*)DEBUG_ToLinear( &value.addr );

    switch(format)
    {
	case 'u': {
		WCHAR wch;
		if (count == 1) count = 256;
                while (count--)
                {
		    if (!DEBUG_READ_MEM_VERBOSE(pnt, &wch, sizeof(wch)))
		       break;
                    pnt += sizeof(wch);
                    fputc( (char)wch, stderr );
                }
		DEBUG_Printf(DBG_CHN_MESG,"\n");
		return;
	    }
          case 's': {
	        char ch;

		if (count == 1) count = 256;
                while (count--)
                {
                    if (!DEBUG_READ_MEM_VERBOSE(pnt, &ch, sizeof(ch)))
		       break;
                    pnt++;
                    fputc( ch, stderr );
                }
		DEBUG_Printf(DBG_CHN_MESG,"\n");
		return;
	  }
	case 'i':
		while (count--)
                {
                    DEBUG_PrintAddress( &value.addr, DEBUG_CurrThread->dbg_mode, TRUE );
                    DEBUG_Printf(DBG_CHN_MESG,": ");
                    DEBUG_Disasm( &value.addr, TRUE );
                    DEBUG_Printf(DBG_CHN_MESG,"\n");
		}
		return;
#define DO_DUMP2(_t,_l,_f,_vv) { \
	        _t _v; \
		for(i=0; i<count; i++) { \
                    if (!DEBUG_READ_MEM_VERBOSE(pnt, &_v, sizeof(_t))) break; \
                    DEBUG_Printf(DBG_CHN_MESG,_f,(_vv)); \
                    pnt += sizeof(_t); value.addr.off += sizeof(_t); \
                    if ((i % (_l)) == (_l)-1) { \
                        DEBUG_Printf(DBG_CHN_MESG,"\n"); \
                        DEBUG_PrintAddress( &value.addr, DEBUG_CurrThread->dbg_mode, FALSE );\
                        DEBUG_Printf(DBG_CHN_MESG,": ");\
                    } \
		} \
		DEBUG_Printf(DBG_CHN_MESG,"\n"); \
        } \
	return
#define DO_DUMP(_t,_l,_f) DO_DUMP2(_t,_l,_f,_v) 

        case 'x': DO_DUMP(int, 4, " %8.8x");
	case 'd': DO_DUMP(unsigned int, 4, " %10d");	
	case 'w': DO_DUMP(unsigned short, 8, " %04x");
        case 'c': DO_DUMP2(char, 32, " %c", (_v < 0x20) ? ' ' : _v);
	case 'b': DO_DUMP2(char, 16, " %02x", (_v) & 0xff);
	}
}
