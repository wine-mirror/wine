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

package winapi_fixup_statements;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw(fixup_statements);

use config qw($wine_dir);
use options qw($options);
use output qw($output);

use c_parser;
use winapi_module_user qw(
    get_message_result_kind
    get_message_wparam_kind
    get_message_lparam_kind
);

########################################################################
# fixup_function_call

sub fixup_function_call($$) {
    my $name = shift;
    my @arguments = @{(shift)};;

    return "$name(" . join(", ", @arguments) . ")";
}

########################################################################
# _parse_makelong

sub _parse_makelong($) {
    local $_ = shift;

    my $low;
    my $high;

    my $name;
    my @arguments;
    my @argument_lines;
    my @argument_columns;

    my $parser = new c_parser;

    my $line = 1;
    my $column = 0;
    if($parser->parse_c_function_call(\$_, \$line, \$column, \$name, \@arguments, \@argument_lines, \@argument_columns) &&
       $name =~ /^MAKE(?:LONG|LPARAM|LRESULT|WPARAM)$/)
    {
	$low = $arguments[0];
	$high = $arguments[1];
    } elsif(/^(?:\(\w+\)\s*)?0L?$/) {
	$low = "0";
	$high = "0";
    } else {
	$low = "($_) & 0xffff";
	$high = "($_) << 16";
    }

    $low =~ s/^\s*(.*?)\s*$/$1/;
    $high =~ s/^\s*(.*?)\s*$/$1/;

    return ($low, $high);
}

########################################################################
# fixup_function_call_2_windowsx

sub fixup_user_message_2_windowsx($$) {
    my $name = shift;
    (my $hwnd, my $msg, my $wparam, my $lparam) = @{(shift)};

    if($msg !~ /^WM_/) {
	return undef;
    } elsif($msg =~ /^(?:WM_BEGINDRAG|WM_ENTERMENULOOP|WM_EXITMENULOOP|WM_HELP|
		       WM_ISACTIVEICON|WM_LBTRACKPOINT|WM_NEXTMENU)$/x)
    {
	return undef;
    } elsif($msg =~ /^WM_(?:GET|SET)TEXT$/) {
	return undef;
    }

    my $suffix;
    $name =~ /([AW])?$/;
    if(defined($1)) {
	$suffix = $1;
    } else {
	$suffix = "";
    }

    $wparam =~ s/^\(WPARAM\)\s*//;
    $lparam =~ s/^\(LPARAM\)\s*//;

    my @arguments;
    if(0) {
	# Nothing
    } elsif($msg =~ /^WM_COMMAND$/) {
	(my $id, my $code_notify) = _parse_makelong($wparam);
	my $hwndctl = $lparam;
	@arguments = ($id, $hwndctl, $code_notify);
    } elsif($msg =~ /^WM_(?:COPY|CUT|PASTE)$/) {
	@arguments = ();
    } elsif($msg =~ /^WM_(?:CHARTO|VKEYTO)ITEM$/) {
	(my $key, my $caret) = _parse_makelong($wparam);
	my $hwndctl = $lparam;
	@arguments = ($key, $hwndctl, $caret);
    } elsif($msg =~ /^WM_(?:COMPARE|DELETE|DRAW|MEASURE)ITEM$/) {
	@arguments = ($lparam);
    } elsif($msg =~ s/^WM_GETTEXT$/$&$suffix/) {
	@arguments = ($wparam, $lparam);
    } elsif($msg =~ /^WM_INITMENU$/) {
	my $hmenu = $wparam;
	@arguments =  ($hmenu);
    } elsif($msg =~ /^WM_INITMENUPOPUP$/) {
	my $hmenu = $wparam;
	(my $item, my $system_menu) = _parse_makelong($lparam);
	@arguments =  ($hmenu, $item, $system_menu);
    } elsif($msg =~ /^WM_MENUCHAR$/) {
	(my $ch, my $flags) = _parse_makelong($wparam);
	my $hmenu = $lparam;
	@arguments = ($ch, $flags, $hmenu);
    } elsif($msg =~ /^WM_MENUSELECT$/) {
	(my $item, my $flags) = _parse_makelong($wparam);
	my $hmenu = $lparam;
	my $hmenu_popup = "NULL"; # FIXME: Is this really correct?
	@arguments = ($hmenu, $item, $hmenu_popup, $flags);
    } elsif($msg =~ s/^WM_(NC)?LBUTTONDBLCLK$/WM_$1LBUTTONDOWN/) {
	my $double_click = "TRUE";
	my $key_flags = $wparam;
	(my $x, my $y) = _parse_makelong($lparam);
	@arguments = ($double_click, $x, $y, $key_flags);
    } elsif($msg =~ /^WM_(NC)?LBUTTONDOWN$/) {
	my $double_click = "FALSE";
	my $key_flags = $wparam;
	(my $x, my $y) = _parse_makelong($lparam);
	@arguments = ($double_click, $x, $y, $key_flags);
    } elsif($msg =~ /^WM_LBUTTONUP$/) {
	my $key_flags = $wparam;
	(my $x, my $y) = _parse_makelong($lparam);
	@arguments = ($x, $y, $key_flags);
    } elsif($msg =~ /^WM_SETCURSOR$/) {
	my $hwnd_cursor = $wparam;
	(my $code_hit_test, my $msg2) = _parse_makelong($lparam);
	@arguments = ($hwnd_cursor, $code_hit_test, $msg2);
    } elsif($msg =~ s/^WM_SETTEXT$/$&$suffix/) {
	my $text = $lparam;
	@arguments = ($text);
    } elsif($msg =~ /^WM_(?:SYS)?KEYDOWN$/) {
	my $vk = $wparam;
	(my $repeat, my $flags) = _parse_makelong($lparam);
	@arguments = ($vk, $repeat, $flags);
    } else {
	@arguments = ($wparam, $lparam);
    }
    unshift @arguments, $hwnd;

    return "FORWARD_" . $msg . "(" . join(", ", @arguments) . ", $name)";
}

########################################################################
# _get_messages

sub _get_messages($) {
    local $_ = shift;

    if(/^(?:BM|CB|EM|LB|STM|WM)_\w+(.*?)$/) {
	if(!$1) {
	    return ($_);
	} else {
	    return ();
	}
    } elsif(/^(.*?)\s*\?\s*((?:BM|CB|EM|LB|STM|WM)_\w+)\s*:\s*((?:BM|CB|EM|LB|STM|WM)_\w+)$/) {
	return ($2, $3);
    } elsif(/^\w+$/) {
	return ();
    } elsif(/^RegisterWindowMessage[AW]\s*\(.*?\)$/) {
	return ();
    } else {
	$output->write("warning: _get_messages: '$_'\n");
	return ();
    }
}

########################################################################
# _fixup_user_message

sub _fixup_user_message($$) {
    my $name = shift;
    (my $hwnd, my $msg, my $wparam, my $lparam) = @{(shift)};

    my $modified = 0;

    my $wkind;
    my $lkind;
    foreach my $msg (_get_messages($msg)) {
	my $new_wkind = get_message_wparam_kind($msg);
	if(defined($wkind) && $new_wkind ne $wkind) {
	    $output->write("messsages used together do not have the same type\n");
	} else {
	    $wkind = $new_wkind;
	}

	my $new_lkind = get_message_lparam_kind($msg);
	if(defined($lkind) && $new_lkind ne $lkind) {
	    $output->write("messsages used together do not have the same type\n");
	} else {
	    $lkind = $new_lkind;
	}
    }

    my @entries = (
	[ \$wparam, $wkind, "W", "w" ],
	[ \$lparam, $lkind, "L", "l" ]
    );
    foreach my $entry (@entries) {
	(my $refparam, my $kind, my $upper, my $lower) = @$entry;

	if(!defined($kind)) {
	    if($msg =~ /^WM_/) {
		$output->write("messsage $msg not properly defined\n");
		$modified = 0;
		last;
	    }
	} elsif($kind eq "ptr") {
	    if($$refparam =~ /^(\(${upper}PARAM\))?\s*($lower[pP]aram)$/) {
		if(defined($1)) {
		    $$refparam = $2;
		    $modified = 1;
		}
	    } elsif($$refparam =~ /^(\(${upper}PARAM\))?\s*0$/) {
	        $$refparam = "(${upper}PARAM) NULL";
		$modified = 1;
	    } elsif($$refparam !~ /^\(${upper}PARAM\)\s*/) {
                $$refparam = "(${upper}PARAM) $$refparam";
		$modified = 1;
	    }
	} elsif($kind eq "long") {
	    if($$refparam =~ s/^\(${upper}PARAM\)\s*//) {
		$modified = 1;
	    }
	}
    }

    if($modified) {
	my @arguments = ($hwnd, $msg, $wparam, $lparam);
	return "$name(" . join(", ", @arguments) . ")";
    } else {
	return undef;
    }
}

########################################################################
# fixup_statements

sub fixup_statements($$) {
    my $function = shift;
    my $editor = shift;

    my $file = $function->file;
    my $linkage = $function->linkage;
    my $name = $function->name;
    my $statements_line = $function->statements_line;
    my $statements_column = $function->statements_column;
    my $statements = $function->statements;

    if(!defined($statements)) {
	return;
    }

    my $parser = new c_parser($file);

    my $found_function_call = sub {
	my $begin_line = shift;
	my $begin_column = shift;
	my $end_line = shift;
	my $end_column = shift;
	my $name = shift;
	my $arguments = shift;

	foreach my $argument (@$arguments) {
	    $argument =~ s/^\s*(.*?)\s*$/$1/;
	}

	my $fixup_function_call;
	if($name =~ /^(?:DefWindowProc|SendMessage)[AW]$/)
	{
	    if($options->statements_windowsx) {
		$fixup_function_call = \&fixup_user_message_2_windowsx;
	    } else {
		$fixup_function_call = \&_fixup_user_message;
	    }
	}

	if(defined($fixup_function_call)) {
	    my $replace = &$fixup_function_call($name, $arguments);

	    if(defined($replace)) {
		$editor->replace($begin_line, $begin_column, $end_line, $end_column, $replace);
	    }
	} elsif($options->debug) {
	    $output->write("$begin_line.$begin_column-$end_line.$end_column: " .
			   "$name(" . join(", ", @$arguments) . ")\n");
	}

	return 0;
    };

    $parser->set_found_function_call_callback($found_function_call);

    my $line = $statements_line;
    my $column = 0;
    if(!$parser->parse_c_statements(\$statements, \$line, \$column)) {
	$output->write("error: can't parse statements\n");
    }
}

1;
