/*
 *  Prototype search and parsing functions
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
#include "wine/port.h"

#include "winedump.h"

static char *grep_buff = NULL;
static char *fgrep_buff = NULL;

static int symbol_from_prototype (parsed_symbol *sym, const char *prototype);
static const char *get_type (parsed_symbol *sym, const char *proto, int arg);


/*******************************************************************
 *         symbol_search
 *
 * Call Patrik Stridvall's 'function_grep.pl' script to retrieve a
 * function prototype from include file(s)
 */
int symbol_search (parsed_symbol *sym)
{
  static const size_t MAX_RESULT_LEN = 1024;
  FILE *grep;
  int attempt = 0;

  assert (globals.do_code);
  assert (globals.directory);
  assert (sym && sym->symbol);

  if (!symbol_is_valid_c (sym))
    return - 1;

  if (!grep_buff)
    grep_buff = (char *) malloc (MAX_RESULT_LEN);

  if (!fgrep_buff)
    fgrep_buff = (char *) malloc (MAX_RESULT_LEN);

  if (!grep_buff || !fgrep_buff)
    fatal ("Out of Memory");

  /* Use 'grep' to tell us which possible files the function is in,
   * then use 'function_grep.pl' to get the prototype. If this fails the
   * first time then give grep a more general query (that doesn't
   * require an opening argument brace on the line with the function name).
   */
  while (attempt < 2)
  {
    FILE *f_grep;
    char *cmd = str_create (4, "grep -d recurse -l \"", sym->symbol,
        !attempt ? "[:blank:]*(\" " : "\" ", globals.directory);

    if (VERBOSE)
      puts (cmd);

    fflush (NULL); /* See 'man popen' */

    if (!(grep = popen (cmd, "r")))
      fatal ("Cannot execute grep -l");
    free (cmd);

    while (fgets (grep_buff, MAX_RESULT_LEN, grep))
    {
      int i;
      const char *extension = grep_buff;
      for (i = 0; grep_buff[i] && grep_buff[i] != '\n' ; i++) {
	if (grep_buff[i] == '.')
	  extension = &grep_buff[i];
      }
      grep_buff[i] = '\0';

      /* Definitely not in these: */
      if (strcmp(extension,".dll") == 0 ||
	  strcmp(extension,".lib") == 0 ||
	  strcmp(extension,".so")  == 0 ||
	  strcmp(extension,".o")   == 0)
	continue;

      if (VERBOSE)
        puts (grep_buff);

      cmd = str_create (5, "function_grep.pl ", sym->symbol,
                        " \"", grep_buff, "\"");

      if (VERBOSE)
        puts (cmd);

      fflush (NULL); /* See 'man popen' */

      if (!(f_grep = popen (cmd, "r")))
        fatal ("Cannot execute function_grep.pl");
      free (cmd);

      while (fgets (grep_buff, MAX_RESULT_LEN, f_grep))
      {
        char *iter = grep_buff;

        /* Keep only the first line */
        symbol_clean_string(grep_buff);

        for (i = 0; grep_buff[i] && grep_buff[i] != '\n' ; i++)
          ;
        grep_buff[i] = '\0';

        if (VERBOSE)
          puts (grep_buff);

        while ((iter = strstr (iter, sym->symbol)))
        {
          if (iter > grep_buff && (iter[-1] == ' ' || iter[-1] == '*') &&
             (iter[strlen (sym->symbol)] == ' ' ||
              iter[strlen (sym->symbol)] == '('))
          {
            if (VERBOSE)
              printf ("Prototype '%s' looks OK, processing\n", grep_buff);

            if (!symbol_from_prototype (sym, grep_buff))
            {
              pclose (f_grep);
              pclose (grep);
              return 0;  /* OK */
            }
            if (VERBOSE)
              puts ("Failed, trying next");
          }
          else
            iter += strlen (sym->symbol);
        }
      }
      pclose (f_grep);
    }
    pclose (grep);
    attempt++;
  }

  return -1; /* Not found */
}


/*******************************************************************
 *         symbol_from_prototype
 *
 * Convert a C prototype into a symbol
 */
static int symbol_from_prototype (parsed_symbol *sym, const char *proto)
{
  const char *iter;
  int found;

  proto = get_type (sym, proto, -1); /* Get return type */
  if (!proto)
    return -1;

  iter = str_match (proto, sym->symbol, &found);

  if (!found)
  {
    char *call;
    /* Calling Convention */
    iter = strchr (iter, ' ');
    if (!iter)
      return -1;

    call = str_substring (proto, iter);

    if (!strcasecmp (call, "cdecl") || !strcasecmp (call, "__cdecl"))
      sym->flags |= SYM_CDECL;
    else
      sym->flags |= SYM_STDCALL;
    free (call);
    iter = str_match (iter, sym->symbol, &found);

    if (!found)
      return -1;

    if (VERBOSE)
      printf ("Using %s calling convention\n",
              sym->flags & SYM_CDECL ? "cdecl" : "stdcall");
  }
  else
    sym->flags = CALLING_CONVENTION;

  sym->function_name = strdup (sym->symbol);
  proto = iter;

  /* Now should be the arguments */
  if (*proto++ != '(')
    return -1;

  for (; *proto == ' '; proto++);

  if (!strncmp (proto, "void", 4))
    return 0;

  do
  {
    /* Process next argument */
    str_match (proto, "...", &sym->varargs);
    if (sym->varargs)
      return 0;

    if (!(proto = get_type (sym, proto, sym->argc)))
      return -1;

    sym->argc++;

    if (*proto == ',')
      proto++;
    else if (*proto != ')')
      return -1;

  } while (*proto != ')');

  return 0;
}


/*******************************************************************
 *         get_type
 *
 * Read a type from a prototype
 */
static const char *get_type (parsed_symbol *sym, const char *proto, int arg)
{
  int is_const, is_volatile, is_struct, is_signed, is_unsigned, ptrs = 0;
  const char *iter, *type_str, *base_type, *catch_unsigned;
  char dest_type, *type_str_tmp;

  assert (sym && sym->symbol);
  assert (proto && *proto);
  assert (arg < 0 || (unsigned)arg == sym->argc);

  type_str = proto;

  proto = str_match (proto, "const", &is_const);
  proto = str_match (proto, "volatile", &is_volatile);
  proto = str_match (proto, "struct", &is_struct);
  if (!is_struct)
    proto = str_match (proto, "union", &is_struct);

  catch_unsigned = proto;

  proto = str_match (proto, "unsigned", &is_unsigned);
  proto = str_match (proto, "signed", &is_signed);

  /* Can have 'unsigned const' or 'const unsigned' etc */
  if (!is_const)
    proto = str_match (proto, "const", &is_const);
  if (!is_volatile)
    proto = str_match (proto, "volatile", &is_volatile);

  base_type = proto;
  iter = str_find_set (proto, " ,*)");
  if (!iter)
    return NULL;

  if (arg < 0 && (is_signed || is_unsigned))
  {
    /* Prevent calling convention from being swallowed by 'un/signed' alone */
    if (strncmp (base_type, "int", 3) && strncmp (base_type, "long", 4) &&
        strncmp (base_type, "short", 5) && strncmp (base_type, "char", 4))
    {
      iter = proto;
      base_type = catch_unsigned;
    } else
      catch_unsigned = NULL;
  }
  else
    catch_unsigned = NULL;

  /* FIXME: skip const/volatile here too */
  for (proto = iter; *proto; proto++)
    if (*proto == '*')
      ptrs++;
    else if (*proto != ' ')
      break;

  if (!*proto)
    return NULL;

  type_str = type_str_tmp = str_substring (type_str, proto);
  if (iter == base_type || catch_unsigned)
  {
    /* 'unsigned' with no type */
    char *tmp = str_create (2, type_str, " int");
    free (type_str_tmp);
    type_str = type_str_tmp = tmp;
  }
  symbol_clean_string (type_str);

  dest_type = symbol_get_type (type_str);

  if (arg < 0)
  {
    sym->return_text = (char*)type_str;
    sym->return_type = dest_type;
  }
  else
  {
    sym->arg_type [arg] = dest_type;
    sym->arg_flag [arg] = is_const ? CT_CONST : is_volatile ? CT_VOLATILE : 0;

    if (*proto == ',' || *proto == ')')
      sym->arg_name [arg] = str_create_num (1, arg, "arg");
    else
    {
      iter = str_find_set (proto, " ,)");
      if (!iter)
      {
        free (type_str_tmp);
        return NULL;
      }
      sym->arg_name [arg] = str_substring (proto, iter);
      proto = iter;
    }
    sym->arg_text [arg] = (char*)type_str;

  }
  return proto;
}


#ifdef __GNUC__
/*******************************************************************
 *         search_cleanup
 *
 * Free memory used while searching (a niceity)
 */
void search_cleanup (void) __attribute__ ((destructor));
void search_cleanup (void)
{
  free (grep_buff);
  free (fgrep_buff);
}
#endif
