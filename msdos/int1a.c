/*
 * BIOS interrupt 1ah handler
 */

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

#define	BCD_TO_BIN(x) ((x&15) + (x>>4)*10)
#define BIN_TO_BCD(x) ((x%10) + ((x/10)<<4))


/**********************************************************************
 *	    INT1A_GetTicksSinceMidnight
 *
 * Return number of clock ticks since midnight.
 */
DWORD INT1A_GetTicksSinceMidnight(void)
{
    struct tm *bdtime;
    struct timeval tvs;
    time_t seconds;

    /* This should give us the (approximately) correct
     * 18.206 clock ticks per second since midnight.
     */
    gettimeofday( &tvs, NULL );
    seconds = tvs.tv_sec;
    bdtime = localtime( &seconds );
    return (((bdtime->tm_hour * 3600 + bdtime->tm_min * 60 +
              bdtime->tm_sec) * 18206) / 1000) +
                  (tvs.tv_usec / 54927);
}


/**********************************************************************
 *	    INT_Int1aHandler
 *
 * Handler for int 1ah (date and time).
 */
void INT_Int1aHandler( CONTEXT *context )
{
    time_t ltime;
    DWORD ticks;
    struct tm *bdtime;

    switch(AH_reg(context))
    {
	case 0:
            ticks = INT1A_GetTicksSinceMidnight();
            CX_reg(context) = HIWORD(ticks);
            DX_reg(context) = LOWORD(ticks);
            AX_reg(context) = 0;  /* No midnight rollover */
            dprintf_int(stddeb,"int1a_00 // ticks=%ld\n", ticks);
            break;
		
	case 2: 
		ltime = time(NULL);
		bdtime = localtime(&ltime);
		
		CX_reg(context) = (BIN_TO_BCD(bdtime->tm_hour)<<8) |
                                   BIN_TO_BCD(bdtime->tm_min);
		DX_reg(context) = (BIN_TO_BCD(bdtime->tm_sec)<<8);

	case 4:
		ltime = time(NULL);
		bdtime = localtime(&ltime);
		CX_reg(context) = (BIN_TO_BCD(bdtime->tm_year/100)<<8) |
                                   BIN_TO_BCD((bdtime->tm_year-1900)%100);
		DX_reg(context) = (BIN_TO_BCD(bdtime->tm_mon)<<8) |
                                   BIN_TO_BCD(bdtime->tm_mday);
		break;

		/* setting the time,date or RTC is not allow -EB */
	case 1:
		/* set system time */
	case 3: 
		/* set RTC time */
	case 5:
		/* set RTC date */
	case 6:
		/* set ALARM */
	case 7:
		/* cancel ALARM */
		break;

	default:
		INT_BARF( context, 0x1a );
    }
}
