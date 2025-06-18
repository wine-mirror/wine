#
# Copyright 1999, 2000, 2001 Patrik Stridvall
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

package winapi_module_user;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw(
    is_user_function
    get_message_result_type
    get_message_result_kind
    get_message_wparam_type
    get_message_wparam_kind
    get_message_lparam_type
    get_message_lparam_kind
);

use config qw($wine_dir);
use options qw($options);
use output qw($output);

use c_parser;

########################################################################

my $message;

########################################################################
# is_user_function

sub is_user_function($) {
    my $name = shift;
    if($name =~ /^(?:DefWindowProc|SendMessage)[AW]?$/) {
    }
}

########################################################################
# $message

$message = {
    WM_ACTIVATE => {
	id => 0x0006, result => "void", wparam => ["", ""], lparam => "HWND" },
    WM_ACTIVATEAPP => {
	id => 0x001c, result => "void", wparam => "BOOL", lparam => "DWORD" },
    WM_ACTIVATESHELLWINDOW => {
        id => 0x003e, result => "", wparam => "", lparam => "" },
    WM_ACTIVATETOPLEVEL => {
        id => 0x036e, result => "", wparam => "", lparam => "" },
    WM_ALTTABACTIVE => {
        id => 0x0029, result => "", wparam => "", lparam => "" },
    WM_APP => {
        id => 0x8000, result => "", wparam => "", lparam => "" },
    WM_ASKCBFORMATNAME => {
        id => 0x030c, result => "void", wparam => "int", lparam => "LPTSTR" },

    WM_BEGINDRAG => {
	id => 0x022c, result => "", wparam => "", lparam => "" },

    WM_CANCELMODE => {
	id => 0x001f, result => "void", wparam => "void", lparam => "void" },
    WM_CANCELJOURNAL => {
        id => 0x004b, result => "", wparam => "", lparam => "" },
    WM_CAPTURECHANGED => {
        id => 0x0215, result => "void", wparam => "void", lparam => "HWND" },
    WM_CHANGECBCHAIN => {
        id => 0x030d, result => "void", wparam => "HWND", lparam => "HWND" },
    WM_CHILDACTIVATE => {
        id => 0x0022, result => "void", wparam => "void", lparam => "void" },
    WM_CLEAR => {
        id => 0x0303, result => "void", wparam => "void", lparam => "void" },
    WM_CHAR => {
	id => 0x0102, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_CHARTOITEM => {
	id => 0x002f, result => "int", wparam => ["UINT", "UINT"], lparam => "HWND" },
    WM_CLOSE => {
	id => 0x0010, result => "void", wparam => "void", lparam => "void" },
    WM_COALESCE_FIRST => {
        id => 0x0390, result => "", wparam => "", lparam => "" },
    WM_COALESCE_LAST => {
        id => 0x039f, result => "", wparam => "", lparam => "" },
    WM_COMMAND => {
	id => 0x0111, result => "void", wparam => ["UINT", "UINT"], lparam => "HWND" },
    WM_COMMANDHELP => {
        id => 0x0365, result => "", wparam => "", lparam => "" },
    WM_COMMNOTIFY => {
        id => 0x0044, result => "void", wparam => "int", lparam => ["", ""] },
    WM_CONTEXTMENU => {
        id => 0x007b, result => "void", wparam => "HWND", lparam => ["UINT", "UINT"] },
    WM_COPY => {
	id => 0x0301, result => "void", wparam => "void", lparam => "void" },
    WM_COPYDATA => {
        id => 0x004a, result => "", wparam => "", lparam => "" },
    WM_COMPACTING => {
	id => 0x0041, result => "void", wparam => "UINT", lparam => "void" },
    WM_COMPAREITEM => {
	id => 0x0039, result => "int", wparam => "UINT", lparam => "const COMPAREITEMSTRUCT *" },
    WM_CREATE => {
	id => 0x0001, result => "BOOL", wparam => "void", lparam => "LPCREATESTRUCT" },
    WM_CTLCOLOR => {
        id => 0x0019, result => "", wparam => "", lparam => "" },
    WM_CTLCOLORBTN => {
	id => 0x0135, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CTLCOLORDLG => {
	id => 0x136, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CTLCOLOREDIT => {
	id => 0x133, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CTLCOLORLISTBOX => {
	id => 0x134, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CTLCOLORMSGBOX => {
	id => 0x132, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CTLCOLORSCROLLBAR => {
	id => 0x137, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CTLCOLORSTATIC => {
	id => 0x138, result => "HBRUSH", wparam => "HDC", lparam => "HWND" },
    WM_CUT => {
	id => 0x0300, result => "void", wparam => "void", lparam => "void" },

    WM_DDE_ACK => { # FIXME: Only correct if replying to WM_DDE_INITIATE
	id => 0x03E4, result => "void", wparam => "HWND", lparam => ["ATOM", "ATOM"] },
    WM_DDE_INITIATE => {
	id => 0x03E0, result => "void", wparam => "HWND", lparam => ["ATOM", "ATOM"] },
    WM_DEADCHAR => {
	id => 0x0103, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_DEVICECHANGE => {
        id => 0x0219, result => "BOOL", wparam => "UINT", lparam => "DWORD" },
    WM_DELETEITEM => {
	id => 0x002d, result => "void", wparam => "UINT", lparam => "const DELETEITEMSTRUCT *" },
    WM_DEVMODECHANGE => {
	id => 0x001b, result => "void", wparam => "void", lparam => "LPCTSTR" },
    WM_DESTROY => {
	id => 0x0002, result => "void", wparam => "void", lparam => "void" },
    WM_DESTROYCLIPBOARD => {
        id => 0x0307, result => "void", wparam => "void", lparam => "void" },
    WM_DISABLEMODAL => {
        id => 0x036c, result => "", wparam => "", lparam => "" },
    WM_DISPLAYCHANGE => {
        id => 0x007e, result => "void", wparam => "UINT", lparam => ["UINT", "UINT"] },
    WM_DRAGLOOP => {
        id => 0x022d, result => "", wparam => "", lparam => "" },
    WM_DRAGMOVE => {
        id => 0x022f, result => "", wparam => "", lparam => "" },
    WM_DRAGSELECT => {
        id => 0x022e, result => "", wparam => "", lparam => "" },
    WM_DRAWCLIPBOARD => {
        id => 0x0308, result => "void", wparam => "void", lparam => "void" },
    WM_DRAWITEM => {
	id => 0x002b, result => "void", wparam => "UINT", lparam => "const DRAWITEMSTRUCT *" },
    WM_DROPFILES => {
	id => 0x0233, result => "void", wparam => "HDROP", lparam => "void" },
    WM_DROPOBJECT => {
        id => 0x022a, result => "", wparam => "", lparam => "" },

    WM_ENABLE => {
	id => 0x000a, result => "void", wparam => "BOOL", lparam => "void" },
    WM_ENDSESSION => {
	id => 0x0016, result => "void", wparam => "BOOL", lparam => "void" },
    WM_ENTERIDLE => {
	id => 0x0121, result => "void", wparam => "UINT", lparam => "HWND" },
    WM_ENTERSIZEMOVE => {
        id => 0x0231, result => "", wparam => "", lparam => "" },
    WM_ENTERMENULOOP => {
	id => 0x0211, result => "", wparam => "", lparam => "" },
    WM_ERASEBKGND => {
	id => 0x0014, result => "BOOL", wparam => "HDC", lparam => "void" },
    WM_EXITHELPMODE => {
        id => 0x0367, result => "", wparam => "", lparam => "" },
    WM_EXITMENULOOP => {
	id => 0x0212, result => "", wparam => "", lparam => "" },
    WM_EXITSIZEMOVE => {
        id => 0x0232, result => "", wparam => "", lparam => "" },

    WM_FILESYSCHANGE => {
        id => 0x0034, result => "", wparam => "", lparam => "" },
    WM_FLOATSTATUS => {
        id => 0x036d, result => "", wparam => "", lparam => "" },
    WM_FONTCHANGE => {
	id => 0x001d, result => "void", wparam => "void", lparam => "void" },

    WM_GETDLGCODE => {
        id => 0x0087, result => "UINT", wparam => "WPARAM", lparam => "LPMSG" },
    WM_GETFONT => {
        id => 0x0031, result => "HFONT", wparam => "void", lparam => "void" },
    WM_GETHOTKEY => {
        id => 0x0033, result => "", wparam => "", lparam => "" },
    WM_GETICON => {
        id => 0x007f, result => "", wparam => "", lparam => "" },
    WM_GETMINMAXINFO => {
        id => 0x0024, result => "void", wparam => "void", lparam => "LPMINMAXINFO" },
    WM_GETTEXT => {
	id => 0x000d, result => "int", wparam => "int", lparam => "LPTSTR" },
    WM_GETTEXTLENGTH => {
	id => 0x000e, result => "int", wparam => "void", lparam => "void" },

    WM_HELP => {
	id => 0x0053, result => "void", wparam => "void", lparam => "LPHELPINFO" },
    WM_HELPHITTEST => {
        id => 0x0366, result => "", wparam => "", lparam => "" },
    WM_HOTKEY => {
        id => 0x0312, result => "", wparam => "", lparam => "" },
    WM_HSCROLL => {
	id => 0x0114, result => "void", wparam => ["int", "int"], lparam => "HWND" },
    WM_HSCROLLCLIPBOARD => {
        id => 0x030e, result => "void", wparam => "HWND", lparam => "" },

    WM_ICONERASEBKGND => {
	id => 0x0027, result => "BOOL", wparam => "HDC", lparam => "void" },
    WM_IME_CHAR => {
        id => 0x0286, result => "", wparam => "", lparam => "" },
    WM_IME_COMPOSITION => {
        id => 0x010f, result => "", wparam => "", lparam => "" },
    WM_IME_COMPOSITIONFULL => {
        id => 0x0284, result => "", wparam => "", lparam => "" },
    WM_IME_CONTROL => {
        id => 0x0283, result => "", wparam => "", lparam => "" },
    WM_IME_ENDCOMPOSITION => {
        id => 0x010e, result => "", wparam => "", lparam => "" },
    WM_IME_KEYDOWN => {
	id => 0x0290, result => "void", wparam => "UINT", lparam => ["int", "UINT"] },
    WM_IME_KEYLAST => {
        id => 0x010f, result => "", wparam => "", lparam => "" },
    WM_IME_KEYUP => {
	id => 0x0291, result => "void", wparam => "UINT", lparam => ["int", "UINT"] },
    WM_IME_NOTIFY => {
        id => 0x0282, result => "", wparam => "", lparam => "" },
    WM_IME_REQUEST => {
        id => 0x0288, result => "", wparam => "", lparam => "" },
    WM_IME_SELECT => {
        id => 0x0285, result => "", wparam => "", lparam => "" },
    WM_IME_SETCONTEXT => {
        id => 0x0281, result => "", wparam => "", lparam => "" },
    WM_IME_STARTCOMPOSITION => {
        id => 0x010d, result => "", wparam => "", lparam => "" },
    WM_IDLEUPDATECMDUI => {
        id => 0x0363, result => "", wparam => "", lparam => "" },
    WM_INITDIALOG => {
        id => 0x0110, result => "BOOL", wparam => "HWND", lparam => "LPARAM" },
    WM_INITIALUPDATE => {
        id => 0x0364, result => "", wparam => "", lparam => "" },
    WM_INITMENU => {
	id => 0x0116, result => "void", wparam => "HMENU", lparam => "void" },
    WM_INITMENUPOPUP => {
	id => 0x0117, result => "void", wparam => "HMENU", lparam => ["UINT", "BOOL"] },
    WM_INPUTLANGCHANGE => {
        id => 0x0051, result => "", wparam => "", lparam => "" },
    WM_INPUTLANGCHANGEREQUEST => {
        id => 0x0050, result => "", wparam => "", lparam => "" },
    WM_ISACTIVEICON => {
	id => 0x0035, result => "", wparam => "", lparam => "" },

    WM_KEYDOWN => {
	id => 0x0100, result => "void", wparam => "UINT", lparam => ["int", "UINT"] },
    WM_KEYLAST => {
	id => 0x0108, result => "", wparam => "", lparam => "" },
    WM_KICKIDLE => {
	id => 0x036a, result => "", wparam => "", lparam => "" },
    WM_KEYUP => {
	id => 0x0101, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_KILLFOCUS => {
	id => 0x0008, result => "void", wparam => "HWND", lparam => "void" },

    WM_LBTRACKPOINT => {
	id => 0x0131, result => "", wparam => "", lparam => "" },
    WM_LBUTTONDBLCLK => {
	id => 0x0203, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_LBUTTONDOWN => {
	id => 0x0201, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_LBUTTONUP => {
	id => 0x0202, result => "void", wparam => "UINT", lparam => ["", ""] },

    WM_MBUTTONDBLCLK => {
	id => 0x0209, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MBUTTONDOWN => {
	id => 0x0207, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MBUTTONUP => {
	id => 0x0208, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MDIACTIVATE => {
	id => 0x0222, result => "void", wparam => "HWND", lparam => "HWND" },
    WM_MDICASCADE => {
	id => 0x0227, result => "BOOL", wparam => "UINT", lparam => "void" },
    WM_MDICREATE => {
	id => 0x0220, result => "HWND", wparam => "void", lparam => "const LPMDICREATESTRUCT" },
    WM_MDIDESTROY => {
	id => 0x0221, result => "void", wparam => "HWND", lparam => "void" },
    WM_MDIGETACTIVE => {
	id => 0x0229, result => "HWND", wparam => "void", lparam => "void" },
    WM_MDIICONARRANGE => {
	id => 0x0228, result => "void", wparam => "void", lparam => "void" },
    WM_MDIMAXIMIZE => {
	id => 0x0225, result => "void", wparam => "HWND", lparam => "void" },
    WM_MDINEXT => {
	id => 0x0224, result => "HWND", wparam => "HWND", lparam => "BOOL" },
    WM_MDIREFRESHMENU => {
	id => 0x0234, result => "", wparam => "", lparam => "" },
    WM_MDIRESTORE => {
	id => 0x0223, result => "void", wparam => "HWND", lparam => "void" },
    WM_MDISETMENU => {
	id => 0x0230, result => "HMENU", wparam => "BOOL", lparam => "HMENU" },
    WM_MDITILE => {
	id => 0x0226, result => "BOOL", wparam => "UINT", lparam => "void" },
    WM_MEASUREITEM => {
	id => 0x002c, result => "void", wparam => "UINT", lparam => "MEASUREITEMSTRUCT *" },
    WM_MENUSELECT => {
	id => 0x011f, result => "void", wparam => ["", ""], lparam => "HMENU" },
    WM_MENUCHAR => {
	id => 0x0120, result => "DWORD", wparam => ["", "WORD"], lparam => "HMENU" },
    WM_MOUSEACTIVATE => {
	id => 0x0021, result => "int", wparam => "HWND", lparam => ["", ""] },
    WM_MOUSEMOVE => {
	id => 0x0200, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MOUSEWHEEL => {
	id => 0x020a, result => "void", wparam => ["DWORD", "int"], lparam => ["UINT", "UINT"] },
    WM_MOVE => {
	id => 0x0003, result => "void", wparam => "void", lparam => ["", ""] },
    WM_MOVING => {
	id => 0x0216, result => "", wparam => "", lparam => "" },

    WM_NCACTIVATE => {
	id => 0x0086, result => "BOOL", wparam => "BOOL", lparam => "void" },
    WM_NCLBUTTONDBLCLK => {
	id => 0x00a3, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCLBUTTONDOWN => {
	id => 0x00a1, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCLBUTTONUP => {
	id => 0x00a2, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMOUSEMOVE => {
	id => 0x00a0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMBUTTONDBLCLK => {
	id => 0x00a9, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMBUTTONDOWN => {
	id => 0x00a7, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMBUTTONUP => {
	id => 0x00a8, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCRBUTTONDBLCLK => {
	id => 0x00a6, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCRBUTTONDOWN => {
	id => 0x00a4, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCRBUTTONUP => {
	id => 0x00a5, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCCALCSIZE => {
	id => 0x0083, result => "UINT", wparam => "void", lparam => "NCCALCSIZE_PARAMS *" },
    WM_NCCREATE => {
	id => 0x0081, result => "BOOL", wparam => "void", lparam => "LPCREATESTRUCT" },
    WM_NCDESTROY => {
	id => 0x0082, result => "void", wparam => "void", lparam => "void" },
    WM_NCHITTEST => {
	id => 0x0084, result => "UINT", wparam => "void", lparam => ["int", "int"] },
    WM_NCPAINT => {
	id => 0x0085, result => "void", wparam => "HRGN", lparam => "void" },
    WM_NEXTDLGCTL => {
	id => 0x0028, result => "HWND", wparam => "HWND", lparam => "BOOL" },
    WM_NEXTMENU => {
	id => 0x0213, result => "void", wparam => "UINT", lparam => "LPMDINEXTMENU" },
    WM_NOTIFY => {
	id => 0x004e, result => "LRESULT", wparam => "int", lparam => "NMHDR *" },
    WM_NOTIFYFORMAT => {
	id => 0x0055, result => "", wparam => "", lparam => "" },
    WM_NULL => {
	id => 0x0000, result => "", wparam => "", lparam => "" },

    WM_OCC_INITNEW => {
	id => 0x0378, result => "", wparam => "", lparam => "" },
    WM_OCC_LOADFROMSTORAGE => {
	id => 0x0377, result => "", wparam => "", lparam => "" },
    WM_OCC_LOADFROMSTORAGE_EX => {
	id => 0x037b, result => "", wparam => "", lparam => "" },
    WM_OCC_LOADFROMSTREAM => {
	id => 0x0376, result => "", wparam => "", lparam => "" },
    WM_OCC_LOADFROMSTREAM_EX => {
	id => 0x037a, result => "", wparam => "", lparam => "" },
    WM_OTHERWINDOWCREATED => {
	id => 0x003c, result => "", wparam => "", lparam => "" },
    WM_OTHERWINDOWDESTROYED => {
	id => 0x003d, result => "", wparam => "", lparam => "" },

    WM_PAINT => {
	id => 0x000f, result => "void", wparam => "void", lparam => "void" },
    WM_PAINTCLIPBOARD => {
	id => 0x0309, result => "void", wparam => "HWND", lparam => "const LPPAINTSTRUCT" },
    WM_PAINTICON => {
	id => 0x0026, result => "", wparam => "", lparam => "" },
    WM_PALETTEISCHANGING => {
	id => 0x0310, result => "void", wparam => "HWND", lparam => "void" },
    WM_PALETTECHANGED => {
	id => 0x0311, result => "void", wparam => "HWND", lparam => "void" },
    WM_PARENTNOTIFY => {
	id => 0x0210, result => "void", wparam => ["UINT", "int"], lparam => "HWND" },
    WM_PASTE => {
	id => 0x0302, result => "void", wparam => "void", lparam => "void" },
    WM_PENWINFIRST => {
	id => 0x0380, result => "", wparam => "", lparam => "" },
    WM_PENWINLAST => {
	id => 0x038f, result => "", wparam => "", lparam => "" },
    WM_POPMESSAGESTRING => {
	id => 0x0375, result => "", wparam => "", lparam => "" },
    WM_POWER => {
	id => 0x0048, result => "void", wparam => "int", lparam => "void" },
    WM_POWERBROADCAST => {
	id => 0x0218, result => "", wparam => "", lparam => "" },
    WM_PRINT => {
	id => 0x0317, result => "", wparam => "", lparam => "" },
    WM_PRINTCLIENT => {
	id => 0x0318, result => "void", wparam => "HDC", lparam => "DWORD" },

    WM_QUERY3DCONTROLS => {
	id => 0x036f, result => "", wparam => "", lparam => "" },
    WM_QUERYAFXWNDPROC => {
	id => 0x0360, result => "", wparam => "", lparam => "" },
    WM_QUERYCENTERWND => {
	id => 0x036b, result => "", wparam => "", lparam => "" },
    WM_QUERYDRAGICON => {
	id => 0x0037, result => "HICON", wparam => "void", lparam => "void" },
    WM_QUERYDROPOBJECT => {
	id => 0x022b, result => "", wparam => "", lparam => "" },
    WM_QUERYENDSESSION => {
	id => 0x0011, result => "BOOL", wparam => "void", lparam => "void" },
    WM_QUERYNEWPALETTE => {
	id => 0x030f, result => "BOOL", wparam => "void", lparam => "void" },
    WM_QUERYOPEN => {
	id => 0x0013, result => "BOOL", wparam => "void", lparam => "void" },
    WM_QUERYPARKICON => {
	id => 0x0036, result => "", wparam => "", lparam => "" },
    WM_QUERYSAVESTATE => {
	id => 0x0038, result => "", wparam => "", lparam => "" },
    WM_QUEUESYNC => {
	id => 0x0023, result => "void", wparam => "void", lparam => "void" },
    WM_QUEUE_SENTINEL => {
	id => 0x0379, result => "", wparam => "", lparam => "" },
    WM_QUIT => {
	id => 0x0012, result => "void", wparam => "int", lparam => "void" },

    WM_RBUTTONDBLCLK => {
	id => 0x0206, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_RBUTTONDOWN => {
	id => 0x0204, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_RBUTTONUP => {
	id => 0x0205, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_RECALCPARENT => {
	id => 0x0368, result => "", wparam => "", lparam => "" },
    WM_RENDERALLFORMATS => {
	id => 0x0306, result => "void", wparam => "void", lparam => "void" },
    WM_RENDERFORMAT => {
	id => 0x0305, result => "HANDLE", wparam => "UINT", lparam => "void" },

    WM_SETCURSOR => {
	id => 0x0020, result => "BOOL", wparam => "HWND", lparam => ["UINT", "UINT"] },
    WM_SETFOCUS => {
	id => 0x0007, result => "void", wparam => "HWND", lparam => "void" },
    WM_SETFONT => {
	id => 0x0030, result => "void", wparam => "HFONT", lparam => "BOOL" },
    WM_SETHOTKEY => {
	id => 0x0032, result => "", wparam => "", lparam => "" },
    WM_SETICON => {
	id => 0x0080, result => "HICON", wparam => "DWORD", lparam => "HICON" },
    WM_SETMESSAGESTRING => {
	id => 0x0362, result => "", wparam => "", lparam => "" },
    WM_SETREDRAW => {
	id => 0x000b, result => "void", wparam => "BOOL", lparam => "void" },
    WM_SETTEXT => {
	id => 0x000c, result => "void", wparam => "void", lparam => "LPCTSTR" },
    WM_SETVISIBLE => {
	id => 0x0009, result => "", wparam => "", lparam => "" },
    WM_SHOWWINDOW => {
	id => 0x0018, result => "void", wparam => "BOOL", lparam => "UINT" },
    WM_SIZE => {
	id => 0x0005, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_SIZECHILD => {
	id => 0x0369, result => "", wparam => "", lparam => "" },
    WM_SIZECLIPBOARD => {
	id => 0x030b, result => "void", wparam => "HWND", lparam => "const LPRECT" },
    WM_SIZEPARENT => {
	id => 0x0361, result => "", wparam => "", lparam => "" },
    WM_SIZEWAIT => {
	id => 0x0004, result => "", wparam => "", lparam => "" },
    WM_SIZING => {
	id => 0x0214, result => "", wparam => "", lparam => "" },
    WM_SOCKET_DEAD => {
	id => 0x0374, result => "", wparam => "", lparam => "" },
    WM_SOCKET_NOTIFY => {
	id => 0x0373, result => "", wparam => "", lparam => "" },
    WM_SPOOLERSTATUS => {
	id => 0x002a, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_STYLECHANGED => {
	id => 0x007d, result => "void", wparam => "DWORD", lparam => "LPSTYLESTRUCT" },
    WM_STYLECHANGING => {
	id => 0x007c, result => "void", wparam => "DWORD", lparam => "LPSTYLESTRUCT" },
    WM_SYNCPAINT => {
	id => 0x0088, result => "", wparam => "", lparam => "" },
    WM_SYNCTASK => {
	id => 0x0089, result => "", wparam => "", lparam => "" },
    WM_SYSCHAR => {
	id => 0x0106, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_SYSCOLORCHANGE => {
	id => 0x0015, result => "void", wparam => "void", lparam => "void" },
    WM_SYSCOMMAND => {
	id => 0x0112, result => "void", wparam => "UINT", lparam => "int" },
    WM_SYSDEADCHAR => {
	id => 0x0107, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_SYSKEYDOWN => {
	id => 0x0104, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_SYSKEYUP => {
	id => 0x0105, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_SYSTEMERROR => {
	id => 0x0017, result => "", wparam => "", lparam => "" },
    WM_SYSTIMER => {
	id => 0x0118, result => "", wparam => "", lparam => "" },

    WM_TCARD => {
	id => 0x0052, result => "", wparam => "", lparam => "" },
    WM_TESTING => {
	id => 0x003a, result => "", wparam => "", lparam => "" },
    WM_TIMECHANGE => {
	id => 0x001e, result => "void", wparam => "void", lparam => "void" },
    WM_TIMER => {
	id => 0x0113, result => "void", wparam => "UINT", lparam => "void" },

    WM_UNDO => {
	id => 0x0304, result => "void", wparam => "void", lparam => "void" },
    WM_USER => {
	id => 0x0400, result => "", wparam => "", lparam => "" },
    WM_USERCHANGED => {
	id => 0x0054, result => "", wparam => "", lparam => "" },

    WM_VKEYTOITEM => {
	id => 0x002e, result => "int", wparam => ["UINT", "int"], lparam => "HWND" },
    WM_VSCROLL => {
	id => 0x0115, result => "void", wparam => ["int", "int"], lparam => "HWND" },
    WM_VSCROLLCLIPBOARD => {
	id => 0x030a, result => "void", wparam => "HWND", lparam => ["", ""] },

    WM_WINDOWPOSCHANGING => {
	id => 0x0046, result => "BOOL", wparam => "void", lparam => "LPWINDOWPOS" },
    WM_WINDOWPOSCHANGED => {
	id => 0x0047, result => "void", wparam => "void", lparam => "const LPWINDOWPOS" },
    WM_WININICHANGE => {
	id => 0x001a, result => "void", wparam => "void", lparam => "LPCTSTR" }
};

########################################################################
# _get_kind

sub _get_kind($) {
    local $_ = shift;

    if(!$_) {
	return undef;
    } elsif(/^(?:HBRUSH|HDC|HFONT|HMENU|HRGN|HWND)$/ || /\*$/ ||
	    /^LP(?!ARAM)/)
    {
	return "ptr";
    } else {
	return "long";
    }
}

########################################################################
# get_message_result_type

sub get_message_result_type($) {
    my $name = shift;
    return $$message{$name}{result};
}

########################################################################
# get_message_result_kind

sub get_message_result_kind(@) {
    return _get_kind(get_message_result_type(@_));
}

########################################################################
# get_message_wparam_type

sub get_message_wparam_type($) {
    my $name = shift;
    return $$message{$name}{wparam};
}

########################################################################
# get_message_wparam_kind

sub get_message_wparam_kind(@) {
    return _get_kind(get_message_wparam_type(@_));
}

########################################################################
# get_message_lparam_type

sub get_message_lparam_type($) {
    my $name = shift;
    return $$message{$name}{lparam};
}

########################################################################
# get_message_lparam_kind

sub get_message_lparam_kind(@) {
    return _get_kind(get_message_lparam_type(@_));
}

########################################################################
# _parse_file

sub _parse_file($$$) {
    my $file = shift;
    my $found_preprocessor = shift;
    my $found_comment = shift;

    {
	open(IN, "< $file") || die "Error: Can't open $file: $!\n";
	local $/ = undef;
	$_ = <IN>;
	close(IN);
    }

    my @lines = split(/\n/, $_);
    my $max_line = scalar(@lines);

    my $parser = new c_parser($file);

    $parser->set_found_preprocessor_callback($found_preprocessor);
    $parser->set_found_comment_callback($found_comment);

    my $found_line = sub {
	my $line = shift;
	local $_ = shift;

	$output->progress("$file: line $line of $max_line");
    };

    $parser->set_found_line_callback($found_line);

    my $line = 1;
    my $column = 0;

    my $old_prefix = $output->prefix;
    $output->progress("$file");
    $output->prefix("$file: ");

    if(!$parser->parse_c_file(\$_, \$line, \$column)) {
	$output->write("can't parse file\n");
    }

    $output->prefix($old_prefix);
}

########################################################################
# _get_tuple_arguments

sub _get_tuple_arguments($) {
    local $_ = shift;

    my $parser = new c_parser;

    my $line = 1;
    my $column = 0;

    my @arguments;
    my @argument_lines;
    my @argument_columns;
    if(!$parser->parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	return undef;
    }

    return @arguments;
}

########################################################################
# _parse_windowsx_h


sub _parse_windowsx_h($$$) {
    my $last_comment;

    my $found_preprocessor = sub {
	my $begin_line = shift;
	my $begin_column = shift;
	local $_ = shift;

	if(!s/^\#\s*define\s*// || !/^FORWARD_WM_/) {
	    return 1;
	}

	my $name;
	if(s/^FORWARD_(\w+)\([^\)]*\)\s*(.*?)\s*$/$2/s) {
	    $name = $1;
	}

	if($name eq "WM_SYSTEMERROR") {
	    return 1;
	}

	my $result;
	if(s/^\(\s*(\w+)\s*\)(?:\(\s*\w+\s*\))*\(\s*\w+\s*\)//) {
	    $result = $1;
	} else {
	    die "$name: '$_'";
	}

	(undef, $_, my $wparam, my $lparam) = _get_tuple_arguments($_);

	my @names = ();
	if(/^$name$/) {
	    @names = $name;
	} elsif(/^\(\w+\)\s*\?\s*(\w+)\s*:\s*(\w+)$/) {
	    @names = ($1, $2);
	} else {
	    die "$name: '$_'";
	}

	local $_ = $last_comment;
	s%^/\*\s*(.*?)\s*\*/$%$1%;

	my %arguments;
	if(s/^(\w+)\s+\w+\s*\(\s*(.*?)\s*\)$/$2/) {
	    my $result2 = $1;
	    if($result2 eq "INT") { $result2 = "int"; }
	    if($result ne $result2) {
		$output->write("message $name: result type mismatch '$result' != '$result2'\n");
	    }
	    foreach (split(/\s*,\s*/)) {
		if(/^((?:const\s+|volatile\s+)?\w+(?:\s*\*\s*|\s+)?)(\w+)$/) {
		    my $type = $1;
		    my $name = $2;

		    $type =~ s/^\s*(.*?)\s*$/$1/;

		    $arguments{$name} = $type;
		} else {
		    die "$name: '$_'";
		}
	    }
	    # $output->write("$1: $_\n");
	} else {
	    die "$name: '$_'";
	}

	my $find_inner_cast = sub {
	    local $_ = shift;
	    if(/^(?:\(\s*((?:const\s+|volatile\s+)?\w+(?:\s*\*)?)\s*\))*\(.*?\)$/) {
		if(defined($1)) {
		    return $1;
		} else {
		    return "";
		}
	    }

	};

	my @entries = (
	    [ \$wparam, "W", "w" ],
	    [ \$lparam, "L", "l" ]
	);
	foreach my $entry (@entries) {
	    (my $refparam, my $upper, my $lower) = @$entry;

	    local $_ = $$refparam;
	    if(s/^\(\s*$upper(?:)PARAM\s*\)\s*(?:\(\s*((?:const\s+|volatile\s+)?\w+(?:\s*\*)?)\s*\))*\(\s*(.*?)\s*\)$/$2/) {
		if(defined($1)) {
		    $$refparam = $1;
		} else {
		    $$refparam = "";
		}

		if(/^\w+$/) {
		    if(exists($arguments{$_})) {
			$$refparam = $arguments{$_};
		    }
		} elsif(/^\(\s*(\w+)\s*\)\s*\?\s*\(\s*(\w+)\s*\)\s*:\s*(?:\(\s*(\w+)\s*\)|0)$/) {
		    foreach ($1, $2, $3) {
			if(exists($arguments{$_})) {
			    $$refparam = $arguments{$_};
			    last;
			}
		    }
		} elsif(/^\(\((?:const\s+|volatile\s+)?\w+\s*(?:\*\s*)?\)\s*(?:\(\s*\w+\s*\)|\w+)\s*\)\s*\->\s*\w+$/) {
		    $$refparam = "UINT";
		} else {
		    die "$name: '$_'";
		}
	    } elsif(s/^(?:\(\s*$upper(?:)PARAM\s*\)\s*)?MAKE$upper(?:)PARAM\s*//) {
                (my $low, my $high) = _get_tuple_arguments($_);

		$low = &$find_inner_cast($low);
		$high = &$find_inner_cast($high);

		$$refparam = "($low,$high)";
	    } elsif(s/^\(.*?$lower(?:)Param.*?\)$//) {
		$$refparam = $upper . "PARAM";
	    } elsif(s/^\(\s*(.*?)\s*\)$//) {
		$$refparam = "$1";
	    } elsif(s/^0L$//) {
		$$refparam = "void";
	    } else {
		die "$name: '$_'";
            }
        }

	# $output->write("$result: '@names', '$wparam', '$lparam'\n");

        foreach my $name (@names) {
	    my $result2 = $$message{$name}{result};
            my $wparam2 = $$message{$name}{wparam};
	    my $lparam2 = $$message{$name}{lparam};

	    if(ref($wparam2)) {
		$wparam2 = "(" . join(",", @$wparam2) . ")";
	    }

	    if(ref($lparam2)) {
		$lparam2 = "(" . join(",", @$lparam2) . ")";
	    }

	    if($result ne $result2) {
		$output->write("message $name: wrong result type '$result2' should be '$result'\n");
	    }

	    if($wparam ne $wparam2) {
		# if($wparam ne "WPARAM" && $wparam ne "(,)") {
		    $output->write("message $name: wrong wparam type '$wparam2' should be '$wparam'\n");
		# }
	    }

	    if($lparam ne $lparam2) {
		# if($lparam ne "LPARAM" && $lparam ne "(,)") {
		    $output->write("message $name: wrong lparam type '$lparam2' should be '$lparam'\n");
		# }
	    }
	}

	return 1;
    };

    my $found_comment = sub {
	my $begin_line = shift;
	my $begin_column = shift;
	my $comment = shift;

	$last_comment = $comment;

	return 1;
    };

    _parse_file("$wine_dir/include/windowsx.h", $found_preprocessor, $found_comment);
}


########################################################################
# _parse_winuser_h

sub _parse_winuser_h($$$) {
    my %not_found = ();

    my $found_preprocessor = sub {
	my $begin_line = shift;
	my $begin_column = shift;
	local $_ = shift;

	if(/^\#\s*define\s+(WM_\w+)\s+(0x[0-9a-fA-F]+)\s*$/) {
	    my $name = $1;
	    my $id = lc($2);

	    if(exists($$message{$name})) {
		my $id2 = sprintf("0x%04x", $$message{$name}{id});
		if($id ne $id2) {
		    $output->write("message $name: wrong value ($id2) should be $id\n");
		}
	    } else {
		$output->write("message $name: exists but is not supported\n");
		$not_found{$name} = $id;
	    }
	}

	return 1;
    };

    _parse_file("$wine_dir/include/winuser.h", $found_preprocessor);

    foreach my $name (sort(keys(%not_found))) {
	my $id = $not_found{$name};

	print "    $name => {\n";
	print "\tid => $id, result => \"\", wparam => \"\", lparam => \"\" },\n";
    }
}

# _parse_windowsx_h;
# _parse_winuser_h;

1;
