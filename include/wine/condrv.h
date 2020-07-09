/*
 * Console driver ioctls
 *
 * Copyright 2020 Jacek Caban for CodeWeavers
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

#ifndef _INC_CONDRV
#define _INC_CONDRV

#include "winioctl.h"

/* console input ioctls */
#define IOCTL_CONDRV_READ_INPUT            CTL_CODE(FILE_DEVICE_CONSOLE, 10, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_WRITE_INPUT           CTL_CODE(FILE_DEVICE_CONSOLE, 11, METHOD_BUFFERED, FILE_WRITE_PROPERTIES)
#define IOCTL_CONDRV_PEEK                  CTL_CODE(FILE_DEVICE_CONSOLE, 12, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_GET_INPUT_INFO        CTL_CODE(FILE_DEVICE_CONSOLE, 13, METHOD_BUFFERED, FILE_READ_PROPERTIES)

/* console output ioctls */
#define IOCTL_CONDRV_GET_OUTPUT_INFO       CTL_CODE(FILE_DEVICE_CONSOLE, 32, METHOD_BUFFERED, FILE_WRITE_PROPERTIES)

/* console renderer ioctls */
#define IOCTL_CONDRV_GET_RENDERER_EVENTS   CTL_CODE(FILE_DEVICE_CONSOLE, 70, METHOD_BUFFERED, FILE_READ_PROPERTIES)

/* IOCTL_CONDRV_GET_INPUT_INFO result */
struct condrv_input_info
{
    unsigned int  history_mode;   /* whether we duplicate lines in history */
    unsigned int  history_size;   /* number of lines in history */
    unsigned int  edition_mode;   /* index to the edition mode flavors */
    unsigned int  input_count;    /* number of available input records */
};

/* IOCTL_CONDRV_GET_OUTPUT_INFO result */
struct condrv_output_info
{
    short int     cursor_size;        /* size of cursor (percentage filled) */
    short int     cursor_visible;     /* cursor visibility flag */
    short int     cursor_x;           /* position of cursor (x, y) */
    short int     cursor_y;
    short int     width;              /* width of the screen buffer */
    short int     height;             /* height of the screen buffer */
    short int     attr;               /* default fill attributes (screen colors) */
    short int     popup_attr;         /* pop-up color attributes */
    short int     win_left;           /* window actually displayed by renderer */
    short int     win_top;            /* the rect area is expressed within the */
    short int     win_right;          /* boundaries of the screen buffer */
    short int     win_bottom;
    short int     max_width;          /* maximum size (width x height) for the window */
    short int     max_height;
    short int     font_width;         /* font size (width x height) */
    short int     font_height;
    short int     font_weight;        /* font weight */
    short int     font_pitch_family;  /* font pitch & family */
    unsigned int  color_map[16];      /* color table */
};

/* IOCTL_CONDRV_GET_RENDERER_EVENTS result */
struct condrv_renderer_event
{
    short event;
    union
    {
        struct
        {
            short top;
            short bottom;
        } update;
        struct
        {
            short width;
            short height;
        } resize;
        struct
        {
            short x;
            short y;
        } cursor_pos;
        struct
        {
            short visible;
            short size;
        } cursor_geom;
        struct
        {
            short left;
            short top;
            short width;
            short height;
        } display;
    } u;
};

enum condrv_renderer_event_type
{
    CONSOLE_RENDERER_NONE_EVENT,
    CONSOLE_RENDERER_TITLE_EVENT,
    CONSOLE_RENDERER_ACTIVE_SB_EVENT,
    CONSOLE_RENDERER_SB_RESIZE_EVENT,
    CONSOLE_RENDERER_UPDATE_EVENT,
    CONSOLE_RENDERER_CURSOR_POS_EVENT,
    CONSOLE_RENDERER_CURSOR_GEOM_EVENT,
    CONSOLE_RENDERER_DISPLAY_EVENT,
    CONSOLE_RENDERER_EXIT_EVENT,
};

#endif /* _INC_CONDRV */
