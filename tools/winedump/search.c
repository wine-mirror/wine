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

#include "winedump.h"

static char *grep_buff = NULL;
static char *fgrep_buff = NULL;

static BOOL symbol_from_prototype (parsed_symbol *sym, const char *prototype);
static const char *get_type (parsed_symbol *sym, const char *proto, int arg);


/*******************************************************************
 *         symbol_search
 *
 * Call Patrik Stridvall's 'function_grep.pl' script to retrieve a
 * function prototype from include file(s)
 */
BOOL symbol_search (parsed_symbol *sym)
{
  static const size_t MAX_RESULT_LEN = 1024;
  FILE *grep;
  int attempt = 0;

  assert (globals.do_code);
  assert (globals.directory);
  assert (sym && sym->symbol);

  if (!symbol_is_valid_c (sym))
    return FALSE;

  if (!grep_buff)
    grep_buff = xmalloc (MAX_RESULT_LEN);

  if (!fgrep_buff)
    fgrep_buff = xmalloc (MAX_RESULT_LEN);

  /* Use 'grep' to tell us which possible files the function is in,
   * then use 'function_grep.pl' to get the prototype. If this fails the
   * first time then give grep a more general query (that doesn't
   * require an opening argument brace on the line with the function name).
   */
  while (attempt < 2)
  {
    FILE *f_grep;
    char *cmd = strmake( "grep -d recurse -l \"%s%s\" %s", sym->symbol,
                         !attempt ? "[[:blank:]]*(" : "", globals.directory);

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

      cmd = strmake( "function_grep.pl %s \"%s\"", sym->symbol, grep_buff );

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

            if (symbol_from_prototype (sym, grep_buff))
            {
              pclose (f_grep);
              pclose (grep);
              return TRUE;  /* OK */
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

  return FALSE; /* Not found */
}


/*******************************************************************
 *         symbol_from_prototype
 *
 * Convert a C prototype into a symbol
 */
static BOOL symbol_from_prototype (parsed_symbol *sym, const char *proto)
{
  const char *iter;
  BOOL found;

  proto = get_type (sym, proto, -1); /* Get return type */
  if (!proto)
    return FALSE;

  iter = str_match (proto, sym->symbol, &found);

  if (!found)
  {
    char *call;
    /* Calling Convention */
    iter = strchr (iter, ' ');
    if (!iter)
      return FALSE;

    call = str_substring (proto, iter);

    if (!strcasecmp (call, "cdecl") || !strcasecmp (call, "__cdecl"))
      sym->flags |= SYM_CDECL;
    else
      sym->flags |= SYM_STDCALL;
    free (call);
    iter = str_match (iter, sym->symbol, &found);

    if (!found)
      return FALSE;

    if (VERBOSE)
      printf ("Using %s calling convention\n",
              sym->flags & SYM_CDECL ? "cdecl" : "stdcall");
  }
  else
    sym->flags = CALLING_CONVENTION;

  sym->function_name = xstrdup (sym->symbol);
  proto = iter;

  /* Now should be the arguments */
  if (*proto++ != '(')
    return FALSE;

  for (; *proto == ' '; proto++);

  if (!strncmp (proto, "void", 4))
    return TRUE;

  do
  {
    /* Process next argument */
    str_match (proto, "...", &sym->varargs);
    if (sym->varargs)
      return TRUE;

    if (!(proto = get_type (sym, proto, sym->argc)))
      return FALSE;

    sym->argc++;

    if (*proto == ',')
      proto++;
    else if (*proto != ')')
      return FALSE;

  } while (*proto != ')');

  return TRUE;
}


/*******************************************************************
 *         get_type
 *
 * Read a type from a prototype
 */
static const char *get_type (parsed_symbol *sym, const char *proto, int arg)
{
  BOOL is_const, is_volatile, is_struct, is_signed, is_unsigned;
  const char *iter, *base_type, *catch_unsigned, *proto_str;
  char dest_type, *type_str;

  assert (sym && sym->symbol);
  assert (proto && *proto);
  assert (arg < 0 || (unsigned)arg == sym->argc);


  proto_str = str_match (proto, "const", &is_const);
  proto_str = str_match (proto_str, "volatile", &is_volatile);
  proto_str = str_match (proto_str, "struct", &is_struct);
  if (!is_struct)
    proto_str = str_match (proto_str, "union", &is_struct);

  catch_unsigned = proto_str;

  proto_str = str_match (proto_str, "unsigned", &is_unsigned);
  proto_str = str_match (proto_str, "signed", &is_signed);

  /* Can have 'unsigned const' or 'const unsigned' etc */
  if (!is_const)
    proto_str = str_match (proto_str, "const", &is_const);
  if (!is_volatile)
    proto_str = str_match (proto_str, "volatile", &is_volatile);

  base_type = proto_str;
  iter = str_find_set (proto_str, " ,*)");
  if (!iter)
    return NULL;

  if (arg < 0 && (is_signed || is_unsigned))
  {
    /* Prevent calling convention from being swallowed by 'un/signed' alone */
    if (strncmp (base_type, "int", 3) && strncmp (base_type, "long", 4) &&
        strncmp (base_type, "short", 5) && strncmp (base_type, "char", 4))
    {
      iter = proto_str;
      base_type = catch_unsigned;
    } else
      catch_unsigned = NULL;
  }
  else
    catch_unsigned = NULL;

  /* FIXME: skip const/volatile here too */
  for (proto_str = iter; *proto_str; proto_str++)
    if (*proto_str != '*' && *proto_str != ' ')
      break;

  if (!*proto_str)
    return NULL;

  type_str = str_substring (proto, proto_str);
  if (iter == base_type || catch_unsigned)
  {
    /* 'unsigned' with no type */
    type_str = strmake( "%s int", type_str );
  }
  symbol_clean_string (type_str);

  dest_type = symbol_get_type (type_str);

  if (arg < 0)
  {
    sym->return_text = type_str;
    sym->return_type = dest_type;
  }
  else
  {
    sym->arg_type [arg] = dest_type;
    sym->arg_flag [arg] = is_const ? CT_CONST : is_volatile ? CT_VOLATILE : 0;

    if (*proto_str == ',' || *proto_str == ')')
      sym->arg_name [arg] = strmake( "arg%u", arg );
    else
    {
      iter = str_find_set (proto_str, " ,)");
      if (!iter)
      {
        free (type_str);
        return NULL;
      }
      sym->arg_name [arg] = str_substring (proto_str, iter);
      proto_str = iter;
    }
    sym->arg_text [arg] = type_str;

  }
  return proto_str;
}
