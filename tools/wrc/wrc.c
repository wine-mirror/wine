/*
 *
 * Copyright  Martin von Loewis, 1994
 * Copyrignt 1998 Bertho A. Stultiens (BS)
 *
 * 30-Apr-2000 BS	- Integrated a new preprocessor (-E and -N)
 * 20-Jun-1998 BS	- Added -L option to prevent case conversion
 *			  of embedded filenames.
 *
 * 08-Jun-1998 BS	- Added -A option to generate autoregister code
 *			  for winelib operation.
 *
 * 21-May-1998 BS	- Removed the CPP option. Its internal now.
 *			- Added implementations for defines and includes
 *			  on the commandline.
 *
 * 30-Apr-1998 BS	- The options now contain nearly the entire alphabet.
 *			  Seems to be a sign for too much freedom. I implemeted
 *			  most of them as a user choice possibility for things
 *			  that I do not know what to put there by default.
 *			- -l and -L options are now known as -t and -T.
 *
 * 23-Apr-1998 BS	- Finally gave up on backward compatibility on the
 *			  commandline (after a blessing from the newsgroup).
 *			  So, I changed the lot.
 *
 * 17-Apr-1998 BS	- Added many new command-line options but took care
 *			  that it would not break old scripts (sigh).
 *
 * 16-Apr-1998 BS	- There is not much left of the original source...
 *			  I had to rewrite most of it because the parser
 *			  changed completely with all the types etc..
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <endian.h>

#include "wrc.h"
#include "utils.h"
#include "writeres.h"
#include "readres.h"
#include "dumpres.h"
#include "genres.h"
#include "newstruc.h"
#include "preproc.h"
#include "parser.h"

#ifndef BYTE_ORDER
# error BYTE_ORDER is not defined. Please report.
#endif

#if (!defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)) || (BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN)
# error Neither BIG_ENDIAN nor LITTLE_ENDIAN system. This system is not supported. Please report.
#endif


static char usage[] =
	"Usage: wrc [options...] [infile[.rc|.res]]\n"
	"   -a n        Alignment of resource (win16 only, default is 4)\n"
	"   -A          Auto register resources (only with gcc 2.7 and better)\n"
	"   -b          Create an assembly array from a binary .res file\n"
	"   -B x        Set output byte-order x={n[ative], l[ittle], b[ig]}\n"
	"               (win32 only; default is n[ative] which equals "
#if BYTE_ORDER == BIG_ENDIAN
	"big"
#else
	"little"
#endif
	"-endian)\n"
	"   -c          Add 'const' prefix to C constants\n"
	"   -C cp       Set the resource's codepage to cp (default is 0)\n"
	"   -d n        Set debug level to 'n'\n"
	"   -D id[=val] Define preprocessor identifier id=val\n"
	"   -e          Disable recognition of win32 keywords in 16bit compile\n"
	"   -E          Preprocess only\n"
	"   -g          Add symbols to the global c namespace\n"
	"   -h          Also generate a .h file\n"
	"   -H file     Same as -h but written to file\n"
	"   -I path     Set include search dir to path (multiple -I allowed)\n"
	"   -l lan      Set default language to lan (default is neutral {0, 0})\n"
	"   -L          Leave case of embedded filenames as is\n"
	"   -n          Do not generate .s file\n"
	"   -N          Do not preprocess input\n"
	"   -o file     Output to file (default is infile.[res|s|h]\n"
	"   -p prefix   Give a prefix for the generated names\n"
	"   -r          Create binary .res file (compile only)\n"
	"   -s          Add structure with win32/16 (PE/NE) resource directory\n"
	"   -t          Generate indirect loadable resource tables\n"
	"   -T          Generate only indirect loadable resources tables\n"
	"   -V          Print version end exit\n"
	"   -w 16|32    Select win16 or win32 output (default is win32)\n"
	"   -W          Enable pedantic warnings\n"
	"Input is taken from stdin if no sourcefile specified.\n"
	"Debug level 'n' is a bitmask with following meaning:\n"
	"    * 0x01 Tell which resource is parsed (verbose mode)\n"
	"    * 0x02 Dump internal structures\n"
	"    * 0x04 Create a parser trace (yydebug=1)\n"
	"    * 0x08 Preprocessor messages\n"
	"    * 0x10 Preprocessor lex messages\n"
	"    * 0x20 Preprocessor yacc trace\n"
	"The -o option only applies to the final destination file, which is\n"
	"in case of normal compile a .s file. You must use the '-H header.h'\n"
	"option to override the header-filename.\n"
	"If no input filename is given and the output name is not overridden\n"
	"with -o and/or -H, then the output is written to \"wrc.tab.[sh]\"\n"
	;

char version_string[] = "Wine Resource Compiler Version " WRC_FULLVERSION "\n"
			"Copyright 1998-2000 Bertho A. Stultiens\n"
			"          1994 Martin von Loewis\n";

/*
 * Default prefix for resource names used in the C array.
 * Option '-p name' sets it to 'name'
 */
#ifdef NEED_UNDERSCORE_PREFIX
char *prefix = "__Resource";
#else
char *prefix = "_Resource";
#endif

/*
 * Set if compiling in 32bit mode (default).
 */
int win32 = 1;

/*
 * Set when generated C variables should be prefixed with 'const'
 */
int constant = 0;

/*
 * Create a .res file from the source and exit (-r option).
 */
int create_res = 0;

/*
 * debuglevel == DEBUGLEVEL_NONE	Don't bother
 * debuglevel & DEBUGLEVEL_CHAT		Say whats done
 * debuglevel & DEBUGLEVEL_DUMP		Dump internal structures
 * debuglevel & DEBUGLEVEL_TRACE	Create parser trace
 * debuglevel & DEBUGLEVEL_PPMSG	Preprocessor messages
 * debuglevel & DEBUGLEVEL_PPLEX	Preprocessor lex trace
 * debuglevel & DEBUGLEVEL_PPTRACE	Preprocessor yacc trace
 */
int debuglevel = DEBUGLEVEL_NONE;

/*
 * Recognize win32 keywords if set (-w 32 enforces this),
 * otherwise set with -e option.
 */
int extensions = 1;

/*
 * Set when creating C array from .res file (-b option).
 */
int binary = 0;

/*
 * Set when an additional C-header is to be created in compile (-h option).
 */
int create_header = 0;

/*
 * Set when the NE/PE resource directory should be dumped into
 * the output file.
 */
int create_dir = 0;

/*
 * Set when all symbols should be added to the global namespace (-g option)
 */
int global = 0;

/*
 * Set when indirect loadable resource tables should be created (-t)
 */
int indirect = 0;

/*
 * Set when _only_ indirect loadable resource tables should be created (-T)
 */
int indirect_only = 0;

/*
 * NE segment resource aligment (-a option)
 */
int alignment = 4;
int alignment_pwr;

/*
 * Cleared when the assembly file must be suppressed (-n option)
 */
int create_s = 1;

/*
 * Language setting for resources (-l option)
 */
language_t *currentlanguage = NULL;

/*
 * The codepage to write in win32 PE resource segment (-C option)
 */
DWORD codepage = 0;

/*
 * Set when extra warnings should be generated (-W option)
 */
int pedantic = 0;

/*
 * Set when autoregister code must be added to the output (-A option)
 */
int auto_register = 0;

/*
 * Set when the case of embedded filenames should not be converted
 * to lower case (-L option)
 */
int leave_case = 0;

/*
 * The output byte-order of resources (set with -B)
 */
int byteorder = WRC_BO_NATIVE;

/*
 * Set when _only_ to run the preprocessor (-E option)
 */
int preprocess_only = 0;

/*
 * Set when _not_ to run the preprocessor (-N option)
 */
int no_preprocess = 0;

char *output_name;		/* The name given by the -o option */
char *input_name;		/* The name given on the command-line */
char *header_name;		/* The name given by the -H option */
char *temp_name;		/* Temporary file for preprocess pipe */

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

char *cmdline;			/* The entire commandline */
time_t now;			/* The time of start of wrc */

resource_t *resource_top;	/* The top of the parsed resources */

int getopt (int argc, char *const *argv, const char *optstring);
static void rm_tempfile(void);
static void segvhandler(int sig);

int main(int argc,char *argv[])
{
	extern char* optarg;
	extern int   optind;
	int optc;
	int lose = 0;
	int ret;
	int a;
	int i;
	int cmdlen;

	signal(SIGSEGV, segvhandler);

	now = time(NULL);

	/* First rebuild the commandline to put in destination */
	/* Could be done through env[], but not all OS-es support it */
	cmdlen = 4; /* for "wrc " */
	for(i = 1; i < argc; i++)
		cmdlen += strlen(argv[i]) + 1;
	cmdline = (char *)xmalloc(cmdlen);
	strcpy(cmdline, "wrc ");
	for(i = 1; i < argc; i++)
	{
		strcat(cmdline, argv[i]);
		if(i < argc-1)
			strcat(cmdline, " ");
	}

	while((optc = getopt(argc, argv, "a:AbB:cC:d:D:eEghH:I:l:LnNo:p:rstTVw:W")) != EOF)
	{
		switch(optc)
		{
		case 'a':
			alignment = atoi(optarg);
			break;
		case 'A':
			auto_register = 1;
			break;
		case 'b':
			binary = 1;
			break;
		case 'B':
			switch(optarg[0])
			{
			case 'n':
			case 'N':
				byteorder = WRC_BO_NATIVE;
				break;
			case 'l':
			case 'L':
				byteorder = WRC_BO_LITTLE;
				break;
			case 'b':
			case 'B':
				byteorder = WRC_BO_BIG;
				break;
			default:
				fprintf(stderr, "Byteordering must be n[ative], l[ittle] or b[ig]\n");
				lose++;
			}
			break;
		case 'c':
			constant = 1;
			break;
		case 'C':
			codepage = strtol(optarg, NULL, 0);
			break;
		case 'd':
			debuglevel = strtol(optarg, NULL, 0);
			break;
		case 'D':
			add_cmdline_define(optarg);
			break;
		case 'e':
			extensions = 0;
			break;
		case 'E':
			preprocess_only = 1;
			break;
		case 'g':
			global = 1;
			break;
		case 'H':
			header_name = strdup(optarg);
			/* Fall through */
		case 'h':
			create_header = 1;
			break;
		case 'I':
			add_include_path(optarg);
			break;
		case 'l':
			{
				int lan;
				lan = strtol(optarg, NULL, 0);
				currentlanguage = new_language(PRIMARYLANGID(lan), SUBLANGID(lan));
			}
			break;
		case 'L':
			leave_case = 1;
			break;
		case 'n':
			create_s = 0;
			break;
		case 'N':
			no_preprocess = 1;
			break;
		case 'o':
			output_name = strdup(optarg);
			break;
		case 'p':
#ifdef NEED_UNDERSCORE_PREFIX
			prefix = (char *)xmalloc(strlen(optarg)+2);
			prefix[0] = '_';
			strcpy(prefix+1, optarg);
#else
			prefix = xstrdup(optarg);
#endif
			break;
		case 'r':
			create_res = 1;
			break;
		case 's':
			create_dir = 1;
			break;
		case 'T':
			indirect_only = 1;
			/* Fall through */
		case 't':
			indirect = 1;
			break;
		case 'V':
			printf(version_string);
			exit(0);
			break;
		case 'w':
			if(!strcmp(optarg, "16"))
				win32 = 0;
			else if(!strcmp(optarg, "32"))
				win32 = 1;
			else
				lose++;
			break;
		case 'W':
			pedantic = 1;
			break;
		default:
			lose++;
			break;
		}
	}

	if(lose)
	{
		fprintf(stderr, usage);
		return 1;
	}

	/* Check the command line options for invalid combinations */
	if(win32)
	{
		if(!extensions)
		{
			warning("Option -e ignored with 32bit compile\n");
			extensions = 1;
		}
	}

	if(create_res)
	{
		if(constant)
		{
			warning("Option -c ignored with compile to .res\n");
			constant = 0;
		}

		if(create_header)
		{
			warning("Option -[h|H] ignored with compile to .res\n");
			create_header = 0;
		}

		if(indirect)
		{
			warning("Option -l ignored with compile to .res\n");
			indirect = 0;
		}

		if(indirect_only)
		{
			warning("Option -L ignored with compile to .res\n");
			indirect_only = 0;
		}

		if(global)
		{
			warning("Option -g ignored with compile to .res\n");
			global = 0;
		}

		if(create_dir)
		{
			error("Option -r and -s cannot be used together\n");
		}

		if(binary)
		{
			error("Option -r and -b cannot be used together\n");
		}
	}

	if(byteorder != WRC_BO_NATIVE)
	{
		if(binary)
			error("Forced byteordering not supported for binary resources\n");
	}

	if(preprocess_only)
	{
		if(constant)
		{
			warning("Option -c ignored with preprocess only\n");
			constant = 0;
		}

		if(create_header)
		{
			warning("Option -[h|H] ignored with preprocess only\n");
			create_header = 0;
		}

		if(indirect)
		{
			warning("Option -l ignored with preprocess only\n");
			indirect = 0;
		}

		if(indirect_only)
		{
			error("Option -E and -L cannot be used together\n");
		}

		if(global)
		{
			warning("Option -g ignored with preprocess only\n");
			global = 0;
		}

		if(create_dir)
		{
			warning("Option -s ignored with preprocess only\n");
			create_dir = 0;
		}

		if(binary)
		{
			error("Option -E and -b cannot be used together\n");
		}

		if(no_preprocess)
		{
			error("Option -E and -N cannot be used together\n");
		}
	}

#if !defined(HAVE_WINE_CONSTRUCTOR)
	if(auto_register)
	{
		warning("Autoregister code non-operable (HAVE_WINE_CONSTRUCTOR not defined)");
		auto_register = 0;
	}
#endif

	/* Set alignment power */
	a = alignment;
	for(alignment_pwr = 0; alignment_pwr < 10 && a > 1; alignment_pwr++)
	{
		a >>= 1;
	}
	if(a != 1)
	{
		error("Alignment must be between 1 and 1024");
	}
	if((1 << alignment_pwr) != alignment)
	{
		error("Alignment must be a power of 2");
	}

	/* Kill io buffering when some kind of debuglevel is enabled */
	if(debuglevel)
	{
		setbuf(stdout,0);
		setbuf(stderr,0);
	}

	yydebug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
	yy_flex_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
	ppdebug = debuglevel & DEBUGLEVEL_PPTRACE ? 1 : 0;
	pp_flex_debug = debuglevel & DEBUGLEVEL_PPLEX ? 1 : 0;

	/* Set the default defined stuff */
	add_cmdline_define("__WRC__=" WRC_STRINGIZE(WRC_MAJOR_VERSION));
	add_cmdline_define("__WRC_MINOR__=" WRC_STRINGIZE(WRC_MINOR_VERSION));
	add_cmdline_define("__WRC_MICRO__=" WRC_STRINGIZE(WRC_MICRO_VERSION));
	add_cmdline_define("__WRC_PATCH__=" WRC_STRINGIZE(WRC_MICRO_VERSION));

	add_cmdline_define("RC_INVOKED=1");

	if(win32)
	{
		add_cmdline_define("__WIN32__=1");
		add_cmdline_define("__FLAT__=1");
	}

	add_special_define("__FILE__");
	add_special_define("__LINE__");
	add_special_define("__DATE__");
	add_special_define("__TIME__");

	/* Check if the user set a language, else set default */
	if(!currentlanguage)
		currentlanguage = new_language(0, 0);

	/* Check for input file on command-line */
	if(optind < argc)
	{
		input_name = argv[optind];
	}

	if(binary && !input_name)
	{
		error("Binary mode requires .res file as input\n");
	}

	/* Generate appropriate outfile names */
	if(!output_name && !preprocess_only)
	{
		output_name = dup_basename(input_name, binary ? ".res" : ".rc");
		strcat(output_name, create_res ? ".res" : ".s");
	}

	if(!header_name && !create_res)
	{
		header_name = dup_basename(input_name, binary ? ".res" : ".rc");
		strcat(header_name, ".h");
	}

	/* Run the preprocessor on the input */
	if(!no_preprocess && !binary)
	{
		char *real_name;
		/*
		 * Preprocess the input to a temp-file, or stdout if
		 * no output was given.
		 */

		chat("Starting preprocess");

		if(preprocess_only && !output_name)
		{
			ppout = stdout;
		}
		else if(preprocess_only && output_name)
		{
			if(!(ppout = fopen(output_name, "wb")))
				error("Could not open %s for writing\n", output_name);
		}
		else
		{
			if(!(temp_name = tmpnam(NULL)))
				error("Could nor generate a temp-name\n");
			temp_name = xstrdup(temp_name);
			if(!(ppout = fopen(temp_name, "wb")))
				error("Could not create a temp-file\n");

			atexit(rm_tempfile);
		}

		real_name = input_name;	/* Because it gets overwritten */

		if(!input_name)
			ppin = stdin;
		else
		{
			if(!(ppin = fopen(input_name, "rb")))
				error("Could not open %s\n", input_name);
		}

		fprintf(ppout, "# 1 \"%s\" 1\n", input_name ? input_name : "");

		ret = ppparse();

		input_name = real_name;

		if(input_name)
			fclose(ppin);

		fclose(ppout);

		input_name = temp_name;

		if(ret)
			exit(1);	/* Error during preprocess */

		if(preprocess_only)
			exit(0);
	}

	if(!binary)
	{
		/* Go from .rc to .res or .s */
		chat("Starting parse");

		if(!(yyin = fopen(input_name, "rb")))
			error("Could not open %s for input\n", input_name);

		ret = yyparse();

		if(input_name)
			fclose(yyin);

		if(ret)
		{
			/* Error during parse */
			exit(1);
		}

		if(debuglevel & DEBUGLEVEL_DUMP)
			dump_resources(resource_top);

		/* Convert the internal lists to binary data */
		resources2res(resource_top);

		if(create_res)
		{
			chat("Writing .res-file");
			write_resfile(output_name, resource_top);
		}
		else
		{
			if(create_s)
			{
				chat("Writing .s-file");
				write_s_file(output_name, resource_top);
			}
			if(create_header)
			{
				chat("Writing .h-file");
				write_h_file(header_name, resource_top);
			}
		}

	}
	else
	{
		/* Go from .res to .s */
		chat("Reading .res-file");
		resource_top = read_resfile(input_name);
		if(create_s)
		{
			chat("Writing .s-file");
			write_s_file(output_name, resource_top);
		}
		if(create_header)
		{
			chat("Writing .h-file");
			write_h_file(header_name, resource_top);
		}
	}

	return 0;
}


static void rm_tempfile(void)
{
	if(temp_name)
		unlink(temp_name);
}

static void segvhandler(int sig)
{
	fprintf(stderr, "\n%s:%d: Oops, segment violation\n", input_name, line_number);
	fflush(stdout);
	fflush(stderr);
	abort();
}

