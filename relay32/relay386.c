/*
 * 386-specific Win32 relay functions
 *
 * Copyright 1997 Alexandre Julliard
 */


#include <assert.h>
#include <string.h>
#include "winnt.h"
#include "windows.h"
#include "builtin32.h"
#include "selectors.h"
#include "debugstr.h"
#include "debug.h"
#include "main.h"

char **debug_relay_excludelist = NULL, **debug_relay_includelist = NULL;

#ifdef __i386__
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
      if((itemlen == len && !strncmp(*listitem, func, len)) ||
         (itemlen == len2 && !strncmp(*listitem, func, len2)) ||
         !strcmp(*listitem, term)) {
        show = !show;
       break;
      }
    }
    return show;
  }
  return 1;
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
    int i, ret;
    char buffer[80];
    FARPROC32 func;
    unsigned int mask, typemask;
    WORD fs;

    int *args = &ret_addr;
    /* Relay addr is the return address for this function */
    BYTE *relay_addr = (BYTE *)args[-1];
    WORD nb_args = *(WORD *)(relay_addr + 1) / sizeof(int);

    assert(TRACE_ON(relay));
    func = (FARPROC32)BUILTIN32_GetEntryPoint( buffer, relay_addr - 5,
                                               &typemask );
      DPRINTF( "Call %s(", buffer );
      args++;
      for (i = 0, mask = 3; i < nb_args; i++, mask <<= 2)
      {
        if (i) DPRINTF( "," );
	if ((typemask & mask) && HIWORD(args[i]))
        {
	    if (typemask & (2<<(2*i)))
	    	DPRINTF( "%08x L%s", args[i], debugstr_w((LPWSTR)args[i]) );
            else
	    	DPRINTF( "%08x %s", args[i], debugstr_a((LPCSTR)args[i]) );
	}
        else DPRINTF( "%08x", args[i] );
      }
      GET_FS( fs );
      DPRINTF( ") ret=%08x fs=%04x\n", ret_addr, fs );

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
        default:
            ERR(relay, "Unsupported nb args %d\n",
                     nb_args );
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
        default:
            ERR(relay, "Unsupported nb args %d\n",nb_args );
            assert(FALSE);
        }
    }
    DPRINTF( "Ret  %s() retval=%08x ret=%08x fs=%04x\n",
             buffer, ret, ret_addr, fs );
    return ret;
}


/***********************************************************************
 *           RELAY_CallFrom32Regs
 *
 * 'context' contains the register contents at the point of call of
 * the REG_ENTRY_POINT. The stack layout of the stack pointed to by
 * ESP_reg(&context) is as follows:
 *
 * If debugmsg(relay) is OFF:
 *  ...    ...
 * (esp+4) args
 * (esp)   return addr to caller
 * (esp-4) function entry point
 *
 * If debugmsg(relay) is ON:
 *  ...    ...
 * (esp+8) args
 * (esp+4) return addr to caller
 * (esp)   return addr to DEBUG_ENTRY_POINT
 * (esp-4) function entry point
 *
 * As the called function might change the stack layout
 * (e.g. FT_Prolog, FT_ExitNN), we remove all modifications to the stack,
 * so that the called function sees (in both cases):
 *
 *  ...    ...
 * (esp+4) args
 * (esp)   return addr to caller
 *  ...    >128 bytes space free to be modified (ensured by the assembly glue)
 *
 * NOTE: This routine makes no assumption about the relative position of
 *       its own stack to the stack pointed to by ESP_reg(&context),
 *	 except that the latter must have >128 bytes space to grow.
 *	 This means the assembly glue could even switch stacks completely
 *	 (e.g. to allow for large stacks).
 *
 */

void RELAY_CallFrom32Regs( CONTEXT context )
{
    typedef void (CALLBACK *entry_point_t)(CONTEXT *);
    entry_point_t entry_point = *(entry_point_t*) (ESP_reg(&context) - 4);

    __RESTORE_ES;

    if (!TRACE_ON(relay))
    {
        /* Simply call the entry point */
        entry_point( &context );
    }
    else
    {
        char buffer[80];
        unsigned int typemask;
	BYTE *relay_addr;

        /*
	 * Fixup the context structure because of the extra parameter
         * pushed by the relay debugging code.
	 * Note that this implicitly does a RET on the CALL from the
	 * DEBUG_ENTRY_POINT to the REG_ENTRY_POINT;  setting the EIP register
	 * ensures that the assembly glue will directly return to the
	 * caller, just as in the non-debugging case.
	 */

        relay_addr = *(BYTE **) ESP_reg(&context); 
        if (BUILTIN32_GetEntryPoint( buffer, relay_addr - 5, &typemask )) {
	    /* correct win32 spec generated register function found. 
	     * remove extra call stuff from stack
	     */
            ESP_reg(&context) += sizeof(BYTE *);
	    EIP_reg(&context) = *(DWORD *)ESP_reg(&context);
	    DPRINTF("Call %s(regs) ret=%08x\n", buffer, *(int *)ESP_reg(&context) );
	    DPRINTF(" EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
		    EAX_reg(&context), EBX_reg(&context), ECX_reg(&context),
		    EDX_reg(&context), ESI_reg(&context), EDI_reg(&context) );
	    DPRINTF(" EBP=%08lx ESP=%08lx EIP=%08lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx EFL=%08lx\n",
		    EBP_reg(&context), ESP_reg(&context), EIP_reg(&context),
		    DS_reg(&context), ES_reg(&context), FS_reg(&context),
		    GS_reg(&context), EFL_reg(&context) );

	    /* Now call the real function */
	    entry_point( &context );


	    DPRINTF("Ret  %s() retval=regs ret=%08x\n", buffer, *(int *)ESP_reg(&context) );
	    DPRINTF(" EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
		    EAX_reg(&context), EBX_reg(&context), ECX_reg(&context),
		    EDX_reg(&context), ESI_reg(&context), EDI_reg(&context) );
	    DPRINTF(" EBP=%08lx ESP=%08lx EIP=%08lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx EFL=%08lx\n",
		    EBP_reg(&context), ESP_reg(&context), EIP_reg(&context),
		    DS_reg(&context), ES_reg(&context), FS_reg(&context),
		    GS_reg(&context), EFL_reg(&context) );
	} else
	    /* WINE internal register function found. Do not remove anything.
	     * Do not print any debuginfo (it is not a normal relayed one).
	     * Currently only used for snooping.
	     */
	   entry_point( &context );
    }
}
#endif  /* __i386__ */
