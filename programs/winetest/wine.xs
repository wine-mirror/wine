/* -*-C-*- --------------------------------------------------------------------
| Module:      wine.xs                                                         |
| ---------------------------------------------------------------------------- |
| Purpose:     Perl gateway to wine API calls                                  |
|                                                                              |
| Functions:                                                                   |
|     call_wine_API -- call a wine API function                                |
|                                                                              |
------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

enum ret_type
{
    RET_VOID = 0,
    RET_INT  = 1,
    RET_WORD = 2,
    RET_PTR  = 3
};

/* max arguments for a function call */
#define MAX_ARGS    16

extern unsigned long perl_call_wine
(
    char           *module,
    char           *function,
    int            n_args,
    unsigned long  *args,
    unsigned int   *last_error,
    int            debug
);


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
    # Parameters:  module   -- module (dll) to get function from
    #              function -- API function to call
    #              ret_type -- return type
    #              debug    -- debug flag
    #              ...      -- args to pass to API function
    #
    # Returns:     list containing 2 elements: the last error code and the
    #              value returned by the API function
    # --------------------------------------------------------------------
void
call_wine_API(module, function, ret_type, debug, ...)
    char  *module;
    char  *function;
    int   ret_type;
    int   debug;

    PROTOTYPE: $$$$@

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
    int            n_fixed = 4;
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
    if (debug)
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
                if (debug)
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
                if (debug)
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
                if (debug)
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
                if (debug)
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
                if (debug)
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
                    if (debug)
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
    r = perl_call_wine
    (
        module,
        function,
        n_args,
        f_args,
        &last_error,
        debug
    );

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
    switch (ret_type)
    {
        case RET_VOID: PUSHs( &PL_sv_undef ); break;
        case RET_INT:  PUSHs(sv_2mortal(newSViv( (int)r ))); break;
        case RET_WORD: PUSHs(sv_2mortal(newSViv( (int)r & 0xffff ))); break;
        case RET_PTR:  PUSHs(sv_2mortal(newSVpv( (char *)r, 0 ))); break;
        default:       croak( "Bad return type %d", ret_type ); break;
    }

    /*--------------------------------------------------------------
    | Free up allocated memory
    --------------------------------------------------------------*/
    for (i = 0; (i < n_args); i++)
    {
        if (args[i].pval) free(args[i].pval);
    }

    /*--------------------------------------------------------------
    | End call_wine_API
    --------------------------------------------------------------*/


    # --------------------------------------------------------------------
    # Function:    load_library
    # --------------------------------------------------------------------
    # Purpose:     Load a Wine library
    #
    # Parameters:  module   -- module (dll) to load
    #
    # Returns:     module handle
    # --------------------------------------------------------------------
unsigned int
load_library(module)
    char  *module;
    PROTOTYPE: $
