/*
 * Summary: internal header only used during the compilation of libxslt
 * Description: internal header only used during the compilation of libxslt
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XSLT_LIBXSLT_H__
#define __XSLT_LIBXSLT_H__

/*
 * These macros must be defined before including system headers.
 * Do not add any #include directives above this block.
 */
#ifndef NO_LARGEFILE_SOURCE
  #ifndef _LARGEFILE_SOURCE
    #define _LARGEFILE_SOURCE
  #endif
  #ifndef _FILE_OFFSET_BITS
    #define _FILE_OFFSET_BITS 64
  #endif
#endif

#ifdef _WIN32
#include <win32config.h>
#else
#include "config.h"
#endif

#include <libxslt/xsltconfig.h>
#include <libxml/xmlversion.h>

#if !defined LIBXSLT_PUBLIC
#if (defined (__CYGWIN__) || defined _MSC_VER) && !defined IN_LIBXSLT && !defined LIBXSLT_STATIC
#define LIBXSLT_PUBLIC __declspec(dllimport)
#else
#define LIBXSLT_PUBLIC
#endif
#endif

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define mkdir(p,m) _mkdir(p)
#endif

#ifdef __GNUC__
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

#endif /* ! __XSLT_LIBXSLT_H__ */
