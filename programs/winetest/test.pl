#
# Test script for the winetest program
#

use wine;

$wine::debug = 0;

################################################################
# Declarations for functions we use in this script

wine::declare( "kernel32",
               SetLastError       => "void",
               GetLastError       => "int",
               GlobalAddAtomA     => "word",
               GlobalGetAtomNameA => "int",
               GetCurrentThread   => "int",
               GetExitCodeThread  => "int",
               lstrcatA           => "ptr"
);

################################################################
# Test some simple function calls

# Test string arguments
$atom = GlobalAddAtomA("foo");
assert( $atom >= 0xc000 && $atom <= 0xffff );
assert( !defined($wine::err) );

# Test integer and string reference arguments
$buffer = "xxxxxx";
$ret = GlobalGetAtomNameA( $atom, \$buffer, length(buffer) );
assert( !defined($wine::err) );
assert( $ret == 3 );
assert( lc $buffer eq "foo\000xx" );

# Test integer reference
$code = 0;
$ret = GetExitCodeThread( GetCurrentThread(), \$code );
assert( !defined($wine::err) );
assert( $ret );
assert( $code == 0x103 );

# Test string return value
$str = lstrcatA( "foo\0foo", "bar" );
assert( !defined($wine::err) );
assert( $str eq "foobar" );

################################################################
# Test last error handling

SetLastError( 123 );
$ret = GetLastError();
assert( $ret == 123 );

################################################################
# Test various error cases

eval { SetLastError(1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7); };
assert( $@ =~ /Too many arguments at/ );

eval { wine::call_wine_API( "kernel32", "SetLastError", 10, $wine::debug, 0); };
assert( $@ =~ /Bad return type 10 at/ );

eval { foobar(1,2,3); };
assert( $@ =~ /Function 'foobar' not declared at/ );
