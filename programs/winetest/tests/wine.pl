################################################################
# Tests for wine.pm module functions
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

use kernel32;

################################################################
# Test some simple function calls

# Test string arguments
$atom = GlobalAddAtomA("foo");
ok( $atom >= 0xc000 && $atom <= 0xffff );
ok( !defined($wine::err) );

# Test integer and string reference arguments
$buffer = "xxxxxx";
$ret = GlobalGetAtomNameA( $atom, \$buffer, length(buffer) );
ok( !defined($wine::err) );
ok( $ret == 3 );
ok( lc $buffer eq "foo\000xx" );

# Test integer reference
$code = 0;
$ret = GetExitCodeThread( GetCurrentThread(), \$code );
ok( !defined($wine::err) );
ok( $ret );
ok( $code == 0x103 );

# Test string return value
$str = lstrcatA( "foo\0foo", "bar" );
ok( !defined($wine::err) );
ok( $str eq "foobar" );

################################################################
# Test last error handling

SetLastError( 123 );
$ret = GetLastError();
ok( $ret == 123 );

################################################################
# Test various error cases

eval { SetLastError(1,2); };
ok( $@ =~ /Wrong number of arguments, expected 1, got 2/ );

wine::declare("kernel32", "SetLastError" => "int" );  # disable prototype
eval { SetLastError(1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7); };
ok( $@ =~ /Too many arguments/ );

wine::declare("kernel32", "non_existent_func" => ["int",["int"]]);
eval { non_existent_func(1); };
ok( $@ =~ /Could not get address for kernel32\.non_existent_func/ );

my $funcptr = GetProcAddress( GetModuleHandleA("kernel32"), "SetLastError" );
ok( $funcptr );
eval { wine::call_wine_API( $funcptr, 10, $wine::debug-1, 0); };
ok( $@ =~ /Bad return type 10 at/ );

eval { foobar(1,2,3); };
ok( $@ =~ /Function 'foobar' not declared at/ );

################################################################
# Test assert

assert( 1, "cannot fail" );

eval { assert( 0, "this must fail" ); };
ok( $@ =~ /Assertion failed/ );
ok( $@ =~ /this must fail/ );

################################################################
# Test todo blocks

todo_wine
{
    ok( $wine::platform ne "wine", "Must fail only on Wine" );
};

todo( $wine::platform,
      sub { ok( 0, "Failure must be ignored inside todo" ); } );
todo( $wine::platform . "xxx",
      sub { ok( 1, "Success must not cause error inside todo for other platform" ); } );
