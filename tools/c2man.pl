#!/usr/bin/perl

#####################################################################################
# 
# c2man.pl v0.1  Copyright (C) 2000 Mike McCormack
#
# Generates Documents from C source code.
#
# Input is source code with specially formatted comments, output
# is man pages. The functionality is meant to be similar to c2man.
# The following is an example provided in the Wine documentation.
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
# TODO:
#  Write code to generate HTML output with the -Th option.
#  Need somebody who knows about TROFF to help touch up the man page generation.
#  Parse spec files passed with -w option and generate pages for the functions
#   in the spec files only.
#  Modify Makefiles to pass multiple C files to speed up man page generation.
#  Use nm on the shared libraries specified in the spec files to determine which
#   source files should be parsed, and only parse them.(requires wine to be compiled)
#
#####################################################################################
# Input from C source file:
# 
# /******************************************************************
#  *         CopyMetaFile32A   (GDI32.23)
#  *
#  *  Copies the metafile corresponding to hSrcMetaFile to either
#  *  a disk file, if a filename is given, or to a new memory based
#  *  metafile, if lpFileName is NULL.
#  *
#  * RETURNS
#  *
#  *  Handle to metafile copy on success, NULL on failure.
#  *
#  * BUGS
#  *
#  *  Copying to disk returns NULL even if successful.
#  */
# HMETAFILE32 WINAPI CopyMetaFile32A(
#                    HMETAFILE32 hSrcMetaFile, /* handle of metafile to copy */
#                    LPCSTR lpFilename /* filename if copying to a file */
# ) { ... }
#
#####################################################################################
# Output after processing with nroff -man
# 
# CopyMetaFileA(3w)                               CopyMetaFileA(3w)
# 
# 
# NAME
#        CopyMetaFileA - CopyMetaFile32A   (GDI32.23)
#  
# SYNOPSIS
#        HMETAFILE32 CopyMetaFileA
#        (
#             HMETAFILE32 hSrcMetaFile,
#             LPCSTR lpFilename
#        );
#  
# PARAMETERS
#        HMETAFILE32 hSrcMetaFile
#               Handle of metafile to copy.
#  
#        LPCSTR lpFilename
#               Filename if copying to a file.
#  
# DESCRIPTION
#        Copies  the  metafile  corresponding  to  hSrcMetaFile  to
#        either a disk file, if a filename is given, or  to  a  new
#        memory based metafile, if lpFileName is NULL.
#  
# RETURNS
#        Handle to metafile copy on success, NULL on failure.
#  
# BUGS
#        Copying to disk returns NULL even if successful.
#  
# SEE ALSO
#        GetMetaFileA(3w),   GetMetaFileW(3w),   CopyMetaFileW(3w),
#        PlayMetaFile(3w),  SetMetaFileBitsEx(3w),  GetMetaFileBit-
#        sEx(3w)
# 
#####################################################################################

sub output_manpage
{
    my ($buffer,$apiref) = @_;
    my $parameters;
    my $desc;

    # join all the lines of the description together and highlight the headings
    for (@$buffer) {
        s/\n//g;
        s/^\s*//g;
        s/\s*$//g;
        if ( /^([A-Z]+)$/ ) {
            $desc = $desc.".SH $1\n.PP\n";
        }
        elsif ( /^$/ ) {
            $desc = "$desc\n";
        }
        else {
            $desc = "$desc $_";
        }
    }

    #seperate out all the parameters

    $plist = join ( ' ', @$apiref );

    $name_type = $plist;
    $name_type =~ s/\n//g;         # remove newlines
    $name_type =~ s/\(.*$//;
    $name_type =~ s/WINAPI//;

    #check that this is a function that we want
    if ( $funcdb{$apiname."ORD"} eq "" ) { return; }
    print "Generating $apiname.$section\n";

    $plist =~ s/\n//g;         # remove newlines
    $plist =~ s/^.*\(\s*//;       # remove leading bracket and before
    $plist =~ s/\s*\).*$//;       # remove trailing bracket and leftovers
    $plist =~ s/\s*,?\s*\/\*([^*]*)\*\// - $1,/g; # move the comma to the back
    @params = split ( /,/ , $plist);  # split parameters
    for(@params) {
        s/^\s*//;
        s/\s*$//;
    }

    # figure the month and the year
    @datetime = localtime;
    @months = ( "January", "Febuary", "March", "April", "May", "June",
                "July", "August", "September", "October", "November", "December" );
    $date = "$months[$datetime[4]] $datetime[5]";

    # create the manual page
    $manfile = "$mandir/$apiname.$section";
    open(MAN,">$manfile") || die "Couldn't create the man page file $manfile\n";
    print MAN ".\\\" DO NOT MODIFY THIS FILE!  It was generated by gendoc 1.0.\n";
    print MAN ".TH $apiname \"$section\" \"$date\" \"Wine API\" \"The Wine Project\"\n";
    print MAN ".SH NAME\n";
    print MAN "$apiname ($apientry)\n";
    print MAN ".SH SYNOPSIS\n";
    print MAN ".PP\n";
    print MAN "$name_type\n";
    print MAN " (\n";
    for($i=0; $i<@params; $i++) { 
        $x = ($i == (@params-1)) ? "" : ",";
        $c = $params[$i];
        $c =~ s/-.*//;
        print MAN "    $c$x\n";
    }
    print MAN " );\n";
    print MAN ".SH PARAMETERS\n";
    print MAN ".PP\n";
    for($i=0; $i<@params; $i++) { 
        print MAN "    $params[$i]\n";
    }
    print MAN ".SH DESCRIPTION\n";
    print MAN ".PP\n";
    print MAN $desc;
    close(MAN);
}

#
# extract the comments from source file
#
sub parse_source
{
  my $file = $_[0];
  print "Processing $file\n";

  open(SOURCE,"<$file") || die "Couldn't open the source file $file\n";
  $state = 0;
  while(<SOURCE>) {
    if($state == 0 ) {
        if ( /^\/\**$/ ) {
            # find the start of the comment /**************
            $state = 3;
            @buffer = ();
        }
    }
    elsif ($state == 3) {
        #extract the wine API name and DLLNAME.XXX string
        if ( / *([A-Za-z_0-9]+) *\(([A-Za-z0-9_]+\.(([0-9]+)|@))\) *$/ ) {
            $apiname = $1;
            $apientry = $2;
            $state = 1;
        }
        else {
            $state = 0;
        }
    }
    elsif ($state == 1) {
        #save the comment text into buffer, removing leading astericks
        if ( /^ \*\// ) {
            $state = 2;
        }
        else {
            # find the end of the comment
            if ( s/^ \*// ) {
                @buffer = ( @buffer , $_ );
            }
            else {
                $state = 0;
            }
        }
    }
    elsif ($state == 2) {
        # check that the comment is followed by the declaration of
        # a WINAPI function.
        if ( /WINAPI/ ) {
            @apidef = ( $_ );
            #check if the function's parameters end on this line
            if( /\)/ ) {
                output_manpage(\@buffer, \@apidef);
                $state = 0;
            }
            else {
                $state = 4;
            }
        }
        else {
            $state = 0;
        }
    }
    elsif ($state == 4) {
        @apidef = ( @apidef , $_ );
        #find the end of the parameters list
        if( /\)/ ) {
            output_manpage(\@buffer, \@apidef);
            $state = 0;
        }
    }
  }
  close(SOURCE);
}

# generate a database of functions to have man pages created from the source
# creates funclist and funcdb
sub parse_spec
{
    my $spec = $_[0];
    my $name,$type,$ord,$func;

    open(SPEC,"<$spec") || die "Couldn't open the spec file $spec\n";
    while(<SPEC>)
    {
        if( /^#/ ) { next; }
        if( /^name/ ) { next; }
        if( /^type/ ) { next; }
        if( /^init/ ) { next; }
        if( /^rsrc/ ) { next; }
        if( /^import/ ) { next; }
        if( /^\s*$/ ) { next; }
        if( /^\s*(([0-9]+)|@)/ ) {
            s/\(.*\)//; #remove all the args
            ($ord,$type,$name,$func) = split( /\s+/ );
            if(( $type eq "stub" ) || ($type eq "forward")) {next;}
            if( $func eq "" ) { next; } 
            @funclist = ( @funclist , $func );
            $funcdb{$func."ORD"} = $ord;
            $funcdb{$func."TYPE"} = $type;
            $funcdb{$func."NAME"} = $name;
            $funcdb{$func."SPEC"} = $spec;
        }
    }
    close(SPEC);
}

######################################################################

#main starts here

$mandir = "man3w";
$section = "3";

#process args
while(@ARGV) {
    if($ARGV[0] eq "-o") {      # extract output directory
        shift @ARGV;
        $mandir = $ARGV[0];
        shift @ARGV;
        next;
    }
    if($ARGV[0] =~ s/^-S// ) {  # extract man section
        $section = $ARGV[0];
        shift @ARGV;
        next;
    }
    if($ARGV[0] =~ s/^-w// ) {  # extract man section
        shift @ARGV;
        @specfiles = ( @specfiles , $ARGV[0] );
        shift @ARGV;
        next;
    }
    if($ARGV[0] =~ s/^-T// ) {
        die "FIXME: Only NROFF supported\n";
    }
    if($ARGV[0] =~ s/^-[LDiI]// ) {  #compatible with C2MAN flags
        shift @ARGV;
        next;
    }
    last; # stop after there's no more flags
}

#print "manual section: $section\n";
#print "man directory : $mandir\n";
#print "input files   : @ARGV\n";
#print "spec files    : @specfiles\n";

while(@specfiles) {
    parse_spec($specfiles[0]);
    shift @specfiles;
}

#print "Functions: @funclist\n";

while(@ARGV) {
    parse_source($ARGV[0]);
    shift @ARGV;
}

