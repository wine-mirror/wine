
#ifndef __DEBUGTOOLS_H
#define __DEBUGTOOLS_H

#include <stdio.h>

#define DEBUG_RUNTIME
#define stddeb  stdout
#define stdnimp stdout

#define DEBUG_CLASS_COUNT 4

extern short debug_msg_enabled[][DEBUG_CLASS_COUNT];
extern const char* debug_ch_name[];
extern const char* debug_cl_name[];

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

#define debugging_(cl, ch) \
  (dbg_ch_index(ch) >=0 && dbg_cl_index(cl) >= 0 && \
   debug_msg_enabled[dbg_ch_index(ch)][dbg_cl_index(cl)])

#define dprintf(format, args...) \
  fprintf(stddeb, format, ## args)

#define dprintf_(cl, ch, format, args...) \
            if(!debugging_(cl, ch)) ; \
            else dprintf("%s:%s:%s:%d:%s: "format, \
                         debug_cl_name[dbg_cl_index(cl)], \
                         debug_ch_name[dbg_ch_index(ch)], \
	                 __FILE__, __LINE__, __FUNCTION__ , ## args)



#define debugging_fixme(ch) debugging_(fixme, ch)
#define debugging_err(ch) debugging_(err, ch)
#define debugging_warn(ch) debugging_(warn, ch)
#define debugging_info(ch) debugging_(info, ch)

#define dprintf_fixme(ch, format, args...) dprintf_(fixme, ch, format, ## args)
#define dprintf_err(ch, format, args...) dprintf_(err, ch, format, ## args)
#define dprintf_warn(ch, format, args...) dprintf_(warn, ch, format, ## args)
#define dprintf_info(ch, format, args...) dprintf_(info, ch, format, ## args)

#define dbcl_fixme 0
#define dbcl_err   1
#define dbcl_warn  2
#define dbcl_info  3

#endif



