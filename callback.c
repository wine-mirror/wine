static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include "windows.h"
#include "callback.h"
#include "wine.h"
#include "segmem.h"

extern unsigned short SelectorOwners[];
extern unsigned short IF1632_Saved16_ss;
extern unsigned long  IF1632_Saved16_esp;
extern struct segment_descriptor_s *MakeProcThunks;

struct thunk_s
{
    int used;
    unsigned char thunk[10];
};

/**********************************************************************
 *					PushOn16
 */
static void
PushOn16(int size, unsigned int value)
{
    char *p = (char *) (((unsigned int)IF1632_Saved16_ss << 16) +
			(IF1632_Saved16_esp & 0xffff));
    if (size)
    {
	unsigned long *lp = (unsigned long *) p - 1;
	
	*lp = value;
	IF1632_Saved16_esp -= 4;
    }
    else
    {
	unsigned short *sp = (unsigned short *) p - 1;
	
	*sp = value;
	IF1632_Saved16_esp -= 2;
    }
}

/**********************************************************************
 *					FindDataSegmentForCode
 */
static unsigned short
FindDataSegmentForCode(unsigned long csip)
{
    unsigned int seg_idx;
    
    seg_idx = (unsigned short) (csip >> 19);
    return SelectorOwners[seg_idx];
}

/**********************************************************************
 *					CallBack16
 */
int
CallBack16(void *func, int n_args, ...)
{
    va_list ap;
    int i;
    int arg_type, arg_value;
    
    va_start(ap, n_args);

    for (i = 0; i < n_args; i++)
    {
	arg_type = va_arg(ap, int);
	arg_value = va_arg(ap, int);
	PushOn16(arg_type, arg_value);
    }

    va_end(ap);

    return CallTo16((unsigned int) func, 
		    FindDataSegmentForCode((unsigned long) func));
}

/**********************************************************************
 *					CALLBACK_MakeProcInstance
 */
void *
CALLBACK_MakeProcInstance(void *func, int instance)
{
    int handle;
    void *new_func;
    struct thunk_s *tp;
    int i;
    
    tp = (struct thunk_s *) MakeProcThunks->base_addr;
    for (i = 0; i < 0x10000 / sizeof(*tp); i++, tp++)
	if (!tp->used)
	    break;
    
    if (tp->used)
	return (void *) 0;
    
    tp->thunk[0] = 0xb8;
    tp->thunk[1] = (unsigned char) instance;
    tp->thunk[2] = (unsigned char) (instance >> 8);
    tp->thunk[3] = 0x8e;
    tp->thunk[4] = 0xd8;
    tp->thunk[5] = 0xea;
    memcpy(&tp->thunk[6], &func, 4);

    return tp->thunk;
}

/**********************************************************************
 *					CallWindowProc    (USER.122)
 */
LONG CallWindowProc( FARPROC func, HWND hwnd, WORD message,
		     WORD wParam, LONG lParam )
{
    if ((unsigned int)func & 0xffff0000)
    {	
	PushOn16( CALLBACK_SIZE_WORD, hwnd );
	PushOn16( CALLBACK_SIZE_WORD, message );
	PushOn16( CALLBACK_SIZE_WORD, wParam );
	PushOn16( CALLBACK_SIZE_LONG, lParam );
	return CallTo16((unsigned int) func, 
			FindDataSegmentForCode((unsigned long) func));   
    }
    else
	return WIDGETS_Call32WndProc( func, hwnd, message, wParam, lParam );
}
