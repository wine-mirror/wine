#ifndef __WINE_COMCAT_H
#define __WINE_COMCAT_H

#include "rpc.h"
#include "rpcndr.h"

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif

#include "unknwn.h"

typedef GUID CATID;
typedef REFGUID REFCATID;

#endif /*__WINE_COMCAT_H */
