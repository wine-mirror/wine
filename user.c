static char RCSId[] = "$Id: user.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"

#define DEFAULT_MSG_QUEUE_SIZE  8


/**********************************************************************
 *					USER_InitApp
 *
 * Load necessary resources?
 */
int
USER_InitApp(int hInstance)
{
      /* Initialize built-in window classes */
    WIDGETS_Init();
    
      /* Create task message queue */
    if (!SetMessageQueue( DEFAULT_MSG_QUEUE_SIZE )) return 0;
        
    return 1;
}
