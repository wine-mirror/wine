
#ifndef __WINE_DEBUGTOOLS_H
#define __WINE_DEBUGTOOLS_H

#ifdef __WINE__  /* Debugging interface is internal to Wine */

#include <stdarg.h>
#include "config.h"
#include "windef.h"

struct _GUID;

/* Internal definitions (do not use these directly) */

enum __DEBUG_CLASS { __DBCL_FIXME, __DBCL_ERR, __DBCL_WARN, __DBCL_TRACE, __DBCL_COUNT };

#ifndef NO_TRACE_MSGS
# define __GET_DEBUGGING_trace(dbch) ((dbch)[0] & (1 << __DBCL_TRACE))
#else
# define __GET_DEBUGGING_trace(dbch) 0
#endif

#ifndef NO_DEBUG_MSGS
# define __GET_DEBUGGING_warn(dbch)  ((dbch)[0] & (1 << __DBCL_WARN))
# define __GET_DEBUGGING_fixme(dbch) ((dbch)[0] & (1 << __DBCL_FIXME))
#else
# define __GET_DEBUGGING_warn(dbch)  0
# define __GET_DEBUGGING_fixme(dbch) 0
#endif

/* define error macro regardless of what is configured */
#define __GET_DEBUGGING_err(dbch)  ((dbch)[0] & (1 << __DBCL_ERR))

#define __GET_DEBUGGING(dbcl,dbch)  __GET_DEBUGGING_##dbcl(dbch)
#define __SET_DEBUGGING(dbcl,dbch,on) \
    ((on) ? ((dbch)[0] |= 1 << (dbcl)) : ((dbch)[0] &= ~(1 << (dbcl))))

#ifndef __GNUC__
#define __FUNCTION__ ""
#endif

#define __DPRINTF(dbcl,dbch) \
  (!__GET_DEBUGGING(dbcl,(dbch)) || (__wine_dbg_header_##dbcl((dbch),__FUNCTION__),0)) ? \
     (void)0 : (void)wine_dbg_printf

extern int __wine_dbg_header_err( const char *dbg_channel, const char *func );
extern int __wine_dbg_header_warn( const char *dbg_channel, const char *func );
extern int __wine_dbg_header_fixme( const char *dbg_channel, const char *func );
extern int __wine_dbg_header_trace( const char *dbg_channel, const char *func );

/* Exported definitions and macros */

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern const char *debugstr_an (const char * s, int n);
extern const char *debugstr_wn (const WCHAR *s, int n);
extern const char *debugstr_guid( const struct _GUID *id );

extern int wine_dbg_vprintf( const char *format, va_list args );

inline static const char *debugstr_a( const char *s )  { return debugstr_an( s, 80 ); }
inline static const char *debugstr_w( const WCHAR *s ) { return debugstr_wn( s, 80 ); }
inline static const char *debugres_a( const char *s )  { return debugstr_an( s, 80 ); }
inline static const char *debugres_w( const WCHAR *s ) { return debugstr_wn( s, 80 ); }

#ifdef __GNUC__
extern int wine_dbg_printf(const char *format, ...) __attribute__((format (printf,1,2)));
#else
extern int wine_dbg_printf(const char *format, ...);
#endif

#define TRACE        __DPRINTF(trace,__wine_dbch___default)
#define TRACE_(ch)   __DPRINTF(trace,__wine_dbch_##ch)
#define TRACE_ON(ch) __GET_DEBUGGING(trace,__wine_dbch_##ch)

#define WARN         __DPRINTF(warn,__wine_dbch___default)
#define WARN_(ch)    __DPRINTF(warn,__wine_dbch_##ch)
#define WARN_ON(ch)  __GET_DEBUGGING(warn,__wine_dbch_##ch)

#define FIXME        __DPRINTF(fixme,__wine_dbch___default)
#define FIXME_(ch)   __DPRINTF(fixme,__wine_dbch_##ch)
#define FIXME_ON(ch) __GET_DEBUGGING(fixme,__wine_dbch_##ch)

#undef ERR  /* Solaris got an 'ERR' define in <sys/reg.h> */
#define ERR          __DPRINTF(err,__wine_dbch___default)
#define ERR_(ch)     __DPRINTF(err,__wine_dbch_##ch)
#define ERR_ON(ch)   __GET_DEBUGGING(err,__wine_dbch_##ch)

#define DECLARE_DEBUG_CHANNEL(ch) \
    extern char __wine_dbch_##ch[];
#define DEFAULT_DEBUG_CHANNEL(ch) \
    extern char __wine_dbch_##ch[]; static char * const __wine_dbch___default = __wine_dbch_##ch;

#define DPRINTF wine_dbg_printf
#define MESSAGE wine_dbg_printf

#endif  /* __WINE__ */

#endif  /* __WINE_DEBUGTOOLS_H */
