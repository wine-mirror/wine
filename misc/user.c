static char RCSId[] = "$Id: user.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "windows.h"
#include "user.h"

#define DEFAULT_MSG_QUEUE_SIZE  8

#define USER_HEAP_SIZE          0x10000


MDESC *USER_Heap = NULL;


/***********************************************************************
 *           USER_HeapInit
 */
static BOOL USER_HeapInit()
{
    struct segment_descriptor_s * s;
    s = GetNextSegment( 0, 0x10000 );
    if (s == NULL) return FALSE;
    HEAP_Init( &USER_Heap, s->base_addr, USER_HEAP_SIZE );
    free(s);
    return TRUE;
}


/**********************************************************************
 *					USER_InitApp
 *
 * Load necessary resources?
 */
int
USER_InitApp(int hInstance)
{
      /* GDI initialisation */
    if (!GDI_Init()) return 0;

      /* Initialize system colors */
    SYSCOLOR_Init();
    
      /* Create USER heap */
    if (!USER_HeapInit()) return 0;
    
      /* Create the DCEs */
    DCE_Init();
    
      /* Initialize built-in window classes */
    WIDGETS_Init();
    
      /* Create task message queue */
    if (!SetMessageQueue( DEFAULT_MSG_QUEUE_SIZE )) return 0;
        
    return 1;
}
