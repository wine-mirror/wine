#ifndef __WINE_DSHOW_H
#define __WINE_DSHOW_H

#include "windows.h"
#include "windowsx.h"
#include "olectl.h"
#include "ddraw.h"
#include "mmsystem.h"

#include "strmif.h"
#include "amvideo.h"
#include "amaudio.h"
#include "control.h"
#include "evcode.h"
#include "uuids.h"
/*#include "errors.h"*/  /* not yet */
/*#include "edevdefs.h"*/  /* not yet */
/*#include "audevcod.h"*/  /* not yet */
/*#include "dvdevcod.h"*/  /* not yet */

#ifndef AM_NOVTABLE
#define AM_NOVTABLE
#endif /* AM_NOVTABLE */

#ifndef NUMELEMS
#define NUMELEMS(elem)  (sizeof(elem)/sizeof((elem)[0]))
#endif  /* NUMELEMS */

#ifndef OATRUE
#define OATRUE  (-1)
#endif  /* OATRUE */
#ifndef OAFALSE
#define OAFALSE (0)
#endif  /* OAFALSE */


#endif /* __WINE_DSHOW_H */
