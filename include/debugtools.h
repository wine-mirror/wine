
#ifndef __WINE_DEBUGTOOLS_H
#define __WINE_DEBUGTOOLS_H

#ifdef __WINE__  /* Debugging interface is internal to Wine */

#include <stdio.h>
#include "config.h"
#include "debugstr.h"

#define DEBUG_RUNTIME
#define stddeb  stderr

#define DEBUG_CLASS_COUNT 4

extern short debug_msg_enabled[][DEBUG_CLASS_COUNT];

#define dbg_str(name) debug_str_##name
#define dbg_buf(name) debug_buf_##name

#define dbg_decl_str(name, size) \
  char dbg_str(name)[size], *dbg_buf(name)=dbg_str(name)

#define dbg_reset_str(name) \
  dbg_buf(name)=dbg_str(name)

#define dsprintf(name, format, args...) \
  dbg_buf(name)+=sprintf(dbg_buf(name), format, ## args)

#define dbg_ch_index(ch) (dbch_##ch)
#define dbg_cl_index(cl) (dbcl_##cl)

#define DEBUGGING(cl, ch) \
  (dbg_ch_index(ch) >=0 && dbg_cl_index(cl) >= 0 && \
   debug_msg_enabled[dbg_ch_index(ch)][dbg_cl_index(cl)])

#define DPRINTF(format, args...) fprintf(stddeb, format, ## args)

#define DPRINTF_(cl, ch, format, args...) \
  do {if(!DEBUGGING(cl, ch)) ; \
  else DPRINTF(# cl ":" # ch ":%s " format, __FUNCTION__ , ## args); } while (0)

/* use configure to allow user to compile out debugging messages */

#ifndef NO_TRACE_MSGS
#define TRACE(ch, fmt, args...) DPRINTF_(trace, ch, fmt, ## args)
#else
#define TRACE(ch, fmt, args...) do { } while (0)
#endif /* NO_TRACE_MSGS */

#ifndef NO_DEBUG_MSGS
#define WARN(ch, fmt, args...)  DPRINTF_(warn,  ch, fmt, ## args)
#define FIXME(ch, fmt, args...) DPRINTF_(fixme, ch, fmt, ## args)
#define DUMP(format, args...)   DPRINTF(format, ## args)
#else
#define WARN(ch, fmt, args...) do { } while (0)
#define FIXME(ch, fmt, args...) do { } while (0)
#define DUMP(format, args...) do { } while (0)
#endif /* NO_DEBUG_MSGS */

/* define error macro regardless of what is configured */
/* Solaris got an 'ERR' define in <sys/reg.h> */
#undef ERR
#define ERR(ch, fmt, args...)   DPRINTF_(err, ch, fmt, ## args)
#define MSG(format, args...)    fprintf(stderr, format, ## args)

/* if the debug message is compiled out, make these return false */
#ifndef NO_TRACE_MSGS
#define TRACE_ON(ch)  DEBUGGING(trace, ch)
#else
#define TRACE_ON(ch) 0
#endif /* NO_TRACE_MSGS */

#ifndef NO_DEBUG_MSGS
#define WARN_ON(ch)   DEBUGGING(warn, ch)
#define FIXME_ON(ch)  DEBUGGING(fixme, ch)
#else
#define WARN_ON(ch) 0
#define FIXME_ON(ch) 0
#endif /* NO_DEBUG_MSGS */

#define ERR_ON(ch)    DEBUGGING(err, ch)

#endif  /* __WINE__ */

#endif  /* __WINE_DEBUGTOOLS_H */
