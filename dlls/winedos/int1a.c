/*
 * BIOS interrupt 1ah handler
 *
 * Copyright 1993 Erik Bos
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdlib.h>
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

#define        BCD_TO_BIN(x) ((x&15) + (x>>4)*10)
#define BIN_TO_BCD(x) ((x%10) + ((x/10)<<4))


/**********************************************************************
 *         INT1A_GetTicksSinceMidnight
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
 *         DOSVM_Int1aHandler (WINEDOS16.126)
 *
 * Handler for int 1ah
 *     0x00 - 0x07 - date and time
 *     0x?? - 0x?? - Microsoft Real Time Compression Interface
 */
void WINAPI DOSVM_Int1aHandler( CONTEXT86 *context )
{
    time_t ltime;
    DWORD ticks;
    struct tm *bdtime;

    switch(AH_reg(context))
    {
       case 0x00:
            ticks = INT1A_GetTicksSinceMidnight();
            SET_CX( context, HIWORD(ticks) );
            SET_DX( context, LOWORD(ticks) );
            SET_AX( context, 0 );  /* No midnight rollover */
            TRACE("int1a: AH=00 -- ticks=%ld\n", ticks);
            break;

       case 0x02:
               ltime = time(NULL);
               bdtime = localtime(&ltime);

               SET_CX( context, (BIN_TO_BCD(bdtime->tm_hour)<<8) |
                                  BIN_TO_BCD(bdtime->tm_min) );
               SET_DX( context, (BIN_TO_BCD(bdtime->tm_sec)<<8) );

       case 0x04:
               ltime = time(NULL);
               bdtime = localtime(&ltime);
               SET_CX( context, (BIN_TO_BCD(bdtime->tm_year/100)<<8) |
                                  BIN_TO_BCD((bdtime->tm_year-1900)%100) );
               SET_DX( context, (BIN_TO_BCD(bdtime->tm_mon)<<8) |
                                  BIN_TO_BCD(bdtime->tm_mday) );
               break;

               /* setting the time,date or RTC is not allow -EB */
       case 0x01:
               /* set system time */
       case 0x03:
               /* set RTC time */
       case 0x05:
               /* set RTC date */
       case 0x06:
               /* set ALARM */
       case 0x07:
               /* cancel ALARM */
               break;

        case 0xb0: /* Microsoft Real Time Compression */
                switch AL_reg(context)
                {
                    case 0x01:
                        /* not present */
                        break;
                    default:
                        INT_BARF(context, 0x1a);
                }
                break;

       default:
               INT_BARF( context, 0x1a );
    }
}
