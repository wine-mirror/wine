/*
 * Interface to terminfo and termcap libraries
 *
 * Copyright 2010 Eric Pouech
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

#include <stdarg.h>
#include <stdlib.h>
#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#endif
/* avoid redefinition warnings */
#undef KEY_EVENT
#undef MOUSE_MOVED

#if defined(HAVE_CURSES_H) || defined(HAVE_NCURSES_H)
#include <term.h>
#endif

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wincon.h>
#include "console_private.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

static const int vkkeyscan_table[256] =
{
     0,0,0,0,0,0,0,0,8,9,0,0,0,13,0,0,0,0,0,19,145,556,0,0,0,0,0,27,0,0,0,
     0,32,305,478,307,308,309,311,222,313,304,312,443,188,189,190,191,48,
     49,50,51,52,53,54,55,56,57,442,186,444,187,446,447,306,321,322,323,
     324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,339,340,
     341,342,343,344,345,346,219,220,221,310,445,192,65,66,67,68,69,70,71,
     72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,475,476,477,
     448,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,400,0,0,0,0,0,0
};

static const int mapvkey_0[256] =
{
     0,0,0,0,0,0,0,0,14,15,0,0,0,28,0,0,42,29,56,69,58,0,0,0,0,0,0,1,0,0,
     0,0,57,73,81,79,71,75,72,77,80,0,0,0,55,82,83,0,11,2,3,4,5,6,7,8,9,
     10,0,0,0,0,0,0,0,30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,
     19,31,20,22,47,17,45,21,44,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,78,0,74,
     0,53,59,60,61,62,63,64,65,66,67,68,87,88,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,69,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,13,51,12,52,53,41,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,43,27,40,76,96,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static inline void init_complex_char(INPUT_RECORD* ir, BOOL down, WORD vk, WORD kc, DWORD cks)
{
    ir->EventType			 = KEY_EVENT;
    ir->Event.KeyEvent.bKeyDown	         = down;
    ir->Event.KeyEvent.wRepeatCount	 = 1;
    ir->Event.KeyEvent.wVirtualScanCode  = vk;
    ir->Event.KeyEvent.wVirtualKeyCode   = kc;
    ir->Event.KeyEvent.dwControlKeyState = cks;
    ir->Event.KeyEvent.uChar.UnicodeChar = 0;
}

/******************************************************************
 *		TERM_FillSimpleChar
 *
 */
unsigned TERM_FillSimpleChar(WCHAR real_inchar, INPUT_RECORD* ir)
{
    unsigned            vk;
    WCHAR               inchar;
    unsigned            numEvent = 0;
    DWORD               cks = 0;

    switch (real_inchar)
    {
    case   9: inchar = real_inchar;
        real_inchar = 27; /* so that we don't think key is ctrl- something */
        break;
    case  13:
    case  10: inchar = '\r';
        real_inchar = 27; /* Fixme: so that we don't think key is ctrl- something */
        break;
    case 127: inchar = '\b';
        break;
    default:
        inchar = real_inchar;
        break;
    }
    vk = (inchar < 256) ? vkkeyscan_table[inchar] : 0;
    if (vk & 0x0100)
        init_complex_char(&ir[numEvent++], 1, 0x2a, 0x10, SHIFT_PRESSED);
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        init_complex_char(&ir[numEvent++], 1, 0x1d, 0x11, LEFT_CTRL_PRESSED);
    if (vk & 0x0400)
        init_complex_char(&ir[numEvent++], 1, 0x38, 0x12, LEFT_ALT_PRESSED);

    ir[numEvent].EventType                        = KEY_EVENT;
    ir[numEvent].Event.KeyEvent.bKeyDown          = 1;
    ir[numEvent].Event.KeyEvent.wRepeatCount      = 1;
    ir[numEvent].Event.KeyEvent.dwControlKeyState = cks;
    if (vk & 0x0100)
        ir[numEvent].Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        ir[numEvent].Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
    if (vk & 0x0400)
        ir[numEvent].Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
    ir[numEvent].Event.KeyEvent.wVirtualKeyCode = vk;
    ir[numEvent].Event.KeyEvent.wVirtualScanCode = mapvkey_0[vk & 0x00ff]; /* VirtualKeyCodes to ScanCode */

    ir[numEvent].Event.KeyEvent.uChar.UnicodeChar = inchar;
    ir[numEvent + 1] = ir[numEvent];
    ir[numEvent + 1].Event.KeyEvent.bKeyDown = 0;

    numEvent += 2;

    if (vk & 0x0400)
        init_complex_char(&ir[numEvent++], 0, 0x38, 0x12, LEFT_ALT_PRESSED);
    if ((vk & 0x0200) || (unsigned char)real_inchar <= 26)
        init_complex_char(&ir[numEvent++], 0, 0x1d, 0x11, 0);
    if (vk & 0x0100)
        init_complex_char(&ir[numEvent++], 0, 0x2a, 0x10, 0);
    return numEvent;
}

#if defined(SONAME_LIBCURSES) || defined(SONAME_LIBNCURSES)

#ifdef HAVE_NCURSES_H
# define CURSES_NAME "ncurses"
#else
# define CURSES_NAME "curses"
#endif

static void *nc_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f;

MAKE_FUNCPTR(putp)
MAKE_FUNCPTR(setupterm)
MAKE_FUNCPTR(tigetstr)
MAKE_FUNCPTR(tparm)

#undef MAKE_FUNCPTR

/**********************************************************************/

static BOOL TERM_bind_libcurses(void)
{
#ifdef SONAME_LIBNCURSES
    static const char ncname[] = SONAME_LIBNCURSES;
#else
    static const char ncname[] = SONAME_LIBCURSES;
#endif

    if (!(nc_handle = wine_dlopen(ncname, RTLD_NOW, NULL, 0)))
    {
        MESSAGE("Wine cannot find the " CURSES_NAME " library (%s).\n", ncname);
        return FALSE;
    }

#define LOAD_FUNCPTR(f)                                      \
    if((p_##f = wine_dlsym(nc_handle, #f, NULL, 0)) == NULL) \
    {                                                        \
        WARN("Can't find symbol %s\n", #f);                  \
        goto sym_not_found;                                  \
    }

    LOAD_FUNCPTR(putp)
    LOAD_FUNCPTR(setupterm)
    LOAD_FUNCPTR(tigetstr)
    LOAD_FUNCPTR(tparm)

#undef LOAD_FUNCPTR

    return TRUE;

sym_not_found:
    MESSAGE(
      "Wine cannot find certain functions that it needs inside the "
       CURSES_NAME "\nlibrary.  To enable Wine to use " CURSES_NAME
      " please upgrade your " CURSES_NAME "\nlibraries\n");
    wine_dlclose(nc_handle, NULL, 0);
    nc_handle = NULL;
    return FALSE;
}

#define putp      p_putp
#define setupterm p_setupterm
#define tigetstr  p_tigetstr
#define tparm     p_tparm

struct dbkey_descr
{
    enum dbkey_kind {dbk_simple, dbk_complex} kind;
    DWORD_PTR   p1;
    DWORD_PTR   p2;
    DWORD_PTR   p3;
};

struct dbkey_init
{
    const char*         string_normal;
    const char*         string_xterm;
    struct dbkey_descr  descr;
};

static const struct dbkey_init TERM_dbkey_init[] = {
    {"kcud1", "kDN",  {dbk_complex, 0x50, 0x28, 0}},
    {"kcuu1", "kUP",  {dbk_complex, 0x48, 0x26, 0}},
    {"kcub1", "kLFT", {dbk_complex, 0x4b, 0x25, 0}},
    {"kcuf1", "kRIT", {dbk_complex, 0x4d, 0x27, 0}},
    {"khome", "kHOM", {dbk_complex, 0x47, 0x24, 0}},
    {"kbs",   NULL,   {dbk_simple,  0x7f, 0x00, 0}},
    {"kf1",   NULL,   {dbk_complex, 0x3b, 0x70, 0}},
    {"kf2",   NULL,   {dbk_complex, 0x3c, 0x71, 0}},
    {"kf3",   NULL,   {dbk_complex, 0x3d, 0x72, 0}},
    {"kf4",   NULL,   {dbk_complex, 0x3e, 0x73, 0}},
    {"kf5",   NULL,   {dbk_complex, 0x3f, 0x74, 0}},
    {"kf6",   NULL,   {dbk_complex, 0x40, 0x75, 0}},
    {"kf7",   NULL,   {dbk_complex, 0x41, 0x76, 0}},
    {"kf8",   NULL,   {dbk_complex, 0x42, 0x77, 0}},
    {"kf9",   NULL,   {dbk_complex, 0x43, 0x78, 0}},
    {"kf10",  NULL,   {dbk_complex, 0x44, 0x79, 0}},
    {"kf11",  NULL,   {dbk_complex, 0xd9, 0x7a, 0}},
    {"kf12",  NULL,   {dbk_complex, 0xda, 0x7b, 0}},
    {"kf13",  NULL,   {dbk_complex, 0x3b, 0x70, SHIFT_PRESSED}},
    {"kf14",  NULL,   {dbk_complex, 0x3c, 0x71, SHIFT_PRESSED}},
    {"kf15",  NULL,   {dbk_complex, 0x3d, 0x72, SHIFT_PRESSED}},
    {"kf16",  NULL,   {dbk_complex, 0x3e, 0x73, SHIFT_PRESSED}},
    {"kf17",  NULL,   {dbk_complex, 0x3f, 0x74, SHIFT_PRESSED}},
    {"kf18",  NULL,   {dbk_complex, 0x40, 0x75, SHIFT_PRESSED}},
    {"kf19",  NULL,   {dbk_complex, 0x41, 0x76, SHIFT_PRESSED}},
    {"kf20",  NULL,   {dbk_complex, 0x42, 0x77, SHIFT_PRESSED}},
    {"kf21",  NULL,   {dbk_complex, 0x43, 0x78, SHIFT_PRESSED}},
    {"kf22",  NULL,   {dbk_complex, 0x44, 0x79, SHIFT_PRESSED}},
    {"kf23",  NULL,   {dbk_complex, 0xd9, 0x7a, SHIFT_PRESSED}},
    {"kf24",  NULL,   {dbk_complex, 0xda, 0x7b, SHIFT_PRESSED}},
    {"kf25",  NULL,   {dbk_complex, 0x3b, 0x70, LEFT_CTRL_PRESSED}},
    {"kf26",  NULL,   {dbk_complex, 0x3c, 0x71, LEFT_CTRL_PRESSED}},
    {"kf27",  NULL,   {dbk_complex, 0x3d, 0x72, LEFT_CTRL_PRESSED}},
    {"kf28",  NULL,   {dbk_complex, 0x3e, 0x73, LEFT_CTRL_PRESSED}},
    {"kf29",  NULL,   {dbk_complex, 0x3f, 0x74, LEFT_CTRL_PRESSED}},
    {"kf30",  NULL,   {dbk_complex, 0x40, 0x75, LEFT_CTRL_PRESSED}},
    {"kf31",  NULL,   {dbk_complex, 0x41, 0x76, LEFT_CTRL_PRESSED}},
    {"kf32",  NULL,   {dbk_complex, 0x42, 0x77, LEFT_CTRL_PRESSED}},
    {"kf33",  NULL,   {dbk_complex, 0x43, 0x78, LEFT_CTRL_PRESSED}},
    {"kf34",  NULL,   {dbk_complex, 0x44, 0x79, LEFT_CTRL_PRESSED}},
    {"kf35",  NULL,   {dbk_complex, 0xd9, 0x7a, LEFT_CTRL_PRESSED}},
    {"kf36",  NULL,   {dbk_complex, 0xda, 0x7b, LEFT_CTRL_PRESSED}},
    {"kf37",  NULL,   {dbk_complex, 0x3b, 0x70, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf38",  NULL,   {dbk_complex, 0x3c, 0x71, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf39",  NULL,   {dbk_complex, 0x3d, 0x72, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf40",  NULL,   {dbk_complex, 0x3e, 0x73, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf41",  NULL,   {dbk_complex, 0x3f, 0x74, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf42",  NULL,   {dbk_complex, 0x40, 0x75, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf43",  NULL,   {dbk_complex, 0x41, 0x76, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf44",  NULL,   {dbk_complex, 0x42, 0x77, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf45",  NULL,   {dbk_complex, 0x43, 0x78, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf46",  NULL,   {dbk_complex, 0x44, 0x79, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf47",  NULL,   {dbk_complex, 0xd9, 0x7a, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf48",  NULL,   {dbk_complex, 0xda, 0x7b, LEFT_CTRL_PRESSED|SHIFT_PRESSED}},
    {"kf49",  NULL,   {dbk_complex, 0x3b, 0x70, LEFT_ALT_PRESSED}},
    {"kf50",  NULL,   {dbk_complex, 0x3c, 0x71, LEFT_ALT_PRESSED}},
    {"kf51",  NULL,   {dbk_complex, 0x3d, 0x72, LEFT_ALT_PRESSED}},
    {"kf52",  NULL,   {dbk_complex, 0x3e, 0x73, LEFT_ALT_PRESSED}},
    {"kf53",  NULL,   {dbk_complex, 0x3f, 0x74, LEFT_ALT_PRESSED}},
    {"kf54",  NULL,   {dbk_complex, 0x40, 0x75, LEFT_ALT_PRESSED}},
    {"kf55",  NULL,   {dbk_complex, 0x41, 0x76, LEFT_ALT_PRESSED}},
    {"kf56",  NULL,   {dbk_complex, 0x42, 0x77, LEFT_ALT_PRESSED}},
    {"kf57",  NULL,   {dbk_complex, 0x43, 0x78, LEFT_ALT_PRESSED}},
    {"kf58",  NULL,   {dbk_complex, 0x44, 0x79, LEFT_ALT_PRESSED}},
    {"kf59",  NULL,   {dbk_complex, 0xd9, 0x7a, LEFT_ALT_PRESSED}},
    {"kf60",  NULL,   {dbk_complex, 0xda, 0x7b, LEFT_ALT_PRESSED}},
    {"kf61",  NULL,   {dbk_complex, 0x3b, 0x70, LEFT_ALT_PRESSED|SHIFT_PRESSED}},
    {"kf62",  NULL,   {dbk_complex, 0x3c, 0x71, LEFT_ALT_PRESSED|SHIFT_PRESSED}},
    {"kf63",  NULL,   {dbk_complex, 0x3d, 0x72, LEFT_ALT_PRESSED|SHIFT_PRESSED}},

    {"kdch1", "kDC",  {dbk_complex, 0x53, 0x2e, 0}},
    {"kich1", "kIC",  {dbk_complex, 0x52, 0x2d, 0}},
    {"knp",   "kNXT", {dbk_complex, 0x51, 0x22, 0}},
    {"kpp",   "kPRV", {dbk_complex, 0x49, 0x21, 0}},
    {"kcbt",  NULL,   {dbk_simple,  0x09, 0x00, SHIFT_PRESSED}},
    {"kend",  "kEND", {dbk_complex, 0x4f, 0x23, 0}},

    /* {"kmous", NULL, }, */
    /* Still some keys to manage:
       KEY_DL           KEY_IL          KEY_EIC         KEY_CLEAR               KEY_EOS
       KEY_EOL          KEY_SF          KEY_SR          KEY_STAB                KEY_CTAB
       KEY_CATAB        KEY_ENTER       KEY_SRESET      KEY_RESET               KEY_PRINT
       KEY_LL           KEY_A1          KEY_A3          KEY_B2                  KEY_C1
       KEY_C3           KEY_BEG         KEY_CANCEL      KEY_CLOSE               KEY_COMMAND
       KEY_COPY         KEY_CREATE      KEY_EXIT        KEY_FIND                KEY_HELP
       KEY_MARK         KEY_MESSAGE     KEY_MOVE        KEY_NEXT                KEY_OPEN
       KEY_OPTIONS      KEY_PREVIOUS    KEY_REDO        KEY_REFERENCE           KEY_REFRESH
       KEY_REPLACE      KEY_RESTART     KEY_RESUME      KEY_SAVE                KEY_SBEG
       KEY_SCANCEL      KEY_SCOMMAND    KEY_SCOPY       KEY_SCREATE             KEY_RESIZE
       KEY_SDL          KEY_SELECT      KEY_SEOL        KEY_SEXIT               KEY_SFIND
       KEY_SHELP        KEY_SMESSAGE    KEY_SMOVE       KEY_SNEXT               KEY_SOPTIONS
       KEY_SPREVIOUS    KEY_SPRINT      KEY_SREDO       KEY_SREPLACE            KEY_SRSUME
       KEY_SSAVE        KEY_SSUSPEND    KEY_SUNDO       KEY_SUSPEND             KEY_UNDO
    */
};

struct dbkey_pair
{
    const char*         string;
    unsigned            string_len;
    struct dbkey_descr  descr;
};

static struct dbkey_pair*       TERM_dbkey;
static unsigned                 TERM_dbkey_size;
static unsigned                 TERM_dbkey_index;

static int  TERM_dbkey_cmp(const void* p1, const void* p2)
{
    const struct dbkey_pair*  kp1 = p1;
    const struct dbkey_pair*  kp2 = p2;
    return strcmp(kp1->string, kp2->string);
}

static BOOL TERM_AddKeyDescr(const char* string, const struct dbkey_descr* descr)
{
    if (!string || string == (const char*)-1) return TRUE;
    if (!TERM_dbkey)
    {
        TERM_dbkey_size = 32;
        TERM_dbkey = HeapAlloc(GetProcessHeap(), 0, TERM_dbkey_size * sizeof(struct dbkey_pair));
        if (!TERM_dbkey) return FALSE;
    }
    if (TERM_dbkey_index == TERM_dbkey_size)
    {
        struct dbkey_pair*      new;

        new = HeapReAlloc(GetProcessHeap(), 0, TERM_dbkey, (2 * TERM_dbkey_size) * sizeof(struct dbkey_pair));
        if (!new) return FALSE;
        TERM_dbkey = new;
        TERM_dbkey_size *= 2;
    }
    TERM_dbkey[TERM_dbkey_index].string     = string;
    TERM_dbkey[TERM_dbkey_index].string_len = strlen(string);
    TERM_dbkey[TERM_dbkey_index].descr      = *descr;
    TERM_dbkey_index++;
    return TRUE;
}

static BOOL TERM_BuildKeyDB(void)
{
    unsigned i, j, len;
    struct dbkey_descr descr;
    char tmp[64];

    for (i = 0; i < sizeof(TERM_dbkey_init) / sizeof(TERM_dbkey_init[0]); i++)
    {
        if (!TERM_AddKeyDescr(tigetstr((char *)TERM_dbkey_init[i].string_normal), &TERM_dbkey_init[i].descr))
            return FALSE;
        if (TERM_dbkey_init[i].string_xterm)
        {
            descr = TERM_dbkey_init[i].descr;
            strcpy(tmp, TERM_dbkey_init[i].string_xterm);
            len = strlen(tmp);
            tmp[len + 1] = '\0';
#define X(v, f) do { tmp[len] = v; descr.p3 = (f); if (!TERM_AddKeyDescr(tigetstr(tmp), &descr)) return FALSE; } while (0)
            X('\0', SHIFT_PRESSED);
            X('3',  LEFT_ALT_PRESSED);
            X('4',  SHIFT_PRESSED | LEFT_ALT_PRESSED);
            X('5',  LEFT_CTRL_PRESSED);
            X('6',  LEFT_CTRL_PRESSED|SHIFT_PRESSED);
            X('7',  LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED);
            X('8',  LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED|SHIFT_PRESSED);
#undef X
        }
    }
    for (i = 0; i < TERM_dbkey_index; i++)
    {
        for (j = 0; j < TERM_dbkey_index; j++)
        {
            if (i != j &&
                TERM_dbkey[i].string_len >= TERM_dbkey[j].string_len &&
                !memcmp(TERM_dbkey[i].string, TERM_dbkey[j].string, TERM_dbkey[j].string_len))
                FIXME("substring %d/%s %d/%s\n", i, TERM_dbkey[i].string, j, TERM_dbkey[j].string);
        }
    }
    qsort(TERM_dbkey, TERM_dbkey_index, sizeof(TERM_dbkey[0]), TERM_dbkey_cmp);
    return TRUE;
}

static BOOL TERM_init_done /* = FALSE */;

BOOL TERM_Init(void)
{
    /* if we're not attached to a tty, don't fire the curses support */
    if (!isatty(0) || !isatty(1)) return FALSE;
    if (!getenv("TERM")) return FALSE;
    if (!TERM_bind_libcurses()) return FALSE;
    if (setupterm(NULL, 1 /* really ?? */, NULL) == -1) return FALSE;
    TERM_init_done = TRUE;
    TERM_BuildKeyDB();
    /* set application key mode */
    putp(tigetstr((char *)"smkx"));
    return TRUE;
}

BOOL TERM_Exit(void)
{
    if (TERM_init_done)
    {
        /* put back the cursor key mode */
        putp(tigetstr((char *)"rmkx"));
    }
    return TRUE;
}

/* -1 not found, 0 cannot decide, > 0 found */
int TERM_FillInputRecord(const char* in, size_t len, INPUT_RECORD* ir)
{
    int low = 0, high = TERM_dbkey_index - 1, mid, res;
    struct dbkey_descr* found;

    while (low <= high)
    {
        mid = low + (high - low) / 2;
        res = memcmp(in, TERM_dbkey[mid].string, len);
        if (!res)
        {
            if (len < TERM_dbkey[mid].string_len) return 0;
            found = &TERM_dbkey[mid].descr;
            switch (found->kind)
            {
            case dbk_simple:
                return TERM_FillSimpleChar(found->p1, ir);
            case dbk_complex:
                init_complex_char(&ir[0], 1, found->p1, found->p2, ENHANCED_KEY | found->p3);
                init_complex_char(&ir[1], 0, found->p1, found->p2, ENHANCED_KEY | found->p3);
                return 2;
            }
            return -1;
        }
        else if (res < 0) high = mid - 1;
        else low = mid + 1;
    }
    return -1;
}

#else
BOOL     TERM_Init(void) {return FALSE;}
BOOL     TERM_Exit(void) {return FALSE;}
int      TERM_FillInputRecord(const char* in, size_t len, INPUT_RECORD* ir) {return -1;}
#endif
