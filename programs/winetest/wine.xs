/* -*-C-*-
 * Perl gateway to wine API calls
 *
 * Copyright 2001 John F Sturtz for Codeweavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "windef.h"

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#undef WORD
#include "winbase.h"

/* API return type constants */
enum ret_type
{
    RET_VOID = 0,
    RET_INT  = 1,
    RET_WORD = 2,
    RET_PTR  = 3,
    RET_STR  = 4
};

/* max arguments for a function call */
#define MAX_ARGS    16

extern unsigned long perl_call_wine
(
    FARPROC        function,
    int            n_args,
    unsigned long  *args,
    unsigned int   *last_error,
    int            debug
);

/* Thunk type definitions */

#ifdef __i386__
#pragma pack(1)
struct thunk
{
    BYTE    pushl;
    BYTE    movl[2];
    BYTE    leal_args[3];
    BYTE    pushl_args;
    BYTE    pushl_addr;
    BYTE   *args_ptr;
    BYTE    pushl_nb_args;
    BYTE    nb_args;
    BYTE    pushl_ref;
    SV     *code_ref;
    BYTE    call;
    void   *func;
    BYTE    leave;
    BYTE    ret;
    short   arg_size;
    BYTE    arg_types[MAX_ARGS];
};
#pragma pack(4)
#else
#error You must implement the callback thunk for your CPU
#endif

/*--------------------------------------------------------------
| This contains most of the machine instructions necessary to
| implement the thunk.  All the thunk does is turn around and
| call function callback_bridge(), which is defined in
| winetest.c.
|
| The data from this static thunk can just be copied directly
| into the thunk allocated dynamically below.  That fills in
| most of it, but a couple values need to be filled in after
| the allocation, at run time:
|
|     1) The pointer to the thunk's data area, which we
|        don't know yet, because we haven't allocated it
|        yet ...
|
|     2) The address of the function to call.  We know the
|        address of the function [callback_bridge()], but
|        the value filled into the thunk is an address
|        relative to the thunk itself, so we can't fill it
|        in until we've allocated the actual thunk.
--------------------------------------------------------------*/
static const struct thunk thunk_template =
{
    /* pushl %ebp        */  0x55,
    /* movl %esp,%ebp    */  { 0x89, 0xe5 },
    /* leal 8(%ebp),%edx */  { 0x8d, 0x55, 0x08 },
    /* pushl %edx        */  0x52,
    /* pushl (data addr) */  0x68, NULL,
    /* pushl (nb_args)   */  0x6a, 0,
    /* pushl (code ref)  */  0x68, NULL,
    /* call (func)       */  0xe8, NULL,
    /* leave             */  0xc9,
    /* ret $arg_size     */  0xc2, 0,
    /* arg_types         */  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};


/*----------------------------------------------------------------------
| Function:    convert_value                                           |
| -------------------------------------------------------------------- |
| Purpose:     Convert a C value to a Perl value                       |
|                                                                      |
| Parameters:  type -- constant specifying type of value               |
|              val  -- value to convert                                |
|                                                                      |
| Returns:     Perl SV *                                               |
----------------------------------------------------------------------*/
static SV *convert_value( enum ret_type type, unsigned long val )
{
    switch (type)
    {
        case RET_VOID: return &PL_sv_undef;
        case RET_INT:  return sv_2mortal( newSViv ((int) val ));
        case RET_WORD: return sv_2mortal( newSViv ((int) val & 0xffff ));
        case RET_PTR:  return sv_2mortal( newSViv ((int) val ));
        case RET_STR:  return sv_2mortal( newSVpv ((char *) val, 0 ));
    }
    croak ("Bad return type %d", type);
    return &PL_sv_undef;
}


/*----------------------------------------------------------------------
| Function:    callback_bridge                                         |
| -------------------------------------------------------------------- |
| Purpose:     Central pass-through point for Wine API callbacks       |
|                                                                      |
|     Wine API callback thunks are set up so that they call this       |
|     function, which turns around and calls the user's declared       |
|     Perl callback sub.                                               |
|                                                                      |
| Parameters:  data -- pointer to thunk data area                      |
|              args -- array of args passed from Wine API to callback  |
|                                                                      |
| Returns:     Whatever the Perl sub returns                           |
----------------------------------------------------------------------*/
static int callback_bridge( SV *callback_ref, int nb_args, BYTE arg_types[], unsigned long args[] )
{
    /* Locals */
    int  i, n;
    SV   *sv;

    int  r = 0;

    /* Perl/C interface voodoo */
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);

    /* Push args on stack, according to type */
    for (i = 0; i < nb_args; i++)
    {
        sv = convert_value (arg_types[i], args[i]);
        PUSHs (sv);
    }
    PUTBACK;

    /* Call Perl sub */
    n = perl_call_sv (callback_ref, G_SCALAR);

    /* Nab return value */
    SPAGAIN;
    if (n == 1)
    {
        r = POPi;
    }
    PUTBACK;
    FREETMPS;
    LEAVE;

    /* [todo]  Pass through Perl sub return value */
    return (r);
}


/*----------------------------------------------------------------------
| XS module                                                            |
|                                                                      |
|                                                                      |
----------------------------------------------------------------------*/
MODULE = wine     PACKAGE = wine


    # --------------------------------------------------------------------
    # Function:    call_wine_API
    # --------------------------------------------------------------------
    # Purpose:     Call perl_call_wine(), which calls a wine API function
    #
    # Parameters:  function -- API function to call
    #              ret_type -- return type
    #              debug    -- debug flag
    #              ...      -- args to pass to API function
    #
    # Returns:     list containing 2 elements: the last error code and the
    #              value returned by the API function
    # --------------------------------------------------------------------
void
call_wine_API(function, ret_type, debug, ...)
    unsigned long function;
    int   ret_type;
    int   debug;

    PROTOTYPE: $$$@

    PPCODE:
    /*--------------------------------------------------------------
    | Begin call_wine_API
    --------------------------------------------------------------*/

    /* Local types */
    struct arg
    {
        int           ival;
        void          *pval;
    };

    /* Locals */
    int            n_fixed = 3;
    int            n_args = (items - n_fixed);
    struct arg     args[MAX_ARGS+1];
    unsigned long  f_args[MAX_ARGS+1];
    unsigned int   i, n;
    unsigned int   last_error = 0xdeadbeef;
    char           *p;
    SV             *sv;
    unsigned long  r;

    if (n_args > MAX_ARGS) croak("Too many arguments");

    /*--------------------------------------------------------------
    | Prepare function args
    --------------------------------------------------------------*/
    if (debug > 1)
    {
        fprintf( stderr, "    [wine.xs/call_wine_API()]\n");
    }
    for (i = 0; (i < n_args); i++)
    {
        sv = ST (n_fixed + i);
        args[i].pval = NULL;

        if (! SvOK (sv))
            continue;

        /*--------------------------------------------------------------
        | Ref
        --------------------------------------------------------------*/
        if (SvROK (sv))
        {
            sv = SvRV (sv);

            /*--------------------------------------------------------------
            | Integer ref -- pass address of value
            --------------------------------------------------------------*/
            if (SvIOK (sv))
            {
                args[i].ival = SvIV (sv);
                f_args[i] = (unsigned long) &(args[i].ival);
                if (debug > 1)
                {
                    fprintf( stderr, "        [RV->IV] 0x%lx\n", f_args[i]);
                }
            }

            /*--------------------------------------------------------------
            | Number ref -- convert and pass address of value
            --------------------------------------------------------------*/
            else if (SvNOK (sv))
            {
                args[i].ival = (unsigned long) SvNV (sv);
                f_args[i] = (unsigned long) &(args[i].ival);
                if (debug > 1)
                {
                    fprintf( stderr, "        [RV->NV] 0x%lx\n", f_args[i]);
                }
            }

            /*--------------------------------------------------------------
            | String ref -- pass pointer
            --------------------------------------------------------------*/
            else if (SvPOK (sv))
            {
                f_args[i] = (unsigned long) ((char *) SvPV (sv, PL_na));
                if (debug > 1)
                {
                    fprintf( stderr, "        [RV->PV] 0x%lx\n", f_args[i]);
                }
            }
        }

        /*--------------------------------------------------------------
        | Scalar
        --------------------------------------------------------------*/
        else
        {

            /*--------------------------------------------------------------
            | Integer -- pass value
            --------------------------------------------------------------*/
            if (SvIOK (sv))
            {
                f_args[i] = (unsigned long) SvIV (sv);
                if (debug > 1)
                {
                    fprintf( stderr, "        [IV]     %ld (0x%lx)\n", f_args[i], f_args[i]);
                }
            }

            /*--------------------------------------------------------------
            | Number -- convert and pass value
            --------------------------------------------------------------*/
            else if (SvNOK (sv))
            {
                f_args[i] = (unsigned long) SvNV (sv);
                if (debug > 1)
                {
                    fprintf( stderr, "        [NV]     %ld (0x%lx)\n", f_args[i], f_args[i]);
                }
            }

            /*--------------------------------------------------------------
            | String -- pass pointer to copy
            --------------------------------------------------------------*/
            else if (SvPOK (sv))
            {
                p = SvPV (sv, n);
                if ((args[i].pval = malloc( n+2 )))
                {
                    memcpy (args[i].pval, p, n);
                    ((char *)(args[i].pval))[n] = 0;  /* add final NULL */
                    ((char *)(args[i].pval))[n+1] = 0;  /* and another one for Unicode too */
                    f_args[i] = (unsigned long) args[i].pval;
                    if (debug > 1)
                    {
                        fprintf( stderr, "        [PV]     0x%lx\n", f_args[i]);
                    }
                }
            }
        }

    }  /* end for */

    /*--------------------------------------------------------------
    | Here we go
    --------------------------------------------------------------*/
    r = perl_call_wine( (FARPROC)function, n_args, f_args, &last_error, debug );

    /*--------------------------------------------------------------
    | Handle modified parameter values
    |
    | There are four possibilities for parameter values:
    |
    |     1) integer value
    |     2) string value
    |     3) ref to integer value
    |     4) ref to string value
    |
    | In cases 1 and 2, the intent is that the values won't be
    | modified, because they're not passed by ref.  So we leave
    | them alone here.
    |
    | In case 4, the address of the actual string buffer has
    | already been passed to the wine API function, which had
    | opportunity to modify it if it wanted to.  So again, we
    | don't have anything to do here.
    |
    | The case we need to handle is case 3.  For integers passed
    | by ref, we created a local containing the initial value,
    | and passed its address to the wine API function, which
    | (potentially) modified it.  Now we have to copy the
    | (potentially) new value back to the Perl variable passed
    | in, using sv_setiv().  (Which will take fewer lines of code
    | to do than it took lines of comment to describe ...)
    --------------------------------------------------------------*/
    for (i = 0; (i < n_args); i++)
    {
        sv = ST (n_fixed + i);
        if (! SvOK (sv))
            continue;
        if (SvROK (sv) && (sv = SvRV (sv)) && SvIOK (sv))
        {
            sv_setiv (sv, args[i].ival);
        }
    }

    /*--------------------------------------------------------------
    | Put appropriate return value on the stack for Perl to pick
    | up
    --------------------------------------------------------------*/
    EXTEND(SP,2);
    if (last_error != 0xdeadbeef) PUSHs(sv_2mortal(newSViv(last_error)));
    else PUSHs( &PL_sv_undef );
    PUSHs (convert_value (ret_type, r));

    /*--------------------------------------------------------------
    | Free up allocated memory
    --------------------------------------------------------------*/
    for (i = 0; (i < n_args); i++)
    {
        if (args[i].pval) free(args[i].pval);
    }


    # --------------------------------------------------------------------
    # Function:    load_library
    # --------------------------------------------------------------------
    # Purpose:     Load a Wine library
    #
    # Parameters:  module   -- module (dll) to load
    #
    # Returns:     module handle
    # --------------------------------------------------------------------
void
load_library(module)
    char  *module;
    PROTOTYPE: $

    PPCODE:
    ST(0) = newSViv( (I32)LoadLibraryA(module) );
    XSRETURN(1);


    # --------------------------------------------------------------------
    # Function:    get_proc_address
    # --------------------------------------------------------------------
    # Purpose:     Retrive a function address
    #
    # Parameters:  module   -- module handle
    # --------------------------------------------------------------------
void
get_proc_address(module,func)
    unsigned long module;
    char  *func;
    PROTOTYPE: $$

    PPCODE:
    ST(0) = newSViv( (I32)GetProcAddress( (HMODULE)module, func ) );
    XSRETURN(1);


    # --------------------------------------------------------------------
    # Function:    alloc_thunk
    # --------------------------------------------------------------------
    # Purpose:     Allocate a thunk for a wine API callback
    #
    #   This is used when a Wine API function is called from Perl, and
    #   that API function takes a callback as one of its parameters.
    #
    #   The Wine API function, of course, must be passed the address of
    #   a C function as the callback.  But if the API is called from Perl,
    #   we want the user to be able to specify a Perl sub as the callback,
    #   and have control returned there each time the callback is called.
    #
    #   This function takes a code ref to a Perl sub as one of its
    #   arguments.  It then creates a unique C function (a thunk) on the
    #   fly, which can be passed to the Wine API function as its callback.
    #
    #   The thunk has its own data area (as thunks are wont to do); one
    #   of the things stashed there is aforementioned Perl code ref.  So
    #   the sequence of events is as follows:
    #
    #       1) From Perl, user calls alloc_callback(), passing a ref
    #          to a Perl sub to use as the callback.
    #
    #       2) alloc_callback() calls this routine.  This routine
    #          creates a thunk, and stashes the above code ref in
    #          it.  This function then returns a pointer to the thunk
    #          to Perl.
    #
    #       3) From Perl, user calls Wine API function.  As the parameter
    #          which is supposed to be the address of the callback, the
    #          user passes the pointer to the thunk allocated above.
    #
    #       4) The Wine API function gets called.  It periodically calls
    #          the callback, which executes the thunk.
    #
    #       5) Each time the thunk is executed, it calls callback_bridge()
    #          (defined in winetest.c).
    #
    #       6) callback_bridge() fishes the Perl code ref out of the
    #          thunk data area and calls the Perl callback.
    #
    #   Voila.  The Perl callback gets called each time the Wine API
    #   function calls its callback.
    #
    # Parameters:  [todo]  Parameters ...
    #
    # Returns:     Pointer to thunk
    # --------------------------------------------------------------------
void
alloc_thunk(...)

    PPCODE:

    /* Locals */
    struct thunk *thunk;
    int i;

    /* Allocate the thunk */
    if (!(thunk = malloc( sizeof(*thunk) ))) croak( "Out of memory" );

    (*thunk) = thunk_template;
    thunk->args_ptr = thunk->arg_types;
    thunk->nb_args  = items - 1;
    thunk->code_ref = SvRV (ST (0));
    thunk->func     = (void *)((char *) callback_bridge - (char *) &thunk->leave);
    thunk->arg_size = thunk->nb_args * sizeof(int);

    /* Stash callback arg types */
    for (i = 1; i < items; i++) thunk->arg_types[i - 1] = SvIV (ST (i));

    /*--------------------------------------------------------------
    | Push the address of the thunk on the stack for return
    |
    | [todo]  We need to free up the memory allocated somehow ...
    --------------------------------------------------------------*/
    ST (0) = newSViv ((I32) thunk);
    XSRETURN (1);
