#!/usr/bin/perl
#
# Generate code page .c files from ftp.unicode.org descriptions
#
# Copyright 2000 Alexandre Julliard
#

# base directory for ftp.unicode.org files
$BASEDIR = "ftp.unicode.org/Public/";
$MAPPREFIX = $BASEDIR . "MAPPINGS/";

# UnicodeData file
$UNICODEDATA = $BASEDIR . "UNIDATA/UnicodeData.txt";

# Defaults mapping
$DEFAULTS = "./defaults";

# Default char for undefined mappings
$DEF_CHAR = ord '?';

@allfiles =
(
    [ 37,    "VENDORS/MICSFT/EBCDIC/CP037.TXT",   "IBM EBCDIC US Canada" ],
    [ 42,    "VENDORS/ADOBE/symbol.txt",          "Symbol" ],
    [ 424,   "VENDORS/MISC/CP424.TXT",            "IBM EBCDIC Hebrew" ],
    [ 437,   "VENDORS/MICSFT/PC/CP437.TXT",       "OEM United States" ],
    [ 500,   "VENDORS/MICSFT/EBCDIC/CP500.TXT",   "IBM EBCDIC International" ],
    [ 737,   "VENDORS/MICSFT/PC/CP737.TXT",       "OEM Greek 437G" ],
    [ 775,   "VENDORS/MICSFT/PC/CP775.TXT",       "OEM Baltic" ],
    [ 850,   "VENDORS/MICSFT/PC/CP850.TXT",       "OEM Multilingual Latin 1" ],
    [ 852,   "VENDORS/MICSFT/PC/CP852.TXT",       "OEM Slovak Latin 2" ],
    [ 855,   "VENDORS/MICSFT/PC/CP855.TXT",       "OEM Cyrillic" ],
    [ 856,   "VENDORS/MISC/CP856.TXT",            "Hebrew PC" ],
    [ 857,   "VENDORS/MICSFT/PC/CP857.TXT",       "OEM Turkish" ],
    [ 860,   "VENDORS/MICSFT/PC/CP860.TXT",       "OEM Portuguese" ],
    [ 861,   "VENDORS/MICSFT/PC/CP861.TXT",       "OEM Icelandic" ],
    [ 862,   "VENDORS/MICSFT/PC/CP862.TXT",       "OEM Hebrew" ],
    [ 863,   "VENDORS/MICSFT/PC/CP863.TXT",       "OEM Canadian French" ],
    [ 864,   "VENDORS/MICSFT/PC/CP864.TXT",       "OEM Arabic" ],
    [ 865,   "VENDORS/MICSFT/PC/CP865.TXT",       "OEM Nordic" ],
    [ 866,   "VENDORS/MICSFT/PC/CP866.TXT",       "OEM Russian" ],
    [ 869,   "VENDORS/MICSFT/PC/CP869.TXT",       "OEM Greek" ],
    [ 874,   "VENDORS/MICSFT/PC/CP874.TXT",       "ANSI/OEM Thai" ],
    [ 875,   "VENDORS/MICSFT/EBCDIC/CP875.TXT",   "IBM EBCDIC Greek" ],
    [ 878,   "VENDORS/MISC/KOI8-R.TXT",           "Russian KOI8" ],
    [ 932,   "VENDORS/MICSFT/WINDOWS/CP932.TXT",  "ANSI/OEM Japanese Shift-JIS" ],
    [ 936,   "VENDORS/MICSFT/WINDOWS/CP936.TXT",  "ANSI/OEM Simplified Chinese GBK" ],
    [ 949,   "VENDORS/MICSFT/WINDOWS/CP949.TXT",  "ANSI/OEM Korean Unified Hangul" ],
    [ 950,   "VENDORS/MICSFT/WINDOWS/CP950.TXT",  "ANSI/OEM Traditional Chinese Big5" ],
    [ 1006,  "VENDORS/MISC/CP1006.TXT",           "IBM Arabic" ],
    [ 1026,  "VENDORS/MICSFT/EBCDIC/CP1026.TXT",  "IBM EBCDIC Latin 5 Turkish" ],
    [ 1250,  "VENDORS/MICSFT/WINDOWS/CP1250.TXT", "ANSI Eastern Europe" ],
    [ 1251,  "VENDORS/MICSFT/WINDOWS/CP1251.TXT", "ANSI Cyrillic" ],
    [ 1252,  "VENDORS/MICSFT/WINDOWS/CP1252.TXT", "ANSI Latin 1" ],
    [ 1253,  "VENDORS/MICSFT/WINDOWS/CP1253.TXT", "ANSI Greek" ],
    [ 1254,  "VENDORS/MICSFT/WINDOWS/CP1254.TXT", "ANSI Turkish" ],
    [ 1255,  "VENDORS/MICSFT/WINDOWS/CP1255.TXT", "ANSI Hebrew" ],
    [ 1256,  "VENDORS/MICSFT/WINDOWS/CP1256.TXT", "ANSI Arabic" ],
    [ 1257,  "VENDORS/MICSFT/WINDOWS/CP1257.TXT", "ANSI Baltic" ],
    [ 1258,  "VENDORS/MICSFT/WINDOWS/CP1258.TXT", "ANSI/OEM Viet Nam" ],
    [ 10000, "VENDORS/MICSFT/MAC/ROMAN.TXT",      "Mac Roman" ],
    [ 10006, "VENDORS/MICSFT/MAC/GREEK.TXT",      "Mac Greek" ],
    [ 10007, "VENDORS/MICSFT/MAC/CYRILLIC.TXT",   "Mac Cyrillic" ],
    [ 10029, "VENDORS/MICSFT/MAC/LATIN2.TXT",     "Mac Latin 2" ],
    [ 10079, "VENDORS/MICSFT/MAC/ICELAND.TXT",    "Mac Icelandic" ],
    [ 10081, "VENDORS/MICSFT/MAC/TURKISH.TXT",    "Mac Turkish" ],
    [ 20866, "VENDORS/MISC/KOI8-R.TXT",           "Russian KOI8" ],
    [ 28591, "ISO8859/8859-1.TXT",                "ISO 8859-1 Latin 1" ],
    [ 28592, "ISO8859/8859-2.TXT",                "ISO 8859-2 Eastern Europe" ],
    [ 28593, "ISO8859/8859-3.TXT",                "ISO 8859-3 Turkish" ],
    [ 28594, "ISO8859/8859-4.TXT",                "ISO 8859-4 Baltic" ],
    [ 28595, "ISO8859/8859-5.TXT",                "ISO 8859-5 Cyrillic" ],
    [ 28596, "ISO8859/8859-6.TXT",                "ISO 8859-6 Arabic" ],
    [ 28597, "ISO8859/8859-7.TXT",                "ISO 8859-7 Greek" ],
    [ 28598, "ISO8859/8859-8.TXT",                "ISO 8859-8 Hebrew" ],
    [ 28599, "ISO8859/8859-9.TXT",                "ISO 8859-9 Latin 5" ]
);

################################################################
# main routine

READ_DEFAULTS();

foreach $file (@allfiles) { HANDLE_FILE( @$file ); }

OUTPUT_CPTABLE();

exit(0);


################################################################
# read in the defaults file
sub READ_DEFAULTS
{
    @unicode_defaults = ();
    @unicode_aliases = ();

    # first setup a few default mappings

    open DEFAULTS or die "Cannot open $DEFAULTS";
    print "Loading $DEFAULTS\n";
    while (<DEFAULTS>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        if (/^(([0-9a-fA-F]+)(,[0-9a-fA-F]+)*)\s+([0-9a-fA-F]+|'.'|none)\s+(\#.*)?/)
        {
            my @src = map hex, split /,/,$1;
            my $dst = $4;
            my $comment = $5;
            if ($#src > 0) { push @unicode_aliases, \@src; }
            next if ($dst eq "none");
            $dst = ($dst =~ /\'.\'/) ? ord substr($dst,1,1) : hex $dst;
            foreach $src (@src)
            {
                die "Duplicate value" if defined($unicode_defaults[$src]);
                $unicode_defaults[$src] = $dst;
            }
            next;
        }
        die "Unrecognized line $_\n";
    }

    # now build mappings from the decomposition field of the Unicode database

    open UNICODEDATA or die "Cannot open $UNICODEDATA";
    while (<UNICODEDATA>)
    {
	# Decode the fields ...
	($code, $name, $cat, $comb, $bidi, 
	 $decomp, $dec, $dig, $num, $mirror, 
	 $oldname, $comment, $upper, $lower, $title) = split /;/;

        next if $decomp eq "";  # no decomposition, skip it

        $src = hex $code;

        if ($decomp =~ /^<([a-zA-Z]+)>\s+([0-9a-fA-F]+)$/)
        {
            # decomposition of the form "<foo> 1234" -> use char if type is known
            next unless ($1 eq "font" ||
                         $1 eq "noBreak" ||
                         $1 eq "circle" ||
                         $1 eq "super" ||
                         $1 eq "sub" ||
                         $1 eq "wide" ||
                         $1 eq "narrow" ||
                         $1 eq "compat" ||
                         $1 eq "small");
            $dst = hex $2;
        }
        elsif ($decomp =~ /^<compat>\s+0020\s+([0-9a-fA-F]+)/)
        {
            # decomposition "<compat> 0020 1234" -> combining accent
            $dst = hex $1;
        }
        elsif ($decomp =~ /^([0-9a-fA-F]+)/)
        {
            # decomposition contains only char values without prefix -> use first char
            $dst = hex $1;
        }
        else
        {
            next;
        }

        next if defined($unicode_defaults[$src]);  # may have been set in the defaults file

        # check for loops
        for ($i = $dst; ; $i = $unicode_defaults[$i])
        {
            die sprintf("loop detected for %04x -> %04x",$src,$dst) if $i == $src;
            last unless defined($unicode_defaults[$i]);
        }
        $unicode_defaults[$src] = $dst;
    }
}


################################################################
# parse the input file
sub READ_FILE
{
    my $name = shift;
    open INPUT,$name or die "Cannot open $name";
    @cp2uni = ();
    @lead_bytes = ();
    @uni2cp = ();

    while (<INPUT>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        next if /\x1a/;  # skip ^Z
        next if (/^0x([0-9a-fA-F]+)\s+\#UNDEFINED/);  # undefined char

        if (/^0x([0-9a-fA-F]+)\s+\#DBCS LEAD BYTE/)
        {
            $cp = hex $1;
            push @lead_bytes,$cp;
            $cp2uni[$cp] = 0;
            next;
        }
        if (/^0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\#.*)?/)
        {
            $cp = hex $1;
            $uni = hex $2;
            $cp2uni[$cp] = $uni unless defined($cp2uni[$cp]);
            $uni2cp[$uni] = $cp unless defined($uni2cp[$uni]);
            next;
        }
        die "$name: Unrecognized line $_\n";
    }
}


################################################################
# parse the symbol.txt file, since its syntax is different from the other ones
sub READ_SYMBOL_FILE
{
    my $name = shift;
    open INPUT,$name or die "Cannot open $name";
    @cp2uni = ();
    @lead_bytes = ();
    @uni2cp = ();

    while (<INPUT>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        next if /\x1a/;  # skip ^Z
        if (/^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+(\#.*)?/)
        {
            $uni = hex $1;
            $cp = hex $2;
            $cp2uni[$cp] = $uni unless defined($cp2uni[$cp]);
            $uni2cp[$uni] = $cp unless defined($uni2cp[$uni]);
            next;
        }
        die "$name: Unrecognized line $_\n";
    }
}


################################################################
# add default mappings once the file had been read
sub ADD_DEFAULT_MAPPINGS
{
    # Apply aliases

    foreach $alias (@unicode_aliases)
    {
        my $target = undef;
        foreach $src (@$alias)
        {
            if (defined($uni2cp[$src]))
            {
                $target = $uni2cp[$src];
                last;
            }
        }
        next unless defined($target);

        # At least one char of the alias set is defined, set the others to the same value
        foreach $src (@$alias)
        {
            $uni2cp[$src] = $target unless defined($uni2cp[$src]);
        }
    }

    # For every src -> target mapping in the defaults table,
    # make uni2cp[src] = uni2cp[target] if uni2cp[target] is defined

    for ($src = 0; $src < 65536; $src++)
    {
        next if defined($uni2cp[$src]);  # source has a definition already
        next unless defined($unicode_defaults[$src]);  # no default for this char
        my $target = $unicode_defaults[$src];

        # do a recursive mapping until we find a target char that is defined
        while (!defined($uni2cp[$target]) &&
               defined($unicode_defaults[$target])) { $target = $unicode_defaults[$target]; }

        if (defined($uni2cp[$target])) { $uni2cp[$src] = $uni2cp[$target]; }
    }

    # Add an identity mapping for all undefined chars

    for ($i = 0; $i < 256; $i++)
    {
        next if defined($cp2uni[$i]);
        next if defined($uni2cp[$i]);
        $cp2uni[$i] = $uni2cp[$i] = $i;
    }
}


################################################################
# dump an SBCS mapping table
sub DUMP_SBCS_TABLE
{
    my ($codepage, $name) = @_;
    my $x, $y;

    # output the ascii->unicode table

    printf OUTPUT "static const unsigned short cp2uni[256] =\n{\n    ";
    my $i = 0;
    for ($i = 0; $i < 256; $i++)
    {
        printf OUTPUT "0x%04x", (defined $cp2uni[$i] ? $cp2uni[$i] : $DEF_CHAR);
        if (($i % 8) != 7) { printf OUTPUT ", "; }
        else { print OUTPUT (($i < 255) ? ",\n    " : "\n};\n\n"); }
    }

    # count the number of unicode->ascii subtables that contain something

    my @filled = ();
    my $subtables = 1;
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $uni2cp[$i];
        $filled[$i >> 8] = 1;
        $subtables++;
        $i = ($i & ~255) + 256;
    }

    # output all the subtables into a single array

    printf OUTPUT "static const unsigned char uni2cp_low[%d] =\n{\n   ", $subtables*256;
    for ($y = 0; $y < 256; $y++)
    {
        next unless $filled[$y];
        printf OUTPUT " /* 0x%02x00 .. 0x%02xff */\n   ", $y, $y;
        for ($x = 0; $x < 256; $x++)
        {
            printf OUTPUT " 0x%02x,", (defined $uni2cp[($y<<8)+$x] ?
                                     $uni2cp[($y<<8)+$x] : $DEF_CHAR);
            if (($x % 8) == 7) { printf OUTPUT "\n   "; }
        }
    }
    printf OUTPUT " /* defaults */\n   ";
    for ($x = 0; $x < 256; $x++)
    {
        printf OUTPUT " 0x%02x", $DEF_CHAR;
        if (($x % 8) != 7) { printf OUTPUT ","; }
        else { print OUTPUT (($x < 255) ? ",\n   " : "\n};\n\n"); }
    }

    # output a table of the offsets of the subtables in the previous array

    printf OUTPUT "static const unsigned short uni2cp_high[256] =\n{\n    ";
    my $pos = 0;
    for ($y = 0; $y < 256; $y++)
    {
        if ($filled[$y])
        {
            printf OUTPUT "0x%04x", $pos;
            $pos += 256;
        }
        else { printf OUTPUT "0x%04x", ($subtables-1) * 256; }
        if (($y % 8) != 7) { printf OUTPUT ", "; }
        else { print OUTPUT (($y < 255) ? ",\n    " : "\n};\n\n"); }
    }

    # output the code page descriptor

    printf OUTPUT "const struct sbcs_table cptable_%03d =\n{\n", $codepage;
    printf OUTPUT "    { %d, 1, 0x%04x, 0x%04x, \"%s\" },\n",
                  $codepage, $DEF_CHAR, $DEF_CHAR, $name;
    printf OUTPUT "    cp2uni,\n";
    printf OUTPUT "    uni2cp_low,\n";
    printf OUTPUT "    uni2cp_high\n};\n";
}


################################################################
# dump a DBCS mapping table
sub DUMP_DBCS_TABLE
{
    my ($codepage, $name) = @_;
    my $i, $x, $y;

    # build a list of lead bytes that are actually used

    my @lblist = ();
    LBLOOP: for ($y = 0; $y <= $#lead_bytes; $y++)
    {
        my $base = $lead_bytes[$y] << 8;
        for ($x = 0; $x < 256; $x++)
        {
            if (defined $cp2uni[$base+$x])
            {
                push @lblist,$lead_bytes[$y];
                next LBLOOP;
            }
        }
    }
    my $unused = ($#lead_bytes > $#lblist);

    # output the ascii->unicode table for the single byte chars

    printf OUTPUT "static const unsigned short cp2uni[%d] =\n{\n    ",
                  256 * ($#lblist + 2 + $unused);
    for ($x = 0; $x < 256; $x++)
    {
        printf OUTPUT "0x%04x", (defined $cp2uni[$x] ? $cp2uni[$x] : $DEF_CHAR);
        if (($x % 8) != 7) { printf OUTPUT ", "; }
        else { print OUTPUT ",\n    "; }
    }

    # output the default table for unused lead bytes

    if ($unused)
    {
        printf OUTPUT "/* unused lead bytes */\n    ";
        for ($x = 0; $x < 256; $x++)
        {
            printf OUTPUT "0x%04x", $DEF_CHAR;
            if (($x % 8) != 7) { printf OUTPUT ", "; }
            else { print OUTPUT ",\n    "; }
        }
    }

    # output the ascii->unicode table for each DBCS lead byte

    for ($y = 0; $y <= $#lblist; $y++)
    {
        my $base = $lblist[$y] << 8;
        printf OUTPUT "/* lead byte %02x */\n    ", $lblist[$y];
        for ($x = 0; $x < 256; $x++)
        {
            printf OUTPUT "0x%04x", (defined $cp2uni[$base+$x] ? $cp2uni[$base+$x] : $DEF_CHAR);
            if (($x % 8) != 7) { printf OUTPUT ", "; }
            else { print OUTPUT (($x < 255 || $y < $#lblist) ? ",\n    " : "\n};\n\n"); }
        }
    }

    # output the lead byte subtables offsets

    my @offsets = ();
    for ($x = 0; $x < 256; $x++) { $offsets[$x] = 0; }
    for ($x = 0; $x <= $#lblist; $x++) { $offsets[$lblist[$x]] = $x + 1; }
    if ($unused)
    {
        # increment all lead bytes offset to take into account the unused table
        for ($x = 0; $x <= $#lead_bytes; $x++) { $offsets[$lead_bytes[$x]]++; }
    }

    printf OUTPUT "static const unsigned char cp2uni_leadbytes[256] =\n{\n    ";
    for ($x = 0; $x < 256; $x++)
    {
        printf OUTPUT "0x%02x", $offsets[$x];
        if (($x % 8) != 7) { printf OUTPUT ", "; }
        else { print OUTPUT (($x < 255) ? ",\n    " : "\n};\n\n"); }
    }

    # count the number of unicode->ascii subtables that contain something

    my @filled = ();
    my $subtables = 1;
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $uni2cp[$i];
        $filled[$i >> 8] = 1;
        $subtables++;
        $i = ($i & ~255) + 256;
    }

    # output all the subtables into a single array

    printf OUTPUT "static const unsigned short uni2cp_low[%d] =\n{\n   ", $subtables*256;
    for ($y = 0; $y < 256; $y++)
    {
        next unless $filled[$y];
        printf OUTPUT " /* 0x%02x00 .. 0x%02xff */\n   ", $y, $y;
        for ($x = 0; $x < 256; $x++)
        {
            printf OUTPUT " 0x%04x,", (defined $uni2cp[($y<<8)+$x] ?
                                     $uni2cp[($y<<8)+$x] : $DEF_CHAR);
            if (($x % 8) == 7) { printf OUTPUT "\n   "; }
        }
    }
    printf OUTPUT " /* defaults */\n   ";
    for ($x = 0; $x < 256; $x++)
    {
        printf OUTPUT " 0x%04x", $DEF_CHAR;
        if (($x % 8) != 7) { printf OUTPUT ","; }
        else { print OUTPUT (($x < 255) ? ",\n   " : "\n};\n\n"); }
    }

    # output a table of the offsets of the subtables in the previous array

    printf OUTPUT "static const unsigned short uni2cp_high[256] =\n{\n    ";
    my $pos = 0;
    for ($y = 0; $y < 256; $y++)
    {
        if ($filled[$y])
        {
            printf OUTPUT "0x%04x", $pos;
            $pos += 256;
        }
        else { printf OUTPUT "0x%04x", ($subtables-1) * 256; }

        if (($y % 8) != 7) { printf OUTPUT ", "; }
        else { print OUTPUT (($y < 255) ? ",\n    " : "\n};\n\n"); }
    }

    # output the code page descriptor

    printf OUTPUT "const struct dbcs_table cptable_%03d =\n{\n", $codepage;
    printf OUTPUT "    { %d, 2, 0x%04x, 0x%04x, \"%s\" },\n",
                  $codepage, $DEF_CHAR, $DEF_CHAR, $name;
    printf OUTPUT "    cp2uni,\n";
    printf OUTPUT "    cp2uni_leadbytes,\n";
    printf OUTPUT "    uni2cp_low,\n";
    printf OUTPUT "    uni2cp_high,\n";
    DUMP_LB_RANGES();
    printf OUTPUT "};\n";
}


################################################################
# dump the list of defined lead byte ranges
sub DUMP_LB_RANGES
{
    my @list = ();
    my $i = 0;
    foreach $i (@lead_bytes) { $list[$i] = 1; }
    my $on = 0;
    printf OUTPUT "    { ";
    for ($i = 0; $i < 256; $i++)
    {
        if ($on)
        {
            if (!defined $list[$i]) { printf OUTPUT "0x%02x, ", $i-1; $on = 0; }
        }
        else
        {
            if ($list[$i]) { printf OUTPUT "0x%02x, ", $i; $on = 1; }
        }
    }
    if ($on) { printf OUTPUT "0xff, "; }
    printf OUTPUT "0x00, 0x00 }\n";
}


################################################################
# read an input file and generate the corresponding .c file
sub HANDLE_FILE
{
    my ($codepage,$filename,$comment) = @_;

    # symbol codepage file is special
    if ($codepage == 42) { READ_SYMBOL_FILE($MAPPREFIX . $filename); }
    else { READ_FILE($MAPPREFIX . $filename); }

    ADD_DEFAULT_MAPPINGS();

    my $output = sprintf "c_%03d.c", $codepage;
    open OUTPUT,">$output" or die "Cannot create $output";

    printf "Building %s from %s (%s)\n", $output, $filename, $comment;

    # dump all tables

    printf OUTPUT "/* code page %03d (%s) */\n", $codepage, $comment;
    printf OUTPUT "/* generated from %s */\n", $MAPPREFIX . $filename;
    printf OUTPUT "/* DO NOT EDIT!! */\n\n";
    printf OUTPUT "#include \"wine/unicode.h\"\n\n";

    if ($#lead_bytes == -1) { DUMP_SBCS_TABLE( $codepage, $comment ); }
    else { DUMP_DBCS_TABLE( $codepage, $comment ); }
}


################################################################
# output the list of codepage tables into the cptable.c file
sub OUTPUT_CPTABLE
{
    @tables_decl = ();

    foreach $file (@allfiles)
    {
        my ($codepage,$filename,$comment) = @$file;
        push @tables_decl, sprintf("extern union cptable cptable_%03d;\n",$codepage);
    }

    push @tables_decl, sprintf("\nstatic const union cptable * const cptables[%d] =\n{\n",$#allfiles+1);
    foreach $file (@allfiles)
    {
        my ($codepage,$filename,$comment) = @$file;
        push @tables_decl, sprintf("    &cptable_%03d,\n", $codepage);
    }
    push @tables_decl, "};";
    REPLACE_IN_FILE( "cptable.c", @tables_decl );
}

################################################################
# replace the contents of a file between ### cpmap ### marks

sub REPLACE_IN_FILE
{
    my $name = shift;
    my @data = @_;
    my @lines = ();
    open(FILE,$name) or die "Can't open $name";
    while (<FILE>)
    {
	push @lines, $_;
	last if /\#\#\# cpmap begin \#\#\#/;
    }
    push @lines, @data;
    while (<FILE>)
    {
	if (/\#\#\# cpmap end \#\#\#/) { push @lines, "\n", $_; last; }
    }
    push @lines, <FILE>;
    open(FILE,">$name") or die "Can't modify $name";
    print FILE @lines;
    close(FILE);
}
