/*
 * 386-specific Win32 relay functions
 *
 * Copyright 1997 Alexandre Julliard
 */


#include <assert.h>
#include <string.h>
#include "winnt.h"
#include "builtin32.h"
#include "selectors.h"
#include "stackframe.h"
#include "syslevel.h"
#include "debugstr.h"
#include "main.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(relay)

char **debug_relay_excludelist = NULL, **debug_relay_includelist = NULL;

/***********************************************************************
 *           RELAY_ShowDebugmsgRelay
 *
 * Simple function to decide if a particular debugging message is
 * wanted.  Called from RELAY_CallFrom32 and from in if1632/relay.c
 */
int RELAY_ShowDebugmsgRelay(const char *func) {

  if(debug_relay_excludelist || debug_relay_includelist) {
    const char *term = strchr(func, ':');
    char **listitem;
    int len, len2, itemlen, show;

    if(debug_relay_excludelist) {
      show = 1;
      listitem = debug_relay_excludelist;
    } else {
      show = 0;
      listitem = debug_relay_includelist;
    }
    assert(term);
    assert(strlen(term) > 2);
    len = term - func;
    len2 = strchr(func, '.') - func;
    assert(len2 && len2 > 0 && len2 < 64);
    term += 2;
    for(; *listitem; listitem++) {
      itemlen = strlen(*listitem);
      if((itemlen == len && !lstrncmpiA(*listitem, func, len)) ||
         (itemlen == len2 && !lstrncmpiA(*listitem, func, len2)) ||
         !lstrcmpiA(*listitem, term)) {
        show = !show;
       break;
      }
    }
    return show;
  }
  return 1;
}


#ifdef __i386__

/***********************************************************************
 *           RELAY_PrintArgs
 */
static inline void RELAY_PrintArgs( int *args, int nb_args, unsigned int typemask )
{
    while (nb_args--)
    {
	if ((typemask & 3) && HIWORD(*args))
        {
	    if (typemask & 2)
	    	DPRINTF( "%08x %s", *args, debugstr_w((LPWSTR)*args) );
            else
	    	DPRINTF( "%08x %s", *args, debugstr_a((LPCSTR)*args) );
	}
        else DPRINTF( "%08x", *args );
        if (nb_args) DPRINTF( "," );
        args++;
        typemask >>= 2;
    }
}


/***********************************************************************
 *           RELAY_CallFrom32
 *
 * Stack layout on entry to this function:
 *  ...      ...
 * (esp+12)  arg2
 * (esp+8)   arg1
 * (esp+4)   ret_addr
 * (esp)     return addr to relay code
 */
int RELAY_CallFrom32( int ret_addr, ... )
{
    int ret;
    char buffer[80];
    unsigned int typemask;
    FARPROC func;
    WORD fs;

    int *args = &ret_addr + 1;
    /* Relay addr is the return address for this function */
    BYTE *relay_addr = (BYTE *)__builtin_return_address(0);
    WORD nb_args = *(WORD *)(relay_addr + 1) / sizeof(int);

    assert(TRACE_ON(relay));
    func = (FARPROC)BUILTIN32_GetEntryPoint( buffer, relay_addr - 5, &typemask );
    DPRINTF( "Call %s(", buffer );
    RELAY_PrintArgs( args, nb_args, typemask );
    GET_FS( fs );
    DPRINTF( ") ret=%08x fs=%04x\n", ret_addr, fs );

    SYSLEVEL_CheckNotLevel( 2 );

    if (*relay_addr == 0xc3) /* cdecl */
    {
        LRESULT (*cfunc)() = (LRESULT(*)())func;
        switch(nb_args)
        {
        case 0: ret = cfunc(); break;
        case 1: ret = cfunc(args[0]); break;
        case 2: ret = cfunc(args[0],args[1]); break;
        case 3: ret = cfunc(args[0],args[1],args[2]); break;
        case 4: ret = cfunc(args[0],args[1],args[2],args[3]); break;
        case 5: ret = cfunc(args[0],args[1],args[2],args[3],args[4]); break;
        case 6: ret = cfunc(args[0],args[1],args[2],args[3],args[4],
                            args[5]); break;
        case 7: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6]); break;
        case 8: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7]); break;
        case 9: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8]); break;
        case 10: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9]); break;
        case 11: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9],args[10]); break;
        case 12: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9],args[10],
                             args[11]); break;
        case 13: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9],args[10],args[11],
                             args[12]); break;
        case 14: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9],args[10],args[11],
                             args[12],args[13]); break;
        case 15: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9],args[10],args[11],
                             args[12],args[13],args[14]); break;
        case 16: ret = cfunc(args[0],args[1],args[2],args[3],args[4],args[5],
                             args[6],args[7],args[8],args[9],args[10],args[11],
                             args[12],args[13],args[14],args[15]); break;
        default:
            ERR( "Unsupported nb of args %d\n", nb_args );
            assert(FALSE);
        }
    }
    else  /* stdcall */
    {
        switch(nb_args)
        {
        case 0: ret = func(); break;
        case 1: ret = func(args[0]); break;
        case 2: ret = func(args[0],args[1]); break;
        case 3: ret = func(args[0],args[1],args[2]); break;
        case 4: ret = func(args[0],args[1],args[2],args[3]); break;
        case 5: ret = func(args[0],args[1],args[2],args[3],args[4]); break;
        case 6: ret = func(args[0],args[1],args[2],args[3],args[4],
                           args[5]); break;
        case 7: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                           args[6]); break;
        case 8: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                           args[6],args[7]); break;
        case 9: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                           args[6],args[7],args[8]); break;
        case 10: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9]); break;
        case 11: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9],args[10]); break;
        case 12: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9],args[10],
                            args[11]); break;
        case 13: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9],args[10],args[11],
                            args[12]); break;
        case 14: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9],args[10],args[11],
                            args[12],args[13]); break;
        case 15: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9],args[10],args[11],
                            args[12],args[13],args[14]); break;
        case 16: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                            args[6],args[7],args[8],args[9],args[10],args[11],
                            args[12],args[13],args[14],args[15]); break;
        default:
            ERR( "Unsupported nb of args %d\n", nb_args );
            assert(FALSE);
        }
    }
    DPRINTF( "Ret  %s() retval=%08x ret=%08x fs=%04x\n",
             buffer, ret, ret_addr, fs );

    SYSLEVEL_CheckNotLevel( 2 );

    return ret;
}


/***********************************************************************
 *           RELAY_CallFrom32Regs
 *
 * Stack layout (esp is ESP_reg(context), not the current %esp):
 *
 * ...
 * (esp+4) first arg
 * (esp)   return addr to caller
 * (esp-4) return addr to DEBUG_ENTRY_POINT
 * (esp-8) ptr to relay entry code for RELAY_CallFrom32Regs
 *  ...    >128 bytes space free to be modified (ensured by the assembly glue)
 */

void WINAPI RELAY_DoCallFrom32Regs( CONTEXT86 *context );
DEFINE_REGS_ENTRYPOINT_0( RELAY_CallFrom32Regs, RELAY_DoCallFrom32Regs )
void WINAPI RELAY_DoCallFrom32Regs( CONTEXT86 *context )
{
    unsigned int typemask;
    char buffer[80];
    int* args;
    FARPROC func;
    BYTE *entry_point;

    BYTE *relay_addr = *((BYTE **)ESP_reg(context) - 1);
    WORD nb_args = *(WORD *)(relay_addr + 1) / sizeof(int);

    /* remove extra stuff from the stack */
    EIP_reg(context) = stack32_pop(context);
    args = (int *)ESP_reg(context);
    ESP_reg(context) += 4 * nb_args;

    assert(TRACE_ON(relay));

    entry_point = (BYTE *)BUILTIN32_GetEntryPoint( buffer, relay_addr - 5, &typemask );
    assert( *entry_point == 0xe8 /* lcall */ );
    func = *(FARPROC *)(entry_point + 5);

    DPRINTF( "Call %s(", buffer );
    RELAY_PrintArgs( args, nb_args, typemask );
    DPRINTF( ") ret=%08lx fs=%04lx\n", EIP_reg(context), FS_reg(context) );

    DPRINTF(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
            EAX_reg(context), EBX_reg(context), ECX_reg(context),
            EDX_reg(context), ESI_reg(context), EDI_reg(context) );
    DPRINTF(" ebp=%08lx esp=%08lx ds=%04lx es=%04lx gs=%04lx flags=%08lx\n",
            EBP_reg(context), ESP_reg(context), DS_reg(context),
            ES_reg(context), GS_reg(context), EFL_reg(context) );

    SYSLEVEL_CheckNotLevel( 2 );

    /* Now call the real function */
    switch(nb_args)
    {
    case 0: func(context); break;
    case 1: func(args[0],context); break;
    case 2: func(args[0],args[1],context); break;
    case 3: func(args[0],args[1],args[2],context); break;
    case 4: func(args[0],args[1],args[2],args[3],context); break;
    case 5: func(args[0],args[1],args[2],args[3],args[4],context); break;
    case 6: func(args[0],args[1],args[2],args[3],args[4],args[5],context); break;
    case 7: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],context); break;
    case 8: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],context); break;
    case 9: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                 context); break;
    case 10: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                  args[9],context); break;
    case 11: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                  args[9],args[10],context); break;
    case 12: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                  args[9],args[10],args[11],context); break;
    case 13: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                  args[9],args[10],args[11],args[12],context); break;
    case 14: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                  args[9],args[10],args[11],args[12],args[13],context); break;
    case 15: func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],
                  args[9],args[10],args[11],args[12],args[13],args[14],context); break;
    case 16: func(args[0],args[1],args[2],args[3],args[4],args[5], args[6],args[7],args[8],
                  args[9],args[10],args[11],args[12],args[13],args[14],args[15],context); break;
    default:
        ERR( "Unsupported nb of args %d\n", nb_args );
        assert(FALSE);
    }

    DPRINTF( "Ret  %s() retval=%08lx ret=%08lx fs=%04lx\n",
             buffer, EAX_reg(context), EIP_reg(context), FS_reg(context) );
    DPRINTF(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
            EAX_reg(context), EBX_reg(context), ECX_reg(context),
            EDX_reg(context), ESI_reg(context), EDI_reg(context) );
    DPRINTF(" ebp=%08lx esp=%08lx ds=%04lx es=%04lx gs=%04lx flags=%08lx\n",
            EBP_reg(context), ESP_reg(context), DS_reg(context),
            ES_reg(context), GS_reg(context), EFL_reg(context) );

    SYSLEVEL_CheckNotLevel( 2 );
}
#endif /* __i386__ */

