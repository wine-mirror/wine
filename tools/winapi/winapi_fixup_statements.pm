package winapi_fixup_statements;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw(&fixup_statements);

use options qw($options);
use output qw($output);

use c_parser;

sub fixup_function_call {
    my $name = shift;
    my @arguments = @{(shift)};;

    return "$name(" . join(", ", @arguments) . ")";
}

sub _parse_makelong {
    my $value = shift;

    my $low;
    my $high;
    if($value =~ /^
       (?:\(\w+\)\s*)?
       MAKE(?:LONG|LPARAM|LRESULT|WPARAM)\s*
       \(\s*(.*?)\s*,\s*(.*?)\s*\)$/sx)
    {
	$low = $1;
	$high = $2;
    } elsif($value =~ /^(?:\(\w+\)\s*)?0L?$/) {
	$low = "0";
	$high = "0";
    } else {
	$low = "($value) & 0xffff";
	$high = "($value) << 16";
    }

    return ($low, $high);
}

sub fixup_function_call_2_forward_wm_call {
    my $name = shift;
    (my $hwnd, my $msg, my $wparam, my $lparam) = @{(shift)};

    if($msg =~ /^(?:WM_BEGINDRAG|WM_ENTERMENULOOP|WM_EXITMENULOOP|WM_HELP|
		  WM_ISACTIVEICON|WM_LBTRACKPOINT|WM_NEXTMENU)$/x) 
    {
	return undef;
    }

    my $suffix;
    $name =~ /([AW])?$/;
    if(defined($1)) {
	$suffix = $1;
    } else {
	$suffix = "";
    }

    $wparam =~ s/^\(WPARAM\)//;
    $lparam =~ s/^\(LPARAM\)//;

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

sub fixup_statements {
    my $function = shift;
    my $editor = shift;

    my $linkage = $function->linkage;
    my $internal_name = $function->internal_name;
    my $statements_line = $function->statements_line;
    my $statements = $function->statements;
    
    if(($linkage eq "extern" && !defined($statements)) ||
       ($linkage eq "" && !defined($statements)))
    {
	return;
    }
    
    if($options->statements_windowsx && defined($statements)) {
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

	    if($options->statements_windowsx &&
	       $name =~ /^(?:DefWindowProc|SendMessage)[AW]$/ &&
	       $$arguments[1] =~ /^WM_\w+$/) 
	    {
		fixup_replace(\&fixup_function_call_2_forward_wm_call, $editor,
			      $begin_line, $begin_column, $end_line, $end_column,
			      $name, $arguments);
	    } elsif(0) {
		$output->write("$begin_line.$begin_column-$end_line.$end_column: " .
			       "$name(" . join(", ", @$arguments) . ")\n");
	    }
	};
	my $line = $statements_line;
	my $column = 1;
	
	if(!&c_parser::parse_c_statements(\$statements, \$line, \$column, $found_function_call)) {
	    $output->write("error: can't parse statements\n");
	}
    }
}

sub fixup_replace {
    my $function = shift;
    my $editor = shift;
    my $begin_line = shift;
    my $begin_column = shift;
    my $end_line = shift;
    my $end_column = shift;

    my $replace = &$function(@_);

    if(defined($replace)) {
	$editor->replace($begin_line, $begin_column, $end_line, $end_column, $replace);
    }
}

1;
