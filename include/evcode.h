/*
 * Copyright (C) 2004 Christian Costa
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

#ifndef __WINE_EVCODE_H
#define __WINE_EVCODE_H

#define EC_SYSTEMBASE                       0x00
#define EC_USER                             0x8000

#define EC_COMPLETE                         0x01
#define EC_USERABORT                        0x02
#define EC_ERRORABORT                       0x03
#define EC_TIME                             0x04
#define EC_REPAINT                          0x05
#define EC_STREAM_ERROR_STOPPED             0x06
#define EC_STREAM_ERROR_STILLPLAYING        0x07
#define EC_ERROR_STILLPLAYING               0x08
#define EC_PALETTE_CHANGED                  0x09
#define EC_VIDEO_SIZE_CHANGED               0x0A
#define EC_QUALITY_CHANGE                   0x0B
#define EC_SHUTTING_DOWN                    0x0C
#define EC_CLOCK_CHANGED                    0x0D
#define EC_OPENING_FILE                     0x10
#define EC_BUFFERING_DATA                   0x11
#define EC_FULLSCREEN_LOST                  0x12
#define EC_ACTIVATE                         0x13
#define EC_NEED_RESTART                     0x14
#define EC_WINDOW_DESTROYED                 0x15
#define EC_DISPLAY_CHANGED                  0x16
#define EC_STARVATION                       0x17
#define EC_OLE_EVENT                        0x18
#define EC_NOTIFY_WINDOW                    0x19
#define EC_STREAM_CONTROL_STOPPED           0x1A
#define EC_STREAM_CONTROL_STARTED           0x1B
#define EC_END_OF_SEGMENT                   0x1C
#define EC_SEGMENT_STARTED                  0x1D
#define EC_LENGTH_CHANGED                   0x1E

#endif /* __WINE_EVCODE_H */
