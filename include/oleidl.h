#ifndef __WINE__
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif
#endif

#ifndef __WINE_OLEIDL_H
#define __WINE_OLEIDL_H


#include "wine/obj_base.h"
#include "wine/obj_misc.h"

/* the following depend only on obj_base.h */
#include "wine/obj_storage.h"

/* the following depend on obj_storage.h */
#include "wine/obj_moniker.h"

/* the following depend on obj_moniker */
#include "wine/obj_dataobject.h"

/* the following depend on obj_dataobject.h */
#include "wine/obj_dragdrop.h"

#include "wine/obj_inplace.h"
#include "wine/obj_cache.h"
#include "wine/obj_oleobj.h"
#include "wine/obj_oleview.h"
#include "wine/obj_errorinfo.h"

#endif /* __WINE_OLEIDL_H */

