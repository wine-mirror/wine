/*
 * CRTDLL string functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * Implementation Notes:
 * MT Safe.
 */

#include "crtdll.h"


DEFAULT_DEBUG_CHANNEL(crtdll);

/* INTERNAL: CRTDLL_malloc() based strndup */
LPSTR __CRTDLL__strndup(LPSTR buf, INT size);
LPSTR __CRTDLL__strndup(LPSTR buf, INT size)
{
  char* ret;
  int len = strlen(buf);
  int max_len;

  max_len = size <= len? size : len + 1;

  ret = CRTDLL_malloc(max_len);
  if (ret)
  {
    memcpy(ret,buf,max_len);
    ret[max_len] = 0;
  }
  return ret;
}


/*********************************************************************
 *                  _strdec           (CRTDLL.282)
 *
 * Return the byte before str2 while it is >= to str1. 
 *
 * PARAMS
 *   str1 [in]  Terminating string
 *
 *   sre2 [in]  string to start searching from
 *
 * RETURNS
 *   The byte before str2, or str1, whichever is greater
 *
 * NOTES
 * This function is implemented as tested with windows, which means
 * it does not have a terminating condition. It always returns
 * the byte before str2. Use with extreme caution!
 */
LPSTR __cdecl CRTDLL__strdec(LPSTR str1, LPSTR str2)
{
  /* Hmm. While the docs suggest that the following should work... */
  /*  return (str2<=str1?0:str2-1); */
  /* ...Version 2.50.4170 (NT) from win98 constantly decrements! */
  str1 = str1; /* remove warning */
  return str2-1;
}


/*********************************************************************
 *                  _strdup          (CRTDLL.285)
 *
 * Duplicate a string.
 */
LPSTR __cdecl CRTDLL__strdup(LPCSTR ptr)
{
    LPSTR ret = CRTDLL_malloc(strlen(ptr)+1);
    if (ret) strcpy( ret, ptr );
    return ret;
}


/*********************************************************************
 *                  _strinc           (CRTDLL.287)
 *
 * Return a pointer to the next character in a string
 */
LPSTR __cdecl CRTDLL__strinc(LPSTR str)
{
  return str+1;
}


/*********************************************************************
 *                   _strnextc         (CRTDLL.290)
 *
 * Return an unsigned int from a string.
 */
UINT __cdecl CRTDLL__strnextc(LPCSTR str)
{
  return (UINT)*str;
}


/*********************************************************************
 *                  _strninc           (CRTDLL.292)
 *
 * Return a pointer to the 'n'th character in a string
 */
LPSTR __cdecl CRTDLL__strninc(LPSTR str, INT n)
{
  return str+n;
}


/*********************************************************************
 *                  _strnset           (CRTDLL.293)
 *
 * Fill a string with a character up to a certain length
 */
LPSTR __cdecl CRTDLL__strnset(LPSTR str, INT c, INT len)
{
  if (len > 0 && str)
    while (*str && len--)
      *str++ = c;
  return str;
}


/*********************************************************************
 *                  _strrev              (CRTDLL.294)
 *
 * Reverse a string in place
 */
LPSTR __cdecl CRTDLL__strrev (LPSTR str)
{
  LPSTR p1;
  LPSTR p2;
 
  if (str && *str)
    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
    {
      *p1 ^= *p2;
      *p2 ^= *p1;
      *p1 ^= *p2;
    }

  return str;
}

/*********************************************************************
 *                  _strset          (CRTDLL.295)
 *
 * Fill a string with a value.
 */
LPSTR  __cdecl CRTDLL__strset (LPSTR str, INT set)
{
  char *ptr = str;

  while (*ptr)
    *ptr++ = set;

  return str;
}


/*********************************************************************
 *                  _strncnt           (CRTDLL.289)
 *
 * Return the length of a string or the maximum given length.
 */
LONG __cdecl CRTDLL__strncnt(LPSTR str, LONG max)
{
  LONG len = strlen(str);
  return (len > max? max : len);
}


/*********************************************************************
 *                  _strspnp           (CRTDLL.296)
 *
 */
LPSTR __cdecl CRTDLL__strspnp(LPSTR str1, LPSTR str2)
{
  str1 += strspn(str1,str2);
  return *str1? str1 : 0;
}


/*********************************************************************
 *                  _swab           (CRTDLL.299)
 *
 * Copy from source to dest alternating bytes (i.e 16 bit big-to-little
 * endian or vice versa).
 */
void __cdecl CRTDLL__swab(LPSTR src, LPSTR dst, INT len)
{
  if (len > 1)
  {
    len = (unsigned)len >> 1;

    while (len--) {
      *dst++ = src[1];
      *dst++ = *src++;
      src++;
    }
  }
}
