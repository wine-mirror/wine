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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/debug.h"
#include "dosexe.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

#define BCD_TO_BIN(x) ((x&15) + (x>>4)*10)
#define BIN_TO_BCD(x) ((x%10) + ((x/10)<<4))


/**********************************************************************
 *         DOSVM_Int1aHandler
 *
 * Handler for int 1ah.
 */
void WINAPI DOSVM_Int1aHandler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x00: /* GET SYSTEM TIME */
        {
            BIOSDATA *data = DOSVM_BiosData();
            SET_CX( context, HIWORD(data->Ticks) );
            SET_DX( context, LOWORD(data->Ticks) );
            SET_AL( context, 0 ); /* FIXME: midnight flag is unsupported */
            TRACE( "GET SYSTEM TIME - ticks=%ld\n", data->Ticks );
        }
        break;

    case 0x01: /* SET SYSTEM TIME */
        FIXME( "SET SYSTEM TIME - not allowed\n" );
        break;

    case 0x02: /* GET REAL-TIME CLOCK TIME */
        TRACE( "GET REAL-TIME CLOCK TIME\n" );
        {
            SYSTEMTIME systime;
            GetLocalTime( &systime );
            SET_CH( context, BIN_TO_BCD(systime.wHour) );
            SET_CL( context, BIN_TO_BCD(systime.wMinute) );
            SET_DH( context, BIN_TO_BCD(systime.wSecond) );
            SET_DL( context, 0 ); /* FIXME: assume no daylight saving */
            RESET_CFLAG(context);
        }
        break;

    case 0x03: /* SET REAL-TIME CLOCK TIME */
        FIXME( "SET REAL-TIME CLOCK TIME - not allowed\n" );
        break;

    case 0x04: /* GET REAL-TIME CLOCK DATE */
        TRACE( "GET REAL-TIME CLOCK DATE\n" );
        {
            SYSTEMTIME systime;
            GetLocalTime( &systime );
            SET_CH( context, BIN_TO_BCD(systime.wYear / 100) );
            SET_CL( context, BIN_TO_BCD(systime.wYear % 100) );
            SET_DH( context, BIN_TO_BCD(systime.wMonth) );
            SET_DL( context, BIN_TO_BCD(systime.wDay) );
            RESET_CFLAG(context);
        }
        break;

    case 0x05: /* SET REAL-TIME CLOCK DATE */
        FIXME( "SET REAL-TIME CLOCK DATE - not allowed\n" );
        break;

    case 0x06: /* SET ALARM */
        FIXME( "SET ALARM - unimplemented\n" );
        break;

    case 0x07: /* CANCEL ALARM */
        FIXME( "CANCEL ALARM - unimplemented\n" );
        break;

    case 0x08: /* SET RTC ACTIVATED POWER ON MODE */
    case 0x09: /* READ RTC ALARM TIME AND STATUS */
    case 0x0a: /* READ SYSTEM-TIMER DAY COUNTER */
    case 0x0b: /* SET SYSTEM-TIMER DAY COUNTER */
    case 0x0c: /* SET RTC DATE/TIME ACTIVATED POWER-ON MODE */
    case 0x0d: /* RESET RTC DATE/TIME ACTIVATED POWER-ON MODE */
    case 0x0e: /* GET RTC DATE/TIME ALARM AND STATUS */
    case 0x0f: /* INITIALIZE REAL-TIME CLOCK */
        INT_BARF( context, 0x1a );
        break;

    case 0xb0:
        if (CX_reg(context) == 0x4d52 && 
            DX_reg(context) == 0x4349 && 
            AL_reg(context) == 0x01)
        {
            /*
             * Microsoft Real-Time Compression Interface (MRCI).
             * Ignoring this call indicates MRCI is not supported.
             */
            TRACE( "Microsoft Real-Time Compression Interface - not supported\n" );
        }
        else
        {
            INT_BARF(context, 0x1a);
        }
        break;

    default:
        INT_BARF( context, 0x1a );
    }
}
