# --------------------------------------------------------------------
# Main routines for the Wine test environment
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

package winetest;

use strict;
use vars qw(@ISA @EXPORT @EXPORT_OK $todo_level
            $successes $failures $todo_successes $todo_failures $winetest_report_success);

require Exporter;

@ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
             assert
             hd
             ok
             todo
             todo_wine
             trace
             wc
             wclen
            );

# Global variables
$wine::debug = defined($ENV{WINETEST_DEBUG}) ? $ENV{WINETEST_DEBUG} : 1;
$wine::platform = defined($ENV{WINETEST_PLATFORM}) ? $ENV{WINETEST_PLATFORM} : "windows";

$todo_level = 0;
$successes = 0;
$failures = 0;
$todo_successes = 0;
$todo_failures = 0;
$winetest_report_success = defined($ENV{WINETEST_REPORT_SUCCESS}) ? $ENV{WINETEST_REPORT_SUCCESS} : 0;

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

1;
