/*
 *  Option processing and main()
 *
 *  Copyright 2000 Jon Griffiths
 */
#include "specmaker.h"


_globals globals; /* All global variables */


static void do_include (const char *arg)
{
  globals.directory = arg;
  globals.do_code = 1;
}


static inline const char* strip_ext (const char *str)
{
  char *ext = strstr(str, ".dll");
  if (ext)
    return str_substring (str, ext);
  else
    return strdup (str);
}


static void do_name (const char *arg)
{
  globals.dll_name = strip_ext (arg);
}


static void do_input (const char *arg)
{
  globals.input_name = strip_ext (arg);
}


static void do_demangle (const char *arg)
{
  globals.do_demangle = 1;
  globals.do_code = 1;
  globals.input_name = arg;
}


static void do_code (void)
{
  globals.do_code = 1;
}


static void do_trace (void)
{
  globals.do_trace = 1;
  globals.do_code = 1;
}


static void do_forward (const char *arg)
{
  globals.forward_dll = arg;
  globals.do_trace = 1;
  globals.do_code = 1;
}

static void do_document (void)
{
  globals.do_documentation = 1;
}

static void do_cdecl (void)
{
  globals.do_cdecl = 1;
}


static void do_quiet (void)
{
  globals.do_quiet = 1;
}


static void do_start (const char *arg)
{
  globals.start_ordinal = atoi (arg);
  if (!globals.start_ordinal)
    fatal ("Invalid -s option (must be numeric)");
}


static void do_end (const char *arg)
{
  globals.end_ordinal = atoi (arg);
  if (!globals.end_ordinal)
    fatal ("Invalid -e option (must be numeric)");
}


static void do_verbose (void)
{
  globals.do_verbose = 1;
}


struct option
{
  const char *name;
  int   has_arg;
  void  (*func) ();
  const char *usage;
};


static const struct option option_table[] = {
  {"-d", 1, do_input,    "-d dll   Use dll for input file (mandatory)"},
  {"-S", 1, do_demangle, "-S sym   Demangle C++ symbol 'sym' and exit"},
  {"-h", 0, do_usage,    "-h       Display this help message"},
  {"-I", 1, do_include,  "-I dir   Look for prototypes in 'dir' (implies -c)"},
  {"-o", 1, do_name,     "-o name  Set the output dll name (default: dll)"},
  {"-c", 0, do_code,     "-c       Generate skeleton code (requires -I)"},
  {"-t", 0, do_trace,    "-t       TRACE arguments (implies -c)"},
  {"-f", 1, do_forward,  "-f dll   Forward calls to 'dll' (implies -t)"},
  {"-D", 0, do_document, "-D       Generate documentation"},
  {"-C", 0, do_cdecl,    "-C       Assume __cdecl calls (default: __stdcall)"},
  {"-s", 1, do_start,    "-s num   Start prototype search after symbol 'num'"},
  {"-e", 1, do_end,      "-e num   End prototype search after symbol 'num'"},
  {"-q", 0, do_quiet,    "-q       Don't show progress (quiet)."},
  {"-v", 0, do_verbose,  "-v       Show lots of detail while working (verbose)."},
  {NULL, 0, NULL, NULL}
};


void do_usage (void)
{
  const struct option *opt;
  printf ("Usage: specmaker [options] [-d dll | -S sym]\n\nOptions:\n");
  for (opt = option_table; opt->name; opt++)
    printf ("   %s\n", opt->usage);
  puts ("\n");
  exit (1);
}


/*******************************************************************
 *          parse_options
 *
 * Parse options from the argv array
 */
static void parse_options (char *argv[])
{
  const struct option *opt;
  char *const *ptr;
  const char *arg = NULL;

  ptr = argv + 1;

  while (*ptr != NULL)
  {
    for (opt = option_table; opt->name; opt++)
    {
      if (opt->has_arg && !strncmp (*ptr, opt->name, strlen (opt->name)))
      {
        arg = *ptr + strlen (opt->name);
        if (*arg == '\0')
        {
          ptr++;
          arg = *ptr;
        }
        break;
      }
      if (!strcmp (*ptr, opt->name))
      {
        arg = NULL;
        break;
      }
    }

    if (!opt->name)
      fatal ("Unrecognized option");

    if (opt->has_arg && arg != NULL)
      opt->func (arg);
    else
      opt->func ("");

    ptr++;
  }

  if (!globals.do_demangle && globals.do_code && !globals.directory)
    fatal ("-I must be used if generating code");

  if (!globals.input_name)
    fatal ("Option -d is mandatory");

  if (VERBOSE && QUIET)
    fatal ("Options -v and -q are mutually exclusive");
}


/*******************************************************************
 *         main
 */
#ifdef __GNUC__
int   main (int argc __attribute__((unused)), char *argv[])
#else
int   main (int argc, char *argv[])
#endif
{
  parsed_symbol symbol;
  int count = 0;

  parse_options (argv);

  memset (&symbol, 0, sizeof (parsed_symbol));

  if (globals.do_demangle)
  {
    int result;
    globals.uc_dll_name = "";
    VERBOSE = 1;
    symbol.symbol = strdup(globals.input_name);
    result = symbol_demangle (&symbol);
    if (symbol.flags & SYM_DATA)
      printf (symbol.arg_text[0]);
    else
      output_prototype (stdout, &symbol);
    fputc ('\n', stdout);
    return result ? 1 : 0;
  }

  dll_open (globals.input_name);

  output_spec_preamble ();
  output_header_preamble ();
  output_c_preamble ();

  while (!dll_next_symbol (&symbol))
  {
    count++;

    if (NORMAL)
      printf ("Export %3d - '%s' ...%c", count, symbol.symbol,
              VERBOSE ? '\n' : ' ');

    if (globals.do_code && count >= globals.start_ordinal
        && (!globals.end_ordinal || count <= globals.end_ordinal))
    {
      /* Attempt to get information about the symbol */
      int result = symbol_demangle (&symbol);

      if (result)
        result = symbol_search (&symbol);

      if (!result && symbol.function_name)
      /* Clean up the prototype */
        symbol_clean_string (symbol.function_name);

      if (NORMAL)
        puts (result ? "[Not Found]" : "[OK]");
    }
    else if (NORMAL)
      puts ("[Ignoring]");

    output_spec_symbol (&symbol);
    output_header_symbol (&symbol);
    output_c_symbol (&symbol);

    symbol_clear (&symbol);
  }

  output_makefile ();
  output_install_script ();

  if (VERBOSE)
    puts ("Finished, Cleaning up...");

  return 0;
}
