/*
 * Option parsing
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"
#include <string.h>

#include "winbase.h"
#include "main.h"
#include "options.h"
#include "version.h"
#include "debugtools.h"

struct option
{
    const char *longname;
    char        shortname;
    int         has_arg;
    void      (*func)( const char *arg );
    const char *usage;
};

static void do_config( const char *arg );
static void do_desktop( const char *arg );
static void do_display( const char *arg );
static void do_dll( const char *arg );
static void do_help( const char *arg );
static void do_managed( const char *arg );
static void do_synchronous( const char *arg );
static void do_version( const char *arg );

static const struct option option_table[] =
{
    { "config",       0, 1, do_config,
      "--config name    Specify config file to use" },
    { "debugmsg",     0, 1, MAIN_ParseDebugOptions,
      "--debugmsg name  Turn debugging-messages on or off" },
    { "desktop",      0, 1, do_desktop,
      "--desktop geom   Use a desktop window of the given geometry" },
    { "display",      0, 1, do_display,
      "--display name   Use the specified display" },
    { "dll",          0, 1, do_dll,
      "--dll name       Enable or disable built-in DLLs" },
    { "dosver",       0, 1, VERSION_ParseDosVersion,
      "--dosver x.xx    DOS version to imitate (e.g. 6.22). Only valid with --winver win31" },
    { "help",       'h', 0, do_help,
      "--help,-h        Show this help message" },
    { "language",     0, 1, MAIN_ParseLanguageOption,
      "--language xx    Set the language (one of Br,Ca,Cs,Cy,Da,De,En,Eo,Es,Fi,Fr,Ga,Gd,Gv\n"
      "                    Hu,It,Ja,Ko,Kw,Nl,No,Pl,Pt,Sk,Sv,Ru,Wa)" },
    { "managed",      0, 0, do_managed,
      "--managed        Allow the window manager to manage created windows" },
    { "synchronous",  0, 0, do_synchronous,
      "--synchronous    Turn on synchronous display mode" },
    { "version",    'v', 0, do_version,
      "--version,-v     Display the Wine version" },
    { "winver",       0, 1, VERSION_ParseWinVersion,
      "--winver         Version to imitate (one of win31,win95,nt351,nt40)" },
    { NULL, }  /* terminator */
};


static void do_help( const char *arg )
{
    OPTIONS_Usage();
}

static void do_version( const char *arg )
{
    MESSAGE( "%s\n", WINE_RELEASE_INFO );
    ExitProcess(0);
}

static void do_synchronous( const char *arg )
{
    Options.synchronous = TRUE;
}

static void do_desktop( const char *arg )
{
    Options.desktopGeometry = strdup( arg );
}

static void do_display( const char *arg )
{
    Options.display = strdup( arg );
}

static void do_dll( const char *arg )
{
    if (Options.dllFlags)
    {
        /* don't overwrite previous value. Should we
         * automatically add the ',' between multiple DLLs ?
         */
        MESSAGE("Only one -dll flag is allowed. Use ',' between multiple DLLs\n");
        ExitProcess(1);
    }
    Options.dllFlags = strdup( arg );
}

static void do_managed( const char *arg )
{
    Options.managed = TRUE;
}

static void do_config( const char *arg )
{
    Options.configFileName = strdup( arg );
}

static inline void remove_options( int *argc, char *argv[], int pos, int count )
{
    while ((argv[pos] = argv[pos+count])) pos++;
    *argc -= count;
}

/***********************************************************************
 *              OPTIONS_Usage
 */
void OPTIONS_Usage(void)
{
    const struct option *opt;
    MESSAGE( "Usage: %s [options] \"program_name [arguments]\"\n\n", argv0 );
    MESSAGE( "Options:\n" );
    for (opt = option_table; opt->longname; opt++) MESSAGE( "   %s\n", opt->usage );
    ExitProcess(0);
}

/***********************************************************************
 *              OPTIONS_ParseOptions
 */
void OPTIONS_ParseOptions( int argc, char *argv[] )
{
    const struct option *opt;
    int i;

    for (i = 1; argv[i]; i++)
    {
        char *p = argv[i];
        if (*p++ != '-') continue;  /* not an option */
        if (*p && !p[1]) /* short name */
        {
            if (*p == '-') break; /* "--" option */
            for (opt = option_table; opt->longname; opt++) if (opt->shortname == *p) break;
        }
        else  /* long name */
        {
            if (*p == '-') p++;
            /* check for the long name */
            for (opt = option_table; opt->longname; opt++)
                if (!strcmp( p, opt->longname )) break;
        }
        if (!opt->longname) continue;

        if (opt->has_arg && argv[i+1])
        {
            opt->func( argv[i+1] );
            remove_options( &argc, argv, i, 2 );
        }
        else
        {
            opt->func( "" );
            remove_options( &argc, argv, i, 1 );
        }
        i--;
    }

    /* check if any option remains */
    for (i = 1; argv[i]; i++)
    {
        if (!strcmp( argv[i], "--" ))
        {
            remove_options( &argc, argv, i, 1 );
            break;
        }
        if (argv[i][0] == '-')
        {
            MESSAGE( "Unknown option '%s'\n", argv[i] );
            OPTIONS_Usage();
        }
    }
    Options.argc = argc;
    Options.argv = argv;
}
