
#ifndef __WINE_DEBUGTOOLS_H
#define __WINE_DEBUGTOOLS_H

#ifdef __WINE__  /* Debugging interface is internal to Wine */

#include <stdio.h>
#include "config.h"
#include "debugstr.h"

#define DEBUG_RUNTIME

/* Internal definitions (do not use these directly) */

enum __DEBUG_CLASS { __DBCL_FIXME, __DBCL_ERR, __DBCL_WARN, __DBCL_TRACE, __DBCL_COUNT };

extern char debug_msg_enabled[][__DBCL_COUNT];

extern const char * const debug_cl_name[__DBCL_COUNT];
extern const char * const debug_ch_name[];

#define __DEBUGGING(dbcl,dbch) (debug_msg_enabled[(dbch)][(dbcl)])

#ifndef __GNUC__
#define __FUNCTION__ ""
#endif

#define __DPRINTF(dbcl,dbch) \
  (!__DEBUGGING(dbcl,dbch) || \
     (dbg_printf("%s:%s:%s ", debug_cl_name[(dbcl)], debug_ch_name[(dbch)], __FUNCTION__),0)) \
    ? 0 : dbg_printf

#define __DUMMY_DPRINTF() 1 ? 0 : ((int (*)(char *, ...)) NULL)


/* Exported definitions and macros */


#define dbg_str(name) debug_str_##name
#define dbg_buf(name) debug_buf_##name

#define dbg_decl_str(name, size) \
  char dbg_str(name)[size], *dbg_buf(name)=dbg_str(name)

#define dbg_reset_str(name) \
  dbg_buf(name)=dbg_str(name)

#define dsprintf(name, format, args...) \
  dbg_buf(name)+=sprintf(dbg_buf(name), format, ## args)

/* use configure to allow user to compile out debugging messages */

#ifndef NO_TRACE_MSGS
#define TRACE        __DPRINTF(__DBCL_TRACE,DBCH_DEFAULT)
#define TRACE_(ch)   __DPRINTF(__DBCL_TRACE,dbch_##ch)
#define TRACE_ON(ch) __DEBUGGING(__DBCL_TRACE,dbch_##ch)
#else
#define TRACE_(ch)   __DUMMY_DPRINTF
#define TRACE_ON(ch) 0
#endif /* NO_TRACE_MSGS */

#ifndef NO_DEBUG_MSGS
#define WARN         __DPRINTF(__DBCL_WARN,DBCH_DEFAULT)
#define WARN_(ch)    __DPRINTF(__DBCL_WARN,dbch_##ch)
#define WARN_ON(ch)  __DEBUGGING(__DBCL_WARN,dbch_##ch)
#define FIXME        __DPRINTF(__DBCL_FIXME,DBCH_DEFAULT)
#define FIXME_(ch)   __DPRINTF(__DBCL_FIXME,dbch_##ch)
#define FIXME_ON(ch) __DEBUGGING(__DBCL_FIXME,dbch_##ch)
#else
#define WARN         __DUMMY_DPRINTF
#define WARN_(ch)    __DUMMY_DPRINTF
#define WARN_ON(ch)  0
#define FIXME        __DUMMY_DPRINTF
#define FIXME_(ch)   __DUMMY_DPRINTF
#define FIXME_ON(ch) 0
#endif /* NO_DEBUG_MSGS */

/* define error macro regardless of what is configured */
/* Solaris got an 'ERR' define in <sys/reg.h> */
#undef ERR
#define ERR        __DPRINTF(__DBCL_ERR,DBCH_DEFAULT)
#define ERR_(ch)   __DPRINTF(__DBCL_ERR,dbch_##ch)
#define ERR_ON(ch) __DEBUGGING(__DBCL_ERR,dbch_##ch)

#define DECLARE_DEBUG_CHANNEL(ch) \
    extern const int dbch_##ch;
#define DEFAULT_DEBUG_CHANNEL(ch) \
    extern const int dbch_##ch; static const int *const DBCH_DEFAULT = &dbch_##ch;

#ifdef OLD_DEBUG_MACROS
/* transition macros */
#undef TRACE
#undef WARN
#undef FIXME
#undef ERR
#define TRACE(ch, fmt, args...) TRACE_(ch)(fmt, ## args)
#define WARN(ch, fmt, args...)  WARN_(ch)(fmt, ## args)
#define FIXME(ch, fmt, args...) FIXME_(ch)(fmt, ## args)
#define ERR(ch, fmt, args...)   ERR_(ch)(fmt, ## args)
#define MSG(format, args...)    fprintf(stderr, format, ## args)
#define DPRINTF dbg_printf
#define DUMP dbg_printf
#endif

#endif  /* __WINE__ */

#endif  /* __WINE_DEBUGTOOLS_H */
