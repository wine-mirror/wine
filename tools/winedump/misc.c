/*
 *  Misc functions
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


/*******************************************************************
 *         str_create
 *
 * Create a single string from many substrings
 */
char *str_create(size_t num_str, ...)
{
  va_list args;
  size_t len = 1, i = 0;
  char *tmp, *t;

  va_start (args, num_str);
  for (i = 0; i < num_str; i++)
    if ((t = va_arg(args, char *)))
      len += strlen (t);
  va_end (args);

  if (!(tmp = malloc (len)))
    fatal ("Out of memory");

  tmp[0] = '\0';

  va_start (args, num_str);
  for (i = 0; i < num_str; i++)
    if ((t = va_arg(args, char *)))
      strcat (tmp, t);
  va_end (args);
  return tmp;
}


/*******************************************************************
 *         str_create_num
 *
 * Create a single string from many substrings, terminating in a number
 */
char *str_create_num(size_t num_str, int num, ...)
{
  va_list args;
  size_t len = 8, i = 0;
  char *tmp, *t;

  va_start (args, num);
  for (i = 0; i < num_str; i++)
    if ((t = va_arg(args, char *)))
      len += strlen (t);
  va_end (args);

  if (!(tmp = malloc (len)))
    fatal ("Out of memory");

  tmp[0] = '\0';

  va_start (args, num);
  for (i = 0; i < num_str; i++)
    if ((t = va_arg(args, char *)))
      strcat (tmp, t);
  va_end (args);
  sprintf (tmp + len - 8, "%d", num);
  return tmp;
}


/*******************************************************************
 *         str_substring
 *
 * Create a new substring from a string
 */
char *str_substring(const char *start, const char *end)
{
  char *newstr;

  assert (start && end && end > start);

  if (!(newstr = malloc (end - start + 1)))
    fatal ("Out of memory");

  memcpy (newstr, start, end - start);
  newstr [end - start] = '\0';

  return newstr;
}


/*******************************************************************
 *         str_replace
 *
 * Swap two strings in another string, in place
 * Modified PD code from 'snippets'
 */
char *str_replace (char *str, const char *oldstr, const char *newstr)
{
  int oldlen, newlen;
  char *p, *q;

  if (!(p = strstr(str, oldstr)))
    return p;
  oldlen = strlen (oldstr);
  newlen = strlen (newstr);
  memmove (q = p + newlen, p + oldlen, strlen (p + oldlen) + 1);
  memcpy (p, newstr, newlen);
  return q;
}


/*******************************************************************
 *         str_match
 *
 * Locate one string in another, ignoring spaces
 */
const char *str_match (const char *str, const char *match, BOOL *found)
{
  assert(str && match && found);

  while (*str == ' ') str++;
  if (!strncmp (str, match, strlen (match)))
  {
    *found = TRUE;
    str += strlen (match);
    while (*str == ' ') str++;
  }
  else
    *found = FALSE;
  return str;
}


/*******************************************************************
 *         str_find_set
 *
 * Locate the first occurrence of a set of characters in a string
 */
const char *str_find_set (const char *str, const char *findset)
{
  assert(str && findset);

  while (*str)
  {
    const char *p = findset;
    while (*p)
      if (*p++ == *str)
        return str;
    str++;
  }
  return NULL;
}


/*******************************************************************
 *         str_toupper
 *
 * Uppercase a string
 */
char *str_toupper (char *str)
{
  char *save = str;
  while (*str)
  {
    *str = toupper (*str);
    str++;
  }
  return save;
}


/*******************************************************************
 *         open_file
 *
 * Open a file returning only on success
 */
FILE *open_file (const char *name, const char *ext, const char *mode)
{
  char  fname[128];
  FILE *fp;

  if (((unsigned)snprintf (fname, sizeof (fname), "%s%s%s",
                 *mode == 'w' ? "./" : "", name, ext) > sizeof (fname)))
    fatal ("File name too long");

  if (VERBOSE)
    printf ("Open file %s\n", fname);

  fp = fopen (fname, mode);
  if (!fp)
    fatal ("Can't open file");
  return fp;
}


/*******************************************************************
 *         fatal
 *
 * Fatal error handling
 */
void  fatal (const char *message)
{
  if (errno)
    perror (message);
  else
    puts (message);
  exit(1);
}
