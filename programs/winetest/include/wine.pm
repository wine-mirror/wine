# --------------------------------------------------------------------
# Module:      wine.pm
#
# Purpose: Module to supply wrapper around and support for gateway to
#          Windows API functions
#
# Copyright 2001 John F Sturtz for Codeweavers
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# --------------------------------------------------------------------

package wine;

use strict;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD $todo_level
            $successes $failures $todo_successes $todo_failures
            $winetest_report_success
            %return_types %prototypes %loaded_modules);

require Exporter;

@ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
             AUTOLOAD
             alloc_callback
             assert
             hd
             ok
             todo
             todo_wine
             trace
             wc
             wclen
            );

$VERSION = '0.01';
bootstrap wine $VERSION;

# Global variables
$wine::err = 0;
$wine::exit_status = 0;
$wine::debug = defined($ENV{WINETEST_DEBUG}) ? $ENV{WINETEST_DEBUG} : 1;
$wine::platform = defined($ENV{WINETEST_PLATFORM}) ? $ENV{WINETEST_PLATFORM} : "windows";

$todo_level = 0;
$successes = 0;
$failures = 0;
$todo_successes = 0;
$todo_failures = 0;
%loaded_modules = ();
$winetest_report_success = defined($ENV{WINETEST_REPORT_SUCCESS}) ? $ENV{WINETEST_REPORT_SUCCESS} : 0;


# --------------------------------------------------------------
# | Return-type constants                                      |
# |                                                            |
# | [todo]  I think there's a way to define these in a C       |
# |         header file, so that both the C functions in the   |
# |         XS module and the Perl routines in the .pm have    |
# |         access to them.  But I haven't worked it out       |
# |         yet ...                                            |
# --------------------------------------------------------------
%return_types = ( 
    "void" => 0,
    "int" => 1, "long" => 1,
    "word" => 2,
    "ptr" => 3,
    "str" => 4, "wstr" => 4
);


# ------------------------------------------------------------------------
# | Sub:       AUTOLOAD                                                  |
# | -------------------------------------------------------------------- |
# | Purpose:   Used to catch calls to undefined routines                 |
# |                                                                      |
# |     Any routine which is called and not defined is assumed to be     |
# |     a call to the Wine API function of the same name.  We trans-     |
# |     late it into a call to the call() subroutine, with FUNCTION      |
# |     set to the function invoked and all other args passed thru.      |
# ------------------------------------------------------------------------
sub AUTOLOAD
{
    # --------------------------------------------------------------
    # | Figure out who we are                                      |
    # --------------------------------------------------------------
    my ($pkg, $func) = (split /::/, $AUTOLOAD)[0,1];

    # --------------------------------------------------------------
    # | Any function that is in the @EXPORT array is passed thru   |
    # | to AutoLoader to pick up the appropriate XS extension      |
    # --------------------------------------------------------------
    if (grep ($_ eq $func, @EXPORT))
    {
        $AutoLoader::AUTOLOAD = $AUTOLOAD;
        goto &AutoLoader::AUTOLOAD;
    }

    # --------------------------------------------------------------
    # | Ignore this                                                |
    # --------------------------------------------------------------
    return
        if ($func eq 'DESTROY');

    # --------------------------------------------------------------
    # | Otherwise, assume any undefined method is the name of a    |
    # | wine API call, and all the args are to be passed through   |
    # --------------------------------------------------------------
    if (defined($prototypes{$func}))
    {
        return call( $func, @_ );
    }
    die "Function '$func' not declared";
} # End AUTOLOAD



# ------------------------------------------------------------------------
# | Sub:         call                                                    |
# | -------------------------------------------------------------------- |
# | Purpose:     Call a wine API function                                |
# |                                                                      |
# | Usage:       call FUNCTION, [ARGS ...]
# |                                                                      |
# | Returns:     value returned by API function called                   |
# ------------------------------------------------------------------------
sub call($@)
{
    my ($function,@args) = @_;
    my ($module,$funcptr,$ret_type,$arg_types) = @{$prototypes{$function}};

    unless ($funcptr)
    {
        my $handle = $loaded_modules{$module};
        $funcptr = get_proc_address( $handle, $function ) or die "Could not get address for $module.$function";
        ${$prototypes{$function}}[1] = $funcptr;
    }

    if ($wine::debug > 1)
    {
        print STDERR "==== Call $function(";
        for (@args)
        {
            print STDERR " ", +(ref () ? ("(" . ${$_} . ")") : "$_");
        }
        print STDERR " " if (scalar @args);
        print STDERR ")\n";
    }

    # Check and translate args before call
    my @args2;
    if (defined($arg_types)) {
	my @arg_types = @$arg_types;

	if($#args != $#arg_types) {
	    die "$function: Wrong number of arguments, expected " .
		($#arg_types + 1) . ", got " . ($#args + 1) . "\n";
	}

	while (defined(my $arg = shift @args) &&
               defined(my $arg_type = shift @arg_types))
        {
	    if($arg_type == 1 || $arg_type == 2) { # int || word
		$arg = int($arg);
	    }
	    push @args2, $arg;
	}
    } else {
	@args2 = @args;
    }

    # Now call call_wine_API(), which will turn around and call
    # the appropriate wine API function.
    my ($err,$r) = call_wine_API( $funcptr, $ret_type, $wine::debug-1, @args2 );

    if ($wine::debug > 1)
    {
        print STDERR "==== Ret  $function()";
        if (defined($r)) { printf STDERR " ret=0x%x", $r; }
        if (defined($err)) { printf STDERR " err=%d", $err; }
        print STDERR "\n";
    }

    # Pass the return value back
    $wine::err = $err;
    return ($r);
}


# ----------------------------------------------------------------------
# | Subroutine:  declare
# ----------------------------------------------------------------------
sub declare($%)
{
    my ($module, %list) = @_;
    my ($handle, $func);

    if (defined($loaded_modules{$module}))
    {
        $handle = $loaded_modules{$module};
    }
    else
    {
        $handle = load_library($module) or die "Could not load '$module'";
        $loaded_modules{$module} = $handle;
    }

    foreach $func (keys %list)
    {
	if(ref($list{$func}) eq "ARRAY") {
	    my ($return_type, $argument_types) = @{$list{$func}};

	    my $ret_type = $return_types{$return_type};
	    my $arg_types = [map { $return_types{$_} } @$argument_types];

	    $prototypes{$func} = [ $module, 0, $ret_type, $arg_types ];
	} else {
	    my $ret_type = $return_types{$list{$func}};

	    $prototypes{$func} = [ $module, 0, $ret_type ];
	}
    }
}


# ------------------------------------------------------------------------
# | Sub:         alloc_callback                                          |
# | -------------------------------------------------------------------- |
# | Purpose:     Allocate a thunk for a Wine API callback function.      |
# |                                                                      |
# |     Basically a thin wrapper over alloc_thunk(); see wine.xs for     |
# |     details ...                                                      |
# |                                                                      |
# | Usage:       alloc_callback SUB_REF, [ ARGS_TYPES ... ]              |
# |                                                                      |
# | Returns:     Pointer to thunk allocated (as an integer value)        |
# |                                                                      |
# |     The returned value is just a raw pointer to a block of memory    |
# |     allocated by the C code (cast into a Perl integer).  It isn't    |
# |     really suitable for anything but to be passed to a wine API      |
# |     function ...                                                     |
# ------------------------------------------------------------------------
sub alloc_callback($@)
{
    # ----------------------------------------------
    # | Locals                                     |
    # |                                            |
    # | [todo]  Check arg types                    |
    # ----------------------------------------------
    my  $sub_ref            = shift;
    my  @callback_arg_types = @_;
 
    # [todo]  Check args
    # [todo]  Some way of specifying args passed to callback

    # --------------------------------------------------------------
    # | Convert arg types to integers                              |
    # --------------------------------------------------------------
    map { $_ = $return_types{$_} } @callback_arg_types;

    # --------------------------------------------------------------
    # | Pass thru to alloc_thunk()                                 |
    # --------------------------------------------------------------
    return alloc_thunk ($sub_ref, @callback_arg_types);
}


# ----------------------------------------------------------------------
# | Subroutine:  hd                                                    |
# |                                                                    |
# | Purpose:     Display a hex dump of a string                        |
# |                                                                    |
# | Usage:       hd STR                                                |
# | Usage:       hd STR, LENGTH                                        |
# |                                                                    |
# | Returns:     (none)                                                |
# ----------------------------------------------------------------------
sub hd($;$)
{
    # Locals
    my  ($buf, $length);
    my  $first;
    my  ($str1, $str2, $str, $t);
    my  ($c, $x);

# Begin sub hd

    # --------------------------------------------------------------
    # | Get args; if no BUF specified, blow                        |
    # --------------------------------------------------------------
    $buf = shift;
    $length = (shift or length ($buf));
    return
        if ((not defined ($buf)) || ($length <= 0));

    # --------------------------------------------------------------
    # | Initialize                                                 |
    # --------------------------------------------------------------
    $first = 1;
    $str1 = "00000:";
    $str2 = "";

    # --------------------------------------------------------------
    # | For each character                                         |
    # --------------------------------------------------------------
    for (0 .. ($length - 1))
    {
        $c = substr ($buf, $_, 1);
        $x = sprintf ("%02x", ord ($c));
        $str1 .= (" " . $x);
        $str2 .= (((ord ($c) >= 33) && (ord ($c) <= 126)) ? $c : ".");

        # --------------------------------------------------------------
        # | Every group of 4, add an extra space                       |
        # --------------------------------------------------------------
        if
        (
            ((($_ + 1) % 16) == 4)  ||
            ((($_ + 1) % 16) == 12)
        )
        {
            $str1 .= " ";
            $str2 .= " ";
        }

        # --------------------------------------------------------------
        # | Every group of 8, add a '-'                                |
        # --------------------------------------------------------------
        elsif
        (
            ((($_ + 1) % 16) == 8)
        )
        {
            $str1 .= " -";
            $str2 .= " ";
        }

        # --------------------------------------------------------------
        # | Every group of 16, dump                                    |
        # --------------------------------------------------------------
        if
        (
            ((($_ + 1) % 16) == 0)      ||
            ($_ == ($length - 1))
        )
        {
            $str = sprintf ("%-64s%s", $str1, $str2);
            if ($first)
            {
                $t = ("-" x length ($str));
                print "  $t\n";
                print "  | $length bytes\n";
                print "  $t\n";
                $first = 0;
            }
            print "  $str\n";
            $str1 = sprintf ("%05d:", ($_ + 1));
            $str2 = "";
            if ($_ == ($length - 1))
            {
                print "  $t\n";
            }
        }

    }  # end for


    # --------------------------------------------------------------
    # | Exit point                                                 |
    # --------------------------------------------------------------
    return;

} # End sub hd



# ----------------------------------------------------------------------
# | Subroutine:  wc                                                    |
# |                                                                    |
# | Purpose:     Generate unicode string                               |
# |                                                                    |
# | Usage:       wc ASCII_STRING                                       |
# |                                                                    |
# | Returns:     string generated                                      |
# ----------------------------------------------------------------------
sub wc($)
{
    return pack("S*",unpack("C*",shift));
} # End sub wc



# ----------------------------------------------------------------------
# | Subroutine:  wclen                                                 |
# |                                                                    |
# | Purpose:     Return length of unicode string                       |
# |                                                                    |
# | Usage:       wclen UNICODE_STRING                                  |
# |                                                                    |
# | Returns:     string generated                                      |
# ----------------------------------------------------------------------
sub wclen($)
{
    # Locals
    my  $str = shift;
    my  ($c1, $c2, $n);

# Begin sub wclen

    $n = 0;
    while (length ($str) > 0)
    {
        $c1 = substr ($str, 0, 1, "");
        $c2 = substr ($str, 0, 1, "");
        (($c1 eq "\x00") && ($c2 eq "\x00")) ? last : $n++;
    }

    return ($n);

} # End sub wclen



# ----------------------------------------------------------------------
# Subroutine:  ok
#
# Purpose:     Print warning if something fails
#
# Usage:       ok CONDITION [DESCRIPTION]
#
# Returns:     (none)
# ----------------------------------------------------------------------
sub ok($;$)
{
    my $assertion = shift;
    my $description = shift;
    my ($filename, $line) = (caller (0))[1,2];
    if ($todo_level)
    {
        if ($assertion)
        {
            print STDERR ("$filename:$line: Test succeeded inside todo block" .
                          ($description ? ": $description" : "") . "\n");
            $todo_failures++;
        }
        else { $todo_successes++; }
    }
    else
    {
        if (!$assertion)
        {
            print STDERR ("$filename:$line: Test failed" .
                          ($description ? ": $description" : "") . "\n");
            $failures++;
        }
        else 
        {
            print STDERR ("$filename:$line: Test succeeded\n") if ($winetest_report_success);
            $successes++;
        }
    }
}


# ----------------------------------------------------------------------
# Subroutine:  assert
#
# Purpose:     Print error and die if something fails
#
# Usage:       assert CONDITION [DESCRIPTION]
#
# Returns:     (none)
# ----------------------------------------------------------------------
sub assert($;$)
{
    my $assertion = shift;
    my $description = shift;
    my ($filename, $line) = (caller (0))[1,2];
    unless ($assertion)
    {
        die ("$filename:$line: Assertion failed" . ($description ? ": $description" : "") . "\n");
    }
}


# ----------------------------------------------------------------------
# Subroutine:  trace
#
# Purpose:     Print debugging traces
#
# Usage:       trace format [arguments]
# ----------------------------------------------------------------------
sub trace($@)
{
    return unless ($wine::debug > 0);
    my $format = shift;
    my $filename = (caller(0))[1];
    $filename =~ s!.*/!!;
    printf "trace:$filename $format", @_;
}

# ----------------------------------------------------------------------
# Subroutine:  todo
#
# Purpose:     Specify a block of code as todo for a given platform
#
# Usage:       todo name coderef
# ----------------------------------------------------------------------
sub todo($$)
{
    my ($platform,$code) = @_;
    if ($wine::platform eq $platform)
    {
        $todo_level++;
        eval &$code;
        $todo_level--;
    }
    else
    {
        eval &$code;
    }
}


# ----------------------------------------------------------------------
# Subroutine:  todo_wine
#
# Purpose:     Specify a block of test as todo for the Wine platform
#
# Usage:       todo_wine { code }
# ----------------------------------------------------------------------
sub todo_wine(&)
{
    my $code = shift;
    todo( "wine", $code );
}


# ----------------------------------------------------------------------
# Subroutine:  END
#
# Purpose:     Called at the end of execution, print results summary
# ----------------------------------------------------------------------
END
{
    return if $?;  # got some other error already
    if ($wine::debug > 0)
    {
        my $filename = (caller(0))[1];
        printf STDERR ("%s: %d tests executed, %d marked as todo, %d %s.\n",
                       $filename, $successes + $failures + $todo_successes + $todo_failures,
                       $todo_successes, $failures + $todo_failures,
                       ($failures + $todo_failures != 1) ? "failures" : "failure" );
    }
    $? = ($failures + $todo_failures < 255) ? $failures + $todo_failures : 255;
}


# Autoload methods go after =cut, and are processed by the autosplit program.
1;
__END__



# ------------------------------------------------------------------------
# | pod documentation                                                    |
# |                                                                      |
# |                                                                      |
# ------------------------------------------------------------------------

=head1 NAME

wine - Perl extension for calling wine API functions

=head1 SYNOPSIS

    use wine;

    wine::declare( "kernel32",
                   SetLastError => ["void", ["int"]],
                   GetLastError => ["int", []] );
    SetLastError( 1234 );
    printf "%d\n", GetLastError();


=head1 DESCRIPTION

This module provides a gateway for calling Win32 API functions from
a Perl script.

=head1 CALLING WIN32 API FUNCTIONS

The functions you want to call must first be declared by calling
the wine::declare method. The first argument is the name of the
module containing the APIs, and the next argument is a list of
function names and their return and argument types. For instance:

    wine::declare( "kernel32",
                   SetLastError => ["void", ["int"]],
                   GetLastError => ["int", []] );

declares that the functions SetLastError and GetLastError are
contained in the kernel32 dll.

Once you have done that you can call the functions directly just
like native Perl functions:

  SetLastError( $some_error );

The supported return types are:

=over 4

=item void

=item word

=item long

=item ptr

=item str

=item wstr

=back

=head1 $wine::err VARIABLE

In the Win32 API, an integer error code is maintained which always
contains the status of the last API function called.  In C code,
it is accessed via the GetLastError() function.  From a Perl script,
it can be accessed via the package global $wine::err.  For example:

    GlobalGetAtomNameA ($atom, \$buf, -1);
    if ($wine::err == 234)
    {
        ...
    }

Wine returns 234 (ERROR_MORE_DATA) from the GlobalGetAtomNameA()
API function in this case because the buffer length passed is -1
(hardly enough room to store anything in ...)

If the called API didn't set the last error code, $wine:;err is
undefined.

=head1 $wine::debug VARIABLE

This variable can be set to 1 to enable debugging of the API calls,
which will print a lot of information about what's going on inside the
wine package while calling an API function.

=head1 OTHER USEFUL FUNCTIONS

The bundle that includes the wine extension also includes a module of
plain ol' Perl subroutines which are useful for interacting with wine
API functions. Currently supported functions are:

=over 4

=item hd BUF [, LENGTH]

Dump a formatted hex dump to STDOUT.  BUF is a string containing
the buffer to dump; LENGTH is the length to dump (length (BUF) if
omitted).  This is handy because wine often writes a null character
into the middle of a buffer, thinking that the next piece of code to
look at the buffer will be a piece of C code that will regard it as
a string terminator.  Little does it know that the buffer is going
to be returned to a Perl script, which may not ...

=item wc STR

Generate and return a wide-character (Unicode) string from the given
ASCII string

=item wclen WSTR

Return the length of the given wide-character string

=item assert CONDITION

Print a message if the assertion fails (i.e., CONDITION is false),
or do nothing quietly if it is true.  The message includes the script
name and line number of the assertion that failed.

=back



=head1 AUTHOR

John F Sturtz, jsturtz@codeweavers.com

=head1 SEE ALSO

wine documentation

=cut
