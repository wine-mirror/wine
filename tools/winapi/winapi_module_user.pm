package winapi_module_user;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw(
    &is_user_function
    &get_message_result_type
    &get_message_result_kind
    &get_message_wparam_type
    &get_message_wparam_kind
    &get_message_lparam_type
    &get_message_lparam_kind
);

use config qw($wine_dir);
use options qw($options);
use output qw($output);

use c_parser;

########################################################################

my $message;

########################################################################
# is_user_function

sub is_user_function {
    my $name = shift;
    if($name =~ /^(?:DefWindowProc|SendMessage)[AW]?$/) {
    }
}

########################################################################
# $message

$message = {
    WM_ACTIVATE => {
	id => 0, result => "void", wparam => ["", ""], lparam => "HWND" },
    WM_ACTIVATEAPP => {
	id => 0, result => "void", wparam => "BOOL", lparam => "LPARAM" },

    WM_BEGINDRAG => {
	id => 0, result => "", wparam => "", lparam => "" },

    WM_CANCELMODE => {
	id => 0, result => "void", wparam => "void", lparam => "void" },
    WM_CHAR => {
	id => 0, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_CHARTOITEM => {
	id => 0x002f, result => "void", wparam => ["UINT", "int"], lparam => "HWND" },
    WM_CLOSE => {
	id => 0, result => "void", wparam => "void", lparam => "void" },
    WM_COMMAND => {
	id => 0, result => "void", wparam => ["int", "UINT"], lparam => "HWND" },
    WM_COPY => {
	id => 0x0301, result => "void", wparam => "void", lparam => "void" },
    WM_COMPACTING => { 
	id => 0, result => "void", wparam => "UINT", lparam => "void" },
    WM_COMPAREITEM => { 
	id => 0, result => "int", wparam => "UINT", lparam => "const COMPAREITEMSTRUCT *" },

    WM_CREATE => {
	id => 0, result => "BOOL", wparam => "void", lparam => "LPCREATESTRUCT" },
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
	id => 0, result => "void", wparam => "void", lparam => "void" },

    WM_DEADCHAR => {
	id => 0, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_DELETEITEM => { 
	id => 0, result => "void", wparam => "UINT", lparam => "const DELETEITEMSTRUCT *" },
    WM_DEVMODECHANGE => {
	id => 0, result => "void", wparam => "void", lparam => "LPCTSTR" },
    WM_DESTROY => {
	id => 0, result => "void", wparam => "void", lparam => "void" },
    WM_DRAWITEM => {
	id => 0, result => "void", wparam => "void", lparam => "const DRAWITEMSTRUCT *" },
    WM_DROPFILES => {
	id => 0, result => "void", wparam => "HDROP", lparam => "void" },

    WM_ENABLE => {
	id => 0, result => "void", wparam => "BOOL", lparam => "void" },
    WM_ENDSESSION => {
	id => 0, result => "void", wparam => "BOOL", lparam => "void" },
    WM_ENTERIDLE => {
	id => 0x0121, result => "void", wparam => "UINT", lparam => "HWND" },
    WM_ENTERMENULOOP => {
	id => 0x0211, result => "", wparam => "", lparam => "" },
    WM_ERASEBKGND => {
	id => 0, result => "BOOL", wparam => "HDC", lparam => "void" },
    WM_EXITMENULOOP => {
	id => 0x0212, result => "", wparam => "", lparam => "" },

    WM_FONTCHANGE => {
	id => 0, result => "void", wparam => "void", lparam => "void" },

    WM_GETTEXT => {
	id => 0, result => "int", wparam => "int", lparam => "LPTSTR" },
    WM_GETTEXTLENGTH => {
	id => 0, result => "int", wparam => "void", lparam => "void" },

    WM_HELP => {
	id => 0x0053, result => "", wparam => "", lparam => "" },
    WM_HSCROLL => {
	id => 0, result => "void", wparam => ["UINT", "int"], lparam => "HWND" },

    WM_ICONERASEBKGND => {
	id => 0, result => "BOOL", wparam => "HDC", lparam => "void" },
    WM_INITMENU => {
	id => 0, result => "void", wparam => "HMENU", lparam => "void" },
    WM_INITMENUPOPUP => {
	id => 0, result => "void", wparam => "HMENU", lparam => ["UINT", "BOOL"] },
    WM_ISACTIVEICON => {
	id => 0, result => "", wparam => "", lparam => "" },

    WM_KEYDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_KEYUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_KILLFOCUS => {
	id => 0, result => "void", wparam => "HWND", lparam => "void" },

    WM_LBTRACKPOINT => {
	id => 0, result => "", wparam => "", lparam => "" },
    WM_LBUTTONDBLCLK => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_LBUTTONDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_LBUTTONUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },

    WM_MBUTTONDBLCLK => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MBUTTONDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MBUTTONUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MEASUREITEM => {
	id => 0, result => "void", wparam => "UINT", lparam => "MEASUREITEMSTRUCT *" },
    WM_MENUSELECT => {
	id => 0, result => "void", wparam => ["", ""], lparam => "HMENU" },
    WM_MENUCHAR => {
	id => 0, result => "DWORD", wparam => ["", ""], lparam => "HMENU" },
    WM_MOUSEACTIVATE => {
	id => 0, result => "int", wparam => "HWND", lparam => ["", ""] },
    WM_MOUSEMOVE => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_MOVE => {
	id => 0, result => "void", wparam => "void", lparam => ["", ""] },

    WM_NCACTIVATE => {
	id => 0, result => "BOOL", wparam => "BOOL", lparam => "void" },
    WM_NCLBUTTONDBLCLK => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCLBUTTONDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCLBUTTONUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMOUSEMOVE => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMBUTTONDBLCLK => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMBUTTONDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCMBUTTONUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCRBUTTONDBLCLK => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCRBUTTONDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCRBUTTONUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_NCCALCSIZE => {
	id => 0, result => "UINT", wparam => "void", lparam => "LPARAM" },
    WM_NCCREATE => {
	id => 0, result => "BOOL", wparam => "void", lparam => "LPCREATESTRUCT" },
    WM_NCDESTROY => {
	id => 0, result => "void", wparam => "void", lparam => "void" },
    WM_NCPAINT => {
	id => 0, result => "void", wparam => "HRGN", lparam => "void" },
    WM_NEXTMENU => {
	id => 0x0213, result => "", wparam => "", lparam => "" },
    WM_NOTIFY => {
	id => 0x004e, result => "LRESULT", wparam => "int", lparam => "NMHDR *" },


    WM_PALETTEISCHANGING => {
	id => 0, result => "void", wparam => "HWND", lparam => "void" },
    WM_PALETTECHANGED => {
	id => 0, result => "void", wparam => "HWND", lparam => "void" },
    WM_PAINT => {
	id => 0, result => "void", wparam => "void", lparam => "void" },
    WM_PASTE => {
	id => 0x0302, result => "void", wparam => "void", lparam => "void" },
    WM_POWER => {
	id => 0, result => "void", wparam => "int", lparam => "void" },

    WM_QUERYDRAGICON => {
	id => 0, result => "HICON", wparam => "void", lparam => "void" },
    WM_QUERYENDSESSION => {
	id => 0, result => "BOOL", wparam => "void", lparam => "void" },
    WM_QUERYNEWPALETTE => {
	id => 0, result => "BOOL", wparam => "void", lparam => "void" },
    WM_QUERYOPEN => {
	id => 0, result => "BOOL", wparam => "void", lparam => "void" },
    WM_QUIT => {
	id => 0, result => "void", wparam => "WPARAM", lparam => "void" },

    WM_RBUTTONDBLCLK => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_RBUTTONDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_RBUTTONUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },

    WM_SETCURSOR => {
	id => 0x0020, result => "", wparam => "HWND", lparam => ["UINT", "UINT"] },
    WM_SETFOCUS => {
	id => 0, result => "void", wparam => "HWND", lparam => "void" },
    WM_SETFONT => {
	id => 0x0030, result => "void", wparam => "HFONT", lparam => "BOOL" },
    WM_SETREDRAW => {
	id => 0, result => "void", wparam => "BOOL", lparam => "void" },
    WM_SETTEXT => {
	id => 0, result => "void", wparam => "void", lparam => "LPCTSTR" },
    WM_SHOWWINDOW => {
	id => 0, result => "void", wparam => "BOOL", lparam => "UINT" },
    WM_SIZE => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_SPOOLERSTATUS => {
	id => 0, result => "void", wparam => "WPARAM", lparam => ["", ""] },
    WM_SYSCHAR => {
	id => 0, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_SYSCOLORCHANGE => {
	id => 0, result => "void", wparam => "void", lparam => "void" },
    WM_SYSDEADCHAR => {
	id => 0, result => "void", wparam => "TCHAR", lparam => ["", ""] },
    WM_SYSKEYDOWN => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },
    WM_SYSKEYUP => {
	id => 0, result => "void", wparam => "UINT", lparam => ["", ""] },

    WM_TIMECHANGE => {
	id => 0, result => "void", wparam => "void", lparam => "void" },

    WM_VKEYTOITEM => {
	id => 0x002e, result => "void", wparam => ["UINT", "int"], lparam => "HWND" },
    WM_VSCROLL => {
	id => 0, result => "void", wparam => ["UINT", "int"], lparam => "HWND" },

    WM_WINDOWPOSCHANGING => {
	id => 0, result => "BOOL", wparam => "void", lparam => "LPWINDOWPOS" },
    WM_WINDOWPOSCHANGED => {
	id => 0, result => "void", wparam => "void", lparam => "LPARAM" },
    WM_WININICHANGE => { 
	id => 0, result => "void", wparam => "void", lparam => "LPCTSTR" }
};

########################################################################
# _get_kind

sub _get_kind {
    local $_ = shift;

    if(!defined($_)) {
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

sub get_message_result_type {
    my $name = shift;
    return $$message{$name}{result};
}

########################################################################
# get_message_result_kind

sub get_message_result_kind {
    return _get_kind(get_message_result_type(@_));
}

########################################################################
# get_message_wparam_type

sub get_message_wparam_type {
    my $name = shift;
    return $$message{$name}{wparam};
}

########################################################################
# get_message_wparam_kind

sub get_message_wparam_kind {
    return _get_kind(get_message_wparam_type(@_));
}

########################################################################
# get_message_lparam_type

sub get_message_lparam_type {
    my $name = shift;
    return $$message{$name}{lparam};
}

########################################################################
# get_message_lparam_kind

sub get_message_lparam_kind {
    return _get_kind(get_message_lparam_type(@_));
}

########################################################################
# _parse_windowsx_h

sub _parse_windowsx_h {
    my $file = "$wine_dir/include/windowsx.h";
    {
	open(IN, "< $file");
	local $/ = undef;
	$_ = <IN>;
	close(IN);
    }

    my $parser = new c_parser($file);

    my $found_preprocessor = sub {
	my $begin_line = shift;
	my $begin_column = shift;
	local $_ = shift;

	if(!s/^\#\s*define\s*// || !/^FORWARD_WM_/) {
	    return 1;
	}
	
	my $msg;
	if(s/^FORWARD_(\w+)\([^\)]*\)\s*(.*?)\s*$/$2/s) {
	    $msg = $1;
	}

	if($msg eq "WM_SYSTEMERROR") {
	    return 1;
	}

	my $return_type;
	if(s/^\(\s*(\w+)\s*\)(?:\(\s*\w+\s*\))*\(\s*\w+\s*\)\(\s*(?:hwnd|\(hwnd\))\s*,\s*(.*?)\s*\)$/$2/) {
	    $return_type = $1;
	} else {
	    die "$msg: '$_'";
	}
	
	my @msgs = ();
	if(s/^$msg\s*,\s*//) {
	    @msgs = $msg;
	} elsif(s/^\(\w+\)\s*\?\s*(\w+)\s*:\s*(\w+)\s*,\s*//s) {
	    @msgs = ($1, $2);
	} else {
	    die "$msg: '$_'";
	}
	
	my $wparam;
	if(s/^\(WPARAM\)(?:\(\s*(\w+)\s*\))*\((.*?)\)\s*,\s*//) {
	    if(defined($1)) {
		$wparam = $1;
	    } else {
		$wparam = "WPARAM";
	    }
	} elsif(s/^MAKEWPARAM\(\s*(.*?)\s*,\s*(.*?)\s*\)\s*,\s*//) {
	    $wparam = "(,)"; # "($1, $2)";
	} elsif(s/^\((.*?)\)$//) {
	    $wparam = "WPARAM";
	} elsif(s/^0L\s*,\s*//) {
	    $wparam = "void";
	} else {
	    die "$msg: '$_'";
	} 

	my $lparam;
	if(s/^\(LPARAM\)(?:\(\s*(\w+)\s*\))*\((.*?)\)$//) {
	    if(defined($1)) {
		$lparam = $1;
	    } else {
		$lparam = "LPARAM";
	    }
	} elsif(s/^MAKELPARAM\(\s*(.*?)\s*,\s*(.*?)\s*\)$//) {
	    $lparam = "(,)"; # "($1, $2)";
	} elsif(s/^\((.*?)\)$//) {
	    $lparam = "LPARAM";
	} elsif(s/^0L$//) {
	    $lparam = "void";
	} else {
	    die "$msg: '$_'";
	} 

	foreach my $msg (@msgs) {
	    $output->write("$msg => { result => \"$return_type\", wparam => \"$wparam\", lparam => \"$lparam\" },\n");
	}

	return 1;
    };

    $parser->set_found_preprocessor_callback($found_preprocessor);

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

