/*
 * 386-specific Win32 relay functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#ifdef __i386__

#include <assert.h>
#include "winnt.h"
#include "windows.h"
#include "builtin32.h"
#include "selectors.h"
#include "debug.h"

static void _dumpstr(unsigned char *s) {
	fputs("\"",stdout);
	while (*s) {
		if (*s<' ') {
			printf("\\0x%02x",*s++);
			continue;
		}
		if (*s=='\\') {
			fputs("\\\\",stdout);
			s++;
			continue;
		}
		fputc(*s++,stdout);
	}
	fputs("\"",stdout);
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
    printf( "Call %s(", buffer );
    args++;
    for (i = 0, mask = 3; i < nb_args; i++, mask <<= 2)
    {
        if (i) printf( "," );
	if ((typemask & mask) && HIWORD(args[i]))
        {
	    if (typemask & (2<<(2*i)))
            {
                char buff[80];
                lstrcpynWtoA( buff, (LPWSTR)args[i], sizeof(buff) );
		buff[sizeof(buff)-1]='\0';
	    	printf( "%08x L", args[i] );
		_dumpstr((unsigned char*)buff);
	    }
            else {
	    	printf( "%08x ", args[i] );
		_dumpstr((unsigned char*)args[i]);
	    }
	}
        else printf( "%08x", args[i] );
    }
    GET_FS( fs );
    printf( ") ret=%08x fs=%04x\n", ret_addr, fs );
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
            fprintf( stderr, "RELAY_CallFrom32: Unsupported nb args %d\n",
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
            fprintf( stderr, "RELAY_CallFrom32: Unsupported nb args %d\n",
                     nb_args );
            assert(FALSE);
        }
    }
    printf( "Ret  %s() retval=%08x ret=%08x fs=%04x\n",
            buffer, ret, ret_addr, fs );
    return ret;
}


/***********************************************************************
 *           RELAY_CallFrom32Regs
 *
 * 'stack' points to the relay addr on the stack.
 * Stack layout:
 *  ...      ...
 * (esp+216) ret_addr
 * (esp+212) return to relay debugging code (only when debugging(relay))
 * (esp+208) entry point to call
 * (esp+4)   CONTEXT
 * (esp)     return addr to relay code
 */
void RELAY_CallFrom32Regs( CONTEXT context,
                           void (CALLBACK *entry_point)(CONTEXT *),
                           BYTE *relay_addr, int ret_addr )
{
    if (!TRACE_ON(relay))
    {
        /* Simply call the entry point */
        entry_point( &context );
    }
    else
    {
        char buffer[80];
        unsigned int typemask;

    	__RESTORE_ES;
        /* Fixup the context structure because of the extra parameter */
        /* pushed by the relay debugging code */

        EIP_reg(&context) = ret_addr;
        ESP_reg(&context) += sizeof(int);

        BUILTIN32_GetEntryPoint( buffer, relay_addr - 5, &typemask );
        printf("Call %s(regs) ret=%08x\n", buffer, ret_addr );
        printf(" EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
                EAX_reg(&context), EBX_reg(&context), ECX_reg(&context),
                EDX_reg(&context), ESI_reg(&context), EDI_reg(&context) );
        printf(" EBP=%08lx ESP=%08lx EIP=%08lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx EFL=%08lx\n",
                EBP_reg(&context), ESP_reg(&context), EIP_reg(&context),
                DS_reg(&context), ES_reg(&context), FS_reg(&context),
                GS_reg(&context), EFL_reg(&context) );

        /* Now call the real function */
        entry_point( &context );

        printf("Ret  %s() retval=regs ret=%08x\n", buffer, ret_addr );
        printf(" EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
                EAX_reg(&context), EBX_reg(&context), ECX_reg(&context),
                EDX_reg(&context), ESI_reg(&context), EDI_reg(&context) );
        printf(" EBP=%08lx ESP=%08lx EIP=%08lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx EFL=%08lx\n",
                EBP_reg(&context), ESP_reg(&context), EIP_reg(&context),
                DS_reg(&context), ES_reg(&context), FS_reg(&context),
                GS_reg(&context), EFL_reg(&context) );
    }
}

#endif  /* __i386__ */
