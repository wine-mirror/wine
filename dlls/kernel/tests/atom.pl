################################################################
# Tests for atom API calls
#
# Copyright 2001 Alexandre Julliard
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
#

use wine;
use winerror;
use kernel32;

my $name = "foobar";

################################################################
# GlobalAddAtom/GlobalFindAtom

# Add an atom
my $atom = GlobalAddAtomA( $name );
ok( ($atom >= 0xc000) && ($atom <= 0xffff) && !defined($wine::err) );

# Verify that it can be found (or not) appropriately
ok( GlobalFindAtomA($name) == $atom );
ok( GlobalFindAtomA(uc $name) == $atom );
ok( !GlobalFindAtomA( "_" . $name ) );

# Add the same atom, specifying string as unicode; should
# find the first one, not add a new one
my $w_atom = GlobalAddAtomW( wc($name) );
ok( $w_atom == $atom && !defined($wine::err) );

# Verify that it can be found (or not) appropriately via
# unicode name
ok( GlobalFindAtomW( wc($name)) == $atom );
ok( GlobalFindAtomW( wc(uc($name)) ) == $atom );
ok( !GlobalFindAtomW( wc("_" . $name) ) );

# Test integer atoms
#
# (0x0000 .. 0xbfff) should be valid;
# (0xc000 .. 0xffff) should be invalid

# test limits
todo_wine
{
    ok( GlobalAddAtomA(0) == 0 && !defined($wine::err) );
    ok( GlobalAddAtomW(0) == 0 && !defined($wine::err) );
};
ok( GlobalAddAtomA(1) == 1 && !defined($wine::err) );
ok( GlobalAddAtomW(1) == 1 && !defined($wine::err) );
ok( GlobalAddAtomA(0xbfff) == 0xbfff && !defined($wine::err) );
ok( GlobalAddAtomW(0xbfff) == 0xbfff && !defined($wine::err) );
ok( !GlobalAddAtomA(0xc000) && ($wine::err == ERROR_INVALID_PARAMETER) );
ok( !GlobalAddAtomW(0xc000) && ($wine::err == ERROR_INVALID_PARAMETER) );
ok( !GlobalAddAtomA(0xffff) && ($wine::err == ERROR_INVALID_PARAMETER) );
ok( !GlobalAddAtomW(0xffff) && ($wine::err == ERROR_INVALID_PARAMETER) );

# test some in between
for ($i = 0x0001; ($i <= 0xbfff); $i += 27)
{
    ok( GlobalAddAtomA($i) && !defined($wine::err) );
    ok( GlobalAddAtomW($i) && !defined($wine::err) );
}
for ($i = 0xc000; ($i <= 0xffff); $i += 29)
{
    ok( !GlobalAddAtomA($i) && ($wine::err == ERROR_INVALID_PARAMETER) );
    ok( !GlobalAddAtomW($i) && ($wine::err == ERROR_INVALID_PARAMETER) );
}


################################################################
# GlobalGetAtomName

# Get the name of the atom we added above
my $buf = "." x 20;
my $len = GlobalGetAtomNameA( $atom, \$buf, length ($buf) );
ok( $len == length ($name) );
ok( $buf eq ($name . "\x00" . ("." x (length ($buf) - length ($name) - 1))) );
$buf = "." x 20;
ok( !GlobalGetAtomNameA( $atom, \$buf, 3 ) );

# Repeat, unicode-style
$buf = wc("." x 20);
$len = GlobalGetAtomNameW( $atom, \$buf, length ($buf) / 2 );
ok( $len == wclen (wc ($name)) );
todo_wine
{
    ok( $buf eq (wc ($name) . "\x00\x00" . ("." x (20 - 2 * (wclen (wc ($name)) + 1) ))) );
};
$buf = wc("." x 20);
ok( !GlobalGetAtomNameW( $atom, \$buf, 3 ) );

# Check error code returns
ok( !GlobalGetAtomNameA( $atom, $buf,  0 ) && $wine::err == ERROR_MORE_DATA );
ok( !GlobalGetAtomNameA( $atom, $buf, -1 ) && $wine::err == ERROR_MORE_DATA );
ok( !GlobalGetAtomNameW( $atom, $buf,  0 ) && $wine::err == ERROR_MORE_DATA );
ok( !GlobalGetAtomNameW( $atom, $buf, -1 ) && $wine::err == ERROR_MORE_DATA );

# Test integer atoms
$buf = "a" x 10;
for ($i = 0; ($i <= 0xbfff); $i += 37)
{
    $len = GlobalGetAtomNameA( $i, \$buf, length ($buf) );
    if ($i)
    {
        ok( ($len > 1) && ($len < 7) );
        ok( $buf eq ("#$i\x00" . "a" x (10 - $len - 1) ));
    }
    else
    {
        todo_wine
        {
            ok( ($len > 1) && ($len < 7) );
            ok( $buf eq ("#$i\x00" . "a" x (10 - $len - 1) ));
        };
    }
}

################################################################
# GlobalDeleteAtom

$name = "barkbar";
$n = 5;

# First make sure it doesn't exist
$atom = GlobalAddAtomA( $name );
while (!GlobalDeleteAtom( $atom )) { }

# Now add it a number of times
for (1 .. $n) { $atom = GlobalAddAtomA( $name ); }

# Make sure it's here
ok( GlobalFindAtomA( $name ));
ok( GlobalFindAtomW( wc($name) ));

# That many deletions should succeed
for (1 .. $n) { ok( !GlobalDeleteAtom( $atom ) ); }

# It should be gone now
ok( !GlobalFindAtomA($name) );
ok( !GlobalFindAtomW( wc($name) ));

# So this one should fail
ok( GlobalDeleteAtom( $atom ) == $atom && $wine::err == ERROR_INVALID_HANDLE );


################################################################
# Test error handling

# ASCII
ok( ($atom = GlobalAddAtomA("a" x 255)) );
ok( !GlobalDeleteAtom($atom) );
ok( !GlobalAddAtomA( "a" x 256 ) && $wine::err == ERROR_INVALID_PARAMETER );
ok( !GlobalFindAtomA( "b" x 256 ) && $wine::err == ERROR_INVALID_PARAMETER );

# Unicode
ok( ($atom = GlobalAddAtomW( wc("a" x 255) )) );
ok( !GlobalDeleteAtom($atom) );
ok( !GlobalAddAtomW( wc("a" x 256) ) && $wine::err == ERROR_INVALID_PARAMETER );
ok( !GlobalFindAtomW( wc("b" x 256) ) && $wine::err == ERROR_INVALID_PARAMETER );
