/*
 * Wine Message Compiler main program
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "wmc.h"
#include "utils.h"
#include "lang.h"
#include "write.h"

static const char usage[] =
	"Usage: wmc [options...] [inputfile.mc]\n"
	"   -B x                       Set output byte-order x={n[ative], l[ittle], b[ig]}\n"
	"                              (default is n[ative] which equals "
#ifdef WORDS_BIGENDIAN
	"big"
#else
	"little"
#endif
	"-endian)\n"
	"   -c                         Set 'custom-bit' in values\n"
	"   -d                         Use decimal values in output\n"
	"   -D                         Set debug flag\n"
	"   -h, --help                 Print this message\n"
	"   -H FILE                    Write header file to FILE (default is inputfile.h)\n"
	"   -i                         Inline messagetable(s)\n"
	"   --nls-dir=DIR              Directory containing the NLS codepage mappings\n"
	"   -o, --output=FILE          Output to FILE (default is infile.rc)\n"
	"   -O, --output-format=FORMAT The output format (`rc', `res', or `pot')\n"
	"   -P, --po-dir=DIR           Directory containing po files for translations\n"
	"   -u                         Input file is in unicode\n"
	"   -U                         Output unicode messagetable(s)\n"
	"   -v                         Show supported codepages and languages\n"
	"   -V, --version              Print version end exit\n"
	"   -W, --pedantic             Enable pedantic warnings\n"
	"Input is taken from stdin if no inputfile is specified.\n"
	"Byteorder of unicode input is based upon the first couple of\n"
	"bytes read, which should be 0x0000..0x00ff.\n"
	;

static const char version_string[] =
	"Wine Message Compiler version " PACKAGE_VERSION "\n"
	"Copyright 2000 Bertho A. Stultiens\n"
	;

/*
 * The output byte-order of resources (set with -B)
 */
int byteorder = WMC_BO_NATIVE;

/*
 * Custom bit (bit 29) in output values must be set (-c option)
 */
int custombit = 0;

/*
 * Output decimal values (-d option)
 */
int decimal = 0;

/*
 * Enable pedantic warnings; check arg references (-W option)
 */
int pedantic = 0;

/*
 * Unicode input (-u option)
 */
int unicodein = 0;

/*
 * Inline the messagetables (don't write *.bin files; -i option)
 */
int rcinline = 0;

/*
 * Debugging flag (-D option)
 */
static int dodebug = 0;

static char *po_dir;

char *output_name = NULL;	/* The name given by the -o option */
const char *input_name = NULL;	/* The name given on the command-line */
char *header_name = NULL;	/* The name given by the -H option */

const char *nlsdirs[3] = { NULL, NLSDIR, NULL };

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

char *cmdline;			/* The entire commandline */
time_t now;			/* The time of start of wmc */

int mcy_debug;

FILE *yyin;

static enum
{
    FORMAT_UNKNOWN,
    FORMAT_RC,
    FORMAT_RES,
    FORMAT_POT
} output_format;

enum long_options_values
{
    LONG_OPT_NLS_DIR = 1,
};

static const char short_options[] = "B:cdDhH:io:O:P:uUvVW";
static const struct long_option long_options[] =
{
	{ "help", 0, 'h' },
	{ "nls-dir", 1, LONG_OPT_NLS_DIR },
	{ "output", 1, 'o' },
	{ "output-format", 1, 'O' },
	{ "pedantic", 0, 'W' },
	{ "po-dir", 1, 'P' },
	{ "version", 0, 'v' },
        { NULL }
};

static void segvhandler(int sig);

static void cleanup_files(void)
{
    if (output_name) unlink( output_name );
    if (header_name) unlink( header_name );
}

static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static void init_argv0_dir( const char *argv0 )
{
#ifndef _WIN32
    char *dir;

#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    dir = realpath( "/proc/self/exe", NULL );
#elif defined (__FreeBSD__) || defined(__DragonFly__)
    dir = realpath( "/proc/curproc/file", NULL );
#else
    dir = realpath( argv0, NULL );
#endif
    if (!dir) return;
    dir = get_dirname( dir );
    if (strendswith( dir, "/tools/wmc" )) nlsdirs[0] = strmake( "%s/../../nls", dir );
    else nlsdirs[0] = strmake( "%s/%s", dir, BIN_TO_NLSDIR );
#endif
}

static void option_callback( int optc, char *optarg )
{
    switch(optc)
    {
    case 'B':
        switch(optarg[0])
        {
        case 'n':
        case 'N':
            byteorder = WMC_BO_NATIVE;
            break;
        case 'l':
        case 'L':
            byteorder = WMC_BO_LITTLE;
            break;
        case 'b':
        case 'B':
            byteorder = WMC_BO_BIG;
            break;
        default:
            error("Byteordering must be n[ative], l[ittle] or b[ig]\n");
        }
        break;
    case 'c':
        custombit = 1;
        break;
    case 'd':
        decimal = 1;
        break;
    case 'D':
        dodebug = 1;
        break;
    case 'h':
        printf("%s", usage);
        exit(0);
        /* No return */
    case 'H':
        header_name = xstrdup(optarg);
        break;
    case 'i':
        rcinline = 1;
        break;
    case 'o':
        output_name = xstrdup(optarg);
        break;
    case 'O':
        if (!strcmp( optarg, "rc" )) output_format = FORMAT_RC;
        else if (!strcmp( optarg, "res" )) output_format = FORMAT_RES;
        else if (!strcmp( optarg, "pot" )) output_format = FORMAT_POT;
        else error("Output format must be rc or res\n" );
        break;
    case 'P':
        po_dir = xstrdup( optarg );
        break;
    case 'u':
        unicodein = 1;
        break;
    case 'U':  /* ignored for backwards compatibility */
        break;
    case 'v':
        show_languages();
        exit(0);
        /* No return */
    case 'V':
        printf(version_string);
        exit(0);
        /* No return */
    case 'W':
        pedantic = 1;
        break;
    case LONG_OPT_NLS_DIR:
        nlsdirs[0] = xstrdup( optarg );
        break;
    case '?':
        fprintf(stderr, "wmc: %s\n\n%s", optarg, usage);
        exit(1);
    }
}

int main(int argc,char *argv[])
{
	int ret;
	int i;
	int cmdlen;
        struct strarray files;

	atexit( cleanup_files );
	signal(SIGSEGV, segvhandler);
	signal( SIGTERM, exit_on_signal );
	signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
	signal( SIGHUP, exit_on_signal );
#endif
        init_argv0_dir( argv[0] );

	/* First rebuild the commandline to put in destination */
	/* Could be done through env[], but not all OS-es support it */
	cmdlen = 5; /* for "wmc " and \0 */
	for(i = 1; i < argc; i++)
		cmdlen += strlen(argv[i]) + 1;
	cmdline = xmalloc(cmdlen);
	strcpy(cmdline, "wmc ");
	for(i = 1; i < argc; i++)
	{
		strcat(cmdline, argv[i]);
		if(i < argc-1)
			strcat(cmdline, " ");
	}

        files = parse_options( argc, argv, short_options, long_options, 0, option_callback );

	mcy_debug = dodebug;
	if(dodebug)
	{
		setbuf(stdout, NULL);
		setbuf(stderr, NULL);
	}

	/* Check for input file on command-line */
        if (files.count) input_name = files.str[0];

        /* Guess output format */
        if (output_format == FORMAT_UNKNOWN)
        {
            if (output_name && strendswith( output_name, ".res" )) output_format = FORMAT_RES;
            else if (output_name && strendswith( output_name, ".pot" )) output_format = FORMAT_POT;
            else output_format = FORMAT_RC;
        }

	/* Generate appropriate outfile names */
	if(!output_name)
	{
            const char *name = input_name ? get_basename(input_name) : "wmc.tab";
            output_name = replace_extension( name, ".mc", ".rc" );
	}

	if(!header_name)
	{
            const char *name = input_name ? get_basename(input_name) : "wmc.tab";
            header_name = replace_extension( name, ".mc", ".h" );
	}

	if(input_name)
	{
		if(!(yyin = fopen(input_name, "rb")))
			error("Could not open %s for input\n", input_name);
	}
	else
		yyin = stdin;

	ret = mcy_parse();

	if(input_name)
		fclose(yyin);

	if(ret)
	{
		/* Error during parse */
		exit(1);
	}

#ifdef WORDS_BIGENDIAN
	byte_swapped = (byteorder == WMC_BO_LITTLE);
#else
	byte_swapped = (byteorder == WMC_BO_BIG);
#endif

        switch (output_format)
        {
        case FORMAT_RC:
            write_h_file(header_name);
            write_rc_file(output_name);
            if(!rcinline)
		write_bin_files();
            break;
        case FORMAT_RES:
            add_translations( po_dir );
            write_res_file( output_name );
            break;
        case FORMAT_POT:
            write_pot_file( output_name );
            break;
        default:
            break;
        }
	output_name = NULL;
	header_name = NULL;
	return 0;
}

static void segvhandler(int sig)
{
	fprintf(stderr, "\n%s:%d: Oops, segment violation\n", input_name, line_number);
	fflush(stdout);
	fflush(stderr);
	abort();
}
