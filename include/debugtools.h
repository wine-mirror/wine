
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
  (!__GET_DEBUGGING(dbcl,(dbch)) || (dbg_header_##dbcl((dbch),__FUNCTION__),0)) ? \
     (void)0 : (void)dbg_printf

/* Exported definitions and macros */

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern LPCSTR debugstr_an (LPCSTR s, int n);
extern LPCSTR debugstr_wn (LPCWSTR s, int n);
extern LPCSTR debugres_a (LPCSTR res);
extern LPCSTR debugres_w (LPCWSTR res);
extern LPCSTR debugstr_guid( const struct _GUID *id );
extern LPCSTR debugstr_hex_dump (const void *ptr, int len);
extern int dbg_header_err( const char *dbg_channel, const char *func );
extern int dbg_header_warn( const char *dbg_channel, const char *func );
extern int dbg_header_fixme( const char *dbg_channel, const char *func );
extern int dbg_header_trace( const char *dbg_channel, const char *func );
extern int dbg_vprintf( const char *format, va_list args );

static inline LPCSTR debugstr_a( LPCSTR s )  { return debugstr_an( s, 80 ); }
static inline LPCSTR debugstr_w( LPCWSTR s ) { return debugstr_wn( s, 80 ); }

#ifdef __GNUC__
extern int dbg_printf(const char *format, ...) __attribute__((format (printf,1,2)));
#else
extern int dbg_printf(const char *format, ...);
#endif

#define TRACE        __DPRINTF(trace,__dbch_default)
#define TRACE_(ch)   __DPRINTF(trace,dbch_##ch)
#define TRACE_ON(ch) __GET_DEBUGGING(trace,dbch_##ch)

#define WARN         __DPRINTF(warn,__dbch_default)
#define WARN_(ch)    __DPRINTF(warn,dbch_##ch)
#define WARN_ON(ch)  __GET_DEBUGGING(warn,dbch_##ch)

#define FIXME        __DPRINTF(fixme,__dbch_default)
#define FIXME_(ch)   __DPRINTF(fixme,dbch_##ch)
#define FIXME_ON(ch) __GET_DEBUGGING(fixme,dbch_##ch)

#undef ERR  /* Solaris got an 'ERR' define in <sys/reg.h> */
#define ERR          __DPRINTF(err,__dbch_default)
#define ERR_(ch)     __DPRINTF(err,dbch_##ch)
#define ERR_ON(ch)   __GET_DEBUGGING(err,dbch_##ch)

#define DECLARE_DEBUG_CHANNEL(ch) \
    extern char dbch_##ch[];
#define DEFAULT_DEBUG_CHANNEL(ch) \
    extern char dbch_##ch[]; static char * const __dbch_default = dbch_##ch;

#define DPRINTF dbg_printf
#define MESSAGE dbg_printf

#endif  /* __WINE__ */

#endif  /* __WINE_DEBUGTOOLS_H */
