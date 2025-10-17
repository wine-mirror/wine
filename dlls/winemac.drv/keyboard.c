/*
 * MACDRV keyboard driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 1999 Ove Kåven
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "macdrv.h"
#include "winuser.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(keyboard);
WINE_DECLARE_DEBUG_CHANNEL(key);


/* Indexed by Mac virtual keycode values defined above. */
static const struct {
    WORD vkey;
    WORD scan;
    BOOL fixed;
} default_map[128] = {
    { 'A',                      0x1E,           FALSE },    /* kVK_ANSI_A */
    { 'S',                      0x1F,           FALSE },    /* kVK_ANSI_S */
    { 'D',                      0x20,           FALSE },    /* kVK_ANSI_D */
    { 'F',                      0x21,           FALSE },    /* kVK_ANSI_F */
    { 'H',                      0x23,           FALSE },    /* kVK_ANSI_H */
    { 'G',                      0x22,           FALSE },    /* kVK_ANSI_G */
    { 'Z',                      0x2C,           FALSE },    /* kVK_ANSI_Z */
    { 'X',                      0x2D,           FALSE },    /* kVK_ANSI_X */
    { 'C',                      0x2E,           FALSE },    /* kVK_ANSI_C */
    { 'V',                      0x2F,           FALSE },    /* kVK_ANSI_V */
    { VK_OEM_102,               0x56,           TRUE },     /* kVK_ISO_Section */
    { 'B',                      0x30,           FALSE },    /* kVK_ANSI_B */
    { 'Q',                      0x10,           FALSE },    /* kVK_ANSI_Q */
    { 'W',                      0x11,           FALSE },    /* kVK_ANSI_W */
    { 'E',                      0x12,           FALSE },    /* kVK_ANSI_E */
    { 'R',                      0x13,           FALSE },    /* kVK_ANSI_R */
    { 'Y',                      0x15,           FALSE },    /* kVK_ANSI_Y */
    { 'T',                      0x14,           FALSE },    /* kVK_ANSI_T */
    { '1',                      0x02,           FALSE },    /* kVK_ANSI_1 */
    { '2',                      0x03,           FALSE },    /* kVK_ANSI_2 */
    { '3',                      0x04,           FALSE },    /* kVK_ANSI_3 */
    { '4',                      0x05,           FALSE },    /* kVK_ANSI_4 */
    { '6',                      0x07,           FALSE },    /* kVK_ANSI_6 */
    { '5',                      0x06,           FALSE },    /* kVK_ANSI_5 */
    { VK_OEM_PLUS,              0x0D,           FALSE },    /* kVK_ANSI_Equal */
    { '9',                      0x0A,           FALSE },    /* kVK_ANSI_9 */
    { '7',                      0x08,           FALSE },    /* kVK_ANSI_7 */
    { VK_OEM_MINUS,             0x0C,           FALSE },    /* kVK_ANSI_Minus */
    { '8',                      0x09,           FALSE },    /* kVK_ANSI_8 */
    { '0',                      0x0B,           FALSE },    /* kVK_ANSI_0 */
    { VK_OEM_6,                 0x1B,           FALSE },    /* kVK_ANSI_RightBracket */
    { 'O',                      0x18,           FALSE },    /* kVK_ANSI_O */
    { 'U',                      0x16,           FALSE },    /* kVK_ANSI_U */
    { VK_OEM_4,                 0x1A,           FALSE },    /* kVK_ANSI_LeftBracket */
    { 'I',                      0x17,           FALSE },    /* kVK_ANSI_I */
    { 'P',                      0x19,           FALSE },    /* kVK_ANSI_P */
    { VK_RETURN,                0x1C,           TRUE },     /* kVK_Return */
    { 'L',                      0x26,           FALSE },    /* kVK_ANSI_L */
    { 'J',                      0x24,           FALSE },    /* kVK_ANSI_J */
    { VK_OEM_7,                 0x28,           FALSE },    /* kVK_ANSI_Quote */
    { 'K',                      0x25,           FALSE },    /* kVK_ANSI_K */
    { VK_OEM_1,                 0x27,           FALSE },    /* kVK_ANSI_Semicolon */
    { VK_OEM_5,                 0x2B,           FALSE },    /* kVK_ANSI_Backslash */
    { VK_OEM_COMMA,             0x33,           FALSE },    /* kVK_ANSI_Comma */
    { VK_OEM_2,                 0x35,           FALSE },    /* kVK_ANSI_Slash */
    { 'N',                      0x31,           FALSE },    /* kVK_ANSI_N */
    { 'M',                      0x32,           FALSE },    /* kVK_ANSI_M */
    { VK_OEM_PERIOD,            0x34,           FALSE },    /* kVK_ANSI_Period */
    { VK_TAB,                   0x0F,           TRUE },     /* kVK_Tab */
    { VK_SPACE,                 0x39,           TRUE },     /* kVK_Space */
    { VK_OEM_3,                 0x29,           FALSE },    /* kVK_ANSI_Grave */
    { VK_BACK,                  0x0E,           TRUE },     /* kVK_Delete */
    { 0,                        0,              FALSE },    /* 0x34 unused */
    { VK_ESCAPE,                0x01,           TRUE },     /* kVK_Escape */
    { VK_RMENU,                 0x38 | 0x100,   TRUE },     /* kVK_RightCommand */
    { VK_LMENU,                 0x38,           TRUE },     /* kVK_Command */
    { VK_LSHIFT,                0x2A,           TRUE },     /* kVK_Shift */
    { VK_CAPITAL,               0x3A,           TRUE },     /* kVK_CapsLock */
    { 0,                        0,              FALSE },    /* kVK_Option */
    { VK_LCONTROL,              0x1D,           TRUE },     /* kVK_Control */
    { VK_RSHIFT,                0x36,           TRUE },     /* kVK_RightShift */
    { 0,                        0,              FALSE },    /* kVK_RightOption */
    { VK_RCONTROL,              0x1D | 0x100,   TRUE },     /* kVK_RightControl */
    { 0,                        0,              FALSE },    /* kVK_Function */
    { VK_F17,                   0x68,           TRUE },     /* kVK_F17 */
    { VK_DECIMAL,               0x53,           TRUE },     /* kVK_ANSI_KeypadDecimal */
    { 0,                        0,              FALSE },    /* 0x42 unused */
    { VK_MULTIPLY,              0x37,           TRUE },     /* kVK_ANSI_KeypadMultiply */
    { 0,                        0,              FALSE },    /* 0x44 unused */
    { VK_ADD,                   0x4E,           TRUE },     /* kVK_ANSI_KeypadPlus */
    { 0,                        0,              FALSE },    /* 0x46 unused */
    { VK_OEM_CLEAR,             0x59,           TRUE },     /* kVK_ANSI_KeypadClear */
    { VK_VOLUME_UP,             0 | 0x100,      TRUE },     /* kVK_VolumeUp */
    { VK_VOLUME_DOWN,           0 | 0x100,      TRUE },     /* kVK_VolumeDown */
    { VK_VOLUME_MUTE,           0 | 0x100,      TRUE },     /* kVK_Mute */
    { VK_DIVIDE,                0x35 | 0x100,   TRUE },     /* kVK_ANSI_KeypadDivide */
    { VK_RETURN,                0x1C | 0x100,   TRUE },     /* kVK_ANSI_KeypadEnter */
    { 0,                        0,              FALSE },    /* 0x4D unused */
    { VK_SUBTRACT,              0x4A,           TRUE },     /* kVK_ANSI_KeypadMinus */
    { VK_F18,                   0x69,           TRUE },     /* kVK_F18 */
    { VK_F19,                   0x6A,           TRUE },     /* kVK_F19 */
    { VK_OEM_NEC_EQUAL,         0x0D | 0x100,   TRUE },     /* kVK_ANSI_KeypadEquals */
    { VK_NUMPAD0,               0x52,           TRUE },     /* kVK_ANSI_Keypad0 */
    { VK_NUMPAD1,               0x4F,           TRUE },     /* kVK_ANSI_Keypad1 */
    { VK_NUMPAD2,               0x50,           TRUE },     /* kVK_ANSI_Keypad2 */
    { VK_NUMPAD3,               0x51,           TRUE },     /* kVK_ANSI_Keypad3 */
    { VK_NUMPAD4,               0x4B,           TRUE },     /* kVK_ANSI_Keypad4 */
    { VK_NUMPAD5,               0x4C,           TRUE },     /* kVK_ANSI_Keypad5 */
    { VK_NUMPAD6,               0x4D,           TRUE },     /* kVK_ANSI_Keypad6 */
    { VK_NUMPAD7,               0x47,           TRUE },     /* kVK_ANSI_Keypad7 */
    { VK_F20,                   0x6B,           TRUE },     /* kVK_F20 */
    { VK_NUMPAD8,               0x48,           TRUE },     /* kVK_ANSI_Keypad8 */
    { VK_NUMPAD9,               0x49,           TRUE },     /* kVK_ANSI_Keypad9 */
    { 0xFF,                     0x7D,           TRUE },     /* kVK_JIS_Yen */
    { 0xC1,                     0x73,           TRUE },     /* kVK_JIS_Underscore */
    { VK_SEPARATOR,             0x7E,           TRUE },     /* kVK_JIS_KeypadComma */
    { VK_F5,                    0x3F,           TRUE },     /* kVK_F5 */
    { VK_F6,                    0x40,           TRUE },     /* kVK_F6 */
    { VK_F7,                    0x41,           TRUE },     /* kVK_F7 */
    { VK_F3,                    0x3D,           TRUE },     /* kVK_F3 */
    { VK_F8,                    0x42,           TRUE },     /* kVK_F8 */
    { VK_F9,                    0x43,           TRUE },     /* kVK_F9 */
    { 0xFF,                     0x72,           TRUE },     /* kVK_JIS_Eisu */
    { VK_F11,                   0x57,           TRUE },     /* kVK_F11 */
    { VK_OEM_RESET,             0x71,           TRUE },     /* kVK_JIS_Kana */
    { VK_F13,                   0x64,           TRUE },     /* kVK_F13 */
    { VK_F16,                   0x67,           TRUE },     /* kVK_F16 */
    { VK_F14,                   0x65,           TRUE },     /* kVK_F14 */
    { 0,                        0,              FALSE },    /* 0x6C unused */
    { VK_F10,                   0x44,           TRUE },     /* kVK_F10 */
    { 0,                        0,              FALSE },    /* 0x6E unused */
    { VK_F12,                   0x58,           TRUE },     /* kVK_F12 */
    { 0,                        0,              FALSE },    /* 0x70 unused */
    { VK_F15,                   0x66,           TRUE },     /* kVK_F15 */
    { VK_INSERT,                0x52 | 0x100,   TRUE },     /* kVK_Help */ /* map to Insert */
    { VK_HOME,                  0x47 | 0x100,   TRUE },     /* kVK_Home */
    { VK_PRIOR,                 0x49 | 0x100,   TRUE },     /* kVK_PageUp */
    { VK_DELETE,                0x53 | 0x100,   TRUE },     /* kVK_ForwardDelete */
    { VK_F4,                    0x3E,           TRUE },     /* kVK_F4 */
    { VK_END,                   0x4F | 0x100,   TRUE },     /* kVK_End */
    { VK_F2,                    0x3C,           TRUE },     /* kVK_F2 */
    { VK_NEXT,                  0x51 | 0x100,   TRUE },     /* kVK_PageDown */
    { VK_F1,                    0x3B,           TRUE },     /* kVK_F1 */
    { VK_LEFT,                  0x4B | 0x100,   TRUE },     /* kVK_LeftArrow */
    { VK_RIGHT,                 0x4D | 0x100,   TRUE },     /* kVK_RightArrow */
    { VK_DOWN,                  0x50 | 0x100,   TRUE },     /* kVK_DownArrow */
    { VK_UP,                    0x48 | 0x100,   TRUE },     /* kVK_UpArrow */
};


static const struct {
    DWORD       vkey;
    const char *name;
} vkey_names[] = {
    { VK_ADD,                   "Num +" },
    { VK_BACK,                  "Backspace" },
    { VK_CAPITAL,               "Caps Lock" },
    { VK_CONTROL,               "Ctrl" },
    { VK_DECIMAL,               "Num Del" },
    { VK_DELETE | 0x100,        "Delete" },
    { VK_DIVIDE | 0x100,        "Num /" },
    { VK_DOWN | 0x100,          "Down" },
    { VK_END | 0x100,           "End" },
    { VK_ESCAPE,                "Esc" },
    { VK_F1,                    "F1" },
    { VK_F2,                    "F2" },
    { VK_F3,                    "F3" },
    { VK_F4,                    "F4" },
    { VK_F5,                    "F5" },
    { VK_F6,                    "F6" },
    { VK_F7,                    "F7" },
    { VK_F8,                    "F8" },
    { VK_F9,                    "F9" },
    { VK_F10,                   "F10" },
    { VK_F11,                   "F11" },
    { VK_F12,                   "F12" },
    { VK_F13,                   "F13" },
    { VK_F14,                   "F14" },
    { VK_F15,                   "F15" },
    { VK_F16,                   "F16" },
    { VK_F17,                   "F17" },
    { VK_F18,                   "F18" },
    { VK_F19,                   "F19" },
    { VK_F20,                   "F20" },
    { VK_F21,                   "F21" },
    { VK_F22,                   "F22" },
    { VK_F23,                   "F23" },
    { VK_F24,                   "F24" },
    { VK_HELP | 0x100,          "Help" },
    { VK_HOME | 0x100,          "Home" },
    { VK_INSERT | 0x100,        "Insert" },
    { VK_LCONTROL,              "Ctrl" },
    { VK_LEFT | 0x100,          "Left" },
    { VK_LMENU,                 "Alt" },
    { VK_LSHIFT,                "Shift" },
    { VK_LWIN | 0x100,          "Win" },
    { VK_MENU,                  "Alt" },
    { VK_MULTIPLY,              "Num *" },
    { VK_NEXT | 0x100,          "Page Down" },
    { VK_NUMLOCK | 0x100,       "Num Lock" },
    { VK_NUMPAD0,               "Num 0" },
    { VK_NUMPAD1,               "Num 1" },
    { VK_NUMPAD2,               "Num 2" },
    { VK_NUMPAD3,               "Num 3" },
    { VK_NUMPAD4,               "Num 4" },
    { VK_NUMPAD5,               "Num 5" },
    { VK_NUMPAD6,               "Num 6" },
    { VK_NUMPAD7,               "Num 7" },
    { VK_NUMPAD8,               "Num 8" },
    { VK_NUMPAD9,               "Num 9" },
    { VK_OEM_CLEAR,             "Num Clear" },
    { VK_OEM_NEC_EQUAL | 0x100, "Num =" },
    { VK_PRIOR | 0x100,         "Page Up" },
    { VK_RCONTROL | 0x100,      "Right Ctrl" },
    { VK_RETURN,                "Return" },
    { VK_RETURN | 0x100,        "Num Enter" },
    { VK_RIGHT | 0x100,         "Right" },
    { VK_RMENU | 0x100,         "Right Alt" },
    { VK_RSHIFT,                "Right Shift" },
    { VK_RWIN | 0x100,          "Right Win" },
    { VK_SEPARATOR,             "Num ," },
    { VK_SHIFT,                 "Shift" },
    { VK_SPACE,                 "Space" },
    { VK_SUBTRACT,              "Num -" },
    { VK_TAB,                   "Tab" },
    { VK_UP | 0x100,            "Up" },
    { VK_VOLUME_DOWN | 0x100,   "Volume Down" },
    { VK_VOLUME_MUTE | 0x100,   "Mute" },
    { VK_VOLUME_UP | 0x100,     "Volume Up" },
};


static const struct {
    WCHAR       wchar;
    const char *name;
} dead_key_names[] = {
    { '^',                      "CIRCUMFLEX ACCENT" },
    { '`',                      "GRAVE ACCENT" },
    { 0x00B4,                   "ACUTE ACCENT" },
    { '~',                      "TILDE" },
    { 0x00A8,                   "DIAERESIS" },
    { 0x00B8,                   "CEDILLA" },
    { 0x02D8,                   "BREVE" },
    { 0x02D9,                   "DOT ABOVE" },
    { 0x00AF,                   "MACRON" },
    { 0x02DA,                   "RING ABOVE" },
    { 0x02DB,                   "OGONEK" },
    { 0x02DC,                   "SMALL TILDE" },
    { 0x02DD,                   "DOUBLE ACUTE ACCENT" },
};


static Boolean char_matches_string(WCHAR wchar, UniChar *string, CollatorRef collatorRef)
{
    Boolean equivalent;
    OSStatus status;

    status = UCCompareText(collatorRef, (UniChar*)&wchar, 1, string, wcslen(string), &equivalent, NULL);
    if (status != noErr)
    {
        WARN("Failed to compare %s to %s\n", debugstr_wn(&wchar, 1), debugstr_w(string));
        return FALSE;
    }
    return equivalent;
}


/* Filter Apple-specific private-use characters (see NSEvent.h) out of a
 * string.  Returns the length of the string after stripping. */
static int strip_apple_private_chars(LPWSTR bufW, int len)
{
    int i;
    for (i = 0; i < len; )
    {
        if (0xF700 <= bufW[i] && bufW[i] <= 0xF8FF)
        {
            memmove(&bufW[i], &bufW[i+1], (len - i - 1) * sizeof(bufW[0]));
            len--;
        }
        else
            i++;
    }
    return len;
}

static struct list layout_list = LIST_INIT( layout_list );
struct layout
{
    struct list entry;
    HKL hkl;
    TISInputSourceRef input_source;
    BOOL enabled; /* is the input source enabled - ie displayed in the input source selector UI */
};

static pthread_mutex_t layout_list_mutex = PTHREAD_MUTEX_INITIALIZER;

int macdrv_layout_list_needs_update = TRUE;

static const NLS_LOCALE_HEADER *locale_table;

static int compare_locale_names(const WCHAR *n1, const WCHAR *n2)
{
    for (;;)
    {
        WCHAR ch1 = *n1++;
        WCHAR ch2 = *n2++;
        if (ch1 >= 'a' && ch1 <= 'z') ch1 -= 'a' - 'A';
        else if (ch1 == '_') ch1 = '-';
        if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 'a' - 'A';
        else if (ch2 == '_') ch2 = '-';
        if (!ch1 || ch1 != ch2) return ch1 - ch2;
    }
}


static const NLS_LOCALE_LCNAME_INDEX *find_lcname_entry(const WCHAR *name)
{
    const NLS_LOCALE_LCNAME_INDEX *lcnames_index;
    const WCHAR *locale_strings;
    int min = 0, max = locale_table->nb_lcnames - 1;

    locale_strings = (const WCHAR *)((char *)locale_table + locale_table->strings_offset);
    lcnames_index = (const NLS_LOCALE_LCNAME_INDEX *)((char *)locale_table + locale_table->lcnames_offset);

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        const WCHAR *str = locale_strings + lcnames_index[pos].name;
        res = compare_locale_names(name, str + 1);
        if (res < 0) max = pos - 1;
        else if (res > 0) min = pos + 1;
        else return &lcnames_index[pos];
    }
    return NULL;
}


static DWORD get_lcid(CFStringRef lang)
{
    const NLS_LOCALE_LCNAME_INDEX *entry;
    const NLS_LOCALE_DATA *locale;
    CFRange range;
    WCHAR str[10];
    ULONG offset;

    if (!locale_table)
    {
        struct
        {
            UINT ctypes;
            UINT unknown1;
            UINT unknown2;
            UINT unknown3;
            UINT locales;
            UINT charmaps;
            UINT geoids;
            UINT scripts;
        } *header;
        LCID system_lcid;
        LARGE_INTEGER size;

        if (NtInitializeNlsFiles((void **)&header, &system_lcid, &size))
        {
            ERR("NtInitializeNlsFiles failed\n");
            return 0;
        }

        if (InterlockedCompareExchangePointer((void **)&locale_table,
                                              (char *)header + header->locales, NULL))
            NtUnmapViewOfSection(GetCurrentProcess(), header);
    }

    range.location = 0;
    range.length = min(CFStringGetLength(lang), ARRAY_SIZE(str) - 1);
    CFStringGetCharacters(lang, range, str);
    str[range.length] = 0;

    if (!(entry = find_lcname_entry(str)))
    {
        ERR("%s not found\n", debugstr_w(str));
        return 0;
    }

    offset = locale_table->locales_offset + entry->idx * locale_table->locale_size;
    locale = (const NLS_LOCALE_DATA *)((const char *)locale_table + offset);
    return locale->inotneutral ? entry->id : locale->idefaultlanguage;
}

static HKL get_hkl(CFStringRef lang, CFStringRef type)
{
    ULONG_PTR lcid = get_lcid(lang);
    struct layout *layout;

    /* Look for the last occurrence of this lcid in the list and if
       present use that value + 0x10000 */
    LIST_FOR_EACH_ENTRY_REV(layout, &layout_list, struct layout, entry)
    {
        ULONG_PTR hkl = HandleToUlong(layout->hkl);

        if (LOWORD(hkl) == lcid)
        {
            lcid = (hkl & ~0xe0000000) + 0x10000;
            break;
        }
    }

    if (!CFEqual(type, kTISTypeKeyboardLayout)) lcid |= 0xe0000000;

    return (HKL)lcid;
}

/******************************************************************
 *                get_layout_from_source
 *
 * Must be called while holding the layout_list_mutex.
 * Note, returned layout may not currently be enabled.
 */
static struct layout *get_layout_from_source(TISInputSourceRef input)
{
    struct layout *ret = NULL, *layout;

    LIST_FOR_EACH_ENTRY(layout, &layout_list, struct layout, entry)
    {
        if (CFEqual(input, layout->input_source))
        {
            ret = layout;
            break;
        }
    }
    return ret;
}

/***********************************************************************
 *            update_layout_list
 *
 * Must be called while holding the layout_list_mutex
 *
 * If an input source has been disabled (ie. removed from the UI) its
 * entry remains in the layout list but is marked as such and is not
 * enumerated by GetKeyboardLayoutList.  This is to ensure the
 * HKL <-> input source mapping is unique.
 */
static void update_layout_list(void)
{
    CFArrayRef sources;
    struct layout *layout;
    int i;

    if (!InterlockedExchange((LONG *)&macdrv_layout_list_needs_update, FALSE)) return;

    sources = macdrv_create_input_source_list();

    LIST_FOR_EACH_ENTRY(layout, &layout_list, struct layout, entry)
        layout->enabled = FALSE;

    for (i = 0; i < CFArrayGetCount(sources); i++)
    {
        CFDictionaryRef dict = CFArrayGetValueAtIndex(sources, i);
        TISInputSourceRef input = (TISInputSourceRef)CFDictionaryGetValue(dict, macdrv_input_source_input_key);
        layout = get_layout_from_source(input);
        if (!layout)
        {
            CFStringRef type = CFDictionaryGetValue(dict, macdrv_input_source_type_key);
            CFStringRef lang = CFDictionaryGetValue(dict, macdrv_input_source_lang_key);

            layout = malloc(sizeof(*layout));
            layout->input_source = (TISInputSourceRef)CFRetain(input);
            layout->hkl = get_hkl(lang, type);

            list_add_tail(&layout_list, &layout->entry);
            TRACE("adding new layout %p\n", layout->hkl);
        }
        else
            TRACE("enabling already existing layout %p\n", layout->hkl);

        layout->enabled = TRUE;
    }

    CFRelease(sources);
}

/***********************************************************************
 *            macdrv_get_hkl_from_source
 *
 * Find the HKL associated with a given input source.
 */
HKL macdrv_get_hkl_from_source(TISInputSourceRef input)
{
    struct layout *layout;
    HKL ret = 0;

    pthread_mutex_lock(&layout_list_mutex);

    update_layout_list();
    layout = get_layout_from_source(input);
    if (layout) ret = layout->hkl;

    pthread_mutex_unlock(&layout_list_mutex);

    return ret;
}


/***********************************************************************
 *              macdrv_compute_keyboard_layout
 */
void macdrv_compute_keyboard_layout(struct macdrv_thread_data *thread_data)
{
    int keyc;
    WCHAR vkey;
    const UCKeyboardLayout *uchr;
    LocaleRef localeRef;
    CollatorRef collatorRef, caseInsensitiveCollatorRef, diacriticInsensitiveCollatorRef;
    UCCollateOptions collateOptions = 0;
    const UInt32 modifier_combos[] = {
        0,
        shiftKey >> 8,
        cmdKey >> 8,
        (shiftKey | cmdKey) >> 8,
        optionKey >> 8,
        (shiftKey | optionKey) >> 8,
    };
    UniChar map[128][ARRAY_SIZE(modifier_combos)][4 + 1];
    int combo;
    BYTE vkey_used[256];
    int ignore_diacritics;
    static const struct {
        WCHAR wchar;
        DWORD vkey;
        /* Mac virtual key code that must match wchar under the current layout.
         * A value of -1 means match-any.
         * TODO: replace -1 with the actual mac virtual key codes for all mappings
         * and their respective layouts to avoid false matches, when possible. */
        int mac_keyc;
    } symbol_vkeys[] = {
        { '-', VK_OEM_PLUS, kVK_ANSI_Equal },
        { '-', VK_OEM_MINUS, -1 },
        { '+', VK_OEM_PLUS, -1 },
        { '_', VK_OEM_MINUS, -1 },
        { ',', VK_OEM_COMMA, -1 },
        { '.', VK_OEM_PERIOD, -1 },
        { '=', VK_OEM_8, kVK_ANSI_Slash },
        { '=', VK_OEM_PLUS, -1 },
        { '!', VK_OEM_8, kVK_ANSI_Slash },
        { 0x00F9, VK_OEM_3, kVK_ANSI_Quote }, /* 0x00F9 is French U accent grave */
        { '$', VK_OEM_1, kVK_ANSI_RightBracket },
        { ':', VK_OEM_2, kVK_ANSI_Period },
        { '*', VK_OEM_5, kVK_ANSI_Backslash },
        { '`', VK_OEM_5, kVK_ANSI_Backslash },
        { ';', VK_OEM_PERIOD, kVK_ANSI_Comma },
        { ')', VK_OEM_4, kVK_ANSI_Minus },
        { '>', VK_OEM_PERIOD, -1 },
        { '<', VK_OEM_COMMA, -1 },
        { '|', VK_OEM_5, -1 },
        { '\\', VK_OEM_5, -1 },
        { '`', VK_OEM_3, -1 },
        { '[', VK_OEM_4, -1 },
        { '~', VK_OEM_3, -1 },
        { 0x00DF, VK_OEM_4, kVK_ANSI_Minus }, /* 0x00DF is ESZETT */
        { 0x00FC, VK_OEM_1, kVK_ANSI_LeftBracket }, /* 0x00FC is German U Umlaut */
        { 0x00F6, VK_OEM_3, kVK_ANSI_Semicolon }, /* 0x00F6 is German O Umlaut */
        { 0x00E4, VK_OEM_7, kVK_ANSI_Quote }, /* 0x00B4 is German A Umlaut */
        { '?', VK_OEM_2, -1 },
        { ']', VK_OEM_6, -1 },
        { '/', VK_OEM_2, -1 },
        { ':', VK_OEM_1, -1 },
        { '}', VK_OEM_6, -1 },
        { '{', VK_OEM_4,  },
        { ';', VK_OEM_1, -1 },
        { '\'', VK_OEM_7, -1 },
        { ':', VK_OEM_PERIOD, -1 },
        { ';', VK_OEM_COMMA, -1 },
        { '"', VK_OEM_7,  -1 },
        { 0x00B4, VK_OEM_4, kVK_ANSI_Equal }, /* 0x00B4 is ACUTE ACCENT */
        { '\'', VK_OEM_2, -1 },
        { 0x00A7, VK_OEM_5, -1 }, /* 0x00A7 is SECTION SIGN */
        { '*', VK_OEM_PLUS, -1 },
        { 0x00B4, VK_OEM_7, -1 },
        { '`', VK_OEM_4, -1 },
        { '[', VK_OEM_6, -1 },
        { '/', VK_OEM_5, -1 },
        { '^', VK_OEM_6, -1 },
        { '*', VK_OEM_2, -1 },
        { '{', VK_OEM_6, -1 },
        { 0x00B4, VK_OEM_6, -1 },
        { '~', VK_OEM_1, -1 },
        { '?', VK_OEM_PLUS, -1 },
        { '?', VK_OEM_4, -1 },
        { 0x00B4, VK_OEM_3, -1 },
        { '?', VK_OEM_COMMA, -1 },
        { '~', VK_OEM_PLUS, -1 },
        { ']', VK_OEM_4, -1 },
        { '\'', VK_OEM_3, -1 },
        { 0x00A7, VK_OEM_7, -1 },
        { '<', VK_OEM_102, -1 },
    };
    int i;

    /* Vkeys that are suitable for assigning to arbitrary keys, organized in
       contiguous ranges. */
    static const struct {
        WORD first, last;
    } vkey_ranges[] = {
        { 'A', 'Z' },
        { '0', '9' },
        { VK_OEM_1, VK_OEM_3 },
        { VK_OEM_4, VK_ICO_CLEAR },
        { 0xe9, 0xf5 },
        { VK_OEM_NEC_EQUAL, VK_OEM_NEC_EQUAL },
        { VK_F1, VK_F24 },
        { 0, 0 }
    };
    int vkey_range;

    if (!thread_data->keyboard_layout_uchr)
    {
        ERR("no keyboard layout UCHR data\n");
        return;
    }

    memset(thread_data->keyc2vkey, 0, sizeof(thread_data->keyc2vkey));
    memset(vkey_used, 0, sizeof(vkey_used));

    for (keyc = 0; keyc < ARRAY_SIZE(default_map); keyc++)
    {
        thread_data->keyc2scan[keyc] = default_map[keyc].scan;
        if (default_map[keyc].fixed)
        {
            vkey = default_map[keyc].vkey;
            thread_data->keyc2vkey[keyc] = vkey;
            vkey_used[vkey] = 1;
            TRACE("keyc 0x%04x -> vkey 0x%04x (fixed)\n", keyc, vkey);
        }
    }

    if (thread_data->iso_keyboard)
    {
        /* In almost all cases, the Mac key codes indicate a physical key position
           and this corresponds nicely to Win32 scan codes.  However, the Mac key
           codes differ in one case between ANSI and ISO keyboards.  For ANSI
           keyboards, the key to the left of the digits and above the Tab key
           produces key code kVK_ANSI_Grave.  For ISO keyboards, the key in that
           some position produces kVK_ISO_Section.  The additional key on ISO
           keyboards, the one to the right of the left Shift key, produces
           kVK_ANSI_Grave, which is just weird.

           Since we want the key in that upper left corner to always produce the
           same scan code (0x29), we need to swap the scan codes of those two
           Mac key codes for ISO keyboards. */
        DWORD temp = thread_data->keyc2scan[kVK_ANSI_Grave];
        thread_data->keyc2scan[kVK_ANSI_Grave] = thread_data->keyc2scan[kVK_ISO_Section];
        thread_data->keyc2scan[kVK_ISO_Section] = temp;
    }

    uchr = (const UCKeyboardLayout*)CFDataGetBytePtr(thread_data->keyboard_layout_uchr);

    LocaleRefFromLocaleString("POSIX", &localeRef);
    UCCreateCollator(localeRef, 0, collateOptions, &collatorRef);
    collateOptions |= kUCCollateComposeInsensitiveMask | kUCCollateWidthInsensitiveMask | kUCCollateCaseInsensitiveMask;
    UCCreateCollator(localeRef, 0, collateOptions, &caseInsensitiveCollatorRef);
    collateOptions |= kUCCollateDiacritInsensitiveMask;
    UCCreateCollator(localeRef, 0, collateOptions, &diacriticInsensitiveCollatorRef);

    /* Using the keyboard layout, build a map of key code + modifiers -> characters. */
    memset(map, 0, sizeof(map));
    for (keyc = 0; keyc < ARRAY_SIZE(map); keyc++)
    {
        if (!thread_data->keyc2scan[keyc]) continue; /* not a known Mac key code */
        if (thread_data->keyc2vkey[keyc]) continue; /* assigned a fixed vkey */

        TRACE("keyc 0x%04x: ", keyc);

        for (combo = 0; combo < ARRAY_SIZE(modifier_combos); combo++)
        {
            UInt32 deadKeyState;
            UniCharCount len;
            OSStatus status;

            deadKeyState = 0;
            status = UCKeyTranslate(uchr, keyc, kUCKeyActionDown, modifier_combos[combo],
                thread_data->keyboard_type, kUCKeyTranslateNoDeadKeysMask,
                &deadKeyState, ARRAY_SIZE(map[keyc][combo]) - 1, &len, map[keyc][combo]);
            if (status != noErr)
                map[keyc][combo][0] = 0;

            TRACE("%s%s", (combo ? ", " : ""), debugstr_w(map[keyc][combo]));
        }

        TRACE("\n");
    }

    /* First try to match key codes to the vkeys for the letters A through Z.
       Try unmodified first, then with various modifier combinations in succession.
       On the first pass, try to get a match lacking diacritical marks.  On the
       second pass, accept matches with diacritical marks. */
    for (ignore_diacritics = 0; ignore_diacritics <= 1; ignore_diacritics++)
    {
        for (combo = 0; combo < ARRAY_SIZE(modifier_combos); combo++)
        {
            for (vkey = 'A'; vkey <= 'Z'; vkey++)
            {
                if (vkey_used[vkey])
                    continue;

                for (keyc = 0; keyc < ARRAY_SIZE(map); keyc++)
                {
                    if (thread_data->keyc2vkey[keyc] || !map[keyc][combo][0])
                        continue;

                    if (char_matches_string(vkey, map[keyc][combo], ignore_diacritics ? diacriticInsensitiveCollatorRef : caseInsensitiveCollatorRef))
                    {
                        thread_data->keyc2vkey[keyc] = vkey;
                        vkey_used[vkey] = 1;
                        TRACE("keyc 0x%04x -> vkey 0x%04x (%s match %s)\n", keyc, vkey,
                              debugstr_wn(&vkey, 1), debugstr_w(map[keyc][combo]));
                        break;
                    }
                }
            }
        }
    }

    /* Next try to match key codes to the vkeys for the digits 0 through 9. */
    for (combo = 0; combo < ARRAY_SIZE(modifier_combos); combo++)
    {
        for (vkey = '0'; vkey <= '9'; vkey++)
        {
            if (vkey_used[vkey])
                continue;

            for (keyc = 0; keyc < ARRAY_SIZE(map); keyc++)
            {
                if (thread_data->keyc2vkey[keyc] || !map[keyc][combo][0])
                    continue;

                if (char_matches_string(vkey, map[keyc][combo], collatorRef))
                {
                    thread_data->keyc2vkey[keyc] = vkey;
                    vkey_used[vkey] = 1;
                    TRACE("keyc 0x%04x -> vkey 0x%04x (%s match %s)\n", keyc, vkey,
                          debugstr_wn(&vkey, 1), debugstr_w(map[keyc][combo]));
                    break;
                }
            }
        }
    }

    /* Now try to match key codes for certain common punctuation characters to
       the most common OEM vkeys (e.g. '.' to VK_OEM_PERIOD). */
    for (combo = 0; combo < ARRAY_SIZE(modifier_combos); combo++)
    {
        for (i = 0; i < ARRAY_SIZE(symbol_vkeys); i++)
        {
            vkey = symbol_vkeys[i].vkey;

            if (vkey_used[vkey])
                continue;

            for (keyc = 0; keyc < ARRAY_SIZE(map); keyc++)
            {
                if (!thread_data->keyc2scan[keyc]) continue; /* not a known Mac key code */
                if (thread_data->keyc2vkey[keyc] || !map[keyc][combo][0])
                    continue;
                if (symbol_vkeys[i].mac_keyc != -1 && symbol_vkeys[i].mac_keyc != keyc)
                    continue;

                if (char_matches_string(symbol_vkeys[i].wchar, map[keyc][combo], collatorRef))
                {
                    thread_data->keyc2vkey[keyc] = vkey;
                    vkey_used[vkey] = 1;
                    TRACE("keyc 0x%04x -> vkey 0x%04x (%s match %s)\n", keyc, vkey,
                          debugstr_wn(&symbol_vkeys[i].wchar, 1), debugstr_w(map[keyc][combo]));
                    break;
                }
            }
        }
    }

    /* For those key codes still without a vkey, try to use the default vkey
       from the default map, if it's still available. */
    for (keyc = 0; keyc < ARRAY_SIZE(default_map); keyc++)
    {
        unsigned int vkey = default_map[keyc].vkey;

        if (!thread_data->keyc2scan[keyc]) continue; /* not a known Mac key code */
        if (thread_data->keyc2vkey[keyc]) continue; /* already assigned */

        if (!vkey_used[vkey])
        {
            thread_data->keyc2vkey[keyc] = vkey;
            vkey_used[vkey] = 1;
            TRACE("keyc 0x%04x -> vkey 0x%04x (default map)\n", keyc, vkey);
        }
    }

    /* For any unassigned key codes which would map to a letter in the default
       map, but whose normal letter vkey wasn't available, try to find a
       different letter. */
    vkey = 'A';
    for (keyc = 0; keyc < ARRAY_SIZE(default_map); keyc++)
    {
        if (default_map[keyc].vkey < 'A' || 'Z' < default_map[keyc].vkey)
            continue; /* not a letter in ANSI layout */
        if (!thread_data->keyc2scan[keyc]) continue; /* not a known Mac key code */
        if (thread_data->keyc2vkey[keyc]) continue; /* already assigned */

        while (vkey <= 'Z' && vkey_used[vkey]) vkey++;
        if (vkey <= 'Z')
        {
            thread_data->keyc2vkey[keyc] = vkey;
            vkey_used[vkey] = 1;
            TRACE("keyc 0x%04x -> vkey 0x%04x (spare letter)\n", keyc, vkey);
        }
        else
            break; /* no more unused letter vkeys, so stop trying */
    }

    /* Same thing but with the digits. */
    vkey = '0';
    for (keyc = 0; keyc < ARRAY_SIZE(default_map); keyc++)
    {
        if (default_map[keyc].vkey < '0' || '9' < default_map[keyc].vkey)
            continue; /* not a digit in ANSI layout */
        if (!thread_data->keyc2scan[keyc]) continue; /* not a known Mac key code */
        if (thread_data->keyc2vkey[keyc]) continue; /* already assigned */

        while (vkey <= '9' && vkey_used[vkey]) vkey++;
        if (vkey <= '9')
        {
            thread_data->keyc2vkey[keyc] = vkey;
            vkey_used[vkey] = 1;
            TRACE("keyc 0x%04x -> vkey 0x%04x (spare digit)\n", keyc, vkey);
        }
        else
            break; /* no more unused digit vkeys, so stop trying */
    }

    /* Last chance.  Assign any available vkey. */
    vkey_range = 0;
    vkey = vkey_ranges[vkey_range].first;
    for (keyc = 0; keyc < ARRAY_SIZE(default_map); keyc++)
    {
        if (!thread_data->keyc2scan[keyc]) continue; /* not a known Mac key code */
        if (thread_data->keyc2vkey[keyc]) continue; /* already assigned */

        while (vkey && vkey_used[vkey])
        {
            if (vkey == vkey_ranges[vkey_range].last)
            {
                vkey_range++;
                vkey = vkey_ranges[vkey_range].first;
            }
            else
                vkey++;
        }

        if (!vkey)
        {
            WARN("No more vkeys available!\n");
            break;
        }

        thread_data->keyc2vkey[keyc] = vkey;
        vkey_used[vkey] = 1;
        TRACE("keyc 0x%04x -> vkey 0x%04x (spare vkey)\n", keyc, vkey);
    }

    UCDisposeCollator(&collatorRef);
    UCDisposeCollator(&caseInsensitiveCollatorRef);
    UCDisposeCollator(&diacriticInsensitiveCollatorRef);
}


/***********************************************************************
 *              macdrv_send_keyboard_input
 */
static void macdrv_send_keyboard_input(HWND hwnd, WORD vkey, WORD scan, unsigned int flags, unsigned int time)
{
    INPUT input;

    TRACE_(key)("hwnd %p vkey=%04x scan=%04x flags=%04x\n", hwnd, vkey, scan, flags);

    input.type              = INPUT_KEYBOARD;
    input.ki.wVk            = vkey;
    input.ki.wScan          = scan;
    input.ki.dwFlags        = flags;
    input.ki.time           = time;
    input.ki.dwExtraInfo    = 0;

    NtUserSendHardwareInput(hwnd, 0, &input, 0);
}


/***********************************************************************
 *           update_modifier_state
 */
static void update_modifier_state(unsigned int modifier, unsigned int modifiers, const BYTE *keystate,
                                  WORD vkey, WORD alt_vkey, WORD scan, WORD alt_scan,
                                  DWORD event_time, BOOL restore)
{
    int key_pressed = (modifiers & modifier) != 0;
    int vkey_pressed = (keystate[vkey] & 0x80) || (keystate[alt_vkey] & 0x80);
    DWORD flags;

    if (key_pressed != vkey_pressed)
    {
        if (key_pressed)
        {
            flags = (scan & 0x100) ? KEYEVENTF_EXTENDEDKEY : 0;
            if (restore)
                flags |= KEYEVENTF_KEYUP;

            macdrv_send_keyboard_input(NULL, vkey, scan & 0xff, flags, event_time);
        }
        else
        {
            flags = restore ? 0 : KEYEVENTF_KEYUP;

            if (keystate[vkey] & 0x80)
            {
                macdrv_send_keyboard_input(NULL, vkey, scan & 0xff,
                                           flags | ((scan & 0x100) ? KEYEVENTF_EXTENDEDKEY : 0),
                                           event_time);
            }
            if (keystate[alt_vkey] & 0x80)
            {
                macdrv_send_keyboard_input(NULL, alt_vkey, alt_scan & 0xff,
                                           flags | ((alt_scan & 0x100) ? KEYEVENTF_EXTENDEDKEY : 0),
                                           event_time);
            }
        }
    }
}


/***********************************************************************
 *              macdrv_key_event
 *
 * Handler for KEY_PRESS and KEY_RELEASE events.
 */
void macdrv_key_event(HWND hwnd, const macdrv_event *event)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();
    WORD vkey, scan;
    DWORD flags;

    TRACE_(key)("win %p/%p key %s keycode %hu modifiers 0x%08llx\n",
                hwnd, event->window, (event->type == KEY_PRESS ? "press" : "release"),
                event->key.keycode, event->key.modifiers);

    thread_data->last_modifiers = event->key.modifiers;

    if (event->key.keycode < ARRAY_SIZE(thread_data->keyc2vkey))
    {
        vkey = thread_data->keyc2vkey[event->key.keycode];
        scan = thread_data->keyc2scan[event->key.keycode];
    }
    else
        vkey = scan = 0;

    TRACE_(key)("keycode %hu converted to vkey 0x%X scan 0x%02x\n",
                event->key.keycode, vkey, scan);

    if (!vkey) return;

    flags = 0;
    if (event->type == KEY_RELEASE) flags |= KEYEVENTF_KEYUP;
    if (scan & 0x100)               flags |= KEYEVENTF_EXTENDEDKEY;

    macdrv_send_keyboard_input(hwnd, vkey, scan & 0xff, flags, event->key.time_ms);
}


/***********************************************************************
 *              macdrv_keyboard_changed
 *
 * Handler for KEYBOARD_CHANGED events.
 */
void macdrv_keyboard_changed(const macdrv_event *event)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();

    TRACE("new keyboard layout uchr data %p, type %u, iso %d\n", event->keyboard_changed.uchr,
          event->keyboard_changed.keyboard_type, event->keyboard_changed.iso_keyboard);

    if (thread_data->keyboard_layout_uchr)
        CFRelease(thread_data->keyboard_layout_uchr);
    thread_data->keyboard_layout_uchr = CFDataCreateCopy(NULL, event->keyboard_changed.uchr);
    thread_data->keyboard_type = event->keyboard_changed.keyboard_type;
    thread_data->iso_keyboard = event->keyboard_changed.iso_keyboard;
    thread_data->active_keyboard_layout = macdrv_get_hkl_from_source(event->keyboard_changed.input_source);
    thread_data->dead_key_state = 0;

    macdrv_compute_keyboard_layout(thread_data);

    NtUserActivateKeyboardLayout(thread_data->active_keyboard_layout, 0);

    send_message(get_active_window(), WM_CANCELMODE, 0, 0);
}


/***********************************************************************
 *              macdrv_hotkey_press
 *
 * Handler for HOTKEY_PRESS events.
 */
void macdrv_hotkey_press(const macdrv_event *event)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();

    TRACE_(key)("vkey 0x%04x mod_flags 0x%04x keycode 0x%04x time %lu\n",
                event->hotkey_press.vkey, event->hotkey_press.mod_flags, event->hotkey_press.keycode,
                event->hotkey_press.time_ms);

    if (event->hotkey_press.keycode < ARRAY_SIZE(thread_data->keyc2vkey))
    {
        WORD scan = thread_data->keyc2scan[event->hotkey_press.keycode];
        BYTE keystate[256];
        BOOL got_keystate;
        DWORD flags;

        if ((got_keystate = NtUserGetAsyncKeyboardState(keystate)))
        {
            update_modifier_state(MOD_ALT, event->hotkey_press.mod_flags, keystate, VK_LMENU, VK_RMENU,
                                  0x38, 0x138, event->hotkey_press.time_ms, FALSE);
            update_modifier_state(MOD_CONTROL, event->hotkey_press.mod_flags, keystate, VK_LCONTROL, VK_RCONTROL,
                                  0x1D, 0x11D, event->hotkey_press.time_ms, FALSE);
            update_modifier_state(MOD_SHIFT, event->hotkey_press.mod_flags, keystate, VK_LSHIFT, VK_RSHIFT,
                                  0x2A, 0x36, event->hotkey_press.time_ms, FALSE);
            update_modifier_state(MOD_WIN, event->hotkey_press.mod_flags, keystate, VK_LWIN, VK_RWIN,
                                  0x15B, 0x15C, event->hotkey_press.time_ms, FALSE);
        }

        activate_on_following_focus();

        flags = (scan & 0x100) ? KEYEVENTF_EXTENDEDKEY : 0;
        macdrv_send_keyboard_input(NULL, event->hotkey_press.vkey, scan & 0xff,
                                   flags, event->key.time_ms);
        macdrv_send_keyboard_input(NULL, event->hotkey_press.vkey, scan & 0xff,
                                   flags | KEYEVENTF_KEYUP, event->key.time_ms);

        if (got_keystate)
        {
            update_modifier_state(MOD_ALT, event->hotkey_press.mod_flags, keystate, VK_LMENU, VK_RMENU,
                                  0x38, 0x138, event->hotkey_press.time_ms, TRUE);
            update_modifier_state(MOD_CONTROL, event->hotkey_press.mod_flags, keystate, VK_LCONTROL, VK_RCONTROL,
                                  0x1D, 0x11D, event->hotkey_press.time_ms, TRUE);
            update_modifier_state(MOD_SHIFT, event->hotkey_press.mod_flags, keystate, VK_LSHIFT, VK_RSHIFT,
                                  0x2A, 0x36, event->hotkey_press.time_ms, TRUE);
            update_modifier_state(MOD_WIN, event->hotkey_press.mod_flags, keystate, VK_LWIN, VK_RWIN,
                                  0x15B, 0x15C, event->hotkey_press.time_ms, TRUE);
        }
    }
}


/***********************************************************************
 *              ImeProcessKey (MACDRV.@)
 */
UINT macdrv_ImeProcessKey(HIMC himc, UINT wparam, UINT lparam, const BYTE *key_state)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();
    WORD scan = HIWORD(lparam) & 0x1ff, vkey = LOWORD(wparam);
    BOOL repeat = !!(lparam >> 30), pressed = !(lparam >> 31);
    unsigned int flags;
    int keyc, done = 0;

    TRACE("himc %p, scan %#x, vkey %#x, repeat %u, pressed %u\n",
          himc, scan, vkey, repeat, pressed);

    if (!macdrv_using_input_method()) return 0;

    if (!pressed)
    {
        /* Only key down events should be sent to the Cocoa input context. We do
           not handle key ups, and instead let those go through as a normal
           WM_KEYUP. */
        return 0;
    }

    switch (vkey)
    {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_CAPITAL:
        case VK_MENU:
        return 0;
    }

    flags = thread_data->last_modifiers;
    if (key_state[VK_SHIFT] & 0x80)
        flags |= NX_SHIFTMASK;
    else
        flags &= ~(NX_SHIFTMASK | NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK);
    if (key_state[VK_CAPITAL] & 0x01)
        flags |= NX_ALPHASHIFTMASK;
    else
        flags &= ~NX_ALPHASHIFTMASK;
    if (key_state[VK_CONTROL] & 0x80)
        flags |= NX_CONTROLMASK;
    else
        flags &= ~(NX_CONTROLMASK | NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK);
    if (key_state[VK_MENU] & 0x80)
        flags |= NX_COMMANDMASK;
    else
        flags &= ~(NX_COMMANDMASK | NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK);

    /* Find the Mac keycode corresponding to the scan code */
    for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
        if (thread_data->keyc2vkey[keyc] == vkey) break;

    if (keyc >= ARRAY_SIZE(thread_data->keyc2vkey)) return 0;

    TRACE("flags 0x%08x keyc 0x%04x\n", flags, keyc);

    macdrv_send_keydown_to_input_source(flags, repeat, keyc, himc, &done);
    while (!done) NtUserMsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_POSTMESSAGE | QS_SENDMESSAGE, 0);

    return done > 0;
}


/***********************************************************************
 *              ActivateKeyboardLayout (MACDRV.@)
 */
BOOL macdrv_ActivateKeyboardLayout(HKL hkl, UINT flags)
{
    BOOL ret = FALSE;
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    struct layout *layout;

    TRACE("hkl %p flags %04x\n", hkl, flags);

    if (hkl == thread_data->active_keyboard_layout)
        return TRUE;

    pthread_mutex_lock(&layout_list_mutex);
    update_layout_list();

    LIST_FOR_EACH_ENTRY(layout, &layout_list, struct layout, entry)
    {
        if (layout->hkl == hkl)
        {
            if (macdrv_select_input_source(layout->input_source))
            {
                ret = TRUE;
                if (thread_data->keyboard_layout_uchr)
                    CFRelease(thread_data->keyboard_layout_uchr);

                macdrv_get_input_source_info(&thread_data->keyboard_layout_uchr, &thread_data->keyboard_type,
                                             &thread_data->iso_keyboard, NULL);
                thread_data->active_keyboard_layout = hkl;
                thread_data->dead_key_state = 0;

                macdrv_compute_keyboard_layout(thread_data);
            }
            break;
        }
    }
    pthread_mutex_unlock(&layout_list_mutex);

    return ret;
}


/***********************************************************************
 *              Beep (MACDRV.@)
 */
void macdrv_Beep(void)
{
    macdrv_beep();
}


/***********************************************************************
 *              GetKeyNameText (MACDRV.@)
 */
INT macdrv_GetKeyNameText(LONG lparam, LPWSTR buffer, INT size)
{
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    int scan, keyc;

    scan = (lparam >> 16) & 0x1FF;
    for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2scan); keyc++)
    {
        if (thread_data->keyc2scan[keyc] == scan)
        {
            const UCKeyboardLayout *uchr;
            UInt32 deadKeyState = 0;
            UniCharCount len;
            OSStatus status;
            unsigned int vkey;
            int i;

            uchr = (const UCKeyboardLayout*)CFDataGetBytePtr(thread_data->keyboard_layout_uchr);
            status = UCKeyTranslate(uchr, keyc, kUCKeyActionDisplay, 0, thread_data->keyboard_type,
                                    0, &deadKeyState, size - 1, &len, (UniChar*)buffer);
            if (status != noErr)
                len = 0;
            if (len && buffer[0] > 32)
                buffer[len] = 0;

            vkey = thread_data->keyc2vkey[keyc];
            if (lparam & (1 << 25))
            {
                /* Caller doesn't care about distinctions between left and
                   right keys. */
                switch (vkey)
                {
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                        vkey = VK_SHIFT; break;
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                        vkey = VK_CONTROL; break;
                    case VK_LMENU:
                    case VK_RMENU:
                        vkey = VK_MENU; break;
                }
            }

            if (scan & 0x100) vkey |= 0x100;

            for (i = 0; i < ARRAY_SIZE(vkey_names); i++)
            {
                if (vkey_names[i].vkey == vkey)
                {
                    len = min(strlen(vkey_names[i].name) + 1, size);
                    ascii_to_unicode(buffer, vkey_names[i].name, len);
                    if (len) buffer[--len] = 0;
                    break;
                }
            }

            if (!len)
            {
                char name[16];
                len = snprintf(name, sizeof(name), "Key 0x%02x", vkey);
                len = min(len + 1, size);
                ascii_to_unicode(buffer, name, len);
                if (len) buffer[--len] = 0;
            }

            if (!len)
                break;

            if (status == noErr && deadKeyState)
            {
                for (i = 0; i < ARRAY_SIZE(dead_key_names); i++)
                {
                    if (dead_key_names[i].wchar == buffer[0])
                    {
                        len = min(strlen(dead_key_names[i].name) + 1, size);
                        ascii_to_unicode(buffer, dead_key_names[i].name, len);
                        if (len) buffer[--len] = 0;
                        break;
                    }
                }
            }

            if (status == noErr && len == 1 && buffer[0] >= 'a' && buffer[0] <= 'z')
                buffer[0] += 'A' - 'a';

            TRACE("lparam 0x%08x -> %s\n", (unsigned int)lparam, debugstr_w(buffer));
            return len;
        }
    }

    WARN("found no name for lparam 0x%08x\n", (unsigned int)lparam);
    return 0;
}


/***********************************************************************
 *     GetKeyboardLayoutList (MACDRV.@)
 */
UINT macdrv_GetKeyboardLayoutList(INT size, HKL *list)
{
    int count = 0;
    struct layout *layout;

    TRACE("%d, %p\n", size, list);

    pthread_mutex_lock(&layout_list_mutex);

    update_layout_list();

    LIST_FOR_EACH_ENTRY(layout, &layout_list, struct layout, entry)
    {
        if (!layout->enabled) continue;
        if (list)
        {
            if (count >= size) break;
            list[count] = layout->hkl;
            TRACE("\t%d: %p\n", count, list[count]);
        }
        count++;
    }
    pthread_mutex_unlock(&layout_list_mutex);

    TRACE("returning %d\n", count);
    return count;
}


/***********************************************************************
 *              MapVirtualKeyEx (MACDRV.@)
 */
UINT macdrv_MapVirtualKeyEx(UINT wCode, UINT wMapType, HKL hkl)
{
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    UINT ret = 0;
    int keyc;

    TRACE("wCode=0x%x, wMapType=%d, hkl %p\n", wCode, wMapType, hkl);

    switch (wMapType)
    {
        case MAPVK_VK_TO_VSC: /* vkey-code to scan-code */
        case MAPVK_VK_TO_VSC_EX:
            switch (wCode)
            {
                case VK_SHIFT: wCode = VK_LSHIFT; break;
                case VK_CONTROL: wCode = VK_LCONTROL; break;
                case VK_MENU: wCode = VK_LMENU; break;
            }

            /* vkey -> keycode -> scan */
            for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
            {
                if (thread_data->keyc2vkey[keyc] == wCode)
                {
                    ret = thread_data->keyc2scan[keyc] & 0xFF;
                    break;
                }
            }

            /* set scan code prefix */
            if (wMapType == MAPVK_VK_TO_VSC_EX &&
                (wCode == VK_RCONTROL || wCode == VK_RMENU))
                ret |= 0xe000;
            break;

        case MAPVK_VSC_TO_VK: /* scan-code to vkey-code */
        case MAPVK_VSC_TO_VK_EX:
            /* scan -> keycode -> vkey */
            for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
                if ((thread_data->keyc2scan[keyc] & 0xFF) == (wCode & 0xFF))
                {
                    ret = thread_data->keyc2vkey[keyc];
                    /* Only stop if it's not a numpad vkey; otherwise keep
                       looking for a potential better vkey. */
                    if (ret && (ret < VK_NUMPAD0 || VK_DIVIDE < ret))
                        break;
                }

            if (wMapType == MAPVK_VSC_TO_VK)
                switch (ret)
                {
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                        ret = VK_SHIFT; break;
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                        ret = VK_CONTROL; break;
                    case VK_LMENU:
                    case VK_RMENU:
                        ret = VK_MENU; break;
                }

            break;

        case MAPVK_VK_TO_CHAR: /* vkey-code to character */
        {
            /* vkey -> keycode -> (UCKeyTranslate) wide char */
            struct macdrv_thread_data *thread_data = macdrv_thread_data();
            const UCKeyboardLayout *uchr;
            UniChar s[10];
            OSStatus status;
            UInt32 deadKeyState;
            UniCharCount len;
            BOOL deadKey = FALSE;

            if ((VK_PRIOR <= wCode && wCode <= VK_HELP) ||
                (VK_F1 <= wCode && wCode <= VK_F24))
                break;

            if (!thread_data || !thread_data->keyboard_layout_uchr)
            {
                WARN("No keyboard layout uchr data\n");
                break;
            }

            uchr = (const UCKeyboardLayout*)CFDataGetBytePtr(thread_data->keyboard_layout_uchr);

            /* Find the Mac keycode corresponding to the vkey */
            for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
                if (thread_data->keyc2vkey[keyc] == wCode) break;

            if (keyc >= ARRAY_SIZE(thread_data->keyc2vkey))
            {
                WARN("Unknown virtual key %X\n", wCode);
                break;
            }

            TRACE("Found keycode %u\n", keyc);

            deadKeyState = 0;
            status = UCKeyTranslate(uchr, keyc, kUCKeyActionDown, 0,
                thread_data->keyboard_type, 0, &deadKeyState, ARRAY_SIZE(s), &len, s);
            if (status == noErr && !len && deadKeyState)
            {
                deadKey = TRUE;
                deadKeyState = 0;
                status = UCKeyTranslate(uchr, keyc, kUCKeyActionDown, 0,
                    thread_data->keyboard_type, kUCKeyTranslateNoDeadKeysMask,
                    &deadKeyState, ARRAY_SIZE(s), &len, s);
            }

            if (status == noErr && len)
                ret = RtlUpcaseUnicodeChar(s[0]) | (deadKey ? 0x80000000 : 0);

            break;
        }
        default: /* reserved */
            FIXME("Unknown wMapType %d\n", wMapType);
            break;
    }

    TRACE("returning 0x%04x\n", ret);
    return ret;
}


/***********************************************************************
 *              RegisterHotKey (MACDRV.@)
 */
BOOL macdrv_RegisterHotKey(HWND hwnd, UINT mod_flags, UINT vkey)
{
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    unsigned int keyc, modifiers = 0;
    int ret;

    TRACE_(key)("hwnd %p mod_flags 0x%04x vkey 0x%04x\n", hwnd, mod_flags, vkey);

    /* Find the Mac keycode corresponding to the vkey */
    for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
        if (thread_data->keyc2vkey[keyc] == vkey) break;

    if (keyc >= ARRAY_SIZE(thread_data->keyc2vkey))
    {
        WARN_(key)("ignoring unknown virtual key 0x%04x\n", vkey);
        return TRUE;
    }

    if (mod_flags & MOD_ALT)        modifiers |= cmdKey;
    if (mod_flags & MOD_CONTROL)    modifiers |= controlKey;
    if (mod_flags & MOD_SHIFT)      modifiers |= shiftKey;
    if (mod_flags & MOD_WIN)
    {
        WARN_(key)("MOD_WIN not supported; ignoring\n");
        return TRUE;
    }

    ret = macdrv_register_hot_key(thread_data->queue, vkey, mod_flags, keyc, modifiers);
    TRACE_(key)("keyc 0x%04x modifiers 0x%08x -> %d\n", keyc, modifiers, ret);

    if (ret == MACDRV_HOTKEY_ALREADY_REGISTERED)
        RtlSetLastWin32Error(ERROR_HOTKEY_ALREADY_REGISTERED);
    else if (ret != MACDRV_HOTKEY_SUCCESS)
        RtlSetLastWin32Error(ERROR_GEN_FAILURE);

    return ret == MACDRV_HOTKEY_SUCCESS;
}


/***********************************************************************
 *              ToUnicodeEx (MACDRV.@)
 *
 * The ToUnicode function translates the specified virtual-key code and keyboard
 * state to the corresponding Windows character or characters.
 *
 * If the specified key is a dead key, the return value is negative. Otherwise,
 * it is one of the following values:
 * Value        Meaning
 * -1           The specified virtual key is a dead-key.  If possible, the
 *              non-combining form of the dead character is written to bufW.
 * 0            The specified virtual key has no translation for the current
 *              state of the keyboard.
 * 1            One Windows character was copied to the buffer.
 * 2 or more    Multiple characters were copied to the buffer. This usually
 *              happens when a dead-key character (accent or diacritic) stored
 *              in the keyboard layout cannot be composed with the specified
 *              virtual key to form a single character.
 *
 */
INT macdrv_ToUnicodeEx(UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                       LPWSTR bufW, int bufW_size, UINT flags, HKL hkl)
{
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    INT ret = 0;
    int keyc;
    BOOL is_menu = (flags & 0x1);
    int status;
    const UCKeyboardLayout *uchr;
    UInt16 keyAction;
    UInt32 modifierKeyState;
    OptionBits options;
    UInt32 deadKeyState, savedDeadKeyState;
    UniCharCount len = 0;
    BOOL dead = FALSE;

    TRACE_(key)("virtKey 0x%04x scanCode 0x%04x lpKeyState %p bufW %p bufW_size %d flags 0x%08x hkl %p\n",
                virtKey, scanCode, lpKeyState, bufW, bufW_size, flags, hkl);

    if (!virtKey)
        goto done;

    /* UCKeyTranslate, below, terminates a dead-key sequence if passed a
       modifier key press.  We want it to effectively ignore modifier key
       presses.  I think that one isn't supposed to call it at all for modifier
       events (e.g. NSFlagsChanged or kEventRawKeyModifiersChanged), since they
       are different event types than key up/down events. */
    switch (virtKey)
    {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_CAPITAL:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_LMENU:
        case VK_RMENU:
            goto done;
    }

    /* There are a number of key combinations for which Windows does not
       produce characters, but Mac keyboard layouts may.  Eat them.  Do this
       here to avoid the expense of UCKeyTranslate() but also because these
       keys shouldn't terminate dead key sequences. */
    if ((VK_PRIOR <= virtKey && virtKey <= VK_HELP) || (VK_F1 <= virtKey && virtKey <= VK_F24))
        goto done;

    /* Shift + <non-digit keypad keys>. */
    if ((lpKeyState[VK_SHIFT] & 0x80) && VK_MULTIPLY <= virtKey && virtKey <= VK_DIVIDE)
        goto done;

    if (lpKeyState[VK_CONTROL] & 0x80)
    {
        /* Control-Tab, with or without other modifiers. */
        if (virtKey == VK_TAB)
            goto done;

        /* Control-Shift-<key>, Control-Alt-<key>, and Control-Alt-Shift-<key>
           for these keys. */
        if ((lpKeyState[VK_SHIFT] & 0x80) || (lpKeyState[VK_MENU] & 0x80))
        {
            switch (virtKey)
            {
                case VK_CANCEL:
                case VK_BACK:
                case VK_ESCAPE:
                case VK_SPACE:
                case VK_RETURN:
                    goto done;
            }
        }
    }

    if (thread_data->keyboard_layout_uchr)
        uchr = (const UCKeyboardLayout*)CFDataGetBytePtr(thread_data->keyboard_layout_uchr);
    else
        uchr = NULL;

    keyAction = (scanCode & 0x8000) ? kUCKeyActionUp : kUCKeyActionDown;

    modifierKeyState = 0;
    if (lpKeyState[VK_SHIFT] & 0x80)
        modifierKeyState |= (shiftKey >> 8);
    if (lpKeyState[VK_CAPITAL] & 0x01)
        modifierKeyState |= (alphaLock >> 8);
    if (lpKeyState[VK_CONTROL] & 0x80)
        modifierKeyState |= (controlKey >> 8);
    if (lpKeyState[VK_MENU] & 0x80)
        modifierKeyState |= (cmdKey >> 8);
    if (thread_data->last_modifiers & (NX_ALTERNATEMASK | NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK))
        modifierKeyState |= (optionKey >> 8);

    /* Find the Mac keycode corresponding to the vkey */
    for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
        if (thread_data->keyc2vkey[keyc] == virtKey) break;

    if (keyc >= ARRAY_SIZE(thread_data->keyc2vkey))
    {
        WARN_(key)("Unknown virtual key 0x%04x\n", virtKey);
        goto done;
    }

    TRACE_(key)("Key code 0x%04x %s, faked modifiers = 0x%04x\n", keyc,
                (keyAction == kUCKeyActionDown) ? "pressed" : "released", (unsigned)modifierKeyState);

    if (is_menu)
    {
        if (keyAction == kUCKeyActionUp)
            goto done;

        options = kUCKeyTranslateNoDeadKeysMask;
        deadKeyState = 0;
    }
    else
    {
        options = 0;
        deadKeyState = thread_data->dead_key_state;
    }
    savedDeadKeyState = deadKeyState;
    status = UCKeyTranslate(uchr, keyc, keyAction, modifierKeyState,
        thread_data->keyboard_type, options, &deadKeyState, bufW_size,
        &len, bufW);
    if (status != noErr)
    {
        ERR_(key)("Couldn't translate keycode 0x%04x, status %d\n", keyc, status);
        goto done;
    }
    if (!is_menu)
    {
        if (keyAction != kUCKeyActionUp && len > 0 && deadKeyState == thread_data->dead_key_state)
            thread_data->dead_key_state = 0;
        else
            thread_data->dead_key_state = deadKeyState;

        if (keyAction == kUCKeyActionUp)
            goto done;
    }

    if (len == 0 && deadKeyState)
    {
        /* Repeat the translation, but disabling dead-key generation to
           learn which dead key it was. */
        status = UCKeyTranslate(uchr, keyc, keyAction, modifierKeyState,
            thread_data->keyboard_type, kUCKeyTranslateNoDeadKeysMask,
            &savedDeadKeyState, bufW_size, &len, bufW);
        if (status != noErr)
        {
            ERR_(key)("Couldn't translate dead keycode 0x%04x, status %d\n", keyc, status);
            goto done;
        }

        dead = TRUE;
    }

    if (len > 0)
        len = strip_apple_private_chars(bufW, len);

    ret = dead ? -len : len;

    /* Control-Return produces line feed instead of carriage return. */
    if (ret > 0 && (lpKeyState[VK_CONTROL] & 0x80) && virtKey == VK_RETURN)
    {
        int i;
        for (i = 0; i < len; i++)
            if (bufW[i] == '\r')
                bufW[i] = '\n';
    }

done:
    /* Null-terminate the buffer, if there's room.  MSDN clearly states that the
       caller must not assume this is done, but some programs (e.g. Audiosurf) do. */
    if (len < bufW_size)
        bufW[len] = 0;

    TRACE_(key)("returning %d / %s\n", ret, debugstr_wn(bufW, len));
    return ret;
}


/***********************************************************************
 *              UnregisterHotKey (MACDRV.@)
 */
void macdrv_UnregisterHotKey(HWND hwnd, UINT modifiers, UINT vkey)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();

    TRACE_(key)("hwnd %p modifiers 0x%04x vkey 0x%04x\n", hwnd, modifiers, vkey);

    if (thread_data)
        macdrv_unregister_hot_key(thread_data->queue, vkey, modifiers);
}


/***********************************************************************
 *              VkKeyScanEx (MACDRV.@)
 *
 * Note: Windows ignores HKL parameter and uses current active layout instead
 */
SHORT macdrv_VkKeyScanEx(WCHAR wChar, HKL hkl)
{
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    SHORT ret = -1;
    int state;
    const UCKeyboardLayout *uchr;

    TRACE("%04x, %p\n", wChar, hkl);

    uchr = (const UCKeyboardLayout*)CFDataGetBytePtr(thread_data->keyboard_layout_uchr);
    if (!uchr)
    {
        TRACE("no keyboard layout UCHR data; returning -1\n");
        return -1;
    }

    for (state = 0; state < 8; state++)
    {
        UInt32 modifierKeyState = 0;
        int keyc;

        if (state & 1)
            modifierKeyState |= (shiftKey >> 8);
        if ((state & 6) == 6)
            modifierKeyState |= (optionKey >> 8);
        else
        {
            if (state & 2)
                modifierKeyState |= (controlKey >> 8);
            if (state & 4)
                modifierKeyState |= (cmdKey >> 8);
        }

        for (keyc = 0; keyc < ARRAY_SIZE(thread_data->keyc2vkey); keyc++)
        {
            UInt32 deadKeyState = 0;
            UniChar uchar;
            UniCharCount len;
            OSStatus status;

            if (!thread_data->keyc2vkey[keyc]) continue;

            status = UCKeyTranslate(uchr, keyc, kUCKeyActionDown, modifierKeyState,
                                    thread_data->keyboard_type, 0, &deadKeyState,
                                    1, &len, &uchar);
            if (status == noErr && len == 1 && uchar == wChar)
            {
                WORD vkey = thread_data->keyc2vkey[keyc];

                ret = vkey | (state << 8);
                if ((VK_NUMPAD0 <= vkey && vkey <= VK_DIVIDE) ||
                    keyc == kVK_ANSI_KeypadClear || keyc == kVK_ANSI_KeypadEnter ||
                    keyc == kVK_ANSI_KeypadEquals)
                {
                    /* Keep searching for a non-numpad match, which is preferred. */
                }
                else
                    goto done;
            }
        }
    }

done:
    TRACE(" -> 0x%04x\n", ret);
    return ret;
}
