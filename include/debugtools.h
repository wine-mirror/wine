
#ifndef __WINE_DEBUGTOOLS_H
#define __WINE_DEBUGTOOLS_H

#ifdef __WINE__  /* Debugging interface is internal to Wine */

#include <stdio.h>
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
  if(!DEBUGGING(cl, ch)) ; \
  else DPRINTF(# cl ":" # ch ":%s " format, __FUNCTION__ , ## args)

#define TRACE(ch, fmt, args...) DPRINTF_(trace, ch, fmt, ## args)
#define WARN(ch, fmt, args...)  DPRINTF_(warn,  ch, fmt, ## args)
#define FIXME(ch, fmt, args...) DPRINTF_(fixme, ch, fmt, ## args)
#define ERR(ch, fmt, args...)   DPRINTF_(err, ch, fmt, ## args)

#define DUMP(format, args...)   DPRINTF(format, ## args)
#define MSG(format, args...)    fprintf(stderr, format, ## args)

#define FIXME_ON(ch)  DEBUGGING(fixme, ch)
#define ERR_ON(ch)    DEBUGGING(err, ch)
#define WARN_ON(ch)   DEBUGGING(warn, ch)
#define TRACE_ON(ch)  DEBUGGING(trace, ch)

#endif  /* __WINE__ */

#endif  /* __WINE_DEBUGTOOLS_H */
