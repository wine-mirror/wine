/*
 * MSVCRT string functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"


DEFAULT_DEBUG_CHANNEL(msvcrt);

/* INTERNAL: MSVCRT_malloc() based strndup */
char* msvcrt_strndup(const char* buf, unsigned int size)
{
  char* ret;
  unsigned int len = strlen(buf), max_len;

  max_len = size <= len? size : len + 1;

  ret = MSVCRT_malloc(max_len);
  if (ret)
  {
    memcpy(ret,buf,max_len);
    ret[max_len] = 0;
  }
  return ret;
}

/*********************************************************************
 *		_strdec (MSVCRT.@)
 */
char* _strdec(const char* str1, const char* str2)
{
  /* Hmm. While the docs suggest that the following should work... */
  /*  return (str2<=str1?0:str2-1); */
  /* ...Version 2.50.4170 (NT) from win98 constantly decrements! */
  str1 = str1; /* remove warning */
  return (char *)str2-1;
}

/*********************************************************************
 *		_strdup (MSVCRT.@)
 */
char* _strdup(const char* str)
{
    char * ret = MSVCRT_malloc(strlen(str)+1);
    if (ret) strcpy( ret, str );
    return ret;
}

/*********************************************************************
 *		_strinc (MSVCRT.@)
 */
char* _strinc(const char* str)
{
  return (char*)str+1;
}

/*********************************************************************
 *		_strnextc (MSVCRT.@)
 */
unsigned int _strnextc(const char* str)
{
  return (unsigned int)*str;
}

/*********************************************************************
 *		_strninc (MSVCRT.@)
 *
 * Return a pointer to the 'n'th character in a string
 */
char* _strninc(char* str, unsigned int n)
{
  return str + n;
}

/*********************************************************************
 *		_strnset (MSVCRT.@)
 */
char* _strnset(char* str, int value, unsigned int len)
{
  if (len > 0 && str)
    while (*str && len--)
      *str++ = value;
  return str;
}

/*********************************************************************
 *		_strrev (MSVCRT.@)
 */
char* _strrev(char* str)
{
  char * p1;
  char * p2;

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
 *		_strset (MSVCRT.@)
 */
char* _strset(char* str, int value)
{
  char *ptr = str;
  while (*ptr)
    *ptr++ = value;

  return str;
}

/*********************************************************************
 *		_strncnt (MSVCRT.@)
 */
unsigned int _strncnt(char* str, unsigned int max)
{
  unsigned int len = strlen(str);
  return (len > max? max : len);
}

/*********************************************************************
 *		_strspnp (MSVCRT.@)
 */
char* _strspnp(char* str1, char* str2)
{
  str1 += strspn(str1,str2);
  return *str1? str1 : 0;
}

/*********************************************************************
 *		_swab (MSVCRT.@)
 */
void _swab(char* src, char* dst, int len)
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
