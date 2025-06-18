/*
 *  Symbol functions
 *
 *  Copyright 2000 Jon Griffiths
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

#include "winedump.h"


/* Items that are swapped in arguments after the symbol structure
 * has been populated
 */
static const char * const swap_after[] =
{
  "\r", " ", /* Remove whitespace, normalise pointers and brackets */
  "\t", " ",
  "  ", " ",
  " * ", " *",
  "* *", "**",
  "* ", "*",
  " ,", ",",
  "( ", "(",
  " )", ")",
  "wchar_t", "WCHAR", /* Help with Unicode compiles */
  "wctype_t", "WCHAR",
  "wint_t", "WCHAR",
  NULL, NULL
};


/* Items containing these substrings are assumed to be wide character
 * strings, unless they contain more that one '*'. A preceding 'LP'
 * counts as a '*', so 'LPWCSTR *' is a pointer, not a string
 */
static const char * const wide_strings[] =
{
  "WSTR", "WCSTR", NULL
};

/* Items containing these substrings are assumed to be wide characters,
 * unless they contain one '*'. A preceding 'LP' counts as a '*',
 * so 'WCHAR *' is string, while 'LPWCHAR *' is a pointer
 */
static const char * const wide_chars[] =
{
  "WCHAR", NULL
};

/* Items containing these substrings are assumed to be ASCII character
 * strings, as above
 */
static const char * const ascii_strings[] =
{
  "STR", "CSTR", NULL
};


/* Items containing these substrings are assumed to be ASCII characters,
 * as above
 */
static const char * const ascii_chars[] =
{
  "CHAR", "char", NULL
};

/* Any type other than the following will produce a FIXME warning with -v
 * when mapped to a long, to allow fixups
 */
static const char * const known_longs[] =
{
  "char", "CHAR", "float", "int", "INT", "short", "SHORT", "long", "LONG",
  "WCHAR", "BOOL", "bool", "INT16", "WORD", "DWORD", NULL
};

void symbol_init(parsed_symbol* sym, const char* name)
{
    memset(sym, 0, sizeof(parsed_symbol));
    sym->symbol = xstrdup(name);
}

/*******************************************************************
 *         symbol_clear
 *
 * Free the memory used by a symbol and initialise it
 */
void symbol_clear(parsed_symbol *sym)
{
 int i;

 assert (sym);
 assert (sym->symbol);

 free (sym->symbol);
 free (sym->return_text);
 free (sym->function_name);

 for (i = sym->argc - 1; i >= 0; i--)
 {
   free (sym->arg_text [i]);
   free (sym->arg_name [i]);
 }
 memset (sym, 0, sizeof (parsed_symbol));
}


/*******************************************************************
 *         symbol_is_valid_c
 *
 * Check if a symbol is a valid C identifier
 */
BOOL symbol_is_valid_c(const parsed_symbol *sym)
{
  char *name;

  assert (sym);
  assert (sym->symbol);

  name = sym->symbol;

  while (*name)
  {
    if (!isalnum (*name) && *name != '_')
      return FALSE;
    name++;
  }
  return TRUE;
}


/*******************************************************************
 *         symbol_get_call_convention
 *
 * Return the calling convention of a symbol
 */
const char *symbol_get_call_convention(const parsed_symbol *sym)
{
  int call = sym->flags ? sym->flags : CALLING_CONVENTION;

  assert (sym);
  assert (sym->symbol);

  if (call & SYM_CDECL)
    return "cdecl";
  return "stdcall";
}


/*******************************************************************
 *         symbol_get_spec_type
 *
 * Get the .spec file text for a symbol's argument
 */
const char *symbol_get_spec_type (const parsed_symbol *sym, size_t arg)
{
  assert (arg < sym->argc);
  switch (sym->arg_type [arg])
  {
  case ARG_STRING:      return "str";
  case ARG_WIDE_STRING: return "wstr";
  case ARG_POINTER:     return "ptr";
  case ARG_DOUBLE:      return "double";
  case ARG_STRUCT:
  case ARG_FLOAT:
  case ARG_LONG:        return "long";
  }
  assert (0);
  return NULL;
}


/*******************************************************************
 *         symbol_get_type
 *
 * Get the ARG_ constant for a type string
 */
int   symbol_get_type (const char *string)
{
  const char *iter = string;
  const char * const *tab;
  int ptrs = 0;

  while (*iter && isspace(*iter))
    iter++;
  if (*iter == 'P' || *iter == 'H')
    ptrs++; /* Win32 type pointer */

  iter = string;
  while (*iter)
  {
    if (*iter == '*' || (*iter == 'L' && iter[1] == 'P')
        || (*iter == '[' && iter[1] == ']'))
      ptrs++;
    if (ptrs > 1)
      return ARG_POINTER;
    iter++;
  }

  /* 0 or 1 pointer */
  tab = wide_strings;
  while (*tab++)
    if (strstr (string, tab[-1]))
    {
      if (ptrs < 2) return ARG_WIDE_STRING;
      else          return ARG_POINTER;
    }
  tab = wide_chars;
  while (*tab++)
    if (strstr (string, tab[-1]))
    {
      if (!ptrs) return ARG_LONG;
      else       return ARG_WIDE_STRING;
    }
  tab = ascii_strings;
  while (*tab++)
    if (strstr (string, tab[-1]))
    {
      if (ptrs < 2) return ARG_STRING;
      else          return ARG_POINTER;
    }
  tab = ascii_chars;
  while (*tab++)
    if (strstr (string, tab[-1]))
    {
      if (!ptrs) return ARG_LONG;
      else {
        if (!strstr (string, "unsigned")) /* unsigned char * => ptr */
          return ARG_STRING;
      }
    }

  if (ptrs)
    return ARG_POINTER; /* Pointer to some other type */

  /* No pointers */
  if (strstr (string, "double"))
    return ARG_DOUBLE;

  if (strstr (string, "float") || strstr (string, "FLOAT"))
    return ARG_FLOAT;

  if (strstr (string, "void") || strstr (string, "VOID"))
    return ARG_VOID;

  if (strstr (string, "struct") || strstr (string, "union"))
    return ARG_STRUCT; /* Struct by value, ugh */

  if (VERBOSE)
  {
    BOOL known = FALSE;

    tab = known_longs;
    while (*tab++)
    if (strstr (string, tab[-1]))
    {
      known = TRUE;
      break;
    }
    /* Unknown types passed by value can be 'grep'ed out for fixup later */
    if (!known)
      printf ("/* FIXME: By value type: Assumed 'int' */ typedef int %s;\n",
              string);
  }
  return ARG_LONG;
}


/*******************************************************************
 *         symbol_clean_string
 *
 * Make a type string more Wine-friendly. Logically const :-)
 */
void  symbol_clean_string (char *str)
{
  const char * const *tab = swap_after;

#define SWAP(i, p, x, y) do { i = p; while ((i = str_replace (i, x, y))); } while(0)

  while (tab [0])
  {
    char *p;
    SWAP (p, str, tab [0], tab [1]);
    tab += 2;
  }
  if (str [strlen (str) - 1] == ' ')
    str [strlen (str) - 1] = '\0'; /* no trailing space */

  if (*str == ' ')
    memmove (str, str + 1, strlen (str)); /* No leading spaces */
}
