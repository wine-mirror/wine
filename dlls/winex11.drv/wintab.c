/*
 * X11 tablet driver
 *
 * Copyright 2003 CodeWeavers (Aric Stewart)
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
#include "wine/port.h"

#include <stdlib.h>

#include "windef.h"
#include "x11drv.h"
#include "wine/library.h"
#include "wine/debug.h"
#include "wintab.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab32);

typedef struct tagWTI_CURSORS_INFO
{
    CHAR   NAME[256];
        /* a displayable zero-terminated string containing the name of the
         * cursor.
         */
    BOOL    ACTIVE;
        /* whether the cursor is currently connected. */
    WTPKT   PKTDATA;
        /* a bit mask indicating the packet data items supported when this
         * cursor is connected.
         */
    BYTE    BUTTONS;
        /* the number of buttons on this cursor. */
    BYTE    BUTTONBITS;
        /* the number of bits of raw button data returned by the hardware.*/
    CHAR   BTNNAMES[1024]; /* FIXME: make this dynamic */
        /* a list of zero-terminated strings containing the names of the
         * cursor's buttons. The number of names in the list is the same as the
         * number of buttons on the cursor. The names are separated by a single
         * zero character; the list is terminated by two zero characters.
         */
    BYTE    BUTTONMAP[32];
        /* a 32 byte array of logical button numbers, one for each physical
         * button.
         */
    BYTE    SYSBTNMAP[32];
        /* a 32 byte array of button action codes, one for each logical
         * button.
         */
    BYTE    NPBUTTON;
        /* the physical button number of the button that is controlled by normal
         * pressure.
         */
    UINT    NPBTNMARKS[2];
        /* an array of two UINTs, specifying the button marks for the normal
         * pressure button. The first UINT contains the release mark; the second
         * contains the press mark.
         */
    UINT    *NPRESPONSE;
        /* an array of UINTs describing the pressure response curve for normal
         * pressure.
         */
    BYTE    TPBUTTON;
        /* the physical button number of the button that is controlled by
         * tangential pressure.
         */
    UINT    TPBTNMARKS[2];
        /* an array of two UINTs, specifying the button marks for the tangential
         * pressure button. The first UINT contains the release mark; the second
         * contains the press mark.
         */
    UINT    *TPRESPONSE;
        /* an array of UINTs describing the pressure response curve for
         * tangential pressure.
         */
    DWORD   PHYSID;
         /* a manufacturer-specific physical identifier for the cursor. This
          * value will distinguish the physical cursor from others on the same
          * device. This physical identifier allows applications to bind
          * functions to specific physical cursors, even if category numbers
          * change and multiple, otherwise identical, physical cursors are
          * present.
          */
    UINT    MODE;
        /* the cursor mode number of this cursor type, if this cursor type has
         * the CRC_MULTIMODE capability.
         */
    UINT    MINPKTDATA;
        /* the minimum set of data available from a physical cursor in this
         * cursor type, if this cursor type has the CRC_AGGREGATE capability.
         */
    UINT    MINBUTTONS;
        /* the minimum number of buttons of physical cursors in the cursor type,
         * if this cursor type has the CRC_AGGREGATE capability.
         */
    UINT    CAPABILITIES;
        /* flags indicating cursor capabilities, as defined below:
            CRC_MULTIMODE
                Indicates this cursor type describes one of several modes of a
                single physical cursor. Consecutive cursor type categories
                describe the modes; the CSR_MODE data item gives the mode number
                of each cursor type.
            CRC_AGGREGATE
                Indicates this cursor type describes several physical cursors
                that cannot be distinguished by software.
            CRC_INVERT
                Indicates this cursor type describes the physical cursor in its
                inverted orientation; the previous consecutive cursor type
                category describes the normal orientation.
         */
    UINT    TYPE;
        /* Manufacturer Unique id for the item type */
} WTI_CURSORS_INFO, *LPWTI_CURSORS_INFO;


typedef struct tagWTI_DEVICES_INFO
{
    CHAR   NAME[256];
        /* a displayable null- terminated string describing the device,
         * manufacturer, and revision level.
         */
    UINT    HARDWARE;
        /* flags indicating hardware and driver capabilities, as defined
         * below:
            HWC_INTEGRATED:
                Indicates that the display and digitizer share the same surface.
            HWC_TOUCH
                Indicates that the cursor must be in physical contact with the
                device to report position.
            HWC_HARDPROX
                Indicates that device can generate events when the cursor is
                entering and leaving the physical detection range.
            HWC_PHYSID_CURSORS
                Indicates that device can uniquely identify the active cursor in
                hardware.
         */
    UINT    NCSRTYPES;
        /* the number of supported cursor types.*/
    UINT    FIRSTCSR;
        /* the first cursor type number for the device. */
    UINT    PKTRATE;
        /* the maximum packet report rate in Hertz. */
    WTPKT   PKTDATA;
        /* a bit mask indicating which packet data items are always available.*/
    WTPKT   PKTMODE;
        /* a bit mask indicating which packet data items are physically
         * relative, i.e., items for which the hardware can only report change,
         * not absolute measurement.
         */
    WTPKT   CSRDATA;
        /* a bit mask indicating which packet data items are only available when
         * certain cursors are connected. The individual cursor descriptions
         * must be consulted to determine which cursors return which data.
         */
    INT     XMARGIN;
    INT     YMARGIN;
    INT     ZMARGIN;
        /* the size of tablet context margins in tablet native coordinates, in
         * the x, y, and z directions, respectively.
         */
    AXIS    X;
    AXIS    Y;
    AXIS    Z;
        /* the tablet's range and resolution capabilities, in the x, y, and z
         * axes, respectively.
         */
    AXIS    NPRESSURE;
    AXIS    TPRESSURE;
        /* the tablet's range and resolution capabilities, for the normal and
         * tangential pressure inputs, respectively.
         */
    AXIS    ORIENTATION[3];
        /* a 3-element array describing the tablet's orientation range and
         * resolution capabilities.
         */
    AXIS    ROTATION[3];
        /* a 3-element array describing the tablet's rotation range and
         * resolution capabilities.
         */
    CHAR   PNPID[256];
        /* a null-terminated string containing the devices Plug and Play ID.*/
}   WTI_DEVICES_INFO, *LPWTI_DEVICES_INFO;

typedef struct tagWTPACKET {
        HCTX pkContext;
        UINT pkStatus;
        LONG pkTime;
        WTPKT pkChanged;
        UINT pkSerialNumber;
        UINT pkCursor;
        DWORD pkButtons;
        DWORD pkX;
        DWORD pkY;
        DWORD pkZ;
        UINT pkNormalPressure;
        UINT pkTangentPressure;
        ORIENTATION pkOrientation;
        ROTATION pkRotation; /* 1.1 */
} WTPACKET, *LPWTPACKET;


#ifdef SONAME_LIBXI

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>

static int           motion_type;
static int           button_press_type;
static int           button_release_type;
static int           key_press_type;
static int           key_release_type;
static int           proximity_in_type;
static int           proximity_out_type;

static HWND          hwndTabletDefault;
static WTPACKET      gMsgPacket;
static DWORD         gSerial;
static INT           button_state[10];

#define             CURSORMAX 10

static LOGCONTEXTA      gSysContext;
static WTI_DEVICES_INFO gSysDevice;
static WTI_CURSORS_INFO gSysCursor[CURSORMAX];
static INT              gNumCursors;


/* XInput stuff */
static void *xinput_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XListInputDevices)
MAKE_FUNCPTR(XFreeDeviceList)
MAKE_FUNCPTR(XOpenDevice)
MAKE_FUNCPTR(XQueryDeviceState)
MAKE_FUNCPTR(XGetDeviceButtonMapping)
MAKE_FUNCPTR(XCloseDevice)
MAKE_FUNCPTR(XSelectExtensionEvent)
MAKE_FUNCPTR(XFreeDeviceState)
#undef MAKE_FUNCPTR

static INT X11DRV_XInput_Init(void)
{
    xinput_handle = wine_dlopen(SONAME_LIBXI, RTLD_NOW, NULL, 0);
    if (xinput_handle)
    {
#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(xinput_handle, #f, NULL, 0)) == NULL) goto sym_not_found;
        LOAD_FUNCPTR(XListInputDevices)
        LOAD_FUNCPTR(XFreeDeviceList)
        LOAD_FUNCPTR(XOpenDevice)
        LOAD_FUNCPTR(XGetDeviceButtonMapping)
        LOAD_FUNCPTR(XCloseDevice)
        LOAD_FUNCPTR(XSelectExtensionEvent)
        LOAD_FUNCPTR(XQueryDeviceState)
        LOAD_FUNCPTR(XFreeDeviceState)
#undef LOAD_FUNCPTR
        return 1;
    }
sym_not_found:
    return 0;
}

static int Tablet_ErrorHandler(Display *dpy, XErrorEvent *event, void* arg)
{
    return 1;
}

void X11DRV_LoadTabletInfo(HWND hwnddefault)
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    int num_devices;
    int loop;
    int cursor_target;
    XDeviceInfo *devices;
    XDeviceInfo *target = NULL;
    BOOL    axis_read_complete= FALSE;

    XAnyClassPtr        any;
    XButtonInfoPtr      Button;
    XValuatorInfoPtr    Val;
    XAxisInfoPtr        Axis;

    XDevice *opendevice;

    if (!X11DRV_XInput_Init())
    {
        ERR("Unable to initialized the XInput library.\n");
        return;
    }

    hwndTabletDefault = hwnddefault;

    /* Do base initializaion */
    strcpy(gSysContext.lcName, "Wine Tablet Context");
    strcpy(gSysDevice.NAME,"Wine Tablet Device");

    gSysContext.lcOptions = CXO_SYSTEM;
    gSysContext.lcLocks = CXL_INSIZE | CXL_INASPECT | CXL_MARGIN |
                               CXL_SENSITIVITY | CXL_SYSOUT;

    gSysContext.lcMsgBase= WT_DEFBASE;
    gSysContext.lcDevice = 0;
    gSysContext.lcPktData =
        PK_CONTEXT | PK_STATUS | PK_SERIAL_NUMBER| PK_TIME | PK_CURSOR |
        PK_BUTTONS |  PK_X | PK_Y | PK_NORMAL_PRESSURE | PK_ORIENTATION;
    gSysContext.lcMoveMask=
        PK_BUTTONS |  PK_X | PK_Y | PK_NORMAL_PRESSURE | PK_ORIENTATION;
    gSysContext.lcStatus = CXS_ONTOP;
    gSysContext.lcPktRate = 100;
    gSysContext.lcBtnDnMask = 0xffffffff;
    gSysContext.lcBtnUpMask = 0xffffffff;
    gSysContext.lcSensX = 65536;
    gSysContext.lcSensY = 65536;
    gSysContext.lcSensX = 65536;
    gSysContext.lcSensZ = 65536;
    gSysContext.lcSysSensX= 65536;
    gSysContext.lcSysSensY= 65536;

    /* Device Defaults */
    gSysDevice.HARDWARE = HWC_HARDPROX|HWC_PHYSID_CURSORS;
    gSysDevice.FIRSTCSR= 0;
    gSysDevice.PKTRATE = 100;
    gSysDevice.PKTDATA =
        PK_CONTEXT | PK_STATUS | PK_SERIAL_NUMBER| PK_TIME | PK_CURSOR |
        PK_BUTTONS |  PK_X | PK_Y | PK_NORMAL_PRESSURE | PK_ORIENTATION;
    strcpy(gSysDevice.PNPID,"non-pluginplay");

    wine_tsx11_lock();

    cursor_target = -1;
    devices = pXListInputDevices(data->display, &num_devices);
    if (!devices)
    {
        WARN("XInput Extenstions reported as not avalable\n");
        wine_tsx11_unlock();
        return;
    }
    for (loop=0; loop < num_devices; loop++)
    {
        int class_loop;

        TRACE("Trying device %i(%s)\n",loop,devices[loop].name);
        if (devices[loop].use == IsXExtensionDevice)
        {
            LPWTI_CURSORS_INFO cursor;

            TRACE("Is Extension Device\n");
            cursor_target++;
            target = &devices[loop];
            cursor = &gSysCursor[cursor_target];

            X11DRV_expect_error(data->display, Tablet_ErrorHandler, NULL);
            opendevice = pXOpenDevice(data->display,target->id);
            if (!X11DRV_check_error() && opendevice)
            {
                unsigned char map[32];
                int i;
                int shft = 0;

                X11DRV_expect_error(data->display,Tablet_ErrorHandler,NULL);
                pXGetDeviceButtonMapping(data->display, opendevice, map, 32);
                if (X11DRV_check_error())
                {
                    TRACE("No buttons, Non Tablet Device\n");
                    pXCloseDevice(data->display, opendevice);
                    cursor_target --;
                    continue;
                }

                for (i=0; i< cursor->BUTTONS; i++,shft++)
                {
                    cursor->BUTTONMAP[i] = map[i];
                    cursor->SYSBTNMAP[i] = (1<<shft);
                }
                pXCloseDevice(data->display, opendevice);
            }
            else
            {
                WARN("Unable to open device %s\n",target->name);
                cursor_target --;
                continue;
            }

            strcpy(cursor->NAME,target->name);

            cursor->ACTIVE = 1;
            cursor->PKTDATA = PK_TIME | PK_CURSOR | PK_BUTTONS |  PK_X | PK_Y |
                              PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE |
                              PK_ORIENTATION;

            cursor->PHYSID = cursor_target;
            cursor->NPBUTTON = 1;
            cursor->NPBTNMARKS[0] = 0 ;
            cursor->NPBTNMARKS[1] = 1 ;
            cursor->CAPABILITIES = CRC_MULTIMODE;
            if (strcasecmp(cursor->NAME,"stylus")==0)
                cursor->TYPE = 0x4825;
            if (strcasecmp(cursor->NAME,"eraser")==0)
                cursor->TYPE = 0xc85a;


            any = (XAnyClassPtr) (target->inputclassinfo);

            for (class_loop = 0; class_loop < target->num_classes; class_loop++)
            {
                switch (any->class)
                {
                    case ValuatorClass:
                        if (!axis_read_complete)
                        {
                            Val = (XValuatorInfoPtr) any;
                            Axis = (XAxisInfoPtr) ((char *) Val + sizeof
                                (XValuatorInfo));

                            if (Val->num_axes>=1)
                            {
                                /* Axis 1 is X */
                                gSysDevice.X.axMin = Axis->min_value;
                                gSysDevice.X.axMax= Axis->max_value;
                                gSysDevice.X.axUnits = TU_INCHES;
                                gSysDevice.X.axResolution = Axis->resolution;
                                gSysContext.lcInOrgX = Axis->min_value;
                                gSysContext.lcSysOrgX = Axis->min_value;
                                gSysContext.lcInExtX = Axis->max_value;
                                gSysContext.lcSysExtX = Axis->max_value;
                                Axis++;
                            }
                            if (Val->num_axes>=2)
                            {
                                /* Axis 2 is Y */
                                gSysDevice.Y.axMin = Axis->min_value;
                                gSysDevice.Y.axMax= Axis->max_value;
                                gSysDevice.Y.axUnits = TU_INCHES;
                                gSysDevice.Y.axResolution = Axis->resolution;
                                gSysContext.lcInOrgY = Axis->min_value;
                                gSysContext.lcSysOrgY = Axis->min_value;
                                gSysContext.lcInExtY = Axis->max_value;
                                gSysContext.lcSysExtY = Axis->max_value;
                                Axis++;
                            }
                            if (Val->num_axes>=3)
                            {
                                /* Axis 3 is Normal Pressure */
                                gSysDevice.NPRESSURE.axMin = Axis->min_value;
                                gSysDevice.NPRESSURE.axMax= Axis->max_value;
                                gSysDevice.NPRESSURE.axUnits = TU_INCHES;
                                gSysDevice.NPRESSURE.axResolution =
                                                        Axis->resolution;
                                Axis++;
                            }
                            if (Val->num_axes >= 5)
                            {
                                /* Axis 4 and 5 are X and Y tilt */
                                XAxisInfoPtr        XAxis = Axis;
                                Axis++;
                                if (max (abs(Axis->max_value),
                                         abs(XAxis->max_value)))
                                {
                                    gSysDevice.ORIENTATION[0].axMin = 0;
                                    gSysDevice.ORIENTATION[0].axMax = 3600;
                                    gSysDevice.ORIENTATION[0].axUnits = TU_CIRCLE;
                                    gSysDevice.ORIENTATION[0].axResolution
                                                                = CASTFIX32(3600);
                                    gSysDevice.ORIENTATION[1].axMin = -1000;
                                    gSysDevice.ORIENTATION[1].axMax = 1000;
                                    gSysDevice.ORIENTATION[1].axUnits = TU_CIRCLE;
                                    gSysDevice.ORIENTATION[1].axResolution
                                                                = CASTFIX32(3600);
                                    Axis++;
                                }
                            }
                            axis_read_complete = TRUE;
                        }
                        break;
                    case ButtonClass:
                    {
                        CHAR *ptr = cursor->BTNNAMES;
                        int i;

                        Button = (XButtonInfoPtr) any;
                        cursor->BUTTONS = Button->num_buttons;
                        for (i = 0; i < cursor->BUTTONS; i++)
                        {
                            strcpy(ptr,cursor->NAME);
                            ptr+=8;
                        }
                    }
                    break;
                }
                any = (XAnyClassPtr) ((char*) any + any->length);
            }
        }
    }
    pXFreeDeviceList(devices);
    wine_tsx11_unlock();
    gSysDevice.NCSRTYPES = cursor_target+1;
    gNumCursors = cursor_target+1;
}

static int figure_deg(int x, int y)
{
    int rc;

    if (y != 0)
    {
        rc = (int) 10 * (atan( (FLOAT)abs(y) / (FLOAT)abs(x)) / (3.1415 / 180));
        if (y>0)
        {
            if (x>0)
                rc += 900;
            else
                rc = 2700 - rc;
        }
        else
        {
            if (x>0)
                rc = 900 - rc;
            else
                rc += 2700;
        }
    }
    else
    {
        if (x >= 0)
            rc = 900;
        else
            rc = 2700;
    }

    return rc;
}

static int get_button_state(int deviceid)
{
    return button_state[deviceid];
}

static void set_button_state(XID deviceid)
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    XDevice *device;
    XDeviceState *state;
    XInputClass  *class;
    int loop;
    int rc = 0;

    wine_tsx11_lock();
    device = pXOpenDevice(data->display,deviceid);
    state = pXQueryDeviceState(data->display,device);

    if (state)
    {
        class = state->data;
        for (loop = 0; loop < state->num_classes; loop++)
        {
            if (class->class == ButtonClass)
            {
                int loop2;
                XButtonState *button_state =  (XButtonState*)class;
                for (loop2 = 0; loop2 < button_state->num_buttons; loop2++)
                {
                    if (button_state->buttons[loop2 / 8] & (1 << (loop2 % 8)))
                    {
                        rc |= (1<<loop2);
                    }
                }
            }
            class = (XInputClass *) ((char *) class + class->length);
        }
    }
    pXFreeDeviceState(state);
    wine_tsx11_unlock();
    button_state[deviceid] = rc;
}

static void motion_event( HWND hwnd, XEvent *event )
{
    XDeviceMotionEvent *motion = (XDeviceMotionEvent *)event;
    LPWTI_CURSORS_INFO cursor = &gSysCursor[motion->deviceid];

    memset(&gMsgPacket,0,sizeof(WTPACKET));

    TRACE("Received tablet motion event (%p)\n",hwnd);

    /* Set cursor to inverted if cursor is the eraser */
    gMsgPacket.pkStatus = (cursor->TYPE == 0xc85a ?TPS_INVERT:0);
    gMsgPacket.pkTime = EVENT_x11_time_to_win32_time(motion->time);
    gMsgPacket.pkSerialNumber = gSerial++;
    gMsgPacket.pkCursor = motion->deviceid;
    gMsgPacket.pkX = motion->axis_data[0];
    gMsgPacket.pkY = motion->axis_data[1];
    gMsgPacket.pkOrientation.orAzimuth = figure_deg(motion->axis_data[3],motion->axis_data[4]);
    gMsgPacket.pkOrientation.orAltitude = ((1000 - 15 * max
                                            (abs(motion->axis_data[3]),
                                             abs(motion->axis_data[4])))
                                           * (gMsgPacket.pkStatus & TPS_INVERT?-1:1));
    gMsgPacket.pkNormalPressure = motion->axis_data[2];
    gMsgPacket.pkButtons = get_button_state(motion->deviceid);
    SendMessageW(hwndTabletDefault,WT_PACKET,0,(LPARAM)hwnd);
}

static void button_event( HWND hwnd, XEvent *event )
{
    XDeviceButtonEvent *button = (XDeviceButtonEvent *) event;
    LPWTI_CURSORS_INFO cursor = &gSysCursor[button->deviceid];

    memset(&gMsgPacket,0,sizeof(WTPACKET));

    TRACE("Received tablet button %s event\n", (event->type == button_press_type)?"press":"release");

    /* Set cursor to inverted if cursor is the eraser */
    gMsgPacket.pkStatus = (cursor->TYPE == 0xc85a ?TPS_INVERT:0);
    set_button_state(button->deviceid);
    gMsgPacket.pkTime = EVENT_x11_time_to_win32_time(button->time);
    gMsgPacket.pkSerialNumber = gSerial++;
    gMsgPacket.pkCursor = button->deviceid;
    gMsgPacket.pkX = button->axis_data[0];
    gMsgPacket.pkY = button->axis_data[1];
    gMsgPacket.pkOrientation.orAzimuth = figure_deg(button->axis_data[3],button->axis_data[4]);
    gMsgPacket.pkOrientation.orAltitude = ((1000 - 15 * max(abs(button->axis_data[3]),
                                                            abs(button->axis_data[4])))
                                           * (gMsgPacket.pkStatus & TPS_INVERT?-1:1));
    gMsgPacket.pkNormalPressure = button->axis_data[2];
    gMsgPacket.pkButtons = get_button_state(button->deviceid);
    SendMessageW(hwndTabletDefault,WT_PACKET,0,(LPARAM)hwnd);
}

static void key_event( HWND hwnd, XEvent *event )
{
    if (event->type == key_press_type)
        FIXME("Received tablet key press event\n");
    else
        FIXME("Received tablet key release event\n");
}

static void proximity_event( HWND hwnd, XEvent *event )
{
    XProximityNotifyEvent *proximity = (XProximityNotifyEvent *) event;
    LPWTI_CURSORS_INFO cursor = &gSysCursor[proximity->deviceid];

    memset(&gMsgPacket,0,sizeof(WTPACKET));

    TRACE("Received tablet proximity event\n");
    /* Set cursor to inverted if cursor is the eraser */
    gMsgPacket.pkStatus = (cursor->TYPE == 0xc85a ?TPS_INVERT:0);
    gMsgPacket.pkStatus |= (event->type==proximity_out_type)?TPS_PROXIMITY:0;
    gMsgPacket.pkTime = EVENT_x11_time_to_win32_time(proximity->time);
    gMsgPacket.pkSerialNumber = gSerial++;
    gMsgPacket.pkCursor = proximity->deviceid;
    gMsgPacket.pkX = proximity->axis_data[0];
    gMsgPacket.pkY = proximity->axis_data[1];
    gMsgPacket.pkOrientation.orAzimuth = figure_deg(proximity->axis_data[3],proximity->axis_data[4]);
    gMsgPacket.pkOrientation.orAltitude = ((1000 - 15 * max(abs(proximity->axis_data[3]),
                                                            abs(proximity->axis_data[4])))
                                           * (gMsgPacket.pkStatus & TPS_INVERT?-1:1));
    gMsgPacket.pkNormalPressure = proximity->axis_data[2];
    gMsgPacket.pkButtons = get_button_state(proximity->deviceid);

    SendMessageW(hwndTabletDefault, WT_PROXIMITY, (event->type == proximity_in_type), (LPARAM)hwnd);
}

int X11DRV_AttachEventQueueToTablet(HWND hOwner)
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    int             num_devices;
    int             loop;
    int             cur_loop;
    XDeviceInfo     *devices;
    XDeviceInfo     *target = NULL;
    XDevice         *the_device;
    XEventClass     event_list[7];
    Window          win = X11DRV_get_whole_window( hOwner );

    if (!win) return 0;

    TRACE("Creating context for window %p (%lx)  %i cursors\n", hOwner, win, gNumCursors);

    wine_tsx11_lock();
    devices = pXListInputDevices(data->display, &num_devices);

    X11DRV_expect_error(data->display,Tablet_ErrorHandler,NULL);
    for (cur_loop=0; cur_loop < gNumCursors; cur_loop++)
    {
        int    event_number=0;

        for (loop=0; loop < num_devices; loop ++)
            if (strcmp(devices[loop].name,gSysCursor[cur_loop].NAME)==0)
                target = &devices[loop];

        TRACE("Opening cursor %i id %i\n",cur_loop,(INT)target->id);

        the_device = pXOpenDevice(data->display, target->id);

        if (!the_device)
        {
            WARN("Unable to Open device\n");
            continue;
        }

        if (the_device->num_classes > 0)
        {
            DeviceKeyPress(the_device, key_press_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;
            DeviceKeyRelease(the_device, key_release_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;
            DeviceButtonPress(the_device, button_press_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;
            DeviceButtonRelease(the_device, button_release_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;
            DeviceMotionNotify(the_device, motion_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;
            ProximityIn(the_device, proximity_in_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;
            ProximityOut(the_device, proximity_out_type, event_list[event_number]);
            if (event_list[event_number]) event_number++;

            if (key_press_type) X11DRV_register_event_handler( key_press_type, key_event );
            if (key_release_type) X11DRV_register_event_handler( key_release_type, key_event );
            if (button_press_type) X11DRV_register_event_handler( button_press_type, button_event );
            if (button_release_type) X11DRV_register_event_handler( button_release_type, button_event );
            if (motion_type) X11DRV_register_event_handler( motion_type, motion_event );
            if (proximity_in_type) X11DRV_register_event_handler( proximity_in_type, proximity_event );
            if (proximity_out_type) X11DRV_register_event_handler( proximity_out_type, proximity_event );

            pXSelectExtensionEvent(data->display, win, event_list, event_number);
        }
    }
    XSync(data->display, False);
    X11DRV_check_error();

    if (NULL != devices) pXFreeDeviceList(devices);
    wine_tsx11_unlock();
    return 0;
}

int X11DRV_GetCurrentPacket(LPWTPACKET *packet)
{
    memcpy(packet,&gMsgPacket,sizeof(WTPACKET));
    return 1;
}


static inline int CopyTabletData(LPVOID target, LPVOID src, INT size)
{
    /*
     * It is valid to call CopyTabletData with NULL.
     * This handles the WTInfo() case where lpOutput is null.
     */
    if(target != NULL)
        memcpy(target,src,size);
    return(size);
}

/***********************************************************************
 *		X11DRV_WTInfoA (X11DRV.@)
 */
UINT X11DRV_WTInfoA(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    /*
     * It is valid to call WTInfoA with lpOutput == NULL, as per standard.
     * lpOutput == NULL signifies the user only wishes
     * to find the size of the data.
     * NOTE:
     * From now on use CopyTabletData to fill lpOutput. memcpy will break
     * the code.
     */
    int rc = 0;
    LPWTI_CURSORS_INFO  tgtcursor;
    TRACE("(%u, %u, %p)\n", wCategory, nIndex, lpOutput);

    switch(wCategory)
    {
        case 0:
            /* return largest necessary buffer */
            TRACE("%i cursors\n",gNumCursors);
            if (gNumCursors>0)
            {
                FIXME("Return proper size\n");
                rc = 200;
            }
            break;
        case WTI_INTERFACE:
            switch (nIndex)
            {
                WORD version;
                case IFC_WINTABID:
                    strcpy(lpOutput,"Wine Wintab 1.1");
                    rc = 16;
                    break;
                case IFC_SPECVERSION:
                    version = (0x01) | (0x01 << 8);
                    rc = CopyTabletData(lpOutput, &version,sizeof(WORD));
                    break;
                case IFC_IMPLVERSION:
                    version = (0x00) | (0x01 << 8);
                    rc = CopyTabletData(lpOutput, &version,sizeof(WORD));
                    break;
                default:
                    FIXME("WTI_INTERFACE unhandled index %i\n",nIndex);
                    rc = 0;
            }
            break;
        case WTI_DEFSYSCTX:
        case WTI_DDCTXS:
        case WTI_DEFCONTEXT:
            switch (nIndex)
            {
                case 0:
                    rc = CopyTabletData(lpOutput, &gSysContext,
                            sizeof(LOGCONTEXTA));
                    break;
                case CTX_NAME:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcName,
                         strlen(gSysContext.lcName)+1);
                    break;
                case CTX_OPTIONS:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcOptions,
                                        sizeof(UINT));
                    break;
                case CTX_STATUS:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcStatus,
                                        sizeof(UINT));
                    break;
                case CTX_LOCKS:
                    rc= CopyTabletData (lpOutput, &gSysContext.lcLocks,
                                        sizeof(UINT));
                    break;
                case CTX_MSGBASE:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcMsgBase,
                                        sizeof(UINT));
                    break;
                case CTX_DEVICE:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcDevice,
                                        sizeof(UINT));
                    break;
                case CTX_PKTRATE:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcPktRate,
                                        sizeof(UINT));
                    break;
                case CTX_PKTMODE:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcPktMode,
                                        sizeof(WTPKT));
                    break;
                case CTX_MOVEMASK:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcMoveMask,
                                        sizeof(WTPKT));
                    break;
                case CTX_BTNDNMASK:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcBtnDnMask,
                                        sizeof(DWORD));
                    break;
                case CTX_BTNUPMASK:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcBtnUpMask,
                                        sizeof(DWORD));
                    break;
                case CTX_INORGX:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcInOrgX,
                                        sizeof(LONG));
                    break;
                case CTX_INORGY:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcInOrgY,
                                        sizeof(LONG));
                    break;
                case CTX_INORGZ:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcInOrgZ,
                                        sizeof(LONG));
                    break;
                case CTX_INEXTX:
                    rc = CopyTabletData(lpOutput, &gSysContext.lcInExtX,
                                        sizeof(LONG));
                    break;
                case CTX_INEXTY:
                     rc = CopyTabletData(lpOutput, &gSysContext.lcInExtY,
                                        sizeof(LONG));
                    break;
                case CTX_INEXTZ:
                     rc = CopyTabletData(lpOutput, &gSysContext.lcInExtZ,
                                        sizeof(LONG));
                    break;
                case CTX_OUTORGX:
                     rc = CopyTabletData(lpOutput, &gSysContext.lcOutOrgX,
                                        sizeof(LONG));
                    break;
                case CTX_OUTORGY:
                      rc = CopyTabletData(lpOutput, &gSysContext.lcOutOrgY,
                                        sizeof(LONG));
                    break;
                case CTX_OUTORGZ:
                       rc = CopyTabletData(lpOutput, &gSysContext.lcOutOrgZ,
                                        sizeof(LONG));
                    break;
               case CTX_OUTEXTX:
                      rc = CopyTabletData(lpOutput, &gSysContext.lcOutExtX,
                                        sizeof(LONG));
                    break;
                case CTX_OUTEXTY:
                       rc = CopyTabletData(lpOutput, &gSysContext.lcOutExtY,
                                        sizeof(LONG));
                    break;
                case CTX_OUTEXTZ:
                       rc = CopyTabletData(lpOutput, &gSysContext.lcOutExtZ,
                                        sizeof(LONG));
                    break;
                case CTX_SENSX:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSensX,
                                        sizeof(LONG));
                    break;
                case CTX_SENSY:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSensY,
                                        sizeof(LONG));
                    break;
                case CTX_SENSZ:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSensZ,
                                        sizeof(LONG));
                    break;
                case CTX_SYSMODE:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSysMode,
                                        sizeof(LONG));
                    break;
                case CTX_SYSORGX:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSysOrgX,
                                        sizeof(LONG));
                    break;
                case CTX_SYSORGY:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSysOrgY,
                                        sizeof(LONG));
                    break;
                case CTX_SYSEXTX:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSysExtX,
                                        sizeof(LONG));
                    break;
                case CTX_SYSEXTY:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSysExtY,
                                        sizeof(LONG));
                    break;
                case CTX_SYSSENSX:
                        rc = CopyTabletData(lpOutput, &gSysContext.lcSysSensX,
                                        sizeof(LONG));
                    break;
                case CTX_SYSSENSY:
                       rc = CopyTabletData(lpOutput, &gSysContext.lcSysSensY,
                                        sizeof(LONG));
                    break;
                default:
                    FIXME("WTI_DEFSYSCTX unhandled index %i\n",nIndex);
                    rc = 0;
            }
            break;
        case WTI_CURSORS:
        case WTI_CURSORS+1:
        case WTI_CURSORS+2:
        case WTI_CURSORS+3:
        case WTI_CURSORS+4:
        case WTI_CURSORS+5:
        case WTI_CURSORS+6:
        case WTI_CURSORS+7:
        case WTI_CURSORS+8:
        case WTI_CURSORS+9:
            tgtcursor = &gSysCursor[wCategory - WTI_CURSORS];
            switch (nIndex)
            {
                case CSR_NAME:
                    rc = CopyTabletData(lpOutput, &tgtcursor->NAME,
                                        strlen(tgtcursor->NAME)+1);
                    break;
                case CSR_ACTIVE:
                    rc = CopyTabletData(lpOutput,&tgtcursor->ACTIVE,
                                        sizeof(BOOL));
                    break;
                case CSR_PKTDATA:
                    rc = CopyTabletData(lpOutput,&tgtcursor->PKTDATA,
                                        sizeof(WTPKT));
                    break;
                case CSR_BUTTONS:
                    rc = CopyTabletData(lpOutput,&tgtcursor->BUTTONS,
                                        sizeof(BYTE));
                    break;
                case CSR_BUTTONBITS:
                    rc = CopyTabletData(lpOutput,&tgtcursor->BUTTONBITS,
                                        sizeof(BYTE));
                    break;
                case CSR_BTNNAMES:
                    FIXME("Button Names not returned correctly\n");
                    rc = CopyTabletData(lpOutput,&tgtcursor->BTNNAMES,
                                        strlen(tgtcursor->BTNNAMES)+1);
                    break;
                case CSR_BUTTONMAP:
                    rc = CopyTabletData(lpOutput,&tgtcursor->BUTTONMAP,
                                        sizeof(BYTE)*32);
                    break;
                case CSR_SYSBTNMAP:
                    rc = CopyTabletData(lpOutput,&tgtcursor->SYSBTNMAP,
                                        sizeof(BYTE)*32);
                    break;
                case CSR_NPBTNMARKS:
                    rc = CopyTabletData(lpOutput,&tgtcursor->NPBTNMARKS,
                                        sizeof(UINT)*2);
                    break;
                case CSR_NPBUTTON:
                    rc = CopyTabletData(lpOutput,&tgtcursor->NPBUTTON,
                                        sizeof(BYTE));
                    break;
                case CSR_NPRESPONSE:
                    FIXME("Not returning CSR_NPRESPONSE correctly\n");
                    rc = 0;
                    break;
                case CSR_TPBUTTON:
                    rc = CopyTabletData(lpOutput,&tgtcursor->TPBUTTON,
                                        sizeof(BYTE));
                    break;
                case CSR_TPBTNMARKS:
                    rc = CopyTabletData(lpOutput,&tgtcursor->TPBTNMARKS,
                                        sizeof(UINT)*2);
                    break;
                case CSR_TPRESPONSE:
                    FIXME("Not returning CSR_TPRESPONSE correctly\n");
                    rc = 0;
                    break;
                case CSR_PHYSID:
                {
                    DWORD id;
                    id = tgtcursor->PHYSID;
                    id += (wCategory - WTI_CURSORS);
                    rc = CopyTabletData(lpOutput,&id,sizeof(DWORD));
                }
                    break;
                case CSR_MODE:
                    rc = CopyTabletData(lpOutput,&tgtcursor->MODE,sizeof(UINT));
                    break;
                case CSR_MINPKTDATA:
                    rc = CopyTabletData(lpOutput,&tgtcursor->MINPKTDATA,
                        sizeof(UINT));
                    break;
                case CSR_MINBUTTONS:
                    rc = CopyTabletData(lpOutput,&tgtcursor->MINBUTTONS,
                        sizeof(UINT));
                    break;
                case CSR_CAPABILITIES:
                    rc = CopyTabletData(lpOutput,&tgtcursor->CAPABILITIES,
                        sizeof(UINT));
                    break;
                case CSR_TYPE:
                    rc = CopyTabletData(lpOutput,&tgtcursor->TYPE,
                        sizeof(UINT));
                    break;
                default:
                    FIXME("WTI_CURSORS unhandled index %i\n",nIndex);
                    rc = 0;
            }
            break;
        case WTI_DEVICES:
            switch (nIndex)
            {
                case DVC_NAME:
                    rc = CopyTabletData(lpOutput,gSysDevice.NAME,
                                        strlen(gSysDevice.NAME)+1);
                    break;
                case DVC_HARDWARE:
                    rc = CopyTabletData(lpOutput,&gSysDevice.HARDWARE,
                                        sizeof(UINT));
                    break;
                case DVC_NCSRTYPES:
                    rc = CopyTabletData(lpOutput,&gSysDevice.NCSRTYPES,
                                        sizeof(UINT));
                    break;
                case DVC_FIRSTCSR:
                    rc = CopyTabletData(lpOutput,&gSysDevice.FIRSTCSR,
                                        sizeof(UINT));
                    break;
                case DVC_PKTRATE:
                    rc = CopyTabletData(lpOutput,&gSysDevice.PKTRATE,
                                        sizeof(UINT));
                    break;
                case DVC_PKTDATA:
                    rc = CopyTabletData(lpOutput,&gSysDevice.PKTDATA,
                                        sizeof(WTPKT));
                    break;
                case DVC_PKTMODE:
                    rc = CopyTabletData(lpOutput,&gSysDevice.PKTMODE,
                                        sizeof(WTPKT));
                    break;
                case DVC_CSRDATA:
                    rc = CopyTabletData(lpOutput,&gSysDevice.CSRDATA,
                                        sizeof(WTPKT));
                    break;
                case DVC_XMARGIN:
                    rc = CopyTabletData(lpOutput,&gSysDevice.XMARGIN,
                                        sizeof(INT));
                    break;
                case DVC_YMARGIN:
                    rc = CopyTabletData(lpOutput,&gSysDevice.YMARGIN,
                                        sizeof(INT));
                    break;
                case DVC_ZMARGIN:
                    rc = 0; /* unsupported */
                    /*
                    rc = CopyTabletData(lpOutput,&gSysDevice.ZMARGIN,
                                        sizeof(INT));
                    */
                    break;
                case DVC_X:
                    rc = CopyTabletData(lpOutput,&gSysDevice.X,
                                        sizeof(AXIS));
                    break;
                case DVC_Y:
                    rc = CopyTabletData(lpOutput,&gSysDevice.Y,
                                        sizeof(AXIS));
                    break;
                case DVC_Z:
                    rc = 0; /* unsupported */
                    /*
                    rc = CopyTabletData(lpOutput,&gSysDevice.Z,
                                        sizeof(AXIS));
                    */
                    break;
                case DVC_NPRESSURE:
                    rc = CopyTabletData(lpOutput,&gSysDevice.NPRESSURE,
                                        sizeof(AXIS));
                    break;
                case DVC_TPRESSURE:
                    rc = 0; /* unsupported */
                    /*
                    rc = CopyTabletData(lpOutput,&gSysDevice.TPRESSURE,
                                        sizeof(AXIS));
                    */
                    break;
                case DVC_ORIENTATION:
                    rc = CopyTabletData(lpOutput,&gSysDevice.ORIENTATION,
                                        sizeof(AXIS)*3);
                    break;
                case DVC_ROTATION:
                    rc = 0; /* unsupported */
                    /*
                    rc = CopyTabletData(lpOutput,&gSysDevice.ROTATION,
                                        sizeof(AXIS)*3);
                    */
                    break;
                case DVC_PNPID:
                    rc = CopyTabletData(lpOutput,gSysDevice.PNPID,
                                        strlen(gSysDevice.PNPID)+1);
                    break;
                default:
                    FIXME("WTI_DEVICES unhandled index %i\n",nIndex);
                    rc = 0;
            }
            break;
        default:
            FIXME("Unhandled Category %i\n",wCategory);
    }
    return rc;
}

#else /* SONAME_LIBXI */

/***********************************************************************
 *		AttachEventQueueToTablet (X11DRV.@)
 */
int X11DRV_AttachEventQueueToTablet(HWND hOwner)
{
    return 0;
}

/***********************************************************************
 *		GetCurrentPacket (X11DRV.@)
 */
int X11DRV_GetCurrentPacket(LPWTPACKET *packet)
{
    return 0;
}

/***********************************************************************
 *		LoadTabletInfo (X11DRV.@)
 */
void X11DRV_LoadTabletInfo(HWND hwnddefault)
{
}

/***********************************************************************
 *		WTInfoA (X11DRV.@)
 */
UINT X11DRV_WTInfoA(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    return 0;
}

#endif /* SONAME_LIBXI */
