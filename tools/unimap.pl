#!/usr/bin/perl
#
# Reads the Unicode 2.0 "unidata2.txt" file and selects encodings
# for case pairs, then builds an include file "casemap.h" for the
# case conversion routines. 

use integer

$INFILE	= "UnicodeData.txt";
$OUT	= ">casemap.h";
$TEST	= ">allcodes";

# Open the data file ...
open INFILE	or die "Can't open input file $INFILE!\n";
open OUT	or die "Can't open output file $OUT!\n";
open TEST	or die "Can't open output file $OUT!\n";

#Initialize the upper and lower hashes
%lwrtable = ();
%uprtable = ();
@low = ("0") x 256;
@upr = ("0") x 256;

while ($line = <INFILE> )
{
	# Decode the fields ...
	($code, $name, $cat, $comb, $bidi, 
	 $decomp, $dec, $dig, $num, $mirror, 
	 $oldname, $comment, $upper, $lower, $title) = split /;/, $line;

	#Get the high byte of the code
	$high = substr $code, 0, 2;
	if ($lower ne "") {
		$low[hex $high] = "lblk" . $high;
		$lwrtable{$code} = $lower;
	}
	if ($upper ne "") {
		$upr[hex $high] = "ublk" . $high;
		$uprtable{$code} = $upper;
	}
	#Write everything to the test file
	printf TEST "%s %s %s\n", $code, 
		$upper ne "" ? $upper : "0000",
		$lower ne "" ? $lower : "0000";

}
close(FILE);
close TEST;

#Generate the header file
print OUT "/*\n";
print OUT " * Automatically generated file -- do not edit!\n";
print OUT " * (Use tools/unimap.pl for generation)\n";
print OUT " *\n";
print OUT " * Mapping tables for Unicode case conversion\n";
print OUT " */\n";
print OUT "\n";
print OUT "#ifndef __WINE_CASEMAP_H\n";
print OUT "#define __WINE_CASEMAP_H\n";
print OUT "\n";
print OUT "#include \"windef.h\"\n";
print OUT "\n";

#Write out the non-trivial mappings
for ($high = 0; $high < 256; $high++) {
	#Check whether the table is needed
	if (length $low[$high] < 6) {
		next;
	}
	printf OUT "/* Lowercase mappings %02X00 - %02XFF */\n",
		$high, $high;
	printf OUT "static const WCHAR lblk%02X[256] = {\n", $high;
	for ($low = 0; $low < 256; $low += 8) {
		@patch = ();
		for ($i = 0; $i < 8; $i++) {
			$code = sprintf "%02X%02X", $high, $low + $i;
			$map = $lwrtable{$code};
			if ($map eq "") {
				$map = $code;
			}
			$patch[$i] = "0x" . $map;
		}
		printf OUT "\t%s, %s, %s, %s, %s, %s, %s, %s,\n",
			@patch;
	}
	print OUT "};\n\n";
}
print OUT "static const WCHAR * const lwrtable[256] = {\n";
for ($i = 0; $i < 256; $i += 8) {
	@patch = @low[$i+0 .. $i+7];
	printf OUT "\t%06s, %06s, %06s, %06s, %06s, %06s, %06s, %06s,\n",
		@patch;
}
print OUT "};\n\n";

for ($high = 0; $high < 256; $high++) {
	#Check whether the table is needed
	if (length $upr[$high] < 6) {
		next;
	}
	printf OUT "/* Uppercase mappings %02X00 - %02XFF */\n",
		$high, $high;
	printf OUT "static const WCHAR ublk%02X[256] = {\n", $high;
	for ($low = 0; $low < 256; $low += 8) {
		@patch = ();
		for ($i = 0; $i < 8; $i++) {
			$code = sprintf "%02X%02X", $high, $low + $i;
			$map = $uprtable{$code};
			if ($map eq "") {
				$map = $code;
			}
			$patch[$i] = "0x" . $map;
		}
		printf OUT "\t%s, %s, %s, %s, %s, %s, %s, %s,\n",
			@patch;
	}
	print OUT "};\n\n";
}
print OUT "static const WCHAR * const uprtable[256] = {\n";
for ($i = 0; $i < 256; $i += 8) {
	@patch = @upr[$i+0 .. $i+7];
	printf OUT "\t%06s, %06s, %06s, %06s, %06s, %06s, %06s, %06s,\n",
		@patch;
}
print OUT "};\n";
print OUT "\n";
print OUT "#endif /* !defined(__WINE_CASEMAP_H) */\n";

close(OUT);
