/*
 * Perl interpreter for running Wine tests
 */

#include <stdio.h>

#include "windef.h"
#include "winbase.h"

#include <EXTERN.h>
#include <perl.h>

/*----------------------------------------------------------------------
| Function:    call_wine_func                                          |
| -------------------------------------------------------------------- |
| Purpose:     Call a wine API function, passing in appropriate number |
|              of args                                                 |
|                                                                      |
| Parameters:  proc   -- function to call                              |
|              n_args -- array of args                                 |
|              a      -- array of args                                 |
|                                                                      |
| Returns:     return value from API function called                   |
----------------------------------------------------------------------*/
static unsigned long call_wine_func
(
    FARPROC        proc,
    int            n_args,
    unsigned long  *a
)
{
    /* Locals */
    unsigned long  rc;

/* Begin call_wine_func */

    /*--------------------------------------------------------------
    | Now we need to call the function with the appropriate number
    | of arguments
    |
    | Anyone who can think of a better way to do this is welcome to
    | come forth with it ...
    --------------------------------------------------------------*/
    switch (n_args)
    {

        case 0:  rc = proc (); break;
        case 1:  rc = proc (a[0]); break;
        case 2:  rc = proc (a[0], a[1]); break;
        case 3:  rc = proc (a[0], a[1], a[2]); break;
        case 4:  rc = proc (a[0], a[1], a[2], a[3]); break;
        case 5:  rc = proc (a[0], a[1], a[2], a[3], a[4]); break;
        case 6:  rc = proc (a[0], a[1], a[2], a[3], a[4], a[5]); break;
        case 7:  rc = proc (a[0], a[1], a[2], a[3], a[4], a[5], a[6]); break;
        case 8:  rc = proc (a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]); break;
        case 9:  rc = proc (a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); break;
        case 10: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9] ); break;
        case 11: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9], a[10] ); break;
        case 12: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9], a[10], a[11] ); break;
        case 13: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9], a[10], a[11], a[12] ); break;
        case 14: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9], a[10], a[11], a[12], a[13] ); break;
        case 15: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9], a[10], a[11], a[12], a[13], a[14] ); break;
        case 16: rc = proc( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                            a[9], a[10], a[11], a[12], a[13], a[14], a[15] ); break;
        default:
            fprintf( stderr, "%d args not supported\n", n_args );
            rc = 0;
            break;
    }

    /*--------------------------------------------------------------
    | Return value from func
    --------------------------------------------------------------*/
    return (rc);
}


/*----------------------------------------------------------------------
| Function:    perl_call_wine                                          |
| -------------------------------------------------------------------- |
| Purpose:     Fetch and call a wine API function from a library       |
|                                                                      |
| Parameters:                                                          |
|                                                                      |
|     module    -- module in function (ostensibly) resides             |
|     function  -- function name                                       |
|     n_args    -- number of args                                      |
|     args      -- args                                                |
|     last_error -- returns the last error code
|     debug     -- debug flag                                          |
|                                                                      |
| Returns:     Return value from API function called                   |
----------------------------------------------------------------------*/
unsigned long perl_call_wine
(
    char           *module,
    char           *function,
    int            n_args,
    unsigned long  *args,
    unsigned int   *last_error,
    int            debug
)
{
    /* Locals */
    HMODULE        hmod;
    FARPROC        proc;
    int            i;
    unsigned long ret, error, old_error;

    static FARPROC pGetLastError;

    /*--------------------------------------------------------------
    | Debug
    --------------------------------------------------------------*/
    if (debug)
    {
        fprintf(stderr,"    perl_call_wine(");
        for (i = 0; (i < n_args); i++)
            fprintf( stderr, "0x%lx%c", args[i], (i < n_args-1) ? ',' : ')' );
        fputc( '\n', stderr );
    }

    /*--------------------------------------------------------------
    | See if we can load specified module
    --------------------------------------------------------------*/
    if (!(hmod = GetModuleHandleA(module)))
    {
        fprintf( stderr, "GetModuleHandleA(%s) failed\n", module);
        exit(1);
    }

    /*--------------------------------------------------------------
    | See if we can get address of specified function from it
    --------------------------------------------------------------*/
    if ((proc = GetProcAddress (hmod, function)) == NULL)
    {
        fprintf (stderr, "    GetProcAddress(%s.%s) failed\n", module, function);
        exit(1);
    }

    /*--------------------------------------------------------------
    | Righty then; call the function ...
    --------------------------------------------------------------*/
    if (!pGetLastError)
        pGetLastError = GetProcAddress( GetModuleHandleA("kernel32"), "GetLastError" );

    if (proc == pGetLastError)
        ret = call_wine_func (proc, n_args, args);
    else
    {
        old_error = GetLastError();
        SetLastError( 0xdeadbeef );
        ret = call_wine_func (proc, n_args, args);
        error = GetLastError();
        if (error != 0xdeadbeef) *last_error = error;
        else SetLastError( old_error );
    }
    return ret;
}


/* perl extension initialisation */
static void xs_init(void)
{
    extern void boot_wine(CV *cv);
    newXS("wine::bootstrap", boot_wine,__FILE__);
}

/* main function */
int main( int argc, char **argv, char **envp )
{
    PerlInterpreter *perl;
    int status;

    envp = environ;  /* envp is not valid (yet) in Winelib */

    if (!(perl = perl_alloc ()))
    {
        fprintf( stderr, "Could not allocate perl interpreter\n" );
        exit(1);
    }
    perl_construct (perl);
    status = perl_parse( perl, xs_init, argc, argv, envp );
    if (!status) status = perl_run(perl);
    perl_destruct (perl);
    perl_free (perl);
    exit( status );
}
