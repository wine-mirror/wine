/*
 *  DLL symbol extraction
 *
 *  Copyright 2000 Jon Griffiths
 */
#include "specmaker.h"

/* DOS/PE Header details */
#define DOS_HEADER_LEN      64
#define DOS_MAGIC           0x5a4d
#define DOS_PE_OFFSET       60
#define PE_HEADER_LEN       248
#define PE_MAGIC            0x4550
#define PE_COUNT_OFFSET     6
#define PE_EXPORTS_OFFSET   120
#define PE_EXPORTS_SIZE     PE_EXPORTS_OFFSET + 4
#define SECTION_HEADER_LEN  40
#define SECTION_ADDR_OFFSET 12
#define SECTION_ADDR_SIZE   SECTION_ADDR_OFFSET + 4
#define SECTION_POS_OFFSET  SECTION_ADDR_SIZE + 4
#define EXPORT_COUNT_OFFSET 24
#define EXPORT_NAME_OFFSET  EXPORT_COUNT_OFFSET + 8

/* Minimum memory needed to read both headers into a buffer */
#define MIN_HEADER_LEN (PE_HEADER_LEN * sizeof (unsigned char))

/* Normalise a pointer in the exports section */
#define REBASE(x) ((x) - exports)

/* Module globals */
static FILE  *dll_file = NULL;
static char **dll_symbols = NULL;
static size_t dll_num_exports = 0;


/* Get a short from a memory block */
static inline size_t get_short (const char *mem)
{
  return *((const unsigned char *)mem) +
         (*((const unsigned char *)mem + 1) << 8);
}

/* Get an integer from a memory block */
static inline size_t get_int (const char *mem)
{
  assert (sizeof (char) == (size_t)1);
  return get_short (mem) + (get_short (mem + 2) << 16);
}

static void dll_close (void);


/*******************************************************************
 *         dll_open
 *
 * Open a DLL and read in exported symbols
 */
void  dll_open (const char *dll_name)
{
  size_t code = 0, code_len = 0, exports, exports_len, count, symbol_data;
  char *buff = NULL;
  dll_file = open_file (dll_name, ".dll", "r");

  atexit (dll_close);

  /* Read in the required DOS and PE Headers */
  if (!(buff = (char *) malloc (MIN_HEADER_LEN)))
    fatal ("Out of memory");

  if (fread (buff, DOS_HEADER_LEN, 1, dll_file) != 1 ||
      get_short (buff) != DOS_MAGIC)
    fatal ("Error reading DOS header");

  if (fseek (dll_file, get_int (buff + DOS_PE_OFFSET), SEEK_SET) == -1)
    fatal ("Error seeking PE header");

  if (fread (buff, PE_HEADER_LEN, 1, dll_file) != 1 ||
      get_int (buff) != PE_MAGIC)
    fatal ("Error reading PE header");

  exports = get_int (buff + PE_EXPORTS_OFFSET);
  exports_len = get_int (buff + PE_EXPORTS_SIZE);

  if (!exports || !exports_len)
    fatal ("No exports in DLL");

  if (!(count = get_short (buff + PE_COUNT_OFFSET)))
    fatal ("No sections in DLL");

  if (VERBOSE)
    printf ("DLL has %d sections\n", count);

  /* Iterate through sections until we find exports */
  while (count--)
  {
    if (fread (buff, SECTION_HEADER_LEN, 1, dll_file) != 1)
      fatal ("Section read error");

    code = get_int (buff + SECTION_ADDR_OFFSET);
    code_len = get_int (buff + SECTION_ADDR_SIZE);

    if (code <= exports && code + code_len > exports)
      break;
  }

  if (!count)
    fatal ("No export section");

  code_len -= (exports - code);

  if (code_len < exports_len)
    fatal ("Corrupt exports");

  /* Load exports section */
  if (fseek (dll_file, get_int (buff + SECTION_POS_OFFSET)
         + exports - code, SEEK_SET) == -1)
    fatal ("Export section seek error");

  if (VERBOSE)
    printf ("Export data size = %d bytes\n", code_len);

  if (!(buff = (char *) realloc (buff, code_len)))
    fatal ("Out of memory");

  if (fread (buff, code_len, 1, dll_file) != 1)
    fatal ("Read error");

  dll_close();

  /* Locate symbol names */
  symbol_data = REBASE( get_int (buff + EXPORT_NAME_OFFSET));

  if (symbol_data > code_len)
    fatal ("Corrupt exports section");

  if (!(dll_num_exports = get_int (buff + EXPORT_COUNT_OFFSET)))
    fatal ("No export count");

  if (!(dll_symbols = (char **) malloc (dll_num_exports * sizeof (char *))))
    fatal ("Out of memory");

  /* Read symbol names into 'dll_symbols' */
  count = 0;
  while (count <  dll_num_exports)
  {
    const int   symbol_offset = get_int (buff + symbol_data + count * 4);
    const char *symbol_name_ptr = REBASE (buff + symbol_offset);

    assert(symbol_name_ptr);
    dll_symbols[count] = strdup (symbol_name_ptr);
    assert(dll_symbols[count]);

    count++;
  }

  if (NORMAL)
    printf ("%d exported symbols in DLL\n", dll_num_exports);

  free (buff);

  /* Set DLL output names */
  if ((buff = strrchr (globals.input_name, '/')))
    globals.input_name = buff + 1; /* Strip path */

  OUTPUT_UC_DLL_NAME = str_toupper( strdup (OUTPUT_DLL_NAME));
}


/*******************************************************************
 *         dll_next_symbol
 *
 * Get next exported symbol from dll
 */
char* dll_next_symbol ()
{
  static unsigned int current_export = 0;

  assert (current_export <= dll_num_exports);

  if (current_export == dll_num_exports)
    return NULL;

  assert (dll_symbols);
  assert (dll_symbols [current_export]);

  return strdup (dll_symbols [current_export++]);
}


/*******************************************************************
 *         dll_close
 *
 * Free resources used by DLL
 */
static void dll_close (void)
{
  size_t i;

  if (dll_file)
  {
    fclose (dll_file);
    dll_file = NULL;
  }

  if (dll_symbols)
  {
    for (i = 0; i < dll_num_exports; i++)
      if (dll_symbols [i])
        free (dll_symbols [i]);
    free (dll_symbols);
    dll_symbols = NULL;
  }
}
