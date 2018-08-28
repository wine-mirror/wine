/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <assert.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "htmlstyle.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const WCHAR backgroundW[] =
    {'b','a','c','k','g','r','o','u','n','d',0};
static const WCHAR background_attachmentW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','a','t','t','a','c','h','m','e','n','t',0};
static const WCHAR background_colorW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','c','o','l','o','r',0};
static const WCHAR background_imageW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','i','m','a','g','e',0};
static const WCHAR background_positionW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','p','o','s','i','t','i','o','n',0};
static const WCHAR background_position_xW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','p','o','s','i','t','i','o','n','-','x',0};
static const WCHAR background_position_yW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','p','o','s','i','t','i','o','n','-','y',0};
static const WCHAR background_repeatW[] =
    {'b','a','c','k','g','r','o','u','n','d','-','r','e','p','e','a','t',0};
static const WCHAR borderW[] =
    {'b','o','r','d','e','r',0};
static const WCHAR border_bottomW[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m',0};
static const WCHAR border_bottom_colorW[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','c','o','l','o','r',0};
static const WCHAR border_bottom_styleW[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','s','t','y','l','e',0};
static const WCHAR border_bottom_widthW[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','w','i','d','t','h',0};
static const WCHAR border_colorW[] =
    {'b','o','r','d','e','r','-','c','o','l','o','r',0};
static const WCHAR border_leftW[] =
    {'b','o','r','d','e','r','-','l','e','f','t',0};
static const WCHAR border_left_colorW[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','c','o','l','o','r',0};
static const WCHAR border_left_styleW[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','s','t','y','l','e',0};
static const WCHAR border_left_widthW[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','w','i','d','t','h',0};
static const WCHAR border_rightW[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t',0};
static const WCHAR border_right_colorW[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','c','o','l','o','r',0};
static const WCHAR border_right_styleW[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','s','t','y','l','e',0};
static const WCHAR border_right_widthW[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','w','i','d','t','h',0};
static const WCHAR border_topW[] =
    {'b','o','r','d','e','r','-','t','o','p',0};
static const WCHAR border_top_colorW[] =
    {'b','o','r','d','e','r','-','t','o','p','-','c','o','l','o','r',0};
static const WCHAR border_styleW[] =
    {'b','o','r','d','e','r','-','s','t','y','l','e',0};
static const WCHAR border_top_styleW[] =
    {'b','o','r','d','e','r','-','t','o','p','-','s','t','y','l','e',0};
static const WCHAR border_top_widthW[] =
    {'b','o','r','d','e','r','-','t','o','p','-','w','i','d','t','h',0};
static const WCHAR border_widthW[] =
    {'b','o','r','d','e','r','-','w','i','d','t','h',0};
static const WCHAR bottomW[] =
    {'b','o','t','t','o','m',0};
/* FIXME: Use unprefixed version (requires Gecko changes). */
static const WCHAR box_sizingW[] =
    {'-','m','o','z','-','b','o','x','-','s','i','z','i','n','g',0};
static const WCHAR clearW[] =
    {'c','l','e','a','r',0};
static const WCHAR clipW[] =
    {'c','l','i','p',0};
static const WCHAR colorW[] =
    {'c','o','l','o','r',0};
static const WCHAR cursorW[] =
    {'c','u','r','s','o','r',0};
static const WCHAR directionW[] =
    {'d','i','r','e','c','t','i','o','n',0};
static const WCHAR displayW[] =
    {'d','i','s','p','l','a','y',0};
static const WCHAR filterW[] =
    {'f','i','l','e','t','e','r',0};
static const WCHAR floatW[] =
    {'f','l','o','a','t',0};
static const WCHAR font_familyW[] =
    {'f','o','n','t','-','f','a','m','i','l','y',0};
static const WCHAR font_sizeW[] =
    {'f','o','n','t','-','s','i','z','e',0};
static const WCHAR font_styleW[] =
    {'f','o','n','t','-','s','t','y','l','e',0};
static const WCHAR font_variantW[] =
    {'f','o','n','t','-','v','a','r','i','a','n','t',0};
static const WCHAR font_weightW[] =
    {'f','o','n','t','-','w','e','i','g','h','t',0};
static const WCHAR heightW[] =
    {'h','e','i','g','h','t',0};
static const WCHAR leftW[] =
    {'l','e','f','t',0};
static const WCHAR letter_spacingW[] =
    {'l','e','t','t','e','r','-','s','p','a','c','i','n','g',0};
static const WCHAR line_heightW[] =
    {'l','i','n','e','-','h','e','i','g','h','t',0};
static const WCHAR list_styleW[] =
    {'l','i','s','t','-','s','t','y','l','e',0};
static const WCHAR list_style_typeW[] =
    {'l','i','s','t','-','s','t','y','l','e','-','t','y','p','e',0};
static const WCHAR list_style_positionW[] =
    {'l','i','s','t','-','s','t','y','l','e','-','p','o','s','i','t','i','o','n',0};
static const WCHAR marginW[] =
    {'m','a','r','g','i','n',0};
static const WCHAR margin_bottomW[] =
    {'m','a','r','g','i','n','-','b','o','t','t','o','m',0};
static const WCHAR margin_leftW[] =
    {'m','a','r','g','i','n','-','l','e','f','t',0};
static const WCHAR margin_rightW[] =
    {'m','a','r','g','i','n','-','r','i','g','h','t',0};
static const WCHAR margin_topW[] =
    {'m','a','r','g','i','n','-','t','o','p',0};
static const WCHAR max_heightW[] =
    {'m','a','x','-','h','e','i','g','h','t',0};
static const WCHAR max_widthW[] =
    {'m','a','x','-','w','i','d','t','h',0};
static const WCHAR min_heightW[] =
    {'m','i','n','-','h','e','i','g','h','t',0};
static const WCHAR min_widthW[] =
    {'m','i','n','-','w','i','d','t','h',0};
static const WCHAR outlineW[] =
    {'o','u','t','l','i','n','e',0};
static const WCHAR overflowW[] =
    {'o','v','e','r','f','l','o','w',0};
static const WCHAR overflow_xW[] =
    {'o','v','e','r','f','l','o','w','-','x',0};
static const WCHAR overflow_yW[] =
    {'o','v','e','r','f','l','o','w','-','y',0};
static const WCHAR paddingW[] =
    {'p','a','d','d','i','n','g',0};
static const WCHAR padding_bottomW[] =
    {'p','a','d','d','i','n','g','-','b','o','t','t','o','m',0};
static const WCHAR padding_leftW[] =
    {'p','a','d','d','i','n','g','-','l','e','f','t',0};
static const WCHAR padding_rightW[] =
    {'p','a','d','d','i','n','g','-','r','i','g','h','t',0};
static const WCHAR padding_topW[] =
    {'p','a','d','d','i','n','g','-','t','o','p',0};
static const WCHAR page_break_afterW[] =
    {'p','a','g','e','-','b','r','e','a','k','-','a','f','t','e','r',0};
static const WCHAR page_break_beforeW[] =
    {'p','a','g','e','-','b','r','e','a','k','-','b','e','f','o','r','e',0};
static const WCHAR positionW[] =
    {'p','o','s','i','t','i','o','n',0};
static const WCHAR rightW[] =
    {'r','i','g','h','t',0};
static const WCHAR table_layoutW[] =
    {'t','a','b','l','e','-','l','a','y','o','u','t',0};
static const WCHAR text_alignW[] =
    {'t','e','x','t','-','a','l','i','g','n',0};
static const WCHAR text_decorationW[] =
    {'t','e','x','t','-','d','e','c','o','r','a','t','i','o','n',0};
static const WCHAR text_indentW[] =
    {'t','e','x','t','-','i','n','d','e','n','t',0};
static const WCHAR text_transformW[] =
    {'t','e','x','t','-','t','r','a','n','s','f','o','r','m',0};
static const WCHAR topW[] =
    {'t','o','p',0};
static const WCHAR vertical_alignW[] =
    {'v','e','r','t','i','c','a','l','-','a','l','i','g','n',0};
static const WCHAR visibilityW[] =
    {'v','i','s','i','b','i','l','i','t','y',0};
static const WCHAR white_spaceW[] =
    {'w','h','i','t','e','-','s','p','a','c','e',0};
static const WCHAR widthW[] =
    {'w','i','d','t','h',0};
static const WCHAR word_spacingW[] =
    {'w','o','r','d','-','s','p','a','c','i','n','g',0};
static const WCHAR word_wrapW[] =
    {'w','o','r','d','-','w','r','a','p',0};
static const WCHAR z_indexW[] =
    {'z','-','i','n','d','e','x',0};

static const WCHAR blinkW[] = {'b','l','i','n','k',0};
static const WCHAR boldW[] = {'b','o','l','d',0};
static const WCHAR bolderW[] = {'b','o','l','d','e','r',0};
static const WCHAR capsW[]  = {'s','m','a','l','l','-','c','a','p','s',0};
static const WCHAR italicW[]  = {'i','t','a','l','i','c',0};
static const WCHAR lighterW[]  = {'l','i','g','h','t','e','r',0};
static const WCHAR line_throughW[] = {'l','i','n','e','-','t','h','r','o','u','g','h',0};
static const WCHAR no_repeatW[] = {'n','o','-','r','e','p','e','a','t',0};
static const WCHAR noneW[] = {'n','o','n','e',0};
static const WCHAR normalW[] = {'n','o','r','m','a','l',0};
static const WCHAR obliqueW[]  = {'o','b','l','i','q','u','e',0};
static const WCHAR overlineW[] = {'o','v','e','r','l','i','n','e',0};
static const WCHAR repeatW[]   = {'r','e','p','e','a','t',0};
static const WCHAR repeat_xW[]  = {'r','e','p','e','a','t','-','x',0};
static const WCHAR repeat_yW[]  = {'r','e','p','e','a','t','-','y',0};
static const WCHAR underlineW[] = {'u','n','d','e','r','l','i','n','e',0};

static const WCHAR style100W[] = {'1','0','0',0};
static const WCHAR style200W[] = {'2','0','0',0};
static const WCHAR style300W[] = {'3','0','0',0};
static const WCHAR style400W[] = {'4','0','0',0};
static const WCHAR style500W[] = {'5','0','0',0};
static const WCHAR style600W[] = {'6','0','0',0};
static const WCHAR style700W[] = {'7','0','0',0};
static const WCHAR style800W[] = {'8','0','0',0};
static const WCHAR style900W[] = {'9','0','0',0};

static const WCHAR *font_style_values[] = {
    italicW,
    normalW,
    obliqueW,
    NULL
};

static const WCHAR *font_variant_values[] = {
    capsW,
    normalW,
    NULL
};

static const WCHAR *font_weight_values[] = {
    style100W,
    style200W,
    style300W,
    style400W,
    style500W,
    style600W,
    style700W,
    style800W,
    style900W,
    boldW,
    bolderW,
    lighterW,
    normalW,
    NULL
};

static const WCHAR *background_repeat_values[] = {
    no_repeatW,
    repeatW,
    repeat_xW,
    repeat_yW,
    NULL
};

static const WCHAR *text_decoration_values[] = {
    blinkW,
    line_throughW,
    noneW,
    overlineW,
    underlineW,
    NULL
};

#define ATTR_FIX_PX         0x0001
#define ATTR_FIX_URL        0x0002
#define ATTR_STR_TO_INT     0x0004
#define ATTR_HEX_INT        0x0008
#define ATTR_REMOVE_COMMA   0x0010
#define ATTR_NO_NULL        0x0020

static const WCHAR pxW[] = {'p','x',0};

typedef struct {
    const WCHAR *name;
    DISPID dispid;
    unsigned flags;
    const WCHAR **allowed_values;
} style_tbl_entry_t;

static const style_tbl_entry_t style_tbl[] = {
    {backgroundW,             DISPID_IHTMLSTYLE_BACKGROUND},
    {background_attachmentW,  DISPID_IHTMLSTYLE_BACKGROUNDATTACHMENT},
    {background_colorW,       DISPID_IHTMLSTYLE_BACKGROUNDCOLOR,       ATTR_HEX_INT},
    {background_imageW,       DISPID_IHTMLSTYLE_BACKGROUNDIMAGE,       ATTR_FIX_URL},
    {background_positionW,    DISPID_IHTMLSTYLE_BACKGROUNDPOSITION},
    {background_position_xW,  DISPID_IHTMLSTYLE_BACKGROUNDPOSITIONX,   ATTR_FIX_PX},
    {background_position_yW,  DISPID_IHTMLSTYLE_BACKGROUNDPOSITIONY,   ATTR_FIX_PX},
    {background_repeatW,      DISPID_IHTMLSTYLE_BACKGROUNDREPEAT,      0, background_repeat_values},
    {borderW,                 DISPID_IHTMLSTYLE_BORDER},
    {border_bottomW,          DISPID_IHTMLSTYLE_BORDERBOTTOM,          ATTR_FIX_PX},
    {border_bottom_colorW,    DISPID_IHTMLSTYLE_BORDERBOTTOMCOLOR,     ATTR_HEX_INT},
    {border_bottom_styleW,    DISPID_IHTMLSTYLE_BORDERBOTTOMSTYLE},
    {border_bottom_widthW,    DISPID_IHTMLSTYLE_BORDERBOTTOMWIDTH,     ATTR_FIX_PX},
    {border_colorW,           DISPID_IHTMLSTYLE_BORDERCOLOR},
    {border_leftW,            DISPID_IHTMLSTYLE_BORDERLEFT,            ATTR_FIX_PX},
    {border_left_colorW,      DISPID_IHTMLSTYLE_BORDERLEFTCOLOR,       ATTR_HEX_INT},
    {border_left_styleW,      DISPID_IHTMLSTYLE_BORDERLEFTSTYLE},
    {border_left_widthW,      DISPID_IHTMLSTYLE_BORDERLEFTWIDTH,       ATTR_FIX_PX},
    {border_rightW,           DISPID_IHTMLSTYLE_BORDERRIGHT,           ATTR_FIX_PX},
    {border_right_colorW,     DISPID_IHTMLSTYLE_BORDERRIGHTCOLOR,      ATTR_HEX_INT},
    {border_right_styleW,     DISPID_IHTMLSTYLE_BORDERRIGHTSTYLE},
    {border_right_widthW,     DISPID_IHTMLSTYLE_BORDERRIGHTWIDTH,      ATTR_FIX_PX},
    {border_styleW,           DISPID_IHTMLSTYLE_BORDERSTYLE},
    {border_topW,             DISPID_IHTMLSTYLE_BORDERTOP,             ATTR_FIX_PX},
    {border_top_colorW,       DISPID_IHTMLSTYLE_BORDERTOPCOLOR,        ATTR_HEX_INT},
    {border_top_styleW,       DISPID_IHTMLSTYLE_BORDERTOPSTYLE},
    {border_top_widthW,       DISPID_IHTMLSTYLE_BORDERTOPWIDTH},
    {border_widthW,           DISPID_IHTMLSTYLE_BORDERWIDTH},
    {bottomW,                 DISPID_IHTMLSTYLE2_BOTTOM,               ATTR_FIX_PX},
    {box_sizingW,             DISPID_IHTMLSTYLE6_BOXSIZING},
    {clearW,                  DISPID_IHTMLSTYLE_CLEAR},
    {clipW,                   DISPID_IHTMLSTYLE_CLIP,                  ATTR_REMOVE_COMMA},
    {colorW,                  DISPID_IHTMLSTYLE_COLOR,                 ATTR_HEX_INT},
    {cursorW,                 DISPID_IHTMLSTYLE_CURSOR},
    {directionW,              DISPID_IHTMLSTYLE2_DIRECTION},
    {displayW,                DISPID_IHTMLSTYLE_DISPLAY},
    {filterW,                 DISPID_IHTMLSTYLE_FILTER},
    {floatW,                  DISPID_IHTMLSTYLE_STYLEFLOAT},
    {font_familyW,            DISPID_IHTMLSTYLE_FONTFAMILY},
    {font_sizeW,              DISPID_IHTMLSTYLE_FONTSIZE,              ATTR_FIX_PX},
    {font_styleW,             DISPID_IHTMLSTYLE_FONTSTYLE,             0, font_style_values},
    {font_variantW,           DISPID_IHTMLSTYLE_FONTVARIANT,           0, font_variant_values},
    {font_weightW,            DISPID_IHTMLSTYLE_FONTWEIGHT,            ATTR_STR_TO_INT, font_weight_values},
    {heightW,                 DISPID_IHTMLSTYLE_HEIGHT,                ATTR_FIX_PX},
    {leftW,                   DISPID_IHTMLSTYLE_LEFT},
    {letter_spacingW,         DISPID_IHTMLSTYLE_LETTERSPACING},
    {line_heightW,            DISPID_IHTMLSTYLE_LINEHEIGHT},
    {list_styleW,             DISPID_IHTMLSTYLE_LISTSTYLE},
    {list_style_positionW,    DISPID_IHTMLSTYLE_LISTSTYLEPOSITION},
    {list_style_typeW,        DISPID_IHTMLSTYLE_LISTSTYLETYPE},
    {marginW,                 DISPID_IHTMLSTYLE_MARGIN},
    {margin_bottomW,          DISPID_IHTMLSTYLE_MARGINBOTTOM,          ATTR_FIX_PX},
    {margin_leftW,            DISPID_IHTMLSTYLE_MARGINLEFT,            ATTR_FIX_PX},
    {margin_rightW,           DISPID_IHTMLSTYLE_MARGINRIGHT,           ATTR_FIX_PX},
    {margin_topW,             DISPID_IHTMLSTYLE_MARGINTOP,             ATTR_FIX_PX},
    {max_heightW,             DISPID_IHTMLSTYLE5_MAXHEIGHT,            ATTR_FIX_PX},
    {max_widthW,              DISPID_IHTMLSTYLE5_MAXWIDTH,             ATTR_FIX_PX},
    {min_heightW,             DISPID_IHTMLSTYLE4_MINHEIGHT},
    {min_widthW,              DISPID_IHTMLSTYLE5_MINWIDTH,             ATTR_FIX_PX},
    {outlineW,                DISPID_IHTMLSTYLE6_OUTLINE,              ATTR_NO_NULL},
    {overflowW,               DISPID_IHTMLSTYLE_OVERFLOW},
    {overflow_xW,             DISPID_IHTMLSTYLE2_OVERFLOWX},
    {overflow_yW,             DISPID_IHTMLSTYLE2_OVERFLOWY},
    {paddingW,                DISPID_IHTMLSTYLE_PADDING},
    {padding_bottomW,         DISPID_IHTMLSTYLE_PADDINGBOTTOM,         ATTR_FIX_PX},
    {padding_leftW,           DISPID_IHTMLSTYLE_PADDINGLEFT,           ATTR_FIX_PX},
    {padding_rightW,          DISPID_IHTMLSTYLE_PADDINGRIGHT,          ATTR_FIX_PX},
    {padding_topW,            DISPID_IHTMLSTYLE_PADDINGTOP,            ATTR_FIX_PX},
    {page_break_afterW,       DISPID_IHTMLSTYLE_PAGEBREAKAFTER},
    {page_break_beforeW,      DISPID_IHTMLSTYLE_PAGEBREAKBEFORE},
    {positionW,               DISPID_IHTMLSTYLE2_POSITION},
    {rightW,                  DISPID_IHTMLSTYLE2_RIGHT},
    {table_layoutW,           DISPID_IHTMLSTYLE2_TABLELAYOUT},
    {text_alignW,             DISPID_IHTMLSTYLE_TEXTALIGN},
    {text_decorationW,        DISPID_IHTMLSTYLE_TEXTDECORATION,        0, text_decoration_values},
    {text_indentW,            DISPID_IHTMLSTYLE_TEXTINDENT,            ATTR_FIX_PX},
    {text_transformW,         DISPID_IHTMLSTYLE_TEXTTRANSFORM},
    {topW,                    DISPID_IHTMLSTYLE_TOP},
    {vertical_alignW,         DISPID_IHTMLSTYLE_VERTICALALIGN,         ATTR_FIX_PX},
    {visibilityW,             DISPID_IHTMLSTYLE_VISIBILITY},
    {white_spaceW,            DISPID_IHTMLSTYLE_WHITESPACE},
    {widthW,                  DISPID_IHTMLSTYLE_WIDTH,                 ATTR_FIX_PX},
    {word_spacingW,           DISPID_IHTMLSTYLE_WORDSPACING},
    {word_wrapW,              DISPID_IHTMLSTYLE3_WORDWRAP},
    {z_indexW,                DISPID_IHTMLSTYLE_ZINDEX,                ATTR_STR_TO_INT}
};

C_ASSERT(ARRAY_SIZE(style_tbl) == STYLEID_MAX_VALUE);

static const WCHAR px_formatW[] = {'%','d','p','x',0};
static const WCHAR emptyW[] = {0};

static const style_tbl_entry_t *lookup_style_tbl(const WCHAR *name)
{
    int c, i, min = 0, max = ARRAY_SIZE(style_tbl)-1;

    while(min <= max) {
        i = (min+max)/2;

        c = strcmpW(style_tbl[i].name, name);
        if(!c)
            return style_tbl+i;

        if(c > 0)
            max = i-1;
        else
            min = i+1;
    }

    return NULL;
}

static inline compat_mode_t get_style_compat_mode(HTMLStyle *style)
{
    return style->elem && style->elem->node.doc ? style->elem->node.doc->document_mode : COMPAT_MODE_QUIRKS;
}

static LPWSTR fix_px_value(LPCWSTR val)
{
    LPCWSTR ptr = val;

    while(*ptr) {
        while(*ptr && isspaceW(*ptr))
            ptr++;
        if(!*ptr)
            break;

        while(*ptr && isdigitW(*ptr))
            ptr++;

        if(!*ptr || isspaceW(*ptr)) {
            LPWSTR ret, p;
            int len = strlenW(val)+1;

            ret = heap_alloc((len+2)*sizeof(WCHAR));
            memcpy(ret, val, (ptr-val)*sizeof(WCHAR));
            p = ret + (ptr-val);
            *p++ = 'p';
            *p++ = 'x';
            strcpyW(p, ptr);

            TRACE("fixed %s -> %s\n", debugstr_w(val), debugstr_w(ret));

            return ret;
        }

        while(*ptr && !isspaceW(*ptr))
            ptr++;
    }

    return NULL;
}

static LPWSTR fix_url_value(LPCWSTR val)
{
    WCHAR *ret, *ptr;

    static const WCHAR urlW[] = {'u','r','l','('};

    if(strncmpW(val, urlW, ARRAY_SIZE(urlW)) || !strchrW(val, '\\'))
        return NULL;

    ret = heap_strdupW(val);

    for(ptr = ret; *ptr; ptr++) {
        if(*ptr == '\\')
            *ptr = '/';
    }

    return ret;
}

static HRESULT set_nsstyle_property(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, const WCHAR *value)
{
    nsAString str_name, str_value, str_empty;
    LPWSTR val = NULL;
    nsresult nsres;
    HRESULT hres = S_OK;

    if(value && *value) {
        unsigned flags = style_tbl[sid].flags;
        if(flags & ATTR_FIX_PX)
            val = fix_px_value(value);
        else if(flags & ATTR_FIX_URL)
            val = fix_url_value(value);

        if(style_tbl[sid].allowed_values) {
            const WCHAR **iter;
            for(iter = style_tbl[sid].allowed_values; *iter; iter++) {
                if(!strcmpiW(*iter, value))
                    break;
            }
            if(!*iter) {
                hres = E_INVALIDARG;
                value = emptyW;
            }
        }
    }

    nsAString_InitDepend(&str_name, style_tbl[sid].name);
    nsAString_InitDepend(&str_value, val ? val : value);
    nsAString_InitDepend(&str_empty, emptyW);

    nsres = nsIDOMCSSStyleDeclaration_SetProperty(nsstyle, &str_name, &str_value, &str_empty);
    if(NS_FAILED(nsres))
        ERR("SetProperty failed: %08x\n", nsres);

    nsAString_Finish(&str_name);
    nsAString_Finish(&str_value);
    nsAString_Finish(&str_empty);
    heap_free(val);

    return hres;
}

static HRESULT var_to_styleval(const VARIANT *v, styleid_t sid, WCHAR *buf, const WCHAR **ret)
{
    switch(V_VT(v)) {
    case VT_NULL:
        *ret = emptyW;
        return S_OK;

    case VT_BSTR:
        *ret = V_BSTR(v);
        return S_OK;

    case VT_BSTR|VT_BYREF:
        *ret = *V_BSTRREF(v);
        return S_OK;

    case VT_I4: {
        unsigned flags = style_tbl[sid].flags;
        static const WCHAR formatW[] = {'%','d',0};
        static const WCHAR hex_formatW[] = {'#','%','0','6','x',0};

        if(flags & ATTR_HEX_INT)
            wsprintfW(buf, hex_formatW, V_I4(v));
        else if(flags & ATTR_FIX_PX)
            wsprintfW(buf, px_formatW, V_I4(v));
        else
            wsprintfW(buf, formatW, V_I4(v));

        *ret = buf;
        return S_OK;
    }
    default:
        FIXME("not implemented for %s\n", debugstr_variant(v));
        return E_NOTIMPL;

    }
}

static HRESULT set_style_property_var(HTMLStyle *style, styleid_t sid, VARIANT *value)
{
    const WCHAR *val;
    WCHAR buf[14];
    HRESULT hres;

    hres = var_to_styleval(value, sid, buf, &val);
    if(FAILED(hres))
        return hres;

    return set_nsstyle_property(style->nsstyle, sid, val);
}

static inline HRESULT set_style_property(HTMLStyle *style, styleid_t sid, const WCHAR *value)
{
    return set_nsstyle_property(style->nsstyle, sid, value);
}

static HRESULT get_nsstyle_attr_nsval(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, nsAString *value)
{
    nsAString str_name;
    nsresult nsres;

    nsAString_InitDepend(&str_name, style_tbl[sid].name);
    nsres = nsIDOMCSSStyleDeclaration_GetPropertyValue(nsstyle, &str_name, value);
    nsAString_Finish(&str_name);
    if(NS_FAILED(nsres)) {
        ERR("SetProperty failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT nsstyle_to_bstr(const WCHAR *val, DWORD flags, BSTR *p)
{
    BSTR ret;
    DWORD len;

    if(!*val) {
        *p = (flags & ATTR_NO_NULL) ? SysAllocStringLen(NULL, 0) : NULL;
        return S_OK;
    }

    ret = SysAllocString(val);
    if(!ret)
        return E_OUTOFMEMORY;

    len = SysStringLen(ret);

    if(flags & ATTR_REMOVE_COMMA) {
        DWORD new_len = len;
        WCHAR *ptr, *ptr2;

        for(ptr = ret; (ptr = strchrW(ptr, ',')); ptr++)
            new_len--;

        if(new_len != len) {
            BSTR new_ret;

            new_ret = SysAllocStringLen(NULL, new_len);
            if(!new_ret) {
                SysFreeString(ret);
                return E_OUTOFMEMORY;
            }

            for(ptr2 = new_ret, ptr = ret; *ptr; ptr++) {
                if(*ptr != ',')
                    *ptr2++ = *ptr;
            }

            SysFreeString(ret);
            ret = new_ret;
        }
    }

    *p = ret;
    return S_OK;
}

HRESULT get_nsstyle_property(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, BSTR *p)
{
    nsAString str_value;
    const PRUnichar *value;
    HRESULT hres;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    hres = nsstyle_to_bstr(value, style_tbl[sid].flags, p);
    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_w(*p));
    return hres;
}

HRESULT get_nsstyle_property_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *p)
{
    nsAString str_value;
    const PRUnichar *value;
    BOOL set = FALSE;
    unsigned flags;
    HRESULT hres = S_OK;

    flags = style_tbl[sid].flags;
    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);

    if(flags & ATTR_STR_TO_INT) {
        const PRUnichar *ptr = value;
        BOOL neg = FALSE;
        INT i = 0;

        if(*ptr == '-') {
            neg = TRUE;
            ptr++;
        }

        while(isdigitW(*ptr))
            i = i*10 + (*ptr++ - '0');

        if(!*ptr) {
            V_VT(p) = VT_I4;
            V_I4(p) = neg ? -i : i;
            set = TRUE;
        }
    }

    if(!set) {
        BSTR str;

        hres = nsstyle_to_bstr(value, flags, &str);
        if(SUCCEEDED(hres)) {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = str;
        }
    }

    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_variant(p));
    return S_OK;
}

static inline HRESULT get_style_property(HTMLStyle *This, styleid_t sid, BSTR *p)
{
    return get_nsstyle_property(This->nsstyle, sid, p);
}

static inline HRESULT get_style_property_var(HTMLStyle *This, styleid_t sid, VARIANT *v)
{
    return get_nsstyle_property_var(This->nsstyle, sid, v);
}

static HRESULT check_style_attr_value(HTMLStyle *This, styleid_t sid, LPCWSTR exval, VARIANT_BOOL *p)
{
    nsAString str_value;
    const PRUnichar *value;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(This->nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    *p = variant_bool(!strcmpW(value, exval));
    nsAString_Finish(&str_value);

    TRACE("%s -> %x\n", debugstr_w(style_tbl[sid].name), *p);
    return S_OK;
}

static inline HRESULT set_style_pos(HTMLStyle *This, styleid_t sid, float value)
{
    WCHAR szValue[25];
    WCHAR szFormat[] = {'%','.','0','f','p','x',0};

    value = floor(value);

    sprintfW(szValue, szFormat, value);

    return set_style_property(This, sid, szValue);
}

static HRESULT set_style_pxattr(HTMLStyle *style, styleid_t sid, LONG value)
{
    WCHAR value_str[16];

    sprintfW(value_str, px_formatW, value);

    return set_style_property(style, sid, value_str);
}

static HRESULT get_nsstyle_pos(HTMLStyle *This, styleid_t sid, float *p)
{
    nsAString str_value;
    HRESULT hres;

    TRACE("%p %d %p\n", This, sid, p);

    *p = 0.0f;

    nsAString_Init(&str_value, NULL);

    hres = get_nsstyle_attr_nsval(This->nsstyle, sid, &str_value);
    if(hres == S_OK)
    {
        WCHAR *ptr;
        const PRUnichar *value;

        nsAString_GetData(&str_value, &value);
        if(value)
        {
            *p = strtolW(value, &ptr, 10);

            if(*ptr && strcmpW(ptr, pxW))
            {
                nsAString_Finish(&str_value);
                FIXME("only px values are currently supported\n");
                hres = E_FAIL;
            }
        }
    }

    TRACE("ret %f\n", *p);

    nsAString_Finish(&str_value);
    return hres;
}

static HRESULT get_nsstyle_pixel_val(HTMLStyle *This, styleid_t sid, LONG *p)
{
    nsAString str_value;
    HRESULT hres;

    if(!p)
        return E_POINTER;

    nsAString_Init(&str_value, NULL);

    hres = get_nsstyle_attr_nsval(This->nsstyle, sid, &str_value);
    if(hres == S_OK) {
        WCHAR *ptr = NULL;
        const PRUnichar *value;

        nsAString_GetData(&str_value, &value);
        if(value) {
            *p = strtolW(value, &ptr, 10);

            if(*ptr == '.') {
                /* Skip all digits. We have tests showing that we should not round the value. */
                while(isdigitW(*++ptr));
            }
        }

        if(!ptr || (*ptr && strcmpW(ptr, pxW)))
            *p = 0;
    }

    nsAString_Finish(&str_value);
    return hres;
}

static BOOL is_valid_border_style(BSTR v)
{
    static const WCHAR styleDotted[] = {'d','o','t','t','e','d',0};
    static const WCHAR styleDashed[] = {'d','a','s','h','e','d',0};
    static const WCHAR styleSolid[]  = {'s','o','l','i','d',0};
    static const WCHAR styleDouble[] = {'d','o','u','b','l','e',0};
    static const WCHAR styleGroove[] = {'g','r','o','o','v','e',0};
    static const WCHAR styleRidge[]  = {'r','i','d','g','e',0};
    static const WCHAR styleInset[]  = {'i','n','s','e','t',0};
    static const WCHAR styleOutset[] = {'o','u','t','s','e','t',0};

    TRACE("%s\n", debugstr_w(v));

    if(!v || strcmpiW(v, noneW)   == 0 || strcmpiW(v, styleDotted) == 0 ||
             strcmpiW(v, styleDashed) == 0 || strcmpiW(v, styleSolid)  == 0 ||
             strcmpiW(v, styleDouble) == 0 || strcmpiW(v, styleGroove) == 0 ||
             strcmpiW(v, styleRidge)  == 0 || strcmpiW(v, styleInset)  == 0 ||
             strcmpiW(v, styleOutset) == 0 )
    {
        return TRUE;
    }

    return FALSE;
}

static inline HTMLStyle *impl_from_IHTMLStyle(IHTMLStyle *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle_QueryInterface(IHTMLStyle *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStyle_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle, riid)) {
        *ppv = &This->IHTMLStyle_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle2, riid)) {
        *ppv = &This->IHTMLStyle2_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle3, riid)) {
        *ppv = &This->IHTMLStyle3_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle4, riid)) {
        *ppv = &This->IHTMLStyle4_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle5, riid)) {
        *ppv = &This->IHTMLStyle5_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle6, riid)) {
        *ppv = &This->IHTMLStyle6_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("unsupported iface %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyle_AddRef(IHTMLStyle *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyle_Release(IHTMLStyle *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->elem);
        if(This->nsstyle)
            nsIDOMCSSStyleDeclaration_Release(This->nsstyle);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyle_GetTypeInfoCount(IHTMLStyle *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle_GetTypeInfo(IHTMLStyle *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle_GetIDsOfNames(IHTMLStyle *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle_Invoke(IHTMLStyle *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle_put_fontFamily(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_FONT_FAMILY, v);
}

static HRESULT WINAPI HTMLStyle_get_fontFamily(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLStyle_put_fontStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_FONT_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_fontStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_fontVariant(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_FONT_VARIANT, v);
}

static HRESULT WINAPI HTMLStyle_get_fontVariant(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_property(This, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontWeight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_FONT_WEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_fontWeight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_FONT_WEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontSize(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_FONT_SIZE, &v);
}

static HRESULT WINAPI HTMLStyle_get_fontSize(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_FONT_SIZE, p);
}

static HRESULT WINAPI HTMLStyle_put_font(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_font(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_color(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_color(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_background(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BACKGROUND, v);
}

static HRESULT WINAPI HTMLStyle_get_background(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BACKGROUND, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BACKGROUND_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BACKGROUND_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BACKGROUND_IMAGE, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundRepeat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BACKGROUND_REPEAT , v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundRepeat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BACKGROUND_REPEAT, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundAttachment(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BACKGROUND_ATTACHMENT, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundAttachment(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BACKGROUND_ATTACHMENT, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BACKGROUND_POSITION, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundPosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BACKGROUND_POSITION, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionX(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR buf[14], *pos_val;
    nsAString pos_str;
    const WCHAR *val;
    DWORD val_len;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var_to_styleval(&v, STYLEID_BACKGROUND_POSITION_X, buf, &val);
    if(FAILED(hres))
        return hres;

    val_len = val ? strlenW(val) : 0;

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *posy;
        DWORD posy_len;

        nsAString_GetData(&pos_str, &pos);
        posy = strchrW(pos, ' ');
        if(!posy) {
            static const WCHAR zero_pxW[] = {' ','0','p','x',0};

            TRACE("no space in %s\n", debugstr_w(pos));
            posy = zero_pxW;
        }

        posy_len = strlenW(posy);
        pos_val = heap_alloc((val_len+posy_len+1)*sizeof(WCHAR));
        if(pos_val) {
            if(val_len)
                memcpy(pos_val, val, val_len*sizeof(WCHAR));
            if(posy_len)
                memcpy(pos_val+val_len, posy, posy_len*sizeof(WCHAR));
            pos_val[val_len+posy_len] = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("setting position to %s\n", debugstr_w(pos_val));
    hres = set_style_property(This, STYLEID_BACKGROUND_POSITION, pos_val);
    heap_free(pos_val);
    return hres;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionX(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString pos_str;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *space;

        nsAString_GetData(&pos_str, &pos);
        space = strchrW(pos, ' ');
        if(!space) {
            WARN("no space in %s\n", debugstr_w(pos));
            space = pos + strlenW(pos);
        }

        if(space != pos) {
            ret = SysAllocStringLen(pos, space-pos);
            if(!ret)
                hres = E_OUTOFMEMORY;
        }else {
            ret = NULL;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("returning %s\n", debugstr_w(ret));
    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionY(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR buf[14], *pos_val;
    nsAString pos_str;
    const WCHAR *val;
    DWORD val_len;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var_to_styleval(&v, STYLEID_BACKGROUND_POSITION_Y, buf, &val);
    if(FAILED(hres))
        return hres;

    val_len = val ? strlenW(val) : 0;

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *space;
        DWORD posx_len;

        nsAString_GetData(&pos_str, &pos);
        space = strchrW(pos, ' ');
        if(space) {
            space++;
        }else {
            static const WCHAR zero_pxW[] = {'0','p','x',' ',0};

            TRACE("no space in %s\n", debugstr_w(pos));
            pos = zero_pxW;
            space = pos + ARRAY_SIZE(zero_pxW)-1;
        }

        posx_len = space-pos;

        pos_val = heap_alloc((posx_len+val_len+1)*sizeof(WCHAR));
        if(pos_val) {
            memcpy(pos_val, pos, posx_len*sizeof(WCHAR));
            if(val_len)
                memcpy(pos_val+posx_len, val, val_len*sizeof(WCHAR));
            pos_val[posx_len+val_len] = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("setting position to %s\n", debugstr_w(pos_val));
    hres = set_style_property(This, STYLEID_BACKGROUND_POSITION, pos_val);
    heap_free(pos_val);
    return hres;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionY(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString pos_str;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *posy;

        nsAString_GetData(&pos_str, &pos);
        posy = strchrW(pos, ' ');
        if(posy) {
            ret = SysAllocString(posy+1);
            if(!ret)
                hres = E_OUTOFMEMORY;
        }else {
            ret = NULL;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("returning %s\n", debugstr_w(ret));
    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_wordSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_WORD_SPACING, &v);
}

static HRESULT WINAPI HTMLStyle_get_wordSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_WORD_SPACING, p);
}

static HRESULT WINAPI HTMLStyle_put_letterSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_LETTER_SPACING, &v);
}

static HRESULT WINAPI HTMLStyle_get_letterSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_LETTER_SPACING, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecoration(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_TEXT_DECORATION , v);
}

static HRESULT WINAPI HTMLStyle_get_textDecoration(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(This, STYLEID_TEXT_DECORATION, v ? noneW : emptyW);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, noneW, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(This, STYLEID_TEXT_DECORATION, v ? underlineW : emptyW);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, underlineW, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(This, STYLEID_TEXT_DECORATION, v ? overlineW : emptyW);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, overlineW, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(This, STYLEID_TEXT_DECORATION, v ? line_throughW : emptyW);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, line_throughW, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(This, STYLEID_TEXT_DECORATION, v ? blinkW : emptyW);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, blinkW, p);
}

static HRESULT WINAPI HTMLStyle_put_verticalAlign(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_VERTICAL_ALIGN, &v);
}

static HRESULT WINAPI HTMLStyle_get_verticalAlign(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_VERTICAL_ALIGN, p);
}

static HRESULT WINAPI HTMLStyle_put_textTransform(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_TEXT_TRANSFORM, v);
}

static HRESULT WINAPI HTMLStyle_get_textTransform(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_TEXT_TRANSFORM, p);
}

static HRESULT WINAPI HTMLStyle_put_textAlign(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_TEXT_ALIGN, v);
}

static HRESULT WINAPI HTMLStyle_get_textAlign(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLStyle_put_textIndent(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_TEXT_INDENT, &v);
}

static HRESULT WINAPI HTMLStyle_get_textIndent(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_TEXT_INDENT, p);
}

static HRESULT WINAPI HTMLStyle_put_lineHeight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_LINE_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_lineHeight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_LINE_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_marginTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MARGIN_TOP, &v);
}

static HRESULT WINAPI HTMLStyle_get_marginTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_MARGIN_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_marginRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MARGIN_RIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_marginRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MARGIN_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_marginBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MARGIN_BOTTOM, &v);
}

static HRESULT WINAPI HTMLStyle_get_marginBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_MARGIN_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_marginLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MARGIN_LEFT, &v);
}

static HRESULT WINAPI HTMLStyle_put_margin(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_MARGIN, v);
}

static HRESULT WINAPI HTMLStyle_get_margin(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_MARGIN, p);
}

static HRESULT WINAPI HTMLStyle_get_marginLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MARGIN_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_PADDING_TOP, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_PADDING_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_PADDING_RIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_PADDING_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_PADDING_BOTTOM, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_PADDING_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_PADDING_LEFT, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_PADDING_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_padding(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_PADDING, v);
}

static HRESULT WINAPI HTMLStyle_get_padding(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_PADDING, p);
}

static HRESULT WINAPI HTMLStyle_put_border(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BORDER, v);
}

static HRESULT WINAPI HTMLStyle_get_border(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BORDER, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTop(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_borderTop(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_RIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_borderRight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottom(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_BOTTOM, v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottom(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeft(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BORDER_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeft(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BORDER_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_borderColor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BORDER_COLOR, v);
}

static HRESULT WINAPI HTMLStyle_get_borderColor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BORDER_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_TOP_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderTopColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BORDER_TOP_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_RIGHT_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderRightColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BORDER_RIGHT_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_BOTTOM_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BORDER_BOTTOM_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_LEFT_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BORDER_LEFT_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderWidth(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_borderWidth(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_TOP_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderTopWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BORDER_TOP_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_RIGHT_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderRightWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BORDER_RIGHT_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_BOTTOM_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_BOTTOM_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BORDER_LEFT_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_LEFT_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR styleWindowInset[]  = {'w','i','n','d','o','w','-','i','n','s','e','t',0};
    HRESULT hres = S_OK;
    BSTR pstyle;
    int i=0;
    int last = 0;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    while(v[i] && hres == S_OK)
    {
        if(v[i] == (WCHAR)' ')
        {
            pstyle = SysAllocStringLen(&v[last], (i-last));
            if( !(is_valid_border_style(pstyle) || strcmpiW(styleWindowInset, pstyle) == 0))
            {
                TRACE("1. Invalid style (%s)\n", debugstr_w(pstyle));
                hres = E_INVALIDARG;
            }
            SysFreeString(pstyle);
            last = i+1;
        }
        i++;
    }

    if(hres == S_OK)
    {
        pstyle = SysAllocStringLen(&v[last], i-last);
        if( !(is_valid_border_style(pstyle) || strcmpiW(styleWindowInset, pstyle) == 0))
        {
            TRACE("2. Invalid style (%s)\n", debugstr_w(pstyle));
            hres = E_INVALIDARG;
        }
        SysFreeString(pstyle);
    }

    if(hres == S_OK)
        hres = set_style_property(This, STYLEID_BORDER_STYLE, v);

    return hres;
}

static HRESULT WINAPI HTMLStyle_get_borderStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_property(This, STYLEID_BORDER_TOP_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderTopStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_property(This, STYLEID_BORDER_RIGHT_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderRightStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_property(This, STYLEID_BORDER_BOTTOM_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_property(This, STYLEID_BORDER_LEFT_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_width(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_width(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_height(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_height(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_styleFloat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_FLOAT, v);
}

static HRESULT WINAPI HTMLStyle_get_styleFloat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_FLOAT, p);
}

static HRESULT WINAPI HTMLStyle_put_clear(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_CLEAR, v);
}

static HRESULT WINAPI HTMLStyle_get_clear(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_CLEAR, p);
}

static HRESULT WINAPI HTMLStyle_put_display(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_DISPLAY, v);
}

static HRESULT WINAPI HTMLStyle_get_display(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLStyle_put_visibility(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_VISIBILITY, v);
}

static HRESULT WINAPI HTMLStyle_get_visibility(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_VISIBILITY, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleType(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_LISTSTYLETYPE, v);
}

static HRESULT WINAPI HTMLStyle_get_listStyleType(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_LISTSTYLETYPE, p);
}

static HRESULT WINAPI HTMLStyle_put_listStylePosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_LISTSTYLEPOSITION, v);
}

static HRESULT WINAPI HTMLStyle_get_listStylePosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_LISTSTYLEPOSITION, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStyleImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_listStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_LIST_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_listStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_LIST_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_whiteSpace(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_WHITE_SPACE, v);
}

static HRESULT WINAPI HTMLStyle_get_whiteSpace(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_WHITE_SPACE, p);
}

static HRESULT WINAPI HTMLStyle_put_top(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_TOP, &v);
}

static HRESULT WINAPI HTMLStyle_get_top(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_left(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_LEFT, &v);
}

static HRESULT WINAPI HTMLStyle_get_left(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_get_position(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyle2_get_position(&This->IHTMLStyle2_iface, p);
}

static HRESULT WINAPI HTMLStyle_put_zIndex(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_Z_INDEX, &v);
}

static HRESULT WINAPI HTMLStyle_get_zIndex(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_Z_INDEX, p);
}

static HRESULT WINAPI HTMLStyle_put_overflow(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR szVisible[] = {'v','i','s','i','b','l','e',0};
    static const WCHAR szScroll[]  = {'s','c','r','o','l','l',0};
    static const WCHAR szHidden[]  = {'h','i','d','d','e','n',0};
    static const WCHAR szAuto[]    = {'a','u','t','o',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* overflow can only be one of the follow values. */
    if(!v || !*v || strcmpiW(szVisible, v) == 0 || strcmpiW(szScroll, v) == 0 ||
             strcmpiW(szHidden, v) == 0  || strcmpiW(szAuto, v) == 0)
    {
        return set_style_property(This, STYLEID_OVERFLOW, v);
    }

    return E_INVALIDARG;
}


static HRESULT WINAPI HTMLStyle_get_overflow(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_property(This, STYLEID_OVERFLOW, p);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakBefore(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_PAGE_BREAK_BEFORE, v);
}

static HRESULT WINAPI HTMLStyle_get_pageBreakBefore(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_PAGE_BREAK_BEFORE, p);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakAfter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_PAGE_BREAK_AFTER, v);
}

static HRESULT WINAPI HTMLStyle_get_pageBreakAfter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_PAGE_BREAK_AFTER, p);
}

static HRESULT WINAPI HTMLStyle_put_cssText(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMCSSStyleDeclaration_SetCssText(This->nsstyle, &text_str);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        FIXME("SetCssStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_cssText(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: Gecko style formatting is different than IE (uppercase). */
    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_GetCssText(This->nsstyle, &text_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *text;

        nsAString_GetData(&text_str, &text);
        *p = *text ? SysAllocString(text) : NULL;
    }else {
        FIXME("GetCssStyle failed: %08x\n", nsres);
        *p = NULL;
    }

    nsAString_Finish(&text_str);
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_pixelTop(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return set_style_pxattr(This, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelTop(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelLeft(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return set_style_pxattr(This, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelLeft(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelWidth(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->()\n", This);

    return set_style_pxattr(This, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelWidth(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelHeight(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return set_style_pxattr(This, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelHeight(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_posTop(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_posTop(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_posLeft(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_posLeft(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_posWidth(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_posWidth(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_WIDTH, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_posHeight(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_posHeight(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_HEIGHT, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_cursor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_CURSOR, v);
}

static HRESULT WINAPI HTMLStyle_get_cursor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLStyle_put_clip(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_CLIP, v);
}

static HRESULT WINAPI HTMLStyle_get_clip(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_CLIP, p);
}

static void set_opacity(HTMLStyle *This, const WCHAR *val)
{
    nsAString name_str, val_str, empty_str;
    nsresult nsres;

    static const WCHAR opacityW[] = {'o','p','a','c','i','t','y',0};

    TRACE("%s\n", debugstr_w(val));

    nsAString_InitDepend(&name_str, opacityW);
    nsAString_InitDepend(&val_str, val);
    nsAString_InitDepend(&empty_str, emptyW);

    nsres = nsIDOMCSSStyleDeclaration_SetProperty(This->nsstyle, &name_str, &val_str, &empty_str);
    if(NS_FAILED(nsres))
        ERR("SetProperty failed: %08x\n", nsres);

    nsAString_Finish(&name_str);
    nsAString_Finish(&val_str);
    nsAString_Finish(&empty_str);
}

static void update_filter(HTMLStyle *This)
{
    const WCHAR *ptr, *ptr2;

    static const WCHAR alphaW[] = {'a','l','p','h','a'};

    if(get_style_compat_mode(This) >= COMPAT_MODE_IE10)
        return;

    ptr = This->elem->filter;
    TRACE("%s\n", debugstr_w(ptr));
    if(!ptr) {
        set_opacity(This, emptyW);
        return;
    }

    while(1) {
        while(isspaceW(*ptr))
            ptr++;
        if(!*ptr)
            break;

        ptr2 = ptr;
        while(isalnumW(*ptr))
            ptr++;
        if(ptr == ptr2) {
            WARN("unexpected char '%c'\n", *ptr);
            break;
        }
        if(*ptr != '(') {
            WARN("expected '('\n");
            continue;
        }

        if(ptr2 + ARRAY_SIZE(alphaW) == ptr && !memcmp(ptr2, alphaW, sizeof(alphaW))) {
            static const WCHAR formatW[] = {'%','f',0};
            static const WCHAR opacityW[] = {'o','p','a','c','i','t','y','='};

            ptr++;
            do {
                while(isspaceW(*ptr))
                    ptr++;

                ptr2 = ptr;
                while(*ptr && *ptr != ',' && *ptr != ')')
                    ptr++;
                if(!*ptr) {
                    WARN("unexpected end of string\n");
                    break;
                }

                if(ptr-ptr2 > ARRAY_SIZE(opacityW) && !memcmp(ptr2, opacityW, sizeof(opacityW))) {
                    float fval = 0.0f, e = 0.1f;
                    WCHAR buf[32];

                    ptr2 += ARRAY_SIZE(opacityW);

                    while(isdigitW(*ptr2))
                        fval = fval*10.0f + (float)(*ptr2++ - '0');

                    if(*ptr2 == '.') {
                        while(isdigitW(*++ptr2)) {
                            fval += e * (float)(*ptr2++ - '0');
                            e *= 0.1f;
                        }
                    }

                    sprintfW(buf, formatW, fval * 0.01f);
                    set_opacity(This, buf);
                }else {
                    FIXME("unknown param %s\n", debugstr_wn(ptr2, ptr-ptr2));
                }

                if(*ptr == ',')
                    ptr++;
            }while(*ptr != ')');
        }else {
            FIXME("unknown filter %s\n", debugstr_wn(ptr2, ptr-ptr2));
            ptr = strchrW(ptr, ')');
            if(!ptr)
                break;
            ptr++;
        }
    }
}

static HRESULT WINAPI HTMLStyle_put_filter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR *new_filter = NULL;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->elem) {
        FIXME("Element already destroyed\n");
        return E_UNEXPECTED;
    }

    if(v) {
        new_filter = heap_strdupW(v);
        if(!new_filter)
            return E_OUTOFMEMORY;
    }

    heap_free(This->elem->filter);
    This->elem->filter = new_filter;

    update_filter(This);
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_filter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        FIXME("Element already destroyed\n");
        return E_UNEXPECTED;
    }

    if(This->elem->filter) {
        *p = SysAllocString(This->elem->filter);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_setAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        VARIANT AttributeValue, LONG lFlags)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s %s %08x)\n", This, debugstr_w(strAttributeName),
          debugstr_variant(&AttributeValue), lFlags);

    if(!strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, &strAttributeName, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    if(hres == S_OK)
    {
        VARIANT ret;
        DISPID dispidNamed = DISPID_PROPERTYPUT;
        DISPPARAMS params;

        params.cArgs = 1;
        params.rgvarg = &AttributeValue;
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidNamed;

        hres = HTMLStyle_Invoke(iface, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
    }
    else
    {
        FIXME("Custom attributes not supported.\n");
    }

    TRACE("ret: %08x\n", hres);

    return hres;
}

static HRESULT WINAPI HTMLStyle_getAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        LONG lFlags, VARIANT *AttributeValue)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s v%p %08x)\n", This, debugstr_w(strAttributeName),
          AttributeValue, lFlags);

    if(!AttributeValue || !strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, &strAttributeName, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    if(hres == S_OK)
    {
        DISPPARAMS params = {NULL, NULL, 0, 0 };

        hres = HTMLStyle_Invoke(iface, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &params, AttributeValue, NULL, NULL);
    }
    else
    {
        FIXME("Custom attributes not supported.\n");
    }

    return hres;
}

static HRESULT WINAPI HTMLStyle_removeAttribute(IHTMLStyle *iface, BSTR strAttributeName,
                                                LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    const style_tbl_entry_t *style_entry;
    nsAString name_str, ret_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName), lFlags, pfSuccess);

    style_entry = lookup_style_tbl(strAttributeName);
    if(!style_entry) {
        DISPID dispid;
        unsigned i;

        hres = IDispatchEx_GetDispID(&This->dispex.IDispatchEx_iface, strAttributeName,
                (lFlags&1) ? fdexNameCaseSensitive : fdexNameCaseInsensitive, &dispid);
        if(hres != S_OK) {
            *pfSuccess = VARIANT_FALSE;
            return S_OK;
        }

        for(i=0; i < ARRAY_SIZE(style_tbl); i++) {
            if(dispid == style_tbl[i].dispid)
                break;
        }

        if(i == ARRAY_SIZE(style_tbl))
            return remove_attribute(&This->dispex, dispid, pfSuccess);
        style_entry = style_tbl+i;
    }

    /* filter property is a special case */
    if(style_entry->dispid == DISPID_IHTMLSTYLE_FILTER) {
        *pfSuccess = variant_bool(This->elem->filter && *This->elem->filter);
        heap_free(This->elem->filter);
        This->elem->filter = NULL;
        update_filter(This);
        return S_OK;
    }

    nsAString_InitDepend(&name_str, style_entry->name);
    nsAString_Init(&ret_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_RemoveProperty(This->nsstyle, &name_str, &ret_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *ret;
        nsAString_GetData(&ret_str, &ret);
        *pfSuccess = variant_bool(*ret);
    }else {
        ERR("RemoveProperty failed: %08x\n", nsres);
    }
    nsAString_Finish(&name_str);
    nsAString_Finish(&ret_str);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLStyle_toString(IHTMLStyle *iface, BSTR *String)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static const IHTMLStyleVtbl HTMLStyleVtbl = {
    HTMLStyle_QueryInterface,
    HTMLStyle_AddRef,
    HTMLStyle_Release,
    HTMLStyle_GetTypeInfoCount,
    HTMLStyle_GetTypeInfo,
    HTMLStyle_GetIDsOfNames,
    HTMLStyle_Invoke,
    HTMLStyle_put_fontFamily,
    HTMLStyle_get_fontFamily,
    HTMLStyle_put_fontStyle,
    HTMLStyle_get_fontStyle,
    HTMLStyle_put_fontVariant,
    HTMLStyle_get_fontVariant,
    HTMLStyle_put_fontWeight,
    HTMLStyle_get_fontWeight,
    HTMLStyle_put_fontSize,
    HTMLStyle_get_fontSize,
    HTMLStyle_put_font,
    HTMLStyle_get_font,
    HTMLStyle_put_color,
    HTMLStyle_get_color,
    HTMLStyle_put_background,
    HTMLStyle_get_background,
    HTMLStyle_put_backgroundColor,
    HTMLStyle_get_backgroundColor,
    HTMLStyle_put_backgroundImage,
    HTMLStyle_get_backgroundImage,
    HTMLStyle_put_backgroundRepeat,
    HTMLStyle_get_backgroundRepeat,
    HTMLStyle_put_backgroundAttachment,
    HTMLStyle_get_backgroundAttachment,
    HTMLStyle_put_backgroundPosition,
    HTMLStyle_get_backgroundPosition,
    HTMLStyle_put_backgroundPositionX,
    HTMLStyle_get_backgroundPositionX,
    HTMLStyle_put_backgroundPositionY,
    HTMLStyle_get_backgroundPositionY,
    HTMLStyle_put_wordSpacing,
    HTMLStyle_get_wordSpacing,
    HTMLStyle_put_letterSpacing,
    HTMLStyle_get_letterSpacing,
    HTMLStyle_put_textDecoration,
    HTMLStyle_get_textDecoration,
    HTMLStyle_put_textDecorationNone,
    HTMLStyle_get_textDecorationNone,
    HTMLStyle_put_textDecorationUnderline,
    HTMLStyle_get_textDecorationUnderline,
    HTMLStyle_put_textDecorationOverline,
    HTMLStyle_get_textDecorationOverline,
    HTMLStyle_put_textDecorationLineThrough,
    HTMLStyle_get_textDecorationLineThrough,
    HTMLStyle_put_textDecorationBlink,
    HTMLStyle_get_textDecorationBlink,
    HTMLStyle_put_verticalAlign,
    HTMLStyle_get_verticalAlign,
    HTMLStyle_put_textTransform,
    HTMLStyle_get_textTransform,
    HTMLStyle_put_textAlign,
    HTMLStyle_get_textAlign,
    HTMLStyle_put_textIndent,
    HTMLStyle_get_textIndent,
    HTMLStyle_put_lineHeight,
    HTMLStyle_get_lineHeight,
    HTMLStyle_put_marginTop,
    HTMLStyle_get_marginTop,
    HTMLStyle_put_marginRight,
    HTMLStyle_get_marginRight,
    HTMLStyle_put_marginBottom,
    HTMLStyle_get_marginBottom,
    HTMLStyle_put_marginLeft,
    HTMLStyle_get_marginLeft,
    HTMLStyle_put_margin,
    HTMLStyle_get_margin,
    HTMLStyle_put_paddingTop,
    HTMLStyle_get_paddingTop,
    HTMLStyle_put_paddingRight,
    HTMLStyle_get_paddingRight,
    HTMLStyle_put_paddingBottom,
    HTMLStyle_get_paddingBottom,
    HTMLStyle_put_paddingLeft,
    HTMLStyle_get_paddingLeft,
    HTMLStyle_put_padding,
    HTMLStyle_get_padding,
    HTMLStyle_put_border,
    HTMLStyle_get_border,
    HTMLStyle_put_borderTop,
    HTMLStyle_get_borderTop,
    HTMLStyle_put_borderRight,
    HTMLStyle_get_borderRight,
    HTMLStyle_put_borderBottom,
    HTMLStyle_get_borderBottom,
    HTMLStyle_put_borderLeft,
    HTMLStyle_get_borderLeft,
    HTMLStyle_put_borderColor,
    HTMLStyle_get_borderColor,
    HTMLStyle_put_borderTopColor,
    HTMLStyle_get_borderTopColor,
    HTMLStyle_put_borderRightColor,
    HTMLStyle_get_borderRightColor,
    HTMLStyle_put_borderBottomColor,
    HTMLStyle_get_borderBottomColor,
    HTMLStyle_put_borderLeftColor,
    HTMLStyle_get_borderLeftColor,
    HTMLStyle_put_borderWidth,
    HTMLStyle_get_borderWidth,
    HTMLStyle_put_borderTopWidth,
    HTMLStyle_get_borderTopWidth,
    HTMLStyle_put_borderRightWidth,
    HTMLStyle_get_borderRightWidth,
    HTMLStyle_put_borderBottomWidth,
    HTMLStyle_get_borderBottomWidth,
    HTMLStyle_put_borderLeftWidth,
    HTMLStyle_get_borderLeftWidth,
    HTMLStyle_put_borderStyle,
    HTMLStyle_get_borderStyle,
    HTMLStyle_put_borderTopStyle,
    HTMLStyle_get_borderTopStyle,
    HTMLStyle_put_borderRightStyle,
    HTMLStyle_get_borderRightStyle,
    HTMLStyle_put_borderBottomStyle,
    HTMLStyle_get_borderBottomStyle,
    HTMLStyle_put_borderLeftStyle,
    HTMLStyle_get_borderLeftStyle,
    HTMLStyle_put_width,
    HTMLStyle_get_width,
    HTMLStyle_put_height,
    HTMLStyle_get_height,
    HTMLStyle_put_styleFloat,
    HTMLStyle_get_styleFloat,
    HTMLStyle_put_clear,
    HTMLStyle_get_clear,
    HTMLStyle_put_display,
    HTMLStyle_get_display,
    HTMLStyle_put_visibility,
    HTMLStyle_get_visibility,
    HTMLStyle_put_listStyleType,
    HTMLStyle_get_listStyleType,
    HTMLStyle_put_listStylePosition,
    HTMLStyle_get_listStylePosition,
    HTMLStyle_put_listStyleImage,
    HTMLStyle_get_listStyleImage,
    HTMLStyle_put_listStyle,
    HTMLStyle_get_listStyle,
    HTMLStyle_put_whiteSpace,
    HTMLStyle_get_whiteSpace,
    HTMLStyle_put_top,
    HTMLStyle_get_top,
    HTMLStyle_put_left,
    HTMLStyle_get_left,
    HTMLStyle_get_position,
    HTMLStyle_put_zIndex,
    HTMLStyle_get_zIndex,
    HTMLStyle_put_overflow,
    HTMLStyle_get_overflow,
    HTMLStyle_put_pageBreakBefore,
    HTMLStyle_get_pageBreakBefore,
    HTMLStyle_put_pageBreakAfter,
    HTMLStyle_get_pageBreakAfter,
    HTMLStyle_put_cssText,
    HTMLStyle_get_cssText,
    HTMLStyle_put_pixelTop,
    HTMLStyle_get_pixelTop,
    HTMLStyle_put_pixelLeft,
    HTMLStyle_get_pixelLeft,
    HTMLStyle_put_pixelWidth,
    HTMLStyle_get_pixelWidth,
    HTMLStyle_put_pixelHeight,
    HTMLStyle_get_pixelHeight,
    HTMLStyle_put_posTop,
    HTMLStyle_get_posTop,
    HTMLStyle_put_posLeft,
    HTMLStyle_get_posLeft,
    HTMLStyle_put_posWidth,
    HTMLStyle_get_posWidth,
    HTMLStyle_put_posHeight,
    HTMLStyle_get_posHeight,
    HTMLStyle_put_cursor,
    HTMLStyle_get_cursor,
    HTMLStyle_put_clip,
    HTMLStyle_get_clip,
    HTMLStyle_put_filter,
    HTMLStyle_get_filter,
    HTMLStyle_setAttribute,
    HTMLStyle_getAttribute,
    HTMLStyle_removeAttribute,
    HTMLStyle_toString
};

static inline HTMLStyle *impl_from_IHTMLStyle2(IHTMLStyle2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle2_iface);
}

static HRESULT WINAPI HTMLStyle2_QueryInterface(IHTMLStyle2 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle2_AddRef(IHTMLStyle2 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle2_Release(IHTMLStyle2 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle2_GetTypeInfoCount(IHTMLStyle2 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle2_GetTypeInfo(IHTMLStyle2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle2_GetIDsOfNames(IHTMLStyle2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle2_Invoke(IHTMLStyle2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle2_put_tableLayout(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_TABLE_LAYOUT, v);
}

static HRESULT WINAPI HTMLStyle2_get_tableLayout(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_TABLE_LAYOUT, p);
}

static HRESULT WINAPI HTMLStyle2_put_borderCollapse(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_borderCollapse(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_direction(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_DIRECTION, v);
}

static HRESULT WINAPI HTMLStyle2_get_direction(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_DIRECTION, p);
}

static HRESULT WINAPI HTMLStyle2_put_behavior(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle2_get_behavior(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_setExpression(IHTMLStyle2 *iface, BSTR propname, BSTR expression, BSTR language)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(propname), debugstr_w(expression), debugstr_w(language));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_getExpression(IHTMLStyle2 *iface, BSTR propname, VARIANT *expression)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), expression);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_removeExpression(IHTMLStyle2 *iface, BSTR propname, VARIANT_BOOL *pfSuccess)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), pfSuccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_position(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_POSITION, v);
}

static HRESULT WINAPI HTMLStyle2_get_position(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_POSITION, p);
}

static HRESULT WINAPI HTMLStyle2_put_unicodeBidi(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_unicodeBidi(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_bottom(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_BOTTOM, &v);
}

static HRESULT WINAPI HTMLStyle2_get_bottom(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle2_put_right(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_RIGHT, &v);
}

static HRESULT WINAPI HTMLStyle2_get_right(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle2_put_pixelBottom(IHTMLStyle2 *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_pixelBottom(IHTMLStyle2 *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_pixelRight(IHTMLStyle2 *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_pixelRight(IHTMLStyle2 *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_posBottom(IHTMLStyle2 *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_posBottom(IHTMLStyle2 *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_posRight(IHTMLStyle2 *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_posRight(IHTMLStyle2 *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_imeMode(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_imeMode(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyAlign(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyAlign(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyPosition(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyPosition(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyOverhang(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyOverhang(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridChar(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridChar(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridLine(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridLine(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridMode(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridMode(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridType(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridType(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGrid(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGrid(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_wordBreak(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_wordBreak(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_lineBreak(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_lineBreak(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textJustify(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textJustify(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textJustifyTrim(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textJustifyTrim(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textKashida(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textKashida(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textAutospace(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textAutospace(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_overflowX(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_OVERFLOW_X, v);
}

static HRESULT WINAPI HTMLStyle2_get_overflowX(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_OVERFLOW_X, p);
}

static HRESULT WINAPI HTMLStyle2_put_overflowY(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_OVERFLOW_Y, v);
}

static HRESULT WINAPI HTMLStyle2_get_overflowY(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_OVERFLOW_Y, p);
}

static HRESULT WINAPI HTMLStyle2_put_accelerator(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_accelerator(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle2Vtbl HTMLStyle2Vtbl = {
    HTMLStyle2_QueryInterface,
    HTMLStyle2_AddRef,
    HTMLStyle2_Release,
    HTMLStyle2_GetTypeInfoCount,
    HTMLStyle2_GetTypeInfo,
    HTMLStyle2_GetIDsOfNames,
    HTMLStyle2_Invoke,
    HTMLStyle2_put_tableLayout,
    HTMLStyle2_get_tableLayout,
    HTMLStyle2_put_borderCollapse,
    HTMLStyle2_get_borderCollapse,
    HTMLStyle2_put_direction,
    HTMLStyle2_get_direction,
    HTMLStyle2_put_behavior,
    HTMLStyle2_get_behavior,
    HTMLStyle2_setExpression,
    HTMLStyle2_getExpression,
    HTMLStyle2_removeExpression,
    HTMLStyle2_put_position,
    HTMLStyle2_get_position,
    HTMLStyle2_put_unicodeBidi,
    HTMLStyle2_get_unicodeBidi,
    HTMLStyle2_put_bottom,
    HTMLStyle2_get_bottom,
    HTMLStyle2_put_right,
    HTMLStyle2_get_right,
    HTMLStyle2_put_pixelBottom,
    HTMLStyle2_get_pixelBottom,
    HTMLStyle2_put_pixelRight,
    HTMLStyle2_get_pixelRight,
    HTMLStyle2_put_posBottom,
    HTMLStyle2_get_posBottom,
    HTMLStyle2_put_posRight,
    HTMLStyle2_get_posRight,
    HTMLStyle2_put_imeMode,
    HTMLStyle2_get_imeMode,
    HTMLStyle2_put_rubyAlign,
    HTMLStyle2_get_rubyAlign,
    HTMLStyle2_put_rubyPosition,
    HTMLStyle2_get_rubyPosition,
    HTMLStyle2_put_rubyOverhang,
    HTMLStyle2_get_rubyOverhang,
    HTMLStyle2_put_layoutGridChar,
    HTMLStyle2_get_layoutGridChar,
    HTMLStyle2_put_layoutGridLine,
    HTMLStyle2_get_layoutGridLine,
    HTMLStyle2_put_layoutGridMode,
    HTMLStyle2_get_layoutGridMode,
    HTMLStyle2_put_layoutGridType,
    HTMLStyle2_get_layoutGridType,
    HTMLStyle2_put_layoutGrid,
    HTMLStyle2_get_layoutGrid,
    HTMLStyle2_put_wordBreak,
    HTMLStyle2_get_wordBreak,
    HTMLStyle2_put_lineBreak,
    HTMLStyle2_get_lineBreak,
    HTMLStyle2_put_textJustify,
    HTMLStyle2_get_textJustify,
    HTMLStyle2_put_textJustifyTrim,
    HTMLStyle2_get_textJustifyTrim,
    HTMLStyle2_put_textKashida,
    HTMLStyle2_get_textKashida,
    HTMLStyle2_put_textAutospace,
    HTMLStyle2_get_textAutospace,
    HTMLStyle2_put_overflowX,
    HTMLStyle2_get_overflowX,
    HTMLStyle2_put_overflowY,
    HTMLStyle2_get_overflowY,
    HTMLStyle2_put_accelerator,
    HTMLStyle2_get_accelerator
};

static inline HTMLStyle *impl_from_IHTMLStyle3(IHTMLStyle3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle3_iface);
}

static HRESULT WINAPI HTMLStyle3_QueryInterface(IHTMLStyle3 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle3_AddRef(IHTMLStyle3 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle3_Release(IHTMLStyle3 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle3_GetTypeInfoCount(IHTMLStyle3 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle3_GetTypeInfo(IHTMLStyle3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle3_GetIDsOfNames(IHTMLStyle3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle3_Invoke(IHTMLStyle3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle3_put_layoutFlow(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_layoutFlow(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const WCHAR zoomW[] = {'z','o','o','m',0};

static HRESULT WINAPI HTMLStyle3_put_zoom(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    VARIANT *var;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    /* zoom property is IE CSS extension that is mostly used as a hack to workaround IE bugs.
     * The value is set to 1 then. We can safely ignore setting zoom to 1. */
    if(V_VT(&v) != VT_I4 || V_I4(&v) != 1)
        WARN("stub for %s\n", debugstr_variant(&v));

    hres = dispex_get_dprop_ref(&This->dispex, zoomW, TRUE, &var);
    if(FAILED(hres))
        return hres;

    return VariantChangeType(var, &v, 0, VT_BSTR);
}

static HRESULT WINAPI HTMLStyle3_get_zoom(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    VARIANT *var;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = dispex_get_dprop_ref(&This->dispex, zoomW, FALSE, &var);
    if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = NULL;
        return S_OK;
    }
    if(FAILED(hres))
        return hres;

    return VariantCopy(p, var);
}

static HRESULT WINAPI HTMLStyle3_put_wordWrap(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_WORD_WRAP, v);
}

static HRESULT WINAPI HTMLStyle3_get_wordWrap(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_WORD_WRAP, p);
}

static HRESULT WINAPI HTMLStyle3_put_textUnderlinePosition(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textUnderlinePosition(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarBaseColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarBaseColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarFaceColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarFaceColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbar3dLightColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbar3dLightColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarShadowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarShadowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarHighlightColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarHighlightColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarDarkShadowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarDarkShadowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarArrowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarArrowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarTrackColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarTrackColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_writingMode(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_writingMode(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_textAlignLast(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textAlignLast(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_textKashidaSpace(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textKashidaSpace(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle3Vtbl HTMLStyle3Vtbl = {
    HTMLStyle3_QueryInterface,
    HTMLStyle3_AddRef,
    HTMLStyle3_Release,
    HTMLStyle3_GetTypeInfoCount,
    HTMLStyle3_GetTypeInfo,
    HTMLStyle3_GetIDsOfNames,
    HTMLStyle3_Invoke,
    HTMLStyle3_put_layoutFlow,
    HTMLStyle3_get_layoutFlow,
    HTMLStyle3_put_zoom,
    HTMLStyle3_get_zoom,
    HTMLStyle3_put_wordWrap,
    HTMLStyle3_get_wordWrap,
    HTMLStyle3_put_textUnderlinePosition,
    HTMLStyle3_get_textUnderlinePosition,
    HTMLStyle3_put_scrollbarBaseColor,
    HTMLStyle3_get_scrollbarBaseColor,
    HTMLStyle3_put_scrollbarFaceColor,
    HTMLStyle3_get_scrollbarFaceColor,
    HTMLStyle3_put_scrollbar3dLightColor,
    HTMLStyle3_get_scrollbar3dLightColor,
    HTMLStyle3_put_scrollbarShadowColor,
    HTMLStyle3_get_scrollbarShadowColor,
    HTMLStyle3_put_scrollbarHighlightColor,
    HTMLStyle3_get_scrollbarHighlightColor,
    HTMLStyle3_put_scrollbarDarkShadowColor,
    HTMLStyle3_get_scrollbarDarkShadowColor,
    HTMLStyle3_put_scrollbarArrowColor,
    HTMLStyle3_get_scrollbarArrowColor,
    HTMLStyle3_put_scrollbarTrackColor,
    HTMLStyle3_get_scrollbarTrackColor,
    HTMLStyle3_put_writingMode,
    HTMLStyle3_get_writingMode,
    HTMLStyle3_put_textAlignLast,
    HTMLStyle3_get_textAlignLast,
    HTMLStyle3_put_textKashidaSpace,
    HTMLStyle3_get_textKashidaSpace
};

/*
 * IHTMLStyle4 Interface
 */
static inline HTMLStyle *impl_from_IHTMLStyle4(IHTMLStyle4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle4_iface);
}

static HRESULT WINAPI HTMLStyle4_QueryInterface(IHTMLStyle4 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle4_AddRef(IHTMLStyle4 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle4_Release(IHTMLStyle4 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle4_GetTypeInfoCount(IHTMLStyle4 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle4_GetTypeInfo(IHTMLStyle4 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle4_GetIDsOfNames(IHTMLStyle4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle4_Invoke(IHTMLStyle4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle4_put_textOverflow(IHTMLStyle4 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle4_get_textOverflow(IHTMLStyle4 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle4_put_minHeight(IHTMLStyle4 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MIN_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle4_get_minHeight(IHTMLStyle4 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_MIN_HEIGHT, p);
}

static const IHTMLStyle4Vtbl HTMLStyle4Vtbl = {
    HTMLStyle4_QueryInterface,
    HTMLStyle4_AddRef,
    HTMLStyle4_Release,
    HTMLStyle4_GetTypeInfoCount,
    HTMLStyle4_GetTypeInfo,
    HTMLStyle4_GetIDsOfNames,
    HTMLStyle4_Invoke,
    HTMLStyle4_put_textOverflow,
    HTMLStyle4_get_textOverflow,
    HTMLStyle4_put_minHeight,
    HTMLStyle4_get_minHeight
};

static inline HTMLStyle *impl_from_IHTMLStyle5(IHTMLStyle5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle5_iface);
}

static HRESULT WINAPI HTMLStyle5_QueryInterface(IHTMLStyle5 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle5_AddRef(IHTMLStyle5 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle5_Release(IHTMLStyle5 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle5_GetTypeInfoCount(IHTMLStyle5 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle5_GetTypeInfo(IHTMLStyle5 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle5_GetIDsOfNames(IHTMLStyle5 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle5_Invoke(IHTMLStyle5 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle5_put_msInterpolationMode(IHTMLStyle5 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_get_msInterpolationMode(IHTMLStyle5 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_put_maxHeight(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MAX_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle5_get_maxHeight(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(p));

    return get_style_property_var(This, STYLEID_MAX_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle5_put_minWidth(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MIN_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle5_get_minWidth(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_MIN_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle5_put_maxWidth(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(This, STYLEID_MAX_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle5_get_maxWidth(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(This, STYLEID_MAX_WIDTH, p);
}

static const IHTMLStyle5Vtbl HTMLStyle5Vtbl = {
    HTMLStyle5_QueryInterface,
    HTMLStyle5_AddRef,
    HTMLStyle5_Release,
    HTMLStyle5_GetTypeInfoCount,
    HTMLStyle5_GetTypeInfo,
    HTMLStyle5_GetIDsOfNames,
    HTMLStyle5_Invoke,
    HTMLStyle5_put_msInterpolationMode,
    HTMLStyle5_get_msInterpolationMode,
    HTMLStyle5_put_maxHeight,
    HTMLStyle5_get_maxHeight,
    HTMLStyle5_put_minWidth,
    HTMLStyle5_get_minWidth,
    HTMLStyle5_put_maxWidth,
    HTMLStyle5_get_maxWidth
};

static inline HTMLStyle *impl_from_IHTMLStyle6(IHTMLStyle6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle6_iface);
}

static HRESULT WINAPI HTMLStyle6_QueryInterface(IHTMLStyle6 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle6_AddRef(IHTMLStyle6 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle6_Release(IHTMLStyle6 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle6_GetTypeInfoCount(IHTMLStyle6 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle6_GetTypeInfo(IHTMLStyle6 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle6_GetIDsOfNames(IHTMLStyle6 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle6_Invoke(IHTMLStyle6 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle6_put_content(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_content(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_contentSide(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_contentSide(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_counterIncrement(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_counterIncrement(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_counterReset(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_counterReset(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outline(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_OUTLINE, v);
}

static HRESULT WINAPI HTMLStyle6_get_outline(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_OUTLINE, p);
}

static HRESULT WINAPI HTMLStyle6_put_outlineWidth(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineWidth(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outlineStyle(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineStyle(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outlineColor(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineColor(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_boxSizing(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_BOX_SIZING, v);
}

static HRESULT WINAPI HTMLStyle6_get_boxSizing(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_BOX_SIZING, p);
}

static HRESULT WINAPI HTMLStyle6_put_boxSpacing(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_boxSpacing(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_orphans(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_orphans(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_windows(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_windows(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_pageBreakInside(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_pageBreakInside(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_emptyCells(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_emptyCells(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_msBlockProgression(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_msBlockProgression(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_quotes(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_quotes(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle6Vtbl HTMLStyle6Vtbl = {
    HTMLStyle6_QueryInterface,
    HTMLStyle6_AddRef,
    HTMLStyle6_Release,
    HTMLStyle6_GetTypeInfoCount,
    HTMLStyle6_GetTypeInfo,
    HTMLStyle6_GetIDsOfNames,
    HTMLStyle6_Invoke,
    HTMLStyle6_put_content,
    HTMLStyle6_get_content,
    HTMLStyle6_put_contentSide,
    HTMLStyle6_get_contentSide,
    HTMLStyle6_put_counterIncrement,
    HTMLStyle6_get_counterIncrement,
    HTMLStyle6_put_counterReset,
    HTMLStyle6_get_counterReset,
    HTMLStyle6_put_outline,
    HTMLStyle6_get_outline,
    HTMLStyle6_put_outlineWidth,
    HTMLStyle6_get_outlineWidth,
    HTMLStyle6_put_outlineStyle,
    HTMLStyle6_get_outlineStyle,
    HTMLStyle6_put_outlineColor,
    HTMLStyle6_get_outlineColor,
    HTMLStyle6_put_boxSizing,
    HTMLStyle6_get_boxSizing,
    HTMLStyle6_put_boxSpacing,
    HTMLStyle6_get_boxSpacing,
    HTMLStyle6_put_orphans,
    HTMLStyle6_get_orphans,
    HTMLStyle6_put_windows,
    HTMLStyle6_get_windows,
    HTMLStyle6_put_pageBreakInside,
    HTMLStyle6_get_pageBreakInside,
    HTMLStyle6_put_emptyCells,
    HTMLStyle6_get_emptyCells,
    HTMLStyle6_put_msBlockProgression,
    HTMLStyle6_get_msBlockProgression,
    HTMLStyle6_put_quotes,
    HTMLStyle6_get_quotes
};

static HRESULT HTMLStyle_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
{
    const style_tbl_entry_t *style_entry;

    style_entry = lookup_style_tbl(name);
    if(style_entry) {
        *dispid = style_entry->dispid;
        return S_OK;
    }

    return DISP_E_UNKNOWNNAME;
}

static const dispex_static_data_vtbl_t HTMLStyle_dispex_vtbl = {
    NULL,
    HTMLStyle_get_dispid,
    NULL,
    NULL
};

static const tid_t HTMLStyle_iface_tids[] = {
    IHTMLStyle6_tid,
    IHTMLStyle5_tid,
    IHTMLStyle4_tid,
    IHTMLStyle3_tid,
    IHTMLStyle2_tid,
    IHTMLStyle_tid,
    0
};
static dispex_static_data_t HTMLStyle_dispex = {
    &HTMLStyle_dispex_vtbl,
    DispHTMLStyle_tid,
    HTMLStyle_iface_tids
};

static HRESULT get_style_from_elem(HTMLElement *elem, nsIDOMCSSStyleDeclaration **ret)
{
    nsIDOMElementCSSInlineStyle *nselemstyle;
    nsresult nsres;

    if(!elem->dom_element) {
        FIXME("comment element\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMElement_QueryInterface(elem->dom_element, &IID_nsIDOMElementCSSInlineStyle,
            (void**)&nselemstyle);
    assert(nsres == NS_OK);

    nsres = nsIDOMElementCSSInlineStyle_GetStyle(nselemstyle, ret);
    nsIDOMElementCSSInlineStyle_Release(nselemstyle);
    if(NS_FAILED(nsres)) {
        ERR("GetStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT HTMLStyle_Create(HTMLElement *elem, HTMLStyle **ret)
{
    nsIDOMCSSStyleDeclaration *nsstyle;
    HTMLStyle *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &nsstyle);
    if(FAILED(hres))
        return hres;

    style = heap_alloc_zero(sizeof(HTMLStyle));
    if(!style) {
        nsIDOMCSSStyleDeclaration_Release(nsstyle);
        return E_OUTOFMEMORY;
    }

    style->IHTMLStyle_iface.lpVtbl = &HTMLStyleVtbl;
    style->IHTMLStyle2_iface.lpVtbl = &HTMLStyle2Vtbl;
    style->IHTMLStyle3_iface.lpVtbl = &HTMLStyle3Vtbl;
    style->IHTMLStyle4_iface.lpVtbl = &HTMLStyle4Vtbl;
    style->IHTMLStyle5_iface.lpVtbl = &HTMLStyle5Vtbl;
    style->IHTMLStyle6_iface.lpVtbl = &HTMLStyle6Vtbl;

    style->ref = 1;
    style->nsstyle = nsstyle;
    style->elem = elem;

    nsIDOMCSSStyleDeclaration_AddRef(nsstyle);

    init_dispex(&style->dispex, (IUnknown*)&style->IHTMLStyle_iface, &HTMLStyle_dispex);

    *ret = style;
    return S_OK;
}

HRESULT get_elem_style(HTMLElement *elem, styleid_t styleid, BSTR *ret)
{
    nsIDOMCSSStyleDeclaration *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &style);
    if(FAILED(hres))
        return hres;

    hres = get_nsstyle_property(style, styleid, ret);
    nsIDOMCSSStyleDeclaration_Release(style);
    return hres;
}

HRESULT set_elem_style(HTMLElement *elem, styleid_t styleid, const WCHAR *val)
{
    nsIDOMCSSStyleDeclaration *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &style);
    if(FAILED(hres))
        return hres;

    hres = set_nsstyle_property(style, styleid, val);
    nsIDOMCSSStyleDeclaration_Release(style);
    return hres;
}
