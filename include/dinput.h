#ifndef _WINE_DINPUT_H
#define _WINE_DINPUT_H

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

#define DIRECTINPUT_VERSION	0x0500

/* Classes */
DEFINE_GUID(CLSID_DirectInput,		0x25E609E0,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice,	0x25E609E1,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

/* Interfaces */
DEFINE_GUID(IID_IDirectInputA,		0x89521360,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputW,		0x89521361,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInput2A,		0x5944E662,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInput2W,		0x5944E663,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDeviceA,	0x5944E680,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDeviceW,	0x5944E681,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDevice2A,	0x5944E682,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDevice2W,	0x5944E683,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputEffect,	0xE7E1F7C0,0x88D2,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);

/* Predefined object types */
DEFINE_GUID(GUID_XAxis,	0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_YAxis,	0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_ZAxis,	0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RxAxis,0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RyAxis,0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RzAxis,0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Slider,0xA36D02E4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Button,0xA36D02F0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Key,	0x55728220,0xD33C,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_POV,	0xA36D02F2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Unknown,0xA36D02F3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

/* Predefined product GUIDs */
DEFINE_GUID(GUID_SysMouse,	0x6F1D2B60,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_SysKeyboard,	0x6F1D2B61,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Joystick,	0x6F1D2B70,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

/* predefined forcefeedback effects */
DEFINE_GUID(GUID_ConstantForce,	0x13541C20,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_RampForce,	0x13541C21,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Square,	0x13541C22,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Sine,		0x13541C23,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Triangle,	0x13541C24,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_SawtoothUp,	0x13541C25,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_SawtoothDown,	0x13541C26,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Spring,	0x13541C27,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Damper,	0x13541C28,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Inertia,	0x13541C29,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Friction,	0x13541C2A,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_CustomForce,	0x13541C2B,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);

typedef struct IDirectInput32A IDirectInput32A,*LPDIRECTINPUT32A;
typedef struct IDirectInputDevice32A IDirectInputDevice32A,*LPDIRECTINPUTDEVICE32A;
typedef struct SysKeyboard32A SysKeyboard32A,*LPSYSKEYBOARD32A;
typedef struct SysMouse32A SysMouse32A,*LPSYSMOUSE32A;

#define DI_OK                           S_OK
#define DI_NOTATTACHED                  S_FALSE
#define DI_BUFFEROVERFLOW               S_FALSE
#define DI_PROPNOEFFECT                 S_FALSE
#define DI_NOEFFECT                     S_FALSE
#define DI_POLLEDDEVICE                 ((HRESULT)0x00000002L)
#define DI_DOWNLOADSKIPPED              ((HRESULT)0x00000003L)
#define DI_EFFECTRESTARTED              ((HRESULT)0x00000004L)
#define DI_TRUNCATED                    ((HRESULT)0x00000008L)
#define DI_TRUNCATEDANDRESTARTED        ((HRESULT)0x0000000CL)

#define DIERR_OLDDIRECTINPUTVERSION     \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OLD_WIN_VERSION)
#define DIERR_BETADIRECTINPUTVERSION    \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_RMODE_APP)
#define DIERR_BADDRIVERVER              \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BAD_DRIVER_LEVEL)
#define DIERR_DEVICENOTREG              REGDB_E_CLASSNOTREG
#define DIERR_NOTFOUND                  \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)
#define DIERR_OBJECTNOTFOUND            \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)
#define DIERR_INVALIDPARAM              E_INVALIDARG
#define DIERR_NOINTERFACE               E_NOINTERFACE
#define DIERR_GENERIC                   E_FAIL
#define DIERR_OUTOFMEMORY               E_OUTOFMEMORY
#define DIERR_UNSUPPORTED               E_NOTIMPL
#define DIERR_NOTINITIALIZED            \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_READY)
#define DIERR_ALREADYINITIALIZED        \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_ALREADY_INITIALIZED)
#define DIERR_NOAGGREGATION             CLASS_E_NOAGGREGATION
#define DIERR_OTHERAPPHASPRIO           E_ACCESSDENIED
#define DIERR_INPUTLOST                 \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_READ_FAULT)
#define DIERR_ACQUIRED                  \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BUSY)
#define DIERR_NOTACQUIRED               \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_ACCESS)
#define DIERR_READONLY                  E_ACCESSDENIED
#define DIERR_HANDLEEXISTS              E_ACCESSDENIED
#ifndef E_PENDING
#define E_PENDING                       0x8000000AL
#endif
#define DIERR_INSUFFICIENTPRIVS         0x80040200L
#define DIERR_DEVICEFULL                0x80040201L
#define DIERR_MOREDATA                  0x80040202L
#define DIERR_NOTDOWNLOADED             0x80040203L
#define DIERR_HASEFFECTS                0x80040204L
#define DIERR_NOTEXCLUSIVEACQUIRED      0x80040205L
#define DIERR_INCOMPLETEEFFECT          0x80040206L
#define DIERR_NOTBUFFERED               0x80040207L
#define DIERR_EFFECTPLAYING             0x80040208L
#define DIERR_UNPLUGGED                 0x80040209L
#define DIERR_REPORTFULL                0x8004020AL

#define DIENUM_STOP                     0
#define DIENUM_CONTINUE                 1

#define DIDEVTYPE_DEVICE                1
#define DIDEVTYPE_MOUSE                 2
#define DIDEVTYPE_KEYBOARD              3
#define DIDEVTYPE_JOYSTICK              4
#define DIDEVTYPE_HID                   0x00010000

#define DIDEVTYPEMOUSE_UNKNOWN          1
#define DIDEVTYPEMOUSE_TRADITIONAL      2
#define DIDEVTYPEMOUSE_FINGERSTICK      3
#define DIDEVTYPEMOUSE_TOUCHPAD         4
#define DIDEVTYPEMOUSE_TRACKBALL        5

#define DIDEVTYPEKEYBOARD_UNKNOWN       0
#define DIDEVTYPEKEYBOARD_PCXT          1
#define DIDEVTYPEKEYBOARD_OLIVETTI      2
#define DIDEVTYPEKEYBOARD_PCAT          3
#define DIDEVTYPEKEYBOARD_PCENH         4
#define DIDEVTYPEKEYBOARD_NOKIA1050     5
#define DIDEVTYPEKEYBOARD_NOKIA9140     6
#define DIDEVTYPEKEYBOARD_NEC98         7
#define DIDEVTYPEKEYBOARD_NEC98LAPTOP   8
#define DIDEVTYPEKEYBOARD_NEC98106      9
#define DIDEVTYPEKEYBOARD_JAPAN106     10
#define DIDEVTYPEKEYBOARD_JAPANAX      11
#define DIDEVTYPEKEYBOARD_J3100        12

#define DIDEVTYPEJOYSTICK_UNKNOWN       1
#define DIDEVTYPEJOYSTICK_TRADITIONAL   2
#define DIDEVTYPEJOYSTICK_FLIGHTSTICK   3
#define DIDEVTYPEJOYSTICK_GAMEPAD       4
#define DIDEVTYPEJOYSTICK_RUDDER        5
#define DIDEVTYPEJOYSTICK_WHEEL         6
#define DIDEVTYPEJOYSTICK_HEADTRACKER   7

typedef struct {
    DWORD	dwSize;
    GUID	guidType;
    DWORD	dwOfs;
    DWORD	dwType;
    DWORD	dwFlags;
    CHAR	tszName[MAX_PATH];
#if(DIRECTINPUT_VERSION >= 0x0500)
    DWORD	dwFFMaxForce;
    DWORD	dwFFForceResolution;
    WORD	wCollectionNumber;
    WORD	wDesignatorIndex;
    WORD	wUsagePage;
    WORD	wUsage;
    DWORD	dwDimension;
    WORD	wExponent;
    WORD	wReserved;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVICEOBJECTINSTANCE32A, *LPDIDEVICEOBJECTINSTANCE32A,*LPCDIDEVICEOBJECTINSTANCE32A;

typedef struct {
    DWORD	dwSize;
    GUID	guidType;
    DWORD	dwOfs;
    DWORD	dwType;
    DWORD	dwFlags;
    WCHAR	tszName[MAX_PATH];
#if(DIRECTINPUT_VERSION >= 0x0500)
    DWORD	dwFFMaxForce;
    DWORD	dwFFForceResolution;
    WORD	wCollectionNumber;
    WORD	wDesignatorIndex;
    WORD	wUsagePage;
    WORD	wUsage;
    DWORD	dwDimension;
    WORD	wExponent;
    WORD	wReserved;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVICEOBJECTINSTANCE32W, *LPDIDEVICEOBJECTINSTANCE32W,*LPCDIDEVICEOBJECTINSTANCE32W;
DECL_WINELIB_TYPE_AW(LPDIDEVICEOBJECTINSTANCE)
DECL_WINELIB_TYPE_AW(DIDEVICEOBJECTINSTANCE)


typedef struct {
    DWORD	dwSize;
    GUID	guidInstance;
    GUID	guidProduct;
    DWORD	dwDevType;
    CHAR	tszInstanceName[MAX_PATH];
    CHAR	tszProductName[MAX_PATH];
#if(DIRECTINPUT_VERSION >= 0x0500)
    GUID	guidFFDriver;
    WORD	wUsagePage;
    WORD	wUsage;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVICEINSTANCE32A, *LPDIDEVICEINSTANCE32A, *LPCDIDEVICEINSTANCE32A;

typedef struct {
    DWORD	dwSize;
    GUID	guidInstance;
    GUID	guidProduct;
    DWORD	dwDevType;
    WCHAR	tszInstanceName[MAX_PATH];
    WCHAR	tszProductName[MAX_PATH];
#if(DIRECTINPUT_VERSION >= 0x0500)
    GUID	guidFFDriver;
    WORD	wUsagePage;
    WORD	wUsage;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVICEINSTANCE32W, *LPDIDEVICEINSTANCE32W, *LPCDIDEVICEINSTANCE32W;
DECL_WINELIB_TYPE_AW(DIDEVICEINSTANCE)
DECL_WINELIB_TYPE_AW(LPDIDEVICEINSTANCE)
DECL_WINELIB_TYPE_AW(LPCDIDEVICEINSTANCE)

typedef BOOL32 (CALLBACK * LPDIENUMDEVICESCALLBACK32A)(LPCDIDEVICEINSTANCE32A,LPVOID);
typedef BOOL32 (CALLBACK * LPDIENUMDEVICESCALLBACK32W)(LPCDIDEVICEINSTANCE32W,LPVOID);
DECL_WINELIB_TYPE_AW(LPDIENUMDEVICESCALLBACK)

typedef BOOL32 (CALLBACK * LPDIENUMDEVICEOBJECTSCALLBACK32A)(LPCDIDEVICEOBJECTINSTANCE32A, LPVOID);
typedef BOOL32 (CALLBACK * LPDIENUMDEVICEOBJECTSCALLBACK32W)(LPCDIDEVICEOBJECTINSTANCE32W,LPVOID);
DECL_WINELIB_TYPE_AW(LPDIENUMDEVICEOBJECTSCALLBACK)

#define DIK_ESCAPE          0x01
#define DIK_1               0x02
#define DIK_2               0x03
#define DIK_3               0x04
#define DIK_4               0x05
#define DIK_5               0x06
#define DIK_6               0x07
#define DIK_7               0x08
#define DIK_8               0x09
#define DIK_9               0x0A
#define DIK_0               0x0B
#define DIK_MINUS           0x0C    /* - on main keyboard */
#define DIK_EQUALS          0x0D
#define DIK_BACK            0x0E    /* backspace */
#define DIK_TAB             0x0F
#define DIK_Q               0x10
#define DIK_W               0x11
#define DIK_E               0x12
#define DIK_R               0x13
#define DIK_T               0x14
#define DIK_Y               0x15
#define DIK_U               0x16
#define DIK_I               0x17
#define DIK_O               0x18
#define DIK_P               0x19
#define DIK_LBRACKET        0x1A
#define DIK_RBRACKET        0x1B
#define DIK_RETURN          0x1C    /* Enter on main keyboard */
#define DIK_LCONTROL        0x1D
#define DIK_A               0x1E
#define DIK_S               0x1F
#define DIK_D               0x20
#define DIK_F               0x21
#define DIK_G               0x22
#define DIK_H               0x23
#define DIK_J               0x24
#define DIK_K               0x25
#define DIK_L               0x26
#define DIK_SEMICOLON       0x27
#define DIK_APOSTROPHE      0x28
#define DIK_GRAVE           0x29    /* accent grave */
#define DIK_LSHIFT          0x2A
#define DIK_BACKSLASH       0x2B
#define DIK_Z               0x2C
#define DIK_X               0x2D
#define DIK_C               0x2E
#define DIK_V               0x2F
#define DIK_B               0x30
#define DIK_N               0x31
#define DIK_M               0x32
#define DIK_COMMA           0x33
#define DIK_PERIOD          0x34    /* . on main keyboard */
#define DIK_SLASH           0x35    /* / on main keyboard */
#define DIK_RSHIFT          0x36
#define DIK_MULTIPLY        0x37    /* * on numeric keypad */
#define DIK_LMENU           0x38    /* left Alt */
#define DIK_SPACE           0x39
#define DIK_CAPITAL         0x3A
#define DIK_F1              0x3B
#define DIK_F2              0x3C
#define DIK_F3              0x3D
#define DIK_F4              0x3E
#define DIK_F5              0x3F
#define DIK_F6              0x40
#define DIK_F7              0x41
#define DIK_F8              0x42
#define DIK_F9              0x43
#define DIK_F10             0x44
#define DIK_NUMLOCK         0x45
#define DIK_SCROLL          0x46    /* Scroll Lock */
#define DIK_NUMPAD7         0x47
#define DIK_NUMPAD8         0x48
#define DIK_NUMPAD9         0x49
#define DIK_SUBTRACT        0x4A    /* - on numeric keypad */
#define DIK_NUMPAD4         0x4B
#define DIK_NUMPAD5         0x4C
#define DIK_NUMPAD6         0x4D
#define DIK_ADD             0x4E    /* + on numeric keypad */
#define DIK_NUMPAD1         0x4F
#define DIK_NUMPAD2         0x50
#define DIK_NUMPAD3         0x51
#define DIK_NUMPAD0         0x52
#define DIK_DECIMAL         0x53    /* . on numeric keypad */
#define DIK_F11             0x57
#define DIK_F12             0x58
#define DIK_F13             0x64    /*                     (NEC PC98) */
#define DIK_F14             0x65    /*                     (NEC PC98) */
#define DIK_F15             0x66    /*                     (NEC PC98) */
#define DIK_KANA            0x70    /* (Japanese keyboard)            */
#define DIK_CONVERT         0x79    /* (Japanese keyboard)            */
#define DIK_NOCONVERT       0x7B    /* (Japanese keyboard)            */
#define DIK_YEN             0x7D    /* (Japanese keyboard)            */
#define DIK_NUMPADEQUALS    0x8D    /* = on numeric keypad (NEC PC98) */
#define DIK_CIRCUMFLEX      0x90    /* (Japanese keyboard)            */
#define DIK_AT              0x91    /*                     (NEC PC98) */
#define DIK_COLON           0x92    /*                     (NEC PC98) */
#define DIK_UNDERLINE       0x93    /*                     (NEC PC98) */
#define DIK_KANJI           0x94    /* (Japanese keyboard)            */
#define DIK_STOP            0x95    /*                     (NEC PC98) */
#define DIK_AX              0x96    /*                     (Japan AX) */
#define DIK_UNLABELED       0x97    /*                        (J3100) */
#define DIK_NUMPADENTER     0x9C    /* Enter on numeric keypad */
#define DIK_RCONTROL        0x9D
#define DIK_NUMPADCOMMA     0xB3    /* , on numeric keypad (NEC PC98) */
#define DIK_DIVIDE          0xB5    /* / on numeric keypad */
#define DIK_SYSRQ           0xB7
#define DIK_RMENU           0xB8    /* right Alt */
#define DIK_PAUSE           0xC5    /* Pause */
#define DIK_HOME            0xC7    /* Home on arrow keypad */
#define DIK_UP              0xC8    /* UpArrow on arrow keypad */
#define DIK_PRIOR           0xC9    /* PgUp on arrow keypad */
#define DIK_LEFT            0xCB    /* LeftArrow on arrow keypad */
#define DIK_RIGHT           0xCD    /* RightArrow on arrow keypad */
#define DIK_END             0xCF    /* End on arrow keypad */
#define DIK_DOWN            0xD0    /* DownArrow on arrow keypad */
#define DIK_NEXT            0xD1    /* PgDn on arrow keypad */
#define DIK_INSERT          0xD2    /* Insert on arrow keypad */
#define DIK_DELETE          0xD3    /* Delete on arrow keypad */
#define DIK_LWIN            0xDB    /* Left Windows key */
#define DIK_RWIN            0xDC    /* Right Windows key */
#define DIK_APPS            0xDD    /* AppMenu key */
#define DIK_POWER           0xDE
#define DIK_SLEEP           0xDF
#define DIK_BACKSPACE       DIK_BACK            /* backspace */
#define DIK_NUMPADSTAR      DIK_MULTIPLY        /* * on numeric keypad */
#define DIK_LALT            DIK_LMENU           /* left Alt */
#define DIK_CAPSLOCK        DIK_CAPITAL         /* CapsLock */
#define DIK_NUMPADMINUS     DIK_SUBTRACT        /* - on numeric keypad */
#define DIK_NUMPADPLUS      DIK_ADD             /* + on numeric keypad */
#define DIK_NUMPADPERIOD    DIK_DECIMAL         /* . on numeric keypad */
#define DIK_NUMPADSLASH     DIK_DIVIDE          /* / on numeric keypad */
#define DIK_RALT            DIK_RMENU           /* right Alt */
#define DIK_UPARROW         DIK_UP              /* UpArrow on arrow keypad */
#define DIK_PGUP            DIK_PRIOR           /* PgUp on arrow keypad */
#define DIK_LEFTARROW       DIK_LEFT            /* LeftArrow on arrow keypad */
#define DIK_RIGHTARROW      DIK_RIGHT           /* RightArrow on arrow keypad */
#define DIK_DOWNARROW       DIK_DOWN            /* DownArrow on arrow keypad */
#define DIK_PGDN            DIK_NEXT            /* PgDn on arrow keypad */

#define DIDFT_ALL		0x00000000
#define DIDFT_RELAXIS		0x00000001
#define DIDFT_ABSAXIS		0x00000002
#define DIDFT_AXIS		0x00000003
#define DIDFT_PSHBUTTON		0x00000004
#define DIDFT_TGLBUTTON		0x00000008
#define DIDFT_BUTTON		0x0000000C
#define DIDFT_POV		0x00000010
#define DIDFT_COLLECTION	0x00000040
#define DIDFT_NODATA		0x00000080
#define DIDFT_ANYINSTANCE	0x00FFFF00
#define DIDFT_INSTANCEMASK	DIDFT_ANYINSTANCE
#define DIDFT_MAKEINSTANCE(n)	((WORD)(n) << 8)
#define DIDFT_GETTYPE(n)	LOBYTE(n)
#define DIDFT_GETINSTANCE(n)	LOWORD((n) >> 8)
#define DIDFT_FFACTUATOR	0x01000000
#define DIDFT_FFEFFECTTRIGGER	0x02000000
#define DIDFT_OUTPUT		0x10000000
#define DIDFT_ENUMCOLLECTION(n)	((WORD)(n) << 8)
#define DIDFT_NOCOLLECTION	0x00FFFF00
#define DIDF_ABSAXIS		0x00000001
#define DIDF_RELAXIS		0x00000002

#define DIGDD_PEEK		0x00000001

typedef struct {
    DWORD	dwOfs;
    DWORD	dwData;
    DWORD	dwTimeStamp;
    DWORD	dwSequence;
} DIDEVICEOBJECTDATA,*LPDIDEVICEOBJECTDATA,*LPCDIDEVICEOBJECTDATA;

typedef struct _DIOBJECTDATAFORMAT {
    const GUID *pguid;
    DWORD	dwOfs;
    DWORD	dwType;
    DWORD	dwFlags;
} DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;

typedef struct {
    DWORD			dwSize;
    DWORD			dwObjSize;
    DWORD			dwFlags;
    DWORD			dwDataSize;
    DWORD			dwNumObjs;
    LPDIOBJECTDATAFORMAT	rgodf;
} DIDATAFORMAT, *LPDIDATAFORMAT,*LPCDIDATAFORMAT;

typedef struct {
    DWORD	dwSize;
    DWORD	dwHeaderSize;
    DWORD	dwObj;
    DWORD	dwHow;
} DIPROPHEADER,*LPDIPROPHEADER,*LPCDIPROPHEADER;

#define DIPH_DEVICE	0
#define DIPH_BYOFFSET	1
#define DIPH_BYID	2
#define DIPH_BYUSAGE	3

#define DIMAKEUSAGEDWORD(UsagePage, Usage) (DWORD)MAKELONG(Usage, UsagePage)

typedef struct DIPROPDWORD {
	DIPROPHEADER	diph;
	DWORD		dwData;
} DIPROPDWORD, *LPDIPROPDWORD;
typedef const DIPROPDWORD *LPCDIPROPDWORD;

typedef struct DIPROPRANGE {
	DIPROPHEADER	diph;
	LONG		lMin;
	LONG		lMax;
} DIPROPRANGE, *LPDIPROPRANGE;
typedef const DIPROPRANGE *LPCDIPROPRANGE;

#define DIPROPRANGE_NOMIN	((LONG)0x80000000)
#define DIPROPRANGE_NOMAX	((LONG)0x7FFFFFFF)

typedef struct DIPROPCAL {
	DIPROPHEADER diph;
	LONG    lMin;
	LONG    lCenter;
	LONG    lMax;
} DIPROPCAL, *LPDIPROPCAL;
typedef const DIPROPCAL *LPCDIPROPCAL;

typedef struct DIPROPGUIDANDPATH {
	DIPROPHEADER diph;
	GUID    guidClass;
	WCHAR   wszPath[MAX_PATH];
} DIPROPGUIDANDPATH, *LPDIPROPGUIDANDPATH,*LPCDIPROPGUIDANDPATH;

/* special property GUIDs */
#define MAKEDIPROP(prop)	((REFGUID)(prop))
#define DIPROP_BUFFERSIZE	MAKEDIPROP(1)
#define DIPROP_AXISMODE		MAKEDIPROP(2)

#define DIPROPAXISMODE_ABS	0
#define DIPROPAXISMODE_REL	1

#define DIPROP_GRANULARITY	MAKEDIPROP(3)
#define DIPROP_RANGE		MAKEDIPROP(4)
#define DIPROP_DEADZONE		MAKEDIPROP(5)
#define DIPROP_SATURATION	MAKEDIPROP(6)
#define DIPROP_FFGAIN		MAKEDIPROP(7)
#define DIPROP_FFLOAD		MAKEDIPROP(8)
#define DIPROP_AUTOCENTER	MAKEDIPROP(9)

#define DIPROPAUTOCENTER_OFF	0
#define DIPROPAUTOCENTER_ON	1

#define DIPROP_CALIBRATIONMODE	MAKEDIPROP(10)

#define DIPROPCALIBRATIONMODE_COOKED	0
#define DIPROPCALIBRATIONMODE_RAW	1

#define DIPROP_CALIBRATION	MAKEDIPROP(11)
#define DIPROP_GUIDANDPATH	MAKEDIPROP(12)


typedef struct DIDEVCAPS {
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	dwDevType;
    DWORD	dwAxes;
    DWORD	dwButtons;
    DWORD	dwPOVs;
#if(DIRECTINPUT_VERSION >= 0x0500)
    DWORD	dwFFSamplePeriod;
    DWORD	dwFFMinTimeResolution;
    DWORD	dwFirmwareRevision;
    DWORD	dwHardwareRevision;
    DWORD	dwFFDriverVersion;
#endif /* DIRECTINPUT_VERSION >= 0x0500 */
} DIDEVCAPS,*LPDIDEVCAPS;

#define DIDC_ATTACHED		0x00000001
#define DIDC_POLLEDDEVICE	0x00000002
#define DIDC_EMULATED		0x00000004
#define DIDC_POLLEDDATAFORMAT	0x00000008
#define DIDC_FORCEFEEDBACK	0x00000100
#define DIDC_FFATTACK		0x00000200
#define DIDC_FFFADE		0x00000400
#define DIDC_SATURATION		0x00000800
#define DIDC_POSNEGCOEFFICIENTS	0x00001000
#define DIDC_POSNEGSATURATION	0x00002000
#define DIDC_DEADBAND		0x00004000

/* SetCooperativeLevel dwFlags */
#define DISCL_EXCLUSIVE		0x00000001
#define DISCL_NONEXCLUSIVE	0x00000002
#define DISCL_FOREGROUND	0x00000004
#define DISCL_BACKGROUND	0x00000008

#define THIS LPDIRECTINPUTDEVICE32A this
typedef struct IDirectInputDeviceA_VTable {
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputDeviceA methods ***/
    STDMETHOD(GetCapabilities)(THIS_ LPDIDEVCAPS) PURE;
    STDMETHOD(EnumObjects)(THIS_ LPDIENUMDEVICEOBJECTSCALLBACK32A,LPVOID,DWORD) PURE;
    STDMETHOD(GetProperty)(THIS_ REFGUID,LPDIPROPHEADER) PURE;
    STDMETHOD(SetProperty)(THIS_ REFGUID,LPCDIPROPHEADER) PURE;
    STDMETHOD(Acquire)(THIS) PURE;
    STDMETHOD(Unacquire)(THIS) PURE;
    STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
    STDMETHOD(GetDeviceData)(THIS_ DWORD,LPDIDEVICEOBJECTDATA,LPDWORD,DWORD) PURE;
    STDMETHOD(SetDataFormat)(THIS_ LPCDIDATAFORMAT) PURE;
    STDMETHOD(SetEventNotification)(THIS_ HANDLE32) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND32,DWORD) PURE;
    STDMETHOD(GetObjectInfo)(THIS_ LPDIDEVICEOBJECTINSTANCE32A,DWORD,DWORD) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ LPDIDEVICEINSTANCE32A) PURE;
    STDMETHOD(RunControlPanel)(THIS_ HWND32,DWORD) PURE;
    STDMETHOD(Initialize)(THIS_ HINSTANCE32,DWORD,REFGUID) PURE;
} IDirectInputDeviceA_VTable,*LPDIRECTINPUTDEVICEA_VTABLE;

struct IDirectInputDevice32A {
	LPDIRECTINPUTDEVICEA_VTABLE	lpvtbl;
	DWORD				ref;
	GUID				guid;
};

/* "Standard" Mouse report... */
struct DIMOUSESTATE {
  LONG lX;
  LONG lY;
  LONG lZ;
  BYTE rgbButtons[4];
};
struct SysMouse32A {
	LPDIRECTINPUTDEVICEA_VTABLE	lpvtbl;
	DWORD				ref;
	GUID				guid;
	BYTE                            absolute;
	/* Previous position for relative moves */
	LONG prevX;
	LONG prevY;
};

struct SysKeyboard32A {
	LPDIRECTINPUTDEVICEA_VTABLE	lpvtbl;
	DWORD				ref;
	GUID				guid;
	BYTE				keystate[256];
};
#undef THIS

#define THIS LPDIRECTINPUT32A this
typedef struct IDirectInputA_VTable {
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/*** IDirectInputA methods ***/
	STDMETHOD(CreateDevice)(THIS_ REFGUID,LPDIRECTINPUTDEVICE32A*,LPUNKNOWN) PURE;
	STDMETHOD(EnumDevices)(THIS_ DWORD,LPDIENUMDEVICESCALLBACK32A,LPVOID,DWORD) PURE;
	STDMETHOD(GetDeviceStatus)(THIS_ REFGUID) PURE;
	STDMETHOD(RunControlPanel)(THIS_ HWND32,DWORD) PURE;
	STDMETHOD(Initialize)(THIS_ HINSTANCE32,DWORD) PURE;
} IDirectInputA_VTable,*LPDIRECTINPUTA_VTABLE;

struct IDirectInput32A {
	LPDIRECTINPUTA_VTABLE	lpvtbl;
	DWORD			ref;
};
#undef THIS
#undef THIS_
#undef STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#undef STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#undef PURE
#undef FAR
#endif
