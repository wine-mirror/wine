/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "win.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

/* FIXME: to make this working, we have to callback all these registered 
 * functions from all over the WINE code. Someone with more knowledge than
 * me please do that. -Marcus
 */
static struct notify
{
    HTASK16   htask;
    FARPROC16 lpfnCallback;
    WORD     wFlags;
} *notifys = NULL;

static int nrofnotifys = 0;

BOOL16 NotifyRegister( HTASK16 htask, FARPROC16 lpfnCallback, WORD wFlags )
{
    int	i;

    dprintf_toolhelp( stddeb, "NotifyRegister(%x,%lx,%x) called.\n",
                      htask, (DWORD)lpfnCallback, wFlags );
    for (i=0;i<nrofnotifys;i++)
        if (notifys[i].htask==htask)
            break;
    if (i==nrofnotifys) {
        if (notifys==NULL)
            notifys=(struct notify*)xmalloc(sizeof(struct notify));
        else
            notifys=(struct notify*)xrealloc(notifys,sizeof(struct notify)*(nrofnotifys+1));
        nrofnotifys++;
    }
    notifys[i].htask=htask;
    notifys[i].lpfnCallback=lpfnCallback;
    notifys[i].wFlags=wFlags;
    return TRUE;
}

BOOL16 NotifyUnregister( HTASK16 htask )
{
    int	i;
    
    dprintf_toolhelp( stddeb, "NotifyUnregister(%x) called.\n", htask );
    for (i=nrofnotifys;i--;)
        if (notifys[i].htask==htask)
            break;
    if (i==-1)
        return FALSE;
    memcpy(notifys+i,notifys+(i+1),sizeof(struct notify)*(nrofnotifys-i-1));
    notifys=(struct notify*)xrealloc(notifys,(nrofnotifys-1)*sizeof(struct notify));
    nrofnotifys--;
    return TRUE;
}
