#! /usr/bin/perl -w
#
# Generate API documentation. See http://www.winehq.org/docs/winedev-guide/api-docs for details.
#
# Copyright (C) 2000 Mike McCormack
# Copyright (C) 2003 Jon Griffiths
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
# TODO
#  Consolidate A+W pairs together, and only write one doc, without the suffix
#  Implement automatic docs of structs/defines in headers
#  SGML gurus - feel free to smarten up the SGML.
#  Add any other relevant information for the dll - imports etc
#  Should we have a special output mode for WineHQ?

use strict;
use bytes;

# Function flags. most of these come from the spec flags
my $FLAG_DOCUMENTED = 1;
my $FLAG_NONAME     = 2;
my $FLAG_I386       = 4;
my $FLAG_REGISTER   = 8;
my $FLAG_APAIR      = 16; # The A version of a matching W function
my $FLAG_WPAIR      = 32; # The W version of a matching A function
my $FLAG_64PAIR     = 64; # The 64 bit version of a matching 32 bit function

# Export list slot labels.
my $EXPORT_ORDINAL  = 0;  # Ordinal.
my $EXPORT_CALL     = 1;  # Call type.
my $EXPORT_EXPNAME  = 2;  # Export name.
my $EXPORT_IMPNAME  = 3;  # Implementation name.
my $EXPORT_FLAGS    = 4;  # Flags - see above.

# Options
my $opt_output_directory = "man3w"; # All default options are for nroff (man pages)
my $opt_manual_section = "3w";
my $opt_source_dir = ".";
my $opt_parent_dir = "";
my $opt_wine_root_dir = "";
my $opt_output_format = "";         # '' = nroff, 'h' = html, 's' = sgml, 'x' = xml
my $opt_output_empty = 0;           # Non-zero = Create 'empty' comments (for every implemented function)
my $opt_fussy = 1;                  # Non-zero = Create only if we have a RETURNS section
my $opt_verbose = 0;                # >0 = verbosity. Can be given multiple times (for debugging)
my @opt_header_file_list = ();
my @opt_spec_file_list = ();
my @opt_source_file_list = ();

# All the collected details about all the .spec files being processed
my %spec_files;
# All the collected details about all the source files being processed
my %source_files;
# All documented functions that are to be placed in the index
my @index_entries_list = ();

# useful globals
my $pwd = `pwd`."/";
$pwd =~ s/\n//;
my @datetime = localtime;
my @months = ( "Jan", "Feb", "Mar", "Apr", "May", "Jun",
               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" );
my $year = $datetime[5] + 1900;
my $date = "$months[$datetime[4]] $year";


sub output_api_comment($);
sub output_api_footer($);
sub output_api_header($);
sub output_api_name($);
sub output_api_synopsis($);
sub output_close_api_file();
sub output_comment($);
sub output_html_index_files();
sub output_html_stylesheet();
sub output_open_api_file($);
sub output_sgml_dll_file($);
sub output_xml_dll_file($);
sub output_sgml_master_file($);
sub output_xml_master_file($);
sub output_spec($);
sub process_comment($);
sub process_extra_comment($);


# Generate the list of exported entries for the dll
sub process_spec_file($)
{
  my $spec_name = shift;
  (my $basename = $spec_name) =~ s/.*\///;
  my ($dll_name, $dll_ext)  = split(/\./, $basename);
  $dll_ext = "dll" if ( $dll_ext eq "spec" );
  my $uc_dll_name  = uc $dll_name;

  my $spec_details =
  {
    NAME => $basename,
    DLL_NAME => $dll_name,
    DLL_EXT => $dll_ext,
    NUM_EXPORTS => 0,
    NUM_STUBS => 0,
    NUM_FUNCS => 0,
    NUM_FORWARDS => 0,
    NUM_VARS => 0,
    NUM_DOCS => 0,
    CONTRIBUTORS => [ ],
    SOURCES => [ ],
    DESCRIPTION => [ ],
    EXPORTS => [ ],
    EXPORTED_NAMES => { },
    IMPLEMENTATION_NAMES => { },
    EXTRA_COMMENTS => [ ],
    CURRENT_EXTRA => [ ] ,
  };

  if ($opt_verbose > 0)
  {
    print "Processing ".$spec_name."\n";
  }

  # We allow opening to fail just to cater for the peculiarities of
  # the Wine build system. This doesn't hurt, in any case
  open(SPEC_FILE, "<$spec_name")
  || (($opt_source_dir ne "")
      && open(SPEC_FILE, "<$opt_source_dir/$spec_name"))
  || return;

  while(<SPEC_FILE>)
  {
    s/^\s+//;            # Strip leading space
    s/\s+\n$/\n/;        # Strip trailing space
    s/\s+/ /g;           # Strip multiple tabs & spaces to a single space
    s/\s*#.*//;          # Strip comments
    s/\(.*\)/ /;         # Strip arguments
    s/\s+/ /g;           # Strip multiple tabs & spaces to a single space (again)
    s/\n$//;             # Strip newline

    my $flags = 0;
    if( /\-noname/ )
    {
      $flags |= $FLAG_NONAME;
    }
    if( /\-i386/ )
    {
      $flags |= $FLAG_I386;
    }
    if( /\-register/ )
    {
      $flags |= $FLAG_REGISTER;
    }
    s/ \-[a-z0-9=_]+//g;   # Strip flags

    if( /^(([0-9]+)|@) / )
    {
      # This line contains an exported symbol
      my ($ordinal, $call_convention, $exported_name, $implementation_name) = split(' ');

      for ($call_convention)
      {
        /^(cdecl|stdcall|varargs|pascal)$/
                 && do { $spec_details->{NUM_FUNCS}++;    last; };
        /^(variable|equate)$/
                 && do { $spec_details->{NUM_VARS}++;     last; };
        /^(extern)$/
                 && do { $spec_details->{NUM_FORWARDS}++; last; };
        /^stub$/ && do { $spec_details->{NUM_STUBS}++;    last; };
        if ($opt_verbose > 0)
        {
          print "Warning: didn't recognise convention \'",$call_convention,"'\n";
        }
        last;
      }

      # Convert ordinal only names so we can find them later
      if ($exported_name eq "@")
      {
        $exported_name = $uc_dll_name.'_'.$ordinal;
      }
      if (!defined($implementation_name))
      {
        $implementation_name = $exported_name;
      }
      if ($implementation_name eq "")
      {
        $implementation_name = $exported_name;
      }

      if ($implementation_name =~ /(.*?)\./)
      {
        $call_convention = "forward"; # Referencing a function from another dll
        $spec_details->{NUM_FUNCS}--;
        $spec_details->{NUM_FORWARDS}++;
      }

      # Add indices for the exported and implementation names
      $spec_details->{EXPORTED_NAMES}{$exported_name} = $spec_details->{NUM_EXPORTS};
      if ($implementation_name ne $exported_name)
      {
        $spec_details->{IMPLEMENTATION_NAMES}{$exported_name} = $spec_details->{NUM_EXPORTS};
      }

      # Add the exported entry
      $spec_details->{NUM_EXPORTS}++;
      my @export = ($ordinal, $call_convention, $exported_name, $implementation_name, $flags);
      push (@{$spec_details->{EXPORTS}},[@export]);
    }
  }
  close(SPEC_FILE);

  # Add this .spec files details to the list of .spec files
  $spec_files{$uc_dll_name} = [$spec_details];
}

# Read each source file, extract comments, and generate API documentation if appropriate
sub process_source_file($)
{
  my $source_file = shift;
  my $source_details =
  {
    CONTRIBUTORS => [ ],
    DEBUG_CHANNEL => "",
  };
  my $comment =
  {
    FILE => $source_file,
    COMMENT_NAME => "",
    ALT_NAME => "",
    DLL_NAME => "",
    DLL_EXT => "",
    ORDINAL => "",
    RETURNS => "",
    PROTOTYPE => [],
    TEXT => [],
  };
  my $parse_state = 0;
  my $ignore_blank_lines = 1;
  my $extra_comment = 0; # 1 if this is an extra comment, i.e it's not a .spec export

  if ($opt_verbose > 0)
  {
    print "Processing ".$source_file."\n";
  }
  open(SOURCE_FILE,"<$source_file")
  || (($opt_source_dir ne ".")
      && open(SOURCE_FILE,"<$opt_source_dir/$source_file"))
  || (($opt_parent_dir ne "")
      && open(SOURCE_FILE,"<$opt_source_dir/$opt_parent_dir/$source_file"))
  || die "couldn't open ".$source_file."\n";

  # Add this source file to the list of source files
  $source_files{$source_file} = [$source_details];

  while(<SOURCE_FILE>)
  {
    s/\n$//;   # Strip newline
    s/^\s+//;  # Strip leading space
    s/\s+$//;  # Strip trailing space
    if (! /^\*\|/ )
    {
      # Strip multiple tabs & spaces to a single space
      s/\s+/ /g;
    }

    if ( / +Copyright *(\([Cc]\))*[0-9 \-\,\/]*([[:alpha:][:^ascii:] \.\-]+)/ )
    {
      # Extract a contributor to this file
      my $contributor = $2;
      $contributor =~ s/ *$//;
      $contributor =~ s/^by //;
      $contributor =~ s/\.$//;
      $contributor =~ s/ (for .*)/ \($1\)/;
      if ($contributor ne "")
      {
        if ($opt_verbose > 3)
        {
          print "Info: Found contributor:'".$contributor."'\n";
        }
        push (@{$source_details->{CONTRIBUTORS}},$contributor);
      }
    }
    elsif ( /WINE_DEFAULT_DEBUG_CHANNEL\(([A-Za-z]*)\)/ )
    {
      # Extract the debug channel to use
      if ($opt_verbose > 3)
      {
        print "Info: Found debug channel:'".$1."'\n";
      }
      $source_details->{DEBUG_CHANNEL} = $1;
    }

    if ($parse_state == 0) # Searching for a comment
    {
      if ( /^\/\**$/ )
      {
        # This file is used by the DLL - Make sure we get our contributors right
        @{$spec_files{$comment->{DLL_NAME}}[0]->{CURRENT_EXTRA}} = ();
        push (@{$spec_files{$comment->{DLL_NAME}}[0]->{SOURCES}},$comment->{FILE});
        # Found a comment start
        $comment->{COMMENT_NAME} = "";
        $comment->{ALT_NAME} = "";
        $comment->{DLL_NAME} = "";
        $comment->{ORDINAL} = "";
        $comment->{RETURNS} = "";
        $comment->{PROTOTYPE} = [];
        $comment->{TEXT} = [];
        $ignore_blank_lines = 1;
        $extra_comment = 0;
        $parse_state = 3;
      }
    }
    elsif ($parse_state == 1) # Reading in a comment
    {
      if ( /^\**\// )
      {
        # Found the end of the comment
        $parse_state = 2;
      }
      elsif ( s/^\*\|/\|/ )
      {
        # A line of comment not meant to be pre-processed
        push (@{$comment->{TEXT}},$_); # Add the comment text
      }
      elsif ( s/^ *\** *// )
      {
        # A line of comment, starting with an asterisk
        if ( /^[A-Z]+$/ || $_ eq "")
        {
          # This is a section start, so skip blank lines before and after it.
          my $last_line = pop(@{$comment->{TEXT}});
          if (defined($last_line) && $last_line ne "")
          {
            # Put it back
            push (@{$comment->{TEXT}},$last_line);
          }
          if ( /^[A-Z]+$/ )
          {
            $ignore_blank_lines = 1;
          }
          else
          {
            $ignore_blank_lines = 0;
          }
        }

        if ($ignore_blank_lines == 0 || $_ ne "")
        {
          push (@{$comment->{TEXT}},$_); # Add the comment text
        }
      }
      else
      {
        # This isn't a well formatted comment: look for the next one
        $parse_state = 0;
      }
    }
    elsif ($parse_state == 2) # Finished reading in a comment
    {
      if ( /(WINAPIV|WINAPI|__cdecl|PASCAL|CALLBACK|FARPROC16)/ ||
           /.*?\(/ )
      {
        # Comment is followed by a function definition
        $parse_state = 4; # Fall through to read prototype
      }
      else
      {
        # Allow cpp directives and blank lines between the comment and prototype
        if ($extra_comment == 1)
        {
          # An extra comment not followed by a function definition
          $parse_state = 5; # Fall through to process comment
        }
        elsif (!/^\#/ && !/^ *$/ && !/^__ASM_GLOBAL_FUNC/)
        {
          # This isn't a well formatted comment: look for the next one
          if ($opt_verbose > 1)
          {
            print "Info: Function '",$comment->{COMMENT_NAME},"' not followed by prototype.\n";
          }
          $parse_state = 0;
        }
      }
    }
    elsif ($parse_state == 3) # Reading in the first line of a comment
    {
      s/^ *\** *//;
      if ( /^([\@A-Za-z0-9_]+) +(\(|\[)([A-Za-z0-9_]+)\.(([0-9]+)|@)(\)|\])\s*(.*)$/ )
      {
        # Found a correctly formed "ApiName (DLLNAME.Ordinal)" line.
        if (defined ($7) && $7 ne "")
        {
          push (@{$comment->{TEXT}},$_); # Add the trailing comment text
        }
        $comment->{COMMENT_NAME} = $1;
        $comment->{DLL_NAME} = uc $3;
        $comment->{ORDINAL} = $4;
        $comment->{DLL_NAME} =~ s/^KERNEL$/KRNL386/; # Too many of these to ignore, _old_ code
        $parse_state = 1;
      }
      elsif ( /^([A-Za-z0-9_-]+) +\{([A-Za-z0-9_]+)\}$/ )
      {
        # Found a correctly formed "CommentTitle {DLLNAME}" line (extra documentation)
        $comment->{COMMENT_NAME} = $1;
        $comment->{DLL_NAME} = uc $2;
        $comment->{ORDINAL} = "";
        $extra_comment = 1;
        $parse_state = 1;
      }
      else
      {
        # This isn't a well formatted comment: look for the next one
        $parse_state = 0;
      }
    }

    if ($parse_state == 4) # Reading in the function definition
    {
      push (@{$comment->{PROTOTYPE}},$_);
      # Strip comments from the line before checking for ')'
      my $stripped_line = $_;
      $stripped_line =~ s/ *(\/\* *)(.*?)( *\*\/ *)//;
      if ( $stripped_line =~ /\)/ )
      {
        # Strip a blank last line
        my $last_line = pop(@{$comment->{TEXT}});
        if (defined($last_line) && $last_line ne "")
        {
          # Put it back
          push (@{$comment->{TEXT}},$last_line);
        }

        if ($opt_output_empty != 0 && @{$comment->{TEXT}} == 0)
        {
          # Create a 'not implemented' comment
          @{$comment->{TEXT}} = ("fixme: This function has not yet been documented.");
        }
        $parse_state = 5;
      }
    }

    if ($parse_state == 5) # Processing the comment
    {
      # Process it, if it has any text
      if (@{$comment->{TEXT}} > 0)
      {
        if ($extra_comment == 1)
        {
          process_extra_comment($comment);
        }
        else
        {
          @{$comment->{TEXT}} = ("DESCRIPTION", @{$comment->{TEXT}});
          process_comment($comment);
        }
      }
      elsif ($opt_verbose > 1 && $opt_output_empty == 0)
      {
        print "Info: Function '",$comment->{COMMENT_NAME},"' has no documentation.\n";
      }
      $parse_state = 0;
    }
  }
  close(SOURCE_FILE);
}

# Standardise a comments text for consistency
sub process_comment_text($)
{
  my $comment = shift;
  my $in_params = 0;
  my @tmp_list = ();
  my $i = 0;

  for (@{$comment->{TEXT}})
  {
    my $line = $_;

    if ( /^\s*$/ || /^[A-Z]+$/ || /^-/ )
    {
      $in_params = 0;
    }
    if ( $in_params > 0 && !/\[/ && !/\]/ )
    {
      # Possibly a continuation of the parameter description
      my $last_line = pop(@tmp_list);
      if ( $last_line =~ /\[/ && $last_line =~ /\]/ )
      {
        $line = $last_line." ".$_;
      }
      else
      {
        $in_params = 0;
        push (@tmp_list, $last_line);
      }
    }
    if ( /^(PARAMS|MEMBERS)$/ )
    {
      $in_params = 1;
    }
    push (@tmp_list, $line);
  }

  @{$comment->{TEXT}} = @tmp_list;

  for (@{$comment->{TEXT}})
  {
    if (! /^\|/ )
    {
      # Map I/O values. These come in too many formats to standardise now....
      s/\[I\]|\[i\]|\[in\]|\[IN\]/\[In\] /g;
      s/\[O\]|\[o\]|\[out\]|\[OUT\]/\[Out\]/g;
      s/\[I\/O\]|\[I\,O\]|\[i\/o\]|\[in\/out\]|\[IN\/OUT\]/\[In\/Out\]/g;
      # TRUE/FALSE/NULL are defines, capitalise them
      s/True|true/TRUE/g;
      s/False|false/FALSE/g;
      s/Null|null/NULL/g;
      # Preferred capitalisations
      s/ wine| WINE/ Wine/g;
      s/ API | api / Api /g;
      s/ DLL | Dll / dll /g;
      s/ URL | url / Url /g;
      s/WIN16|win16/Win16/g;
      s/WIN32|win32/Win32/g;
      s/WIN64|win64/Win64/g;
      s/ ID | id / Id /g;
      # Grammar
      s/([a-z])\.([A-Z])/$1\. $2/g; # Space after full stop
      s/ \:/\:/g;                   # Colons to the left
      s/ \;/\;/g;                   # Semi-colons too
      # Common idioms
      s/^See ([A-Za-z0-9_]+)\.$/See $1\(\)\./;                # Referring to A version from W
      s/^Unicode version of ([A-Za-z0-9_]+)\.$/See $1\(\)\./; # Ditto
      s/^64\-bit version of ([A-Za-z0-9_]+)\.$/See $1\(\)\./; # Referring to 32 bit version from 64
      s/^PARAMETERS$/PARAMS/;  # Name of parameter section should be 'PARAMS'
      # Trademarks
      s/( |\.)(M\$|MS|Microsoft|microsoft|micro\$oft|Micro\$oft)( |\.)/$1Microsoft\(tm\)$3/g;
      s/( |\.)(Windows|windows|windoze|winblows)( |\.)/$1Windows\(tm\)$3/g;
      s/( |\.)(DOS|dos|msdos)( |\.)/$1MS-DOS\(tm\)$3/g;
      s/( |\.)(UNIX|unix)( |\.)/$1Unix\(tm\)$3/g;
      s/( |\.)(LINIX|linux)( |\.)/$1Linux\(tm\)$3/g;
      # Abbreviations
      s/( char )/ character /g;
      s/( chars )/ characters /g;
      s/( info )/ information /g;
      s/( app )/ application /g;
      s/( apps )/ applications /g;
      s/( exe )/ executable /g;
      s/( ptr )/ pointer /g;
      s/( obj )/ object /g;
      s/( err )/ error /g;
      s/( bool )/ boolean /g;
      s/( no\. )/ number /g;
      s/( No\. )/ Number /g;
      # Punctuation
      if ( /\[I|\[O/ && ! /\.$/ )
      {
        $_ = $_."."; # Always have a full stop at the end of parameter desc.
      }
      elsif ($i > 0 && /^[A-Z]*$/  &&
               !(@{$comment->{TEXT}}[$i-1] =~ /\.$/) &&
               !(@{$comment->{TEXT}}[$i-1] =~ /\:$/))
      {

        if (!(@{$comment->{TEXT}}[$i-1] =~ /^[A-Z]*$/))
        {
          # Paragraphs always end with a full stop
          @{$comment->{TEXT}}[$i-1] = @{$comment->{TEXT}}[$i-1].".";
        }
      }
    }
    $i++;
  }
}

# Standardise our comment and output it if it is suitable.
sub process_comment($)
{
  my $comment = shift;

  # Don't process this comment if the function isn't exported
  my $spec_details = $spec_files{$comment->{DLL_NAME}}[0];

  if (!defined($spec_details))
  {
    if ($opt_verbose > 2)
    {
      print "Warning: Function '".$comment->{COMMENT_NAME}."' belongs to '".
            $comment->{DLL_NAME}."' (not passed with -w): not processing it.\n";
    }
    return;
  }

  if ($comment->{COMMENT_NAME} eq "@")
  {
    my $found = 0;

    # Find the name from the .spec file
    for (@{$spec_details->{EXPORTS}})
    {
      if (@$_[$EXPORT_ORDINAL] eq $comment->{ORDINAL})
      {
        $comment->{COMMENT_NAME} = @$_[$EXPORT_EXPNAME];
        $found = 1;
      }
    }

    if ($found == 0)
    {
      # Create an implementation name
      $comment->{COMMENT_NAME} = $comment->{DLL_NAME}."_".$comment->{ORDINAL};
    }
  }

  my $exported_names = $spec_details->{EXPORTED_NAMES};
  my $export_index = $exported_names->{$comment->{COMMENT_NAME}};
  my $implementation_names = $spec_details->{IMPLEMENTATION_NAMES};

  if (!defined($export_index))
  {
    # Perhaps the comment uses the implementation name?
    $export_index = $implementation_names->{$comment->{COMMENT_NAME}};
  }
  if (!defined($export_index))
  {
    # This function doesn't appear to be exported. hmm.
    if ($opt_verbose > 2)
    {
      print "Warning: Function '".$comment->{COMMENT_NAME}."' claims to belong to '".
            $comment->{DLL_NAME}."' but is not exported by it: not processing it.\n";
    }
    return;
  }

  # When the function is exported twice we have the second name below the first
  # (you see this a lot in ntdll, but also in some other places).
  my $first_line = ${$comment->{TEXT}}[1];

  if ( $first_line =~ /^(@|[A-Za-z0-9_]+) +(\(|\[)([A-Za-z0-9_]+)\.(([0-9]+)|@)(\)|\])$/ )
  {
    # Found a second name - mark it as documented
    my $alt_index = $exported_names->{$1};
    if (defined($alt_index))
    {
      if ($opt_verbose > 2)
      {
        print "Info: Found alternate name '",$1,"\n";
      }
      my $alt_export = @{$spec_details->{EXPORTS}}[$alt_index];
      @$alt_export[$EXPORT_FLAGS] |= $FLAG_DOCUMENTED;
      $spec_details->{NUM_DOCS}++;
      ${$comment->{TEXT}}[1] = "";
    }
  }

  if (@{$spec_details->{CURRENT_EXTRA}})
  {
    # We have an extra comment that might be related to this one
    my $current_comment = ${$spec_details->{CURRENT_EXTRA}}[0];
    my $current_name = $current_comment->{COMMENT_NAME};
    if ($comment->{COMMENT_NAME} =~ /^$current_name/ && $comment->{COMMENT_NAME} ne $current_name)
    {
      if ($opt_verbose > 2)
      {
        print "Linking ",$comment->{COMMENT_NAME}," to $current_name\n";
      }
      # Add a reference to this comment to our extra comment
      push (@{$current_comment->{TEXT}}, $comment->{COMMENT_NAME}."()","");
    }
  }

  # We want our docs generated using the implementation name, so they are unique
  my $export = @{$spec_details->{EXPORTS}}[$export_index];
  $comment->{COMMENT_NAME} = @$export[$EXPORT_IMPNAME];
  $comment->{ALT_NAME} = @$export[$EXPORT_EXPNAME];

  # Mark the function as documented
  $spec_details->{NUM_DOCS}++;
  @$export[$EXPORT_FLAGS] |= $FLAG_DOCUMENTED;

  # If we have parameter comments in the prototype, extract them
  my @parameter_comments;
  for (@{$comment->{PROTOTYPE}})
  {
    s/ *\, */\,/g; # Strip spaces from around commas

    if ( s/ *(\/\* *)(.*?)( *\*\/ *)// ) # Strip out comment
    {
      my $parameter_comment = $2;
      if (!$parameter_comment =~ /^\[/ )
      {
        # Add [IO] markers so we format the comment correctly
        $parameter_comment = "[fixme] ".$parameter_comment;
      }
      if ( /( |\*)([A-Za-z_]{1}[A-Za-z_0-9]*)(\,|\))/ )
      {
        # Add the parameter name
        $parameter_comment = $2." ".$parameter_comment;
      }
      push (@parameter_comments, $parameter_comment);
    }
  }

  # If we extracted any prototype comments, add them to the comment text.
  if (@parameter_comments)
  {
    @parameter_comments = ("PARAMS", @parameter_comments);
    my @new_comment = ();
    my $inserted_params = 0;

    for (@{$comment->{TEXT}})
    {
      if ( $inserted_params == 0 && /^[A-Z]+$/ )
      {
        # Found a section header, so this is where we insert
        push (@new_comment, @parameter_comments);
        $inserted_params = 1;
      }
      push (@new_comment, $_);
    }
    if ($inserted_params == 0)
    {
      # Add them to the end
      push (@new_comment, @parameter_comments);
    }
    $comment->{TEXT} = [@new_comment];
  }

  if ($opt_fussy == 1 && $opt_output_empty == 0)
  {
    # Reject any comment that doesn't have a description or a RETURNS section.
    # This is the default for now, 'coz many comments aren't suitable.
    my $found_returns = 0;
    my $found_description_text = 0;
    my $in_description = 0;
    for (@{$comment->{TEXT}})
    {
      if ( /^RETURNS$/ )
      {
        $found_returns = 1;
        $in_description = 0;
      }
      elsif ( /^DESCRIPTION$/ )
      {
        $in_description = 1;
      }
      elsif ($in_description == 1)
      {
        if ( !/^[A-Z]+$/ )
        {
          # Don't reject comments that refer to another doc (e.g. A/W)
          if ( /^See ([A-Za-z0-9_]+)\.$/ )
          {
            if ($comment->{COMMENT_NAME} =~ /W$/ )
            {
              # This is probably a Unicode version of an Ascii function.
              # Create the Ascii name and see if it has been documented
              my $ascii_name = $comment->{COMMENT_NAME};
              $ascii_name =~ s/W$/A/;

              my $ascii_export_index = $exported_names->{$ascii_name};

              if (!defined($ascii_export_index))
              {
                $ascii_export_index = $implementation_names->{$ascii_name};
              }
              if (!defined($ascii_export_index))
              {
                if ($opt_verbose > 2)
                {
                  print "Warning: Function '".$comment->{COMMENT_NAME}."' is not an A/W pair.\n";
                }
              }
              else
              {
                my $ascii_export = @{$spec_details->{EXPORTS}}[$ascii_export_index];
                if (@$ascii_export[$EXPORT_FLAGS] & $FLAG_DOCUMENTED)
                {
                  # Flag these functions as an A/W pair
                  @$ascii_export[$EXPORT_FLAGS] |= $FLAG_APAIR;
                  @$export[$EXPORT_FLAGS] |= $FLAG_WPAIR;
                }
              }
            }
            $found_returns = 1;
          }
          elsif ( /^Unicode version of ([A-Za-z0-9_]+)\.$/ )
          {
            @$export[$EXPORT_FLAGS] |= $FLAG_WPAIR; # Explicitly marked as W version
            $found_returns = 1;
          }
          elsif ( /^64\-bit version of ([A-Za-z0-9_]+)\.$/ )
          {
            @$export[$EXPORT_FLAGS] |= $FLAG_64PAIR; # Explicitly marked as 64 bit version
            $found_returns = 1;
          }
          $found_description_text = 1;
        }
        else
        {
          $in_description = 0;
        }
      }
    }
    if ($found_returns == 0 || $found_description_text == 0)
    {
      if ($opt_verbose > 2)
      {
        print "Info: Function '",$comment->{COMMENT_NAME},"' has no ",
              "description and/or RETURNS section, skipping\n";
      }
      $spec_details->{NUM_DOCS}--;
      @$export[$EXPORT_FLAGS] &= ~$FLAG_DOCUMENTED;
      return;
    }
  }

  process_comment_text($comment);

  # Strip the prototypes return value, call convention, name and brackets
  # (This leaves it as a list of types and names, or empty for void functions)
  my $prototype = join(" ", @{$comment->{PROTOTYPE}});
  $prototype =~ s/  / /g;

  if ( $prototype =~ /(WINAPIV|WINAPI|__cdecl|PASCAL|CALLBACK|FARPROC16)/ )
  {
    $prototype =~ s/^(.*?)\s+(WINAPIV|WINAPI|__cdecl|PASCAL|CALLBACK|FARPROC16)\s+(.*?)\(\s*(.*)/$4/;
    $comment->{RETURNS} = $1;
  }
  else
  {
    $prototype =~ s/^(.*?)([A-Za-z0-9_]+)\s*\(\s*(.*)/$3/;
    $comment->{RETURNS} = $1;
  }

  $prototype =~ s/ *\).*//;        # Strip end bracket
  $prototype =~ s/ *\* */\*/g;     # Strip space around pointers
  $prototype =~ s/ *\, */\,/g;     # Strip space around commas
  $prototype =~ s/^(void|VOID)$//; # If void, leave blank
  $prototype =~ s/\*([A-Za-z_])/\* $1/g; # Separate pointers from parameter name
  @{$comment->{PROTOTYPE}} = split ( /,/ ,$prototype);

  # FIXME: If we have no parameters, make sure we have a PARAMS: None. section

  # Find header file
  my $h_file = "";
  if (@$export[$EXPORT_FLAGS] & $FLAG_NONAME)
  {
    $h_file = "Exported by ordinal only. Use GetProcAddress() to obtain a pointer to the function.";
  }
  else
  {
    if ($comment->{COMMENT_NAME} ne "")
    {
      my $tmp = "grep -s -l $comment->{COMMENT_NAME} @opt_header_file_list 2>/dev/null";
      $tmp = `$tmp`;
      my $exit_value  = $? >> 8;
      if ($exit_value == 0)
      {
        $tmp =~ s/\n.*//g;
        if ($tmp ne "")
        {
          $h_file = "$tmp";
          $h_file =~ s|^.*/\./||;
        }
      }
    }
    elsif ($comment->{ALT_NAME} ne "")
    {
      my $tmp = "grep -s -l $comment->{ALT_NAME} @opt_header_file_list"." 2>/dev/null";
      $tmp = `$tmp`;
      my $exit_value  = $? >> 8;
      if ($exit_value == 0)
      {
        $tmp =~ s/\n.*//g;
        if ($tmp ne "")
        {
          $h_file = "$tmp";
          $h_file =~ s|^.*/\./||;
        }
      }
    }
    $h_file =~ s/^ *//;
    $h_file =~ s/\n//;
    if ($h_file eq "")
    {
      $h_file = "Not declared in a Wine header. The function is either undocumented, or missing from Wine."
    }
    else
    {
      $h_file = "Declared in \"".$h_file."\".";
    }
  }

  # Find source file
  my $c_file = $comment->{FILE};
  if ($opt_wine_root_dir ne "")
  {
    my $cfile = $pwd."/".$c_file;     # Current dir + file
    $cfile =~ s/(.+)(\/.*$)/$1/;      # Strip the filename
    $cfile = `cd $cfile && pwd`;      # Strip any relative parts (e.g. "../../")
    $cfile =~ s/\n//;                 # Strip newline
    my $newfile = $c_file;
    $newfile =~ s/(.+)(\/.*$)/$2/;    # Strip all but the filename
    $cfile = $cfile."/".$newfile;     # Append filename to base path
    $cfile =~ s/$opt_wine_root_dir//; # Get rid of the root directory
    $cfile =~ s/\/\//\//g;            # Remove any double slashes
    $cfile =~ s/^\/+//;               # Strip initial directory slash
    $c_file = $cfile;
  }
  $c_file = "Implemented in \"".$c_file."\".";

  # Add the implementation details
  push (@{$comment->{TEXT}}, "IMPLEMENTATION","",$h_file,"",$c_file);

  if (@$export[$EXPORT_FLAGS] & $FLAG_I386)
  {
    push (@{$comment->{TEXT}}, "", "Available on x86 platforms only.");
  }
  if (@$export[$EXPORT_FLAGS] & $FLAG_REGISTER)
  {
    push (@{$comment->{TEXT}}, "", "This function passes one or more arguments in registers. ",
          "For more details, please read the source code.");
  }
  my $source_details = $source_files{$comment->{FILE}}[0];
  if ($source_details->{DEBUG_CHANNEL} ne "")
  {
    push (@{$comment->{TEXT}}, "", "Debug channel \"".$source_details->{DEBUG_CHANNEL}."\".");
  }

  # Write out the documentation for the API
  output_comment($comment)
}

# process our extra comment and output it if it is suitable.
sub process_extra_comment($)
{
  my $comment = shift;

  my $spec_details = $spec_files{$comment->{DLL_NAME}}[0];

  if (!defined($spec_details))
  {
    if ($opt_verbose > 2)
    {
      print "Warning: Extra comment '".$comment->{COMMENT_NAME}."' belongs to '".
            $comment->{DLL_NAME}."' (not passed with -w): not processing it.\n";
    }
    return;
  }

  # Check first to see if this is documentation for the DLL.
  if ($comment->{COMMENT_NAME} eq $comment->{DLL_NAME})
  {
    if ($opt_verbose > 2)
    {
      print "Info: Found DLL documentation\n";
    }
    for (@{$comment->{TEXT}})
    {
      push (@{$spec_details->{DESCRIPTION}}, $_);
    }
    return;
  }

  # Add the comment to the DLL page as a link
  push (@{$spec_details->{EXTRA_COMMENTS}},$comment->{COMMENT_NAME});

  # If we have a prototype, process as a regular comment
  if (@{$comment->{PROTOTYPE}})
  {
    $comment->{ORDINAL} = "@";

    # Add an index for the comment name
    $spec_details->{EXPORTED_NAMES}{$comment->{COMMENT_NAME}} = $spec_details->{NUM_EXPORTS};

    # Add a fake exported entry
    $spec_details->{NUM_EXPORTS}++;
    my ($ordinal, $call_convention, $exported_name, $implementation_name, $documented) =
     ("@", "fake", $comment->{COMMENT_NAME}, $comment->{COMMENT_NAME}, 0);
    my @export = ($ordinal, $call_convention, $exported_name, $implementation_name, $documented);
    push (@{$spec_details->{EXPORTS}},[@export]);
    @{$comment->{TEXT}} = ("DESCRIPTION", @{$comment->{TEXT}});
    process_comment($comment);
    return;
  }

  if ($opt_verbose > 0)
  {
    print "Processing ",$comment->{COMMENT_NAME},"\n";
  }

  if (@{$spec_details->{CURRENT_EXTRA}})
  {
    my $current_comment = ${$spec_details->{CURRENT_EXTRA}}[0];

    if ($opt_verbose > 0)
    {
      print "Processing old current: ",$current_comment->{COMMENT_NAME},"\n";
    }
    # Output the current comment
    process_comment_text($current_comment);
    output_open_api_file($current_comment->{COMMENT_NAME});
    output_api_header($current_comment);
    output_api_name($current_comment);
    output_api_comment($current_comment);
    output_api_footer($current_comment);
    output_close_api_file();
  }

  if ($opt_verbose > 2)
  {
    print "Setting current to ",$comment->{COMMENT_NAME},"\n";
  }

  my $comment_copy =
  {
    FILE => $comment->{FILE},
    COMMENT_NAME => $comment->{COMMENT_NAME},
    ALT_NAME => $comment->{ALT_NAME},
    DLL_NAME => $comment->{DLL_NAME},
    ORDINAL => $comment->{ORDINAL},
    RETURNS => $comment->{RETURNS},
    PROTOTYPE => [],
    TEXT => [],
  };

  for (@{$comment->{TEXT}})
  {
    push (@{$comment_copy->{TEXT}}, $_);
  }
  # Set this comment to be the current extra comment
  @{$spec_details->{CURRENT_EXTRA}} = ($comment_copy);
}

# Write a standardised comment out in the appropriate format
sub output_comment($)
{
  my $comment = shift;

  if ($opt_verbose > 0)
  {
    print "Processing ",$comment->{COMMENT_NAME},"\n";
  }

  if ($opt_verbose > 4)
  {
    print "--PROTO--\n";
    for (@{$comment->{PROTOTYPE}})
    {
      print "'".$_."'\n";
    }

    print "--COMMENT--\n";
    for (@{$comment->{TEXT} })
    {
      print $_."\n";
    }
  }

  output_open_api_file($comment->{COMMENT_NAME});
  output_api_header($comment);
  output_api_name($comment);
  output_api_synopsis($comment);
  output_api_comment($comment);
  output_api_footer($comment);
  output_close_api_file();
}

# Write out an index file for each .spec processed
sub process_index_files()
{
  foreach my $spec_file (keys %spec_files)
  {
    my $spec_details = $spec_files{$spec_file}[0];
    if (defined ($spec_details->{DLL_NAME}))
    {
      if (@{$spec_details->{CURRENT_EXTRA}})
      {
        # We have an unwritten extra comment, write it
        my $current_comment = ${$spec_details->{CURRENT_EXTRA}}[0];
        process_extra_comment($current_comment);
        @{$spec_details->{CURRENT_EXTRA}} = ();
       }
       output_spec($spec_details);
    }
  }
}

# Write a spec files documentation out in the appropriate format
sub output_spec($)
{
  my $spec_details = shift;

  if ($opt_verbose > 2)
  {
    print "Writing:",$spec_details->{DLL_NAME},"\n";
  }

  # Use the comment output functions for consistency
  my $comment =
  {
    FILE => $spec_details->{DLL_NAME},
    COMMENT_NAME => $spec_details->{DLL_NAME}.".".$spec_details->{DLL_EXT},
    ALT_NAME => $spec_details->{DLL_NAME},
    DLL_NAME => "",
    ORDINAL => "",
    RETURNS => "",
    PROTOTYPE => [],
    TEXT => [],
  };
  my $total_implemented = $spec_details->{NUM_FORWARDS} + $spec_details->{NUM_VARS} +
     $spec_details->{NUM_FUNCS};
  my $percent_implemented = 0;
  if ($total_implemented)
  {
    $percent_implemented = $total_implemented /
     ($total_implemented + $spec_details->{NUM_STUBS}) * 100;
  }
  $percent_implemented = int($percent_implemented);
  my $percent_documented = 0;
  if ($spec_details->{NUM_DOCS})
  {
    # Treat forwards and data as documented funcs for statistics
    $percent_documented = $spec_details->{NUM_DOCS} / $spec_details->{NUM_FUNCS} * 100;
    $percent_documented = int($percent_documented);
  }

  # Make a list of the contributors to this DLL.
  my @contributors;

  foreach my $source_file (keys %source_files)
  {
    my $source_details = $source_files{$source_file}[0];
    for (@{$source_details->{CONTRIBUTORS}})
    {
      push (@contributors, $_);
    }
  }

  my %saw;
  @contributors = grep(!$saw{$_}++, @contributors); # remove dups, from perlfaq4 manpage
  @contributors = sort @contributors;

  # Remove duplicates and blanks
  for(my $i=0; $i<@contributors; $i++)
  {
    if ($i > 0 && ($contributors[$i] =~ /$contributors[$i-1]/ || $contributors[$i-1] eq ""))
    {
      $contributors[$i-1] = $contributors[$i];
    }
  }
  undef %saw;
  @contributors = grep(!$saw{$_}++, @contributors);

  if ($opt_verbose > 3)
  {
    print "Contributors:\n";
    for (@contributors)
    {
      print "'".$_."'\n";
    }
  }
  my $contribstring = join (", ", @contributors);

  # Create the initial comment text
  @{$comment->{TEXT}} = (
    "NAME",
    $comment->{COMMENT_NAME}
  );

  # Add the description, if we have one
  if (@{$spec_details->{DESCRIPTION}})
  {
    push (@{$comment->{TEXT}}, "DESCRIPTION");
    for (@{$spec_details->{DESCRIPTION}})
    {
      push (@{$comment->{TEXT}}, $_);
    }
  }

  # Add the statistics and contributors
  push (@{$comment->{TEXT}},
    "STATISTICS",
    "Forwards: ".$spec_details->{NUM_FORWARDS},
    "Variables: ".$spec_details->{NUM_VARS},
    "Stubs: ".$spec_details->{NUM_STUBS},
    "Functions: ".$spec_details->{NUM_FUNCS},
    "Exports-Total: ".$spec_details->{NUM_EXPORTS},
    "Implemented-Total: ".$total_implemented." (".$percent_implemented."%)",
    "Documented-Total: ".$spec_details->{NUM_DOCS}." (".$percent_documented."%)",
    "CONTRIBUTORS",
    "The following people hold copyrights on the source files comprising this dll:",
    "",
    $contribstring,
    "Note: This list may not be complete.",
    "For a complete listing, see the git commit logs and the File \"AUTHORS\" in the Wine source tree.",
    "",
  );

  if ($opt_output_format eq "h")
  {
    # Add the exports to the comment text
    push (@{$comment->{TEXT}},"EXPORTS");
    my $exports = $spec_details->{EXPORTS};
    for (@$exports)
    {
      my $line = "";

      # @$_ => ordinal, call convention, exported name, implementation name, flags;
      if (@$_[$EXPORT_CALL] eq "forward")
      {
        my $forward_dll = @$_[$EXPORT_IMPNAME];
        $forward_dll =~ s/\.(.*)//;
        $line = @$_[$EXPORT_EXPNAME]." (forward to ".$1."() in ".$forward_dll."())";
      }
      elsif (@$_[$EXPORT_CALL] eq "extern")
      {
        $line = @$_[$EXPORT_EXPNAME]." (extern)";
      }
      elsif (@$_[$EXPORT_CALL] eq "stub")
      {
        $line = @$_[$EXPORT_EXPNAME]." (stub)";
      }
      elsif (@$_[$EXPORT_CALL] eq "fake")
      {
        # Don't add this function here, it gets listed with the extra documentation
        if (!(@$_[$EXPORT_FLAGS] & $FLAG_WPAIR))
        {
          # This function should be indexed
          push (@index_entries_list, @$_[$EXPORT_IMPNAME].",".@$_[$EXPORT_IMPNAME]);
        }
      }
      elsif (@$_[$EXPORT_CALL] eq "equate" || @$_[$EXPORT_CALL] eq "variable")
      {
        $line = @$_[$EXPORT_EXPNAME]." (data)";
      }
      else
      {
        # A function
        if (@$_[$EXPORT_FLAGS] & $FLAG_DOCUMENTED)
        {
          # Documented
          $line = @$_[$EXPORT_EXPNAME]." (implemented as ".@$_[$EXPORT_IMPNAME]."())";
          if (@$_[$EXPORT_EXPNAME] ne @$_[$EXPORT_IMPNAME])
          {
            $line = @$_[$EXPORT_EXPNAME]." (implemented as ".@$_[$EXPORT_IMPNAME]."())";
          }
          else
          {
            $line = @$_[$EXPORT_EXPNAME]."()";
          }
          if (!(@$_[$EXPORT_FLAGS] & $FLAG_WPAIR))
          {
            # This function should be indexed
            push (@index_entries_list, @$_[$EXPORT_EXPNAME].",".@$_[$EXPORT_IMPNAME]);
          }
        }
        else
        {
          $line = @$_[$EXPORT_EXPNAME]." (not documented)";
        }
      }
      if ($line ne "")
      {
        push (@{$comment->{TEXT}}, $line, "");
      }
    }

    # Add links to the extra documentation
    if (@{$spec_details->{EXTRA_COMMENTS}})
    {
      push (@{$comment->{TEXT}}, "SEE ALSO");
      my %htmp;
      @{$spec_details->{EXTRA_COMMENTS}} = grep(!$htmp{$_}++, @{$spec_details->{EXTRA_COMMENTS}});
      for (@{$spec_details->{EXTRA_COMMENTS}})
      {
        push (@{$comment->{TEXT}}, $_."()", "");
      }
    }
  }
  # The dll entry should also be indexed
  push (@index_entries_list, $spec_details->{DLL_NAME}.",".$spec_details->{DLL_NAME});

  # Write out the document
  output_open_api_file($spec_details->{DLL_NAME});
  output_api_header($comment);
  output_api_comment($comment);
  output_api_footer($comment);
  output_close_api_file();

  # Add this dll to the database of dll names
  my $output_file = $opt_output_directory."/dlls.db";

  # Append the dllname to the output db of names
  open(DLLDB,">>$output_file") || die "Couldn't create $output_file\n";
  print DLLDB $spec_details->{DLL_NAME},"\n";
  close(DLLDB);

  if ($opt_output_format eq "s")
  {
    output_sgml_dll_file($spec_details);
    return;
  }

  if ($opt_output_format eq "x")
  {
    output_xml_dll_file($spec_details);
    return;
  }

}

#
# OUTPUT FUNCTIONS
# ----------------
# Only these functions know anything about formatting for a specific
# output type. The functions above work only with plain text.
# This is to allow new types of output to be added easily.

# Open the api file
sub output_open_api_file($)
{
  my $output_name = shift;
  $output_name = $opt_output_directory."/".$output_name;

  if ($opt_output_format eq "h")
  {
    $output_name = $output_name.".html";
  }
  elsif ($opt_output_format eq "s")
  {
    $output_name = $output_name.".sgml";
  }
  elsif ($opt_output_format eq "x")
  {
    $output_name = $output_name.".xml";
  }
  else
  {
    $output_name = $output_name.".".$opt_manual_section;
  }
  open(OUTPUT,">$output_name") || die "Couldn't create file '$output_name'\n";
}

# Close the api file
sub output_close_api_file()
{
  close (OUTPUT);
}

# Output the api file header
sub output_api_header($)
{
  my $comment = shift;

  if ($opt_output_format eq "h")
  {
      print OUTPUT "<!-- Generated file - DO NOT EDIT! -->\n";
      print OUTPUT "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n";
      print OUTPUT "<HTML>\n<HEAD>\n";
      print OUTPUT "<LINK REL=\"StyleSheet\" href=\"apidoc.css\" type=\"text/css\">\n";
      print OUTPUT "<META NAME=\"GENERATOR\" CONTENT=\"tools/c2man.pl\">\n";
      print OUTPUT "<META NAME=\"keywords\" CONTENT=\"Win32,Wine,API,$comment->{COMMENT_NAME}\">\n";
      print OUTPUT "<TITLE>Wine API: $comment->{COMMENT_NAME}</TITLE>\n</HEAD>\n<BODY>\n";
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
      print OUTPUT "<!-- Generated file - DO NOT EDIT! -->\n",
                   "<sect1>\n",
                   "<title>$comment->{COMMENT_NAME}</title>\n";
  }
  else
  {
      print OUTPUT ".\\\" -*- nroff -*-\n.\\\" Generated file - DO NOT EDIT!\n".
                   ".TH ",$comment->{COMMENT_NAME}," ",$opt_manual_section," \"",$date,"\" \"".
                   "Wine API\" \"Wine API\"\n";
  }
}

sub output_api_footer($)
{
  if ($opt_output_format eq "h")
  {
      print OUTPUT "<hr><p><i class=\"copy\">Copyright &copy ".$year." The Wine Project.".
                   " All trademarks are the property of their respective owners.".
                   " Visit <a href=\"http://www.winehq.org\">WineHQ</a> for license details.".
                   " Generated $date.</i></p>\n</body>\n</html>\n";
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
      print OUTPUT "</sect1>\n";
      return;
  }
  else
  {
  }
}

sub output_api_section_start($$)
{
  my $comment = shift;
  my $section_name = shift;

  if ($opt_output_format eq "h")
  {
    print OUTPUT "\n<h2 class=\"section\">",$section_name,"</h2>\n";
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    print OUTPUT "<bridgehead>",$section_name,"</bridgehead>\n";
  }
  else
  {
    print OUTPUT "\n\.SH ",$section_name,"\n";
  }
}

sub output_api_section_end()
{
  # Not currently required by any output formats
}

sub output_api_name($)
{
  my $comment = shift;
  my $readable_name = $comment->{COMMENT_NAME};
  $readable_name =~ s/-/ /g; # make section names more readable

  output_api_section_start($comment,"NAME");


  my $dll_ordinal = "";
  if ($comment->{ORDINAL} ne "")
  {
    $dll_ordinal = "(".$comment->{DLL_NAME}.".".$comment->{ORDINAL}.")";
  }
  if ($opt_output_format eq "h")
  {
    print OUTPUT "<p><b class=\"func_name\">",$readable_name,
                 "</b>&nbsp;&nbsp;<i class=\"dll_ord\">",
                 ,$dll_ordinal,"</i></p>\n";
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    print OUTPUT "<para>\n  <command>",$readable_name,"</command>  <emphasis>",
                 $dll_ordinal,"</emphasis>\n</para>\n";
  }
  else
  {
    print OUTPUT "\\fB",$readable_name,"\\fR ",$dll_ordinal;
  }

  output_api_section_end();
}

sub output_api_synopsis($)
{
  my $comment = shift;
  my @fmt;

  output_api_section_start($comment,"SYNOPSIS");

  if ($opt_output_format eq "h")
  {
    print OUTPUT "<pre class=\"proto\">\n ", $comment->{RETURNS}," ",$comment->{COMMENT_NAME},"\n (\n";
    @fmt = ("", "\n", "<tt class=\"param\">", "</tt>");
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    print OUTPUT "<screen>\n ",$comment->{RETURNS}," ",$comment->{COMMENT_NAME},"\n (\n";
    @fmt = ("", "\n", "<emphasis>", "</emphasis>");
  }
  else
  {
    print OUTPUT $comment->{RETURNS}," ",$comment->{COMMENT_NAME},"\n (\n";
    @fmt = ("", "\n", "\\fI", "\\fR");
  }

  # Since our prototype is output in a pre-formatted block, line up the
  # parameters and parameter comments in the same column.

  # First calculate where the columns should start
  my $biggest_length = 0;
  for(my $i=0; $i < @{$comment->{PROTOTYPE}}; $i++)
  {
    my $line = ${$comment->{PROTOTYPE}}[$i];
    if ($line =~ /(.+?)([A-Za-z_][A-Za-z_0-9]*)$/)
    {
      my $length = length $1;
      if ($length > $biggest_length)
      {
        $biggest_length = $length;
      }
    }
  }

  # Now pad the string with blanks
  for(my $i=0; $i < @{$comment->{PROTOTYPE}}; $i++)
  {
    my $line = ${$comment->{PROTOTYPE}}[$i];
    if ($line =~ /(.+?)([A-Za-z_][A-Za-z_0-9]*)$/)
    {
      my $pad_len = $biggest_length - length $1;
      my $padding = " " x ($pad_len);
      ${$comment->{PROTOTYPE}}[$i] = $1.$padding.$2;
    }
  }

  for(my $i=0; $i < @{$comment->{PROTOTYPE}}; $i++)
  {
    # Format the parameter name
    my $line = ${$comment->{PROTOTYPE}}[$i];
    my $comma = ($i == @{$comment->{PROTOTYPE}}-1) ? "" : ",";
    $line =~ s/(.+?)([A-Za-z_][A-Za-z_0-9]*)$/  $fmt[0]$1$fmt[2]$2$fmt[3]$comma$fmt[1]/;
    print OUTPUT $line;
  }

  if ($opt_output_format eq "h")
  {
    print OUTPUT " )\n</pre>\n";
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    print OUTPUT " )\n</screen>\n";
  }
  else
  {
    print OUTPUT " )\n";
  }

  output_api_section_end();
}

sub output_api_comment($)
{
  my $comment = shift;
  my $open_paragraph = 0;
  my $open_raw = 0;
  my $param_docs = 0;
  my @fmt;

  if ($opt_output_format eq "h")
  {
    @fmt = ("<p>", "</p>\n", "<tt class=\"const\">", "</tt>", "<b class=\"emp\">", "</b>",
            "<tt class=\"coderef\">", "</tt>", "<tt class=\"param\">", "</tt>",
            "<i class=\"in_out\">", "</i>", "<pre class=\"raw\">\n", "</pre>\n",
            "<table class=\"tab\"><colgroup><col><col><col></colgroup><tbody>\n",
            "</tbody></table>\n","<tr><td>","</td></tr>\n","</td>","</td><td>");
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    @fmt = ("<para>\n","\n</para>\n","<constant>","</constant>","<emphasis>","</emphasis>",
            "<command>","</command>","<constant>","</constant>","<emphasis>","</emphasis>",
            "<screen>\n","</screen>\n",
            "<informaltable frame=\"none\">\n<tgroup cols=\"3\">\n<tbody>\n",
            "</tbody>\n</tgroup>\n</informaltable>\n","<row><entry>","</entry></row>\n",
            "</entry>","</entry><entry>");
  }
  else
  {
    @fmt = ("\.PP\n", "\n", "\\fB", "\\fR", "\\fB", "\\fR", "\\fB", "\\fR", "\\fI", "\\fR",
            "\\fB", "\\fR ", "", "", "", "","","\n.PP\n","","");
  }

  # Extract the parameter names
  my @parameter_names;
  for (@{$comment->{PROTOTYPE}})
  {
    if ( /(.+?)([A-Za-z_][A-Za-z_0-9]*)$/ )
    {
      push (@parameter_names, $2);
    }
  }

  for (@{$comment->{TEXT}})
  {
    if ($opt_output_format eq "h" || $opt_output_format eq "s" || $opt_output_format eq "x")
    {
      # Map special characters
      s/\&/\&amp;/g;
      s/\</\&lt;/g;
      s/\>/\&gt;/g;
      s/\([Cc]\)/\&copy;/g;
      s/\(tm\)/&#174;/;
    }

    if ( s/^\|// )
    {
      # Raw output
      if ($open_raw == 0)
      {
        if ($open_paragraph == 1)
        {
          # Close the open paragraph
          print OUTPUT $fmt[1];
          $open_paragraph = 0;
        }
        # Start raw output
        print OUTPUT $fmt[12];
        $open_raw = 1;
      }
      if ($opt_output_format eq "")
      {
        print OUTPUT ".br\n"; # Prevent 'man' running these lines together
      }
      print OUTPUT $_,"\n";
    }
    else
    {
      if ($opt_output_format eq "h")
      {
        # Link to the file in WineHQ cvs
        s/^(Implemented in \")(.+?)(\"\.)/$1$2$3 http:\/\/source.winehq.org\/source\/$2/g;
        s/^(Declared in \")(.+?)(\"\.)/$1$2$3 http:\/\/source.winehq.org\/source\/include\/$2/g;
      }
      # Highlight strings
      s/(\".+?\")/$fmt[2]$1$fmt[3]/g;
      # Highlight literal chars
      s/(\'.\')/$fmt[2]$1$fmt[3]/g;
      s/(\'.{2}\')/$fmt[2]$1$fmt[3]/g;
      # Highlight numeric constants
      s/( |\-|\+|\.|\()([0-9\-\.]+)( |\-|$|\.|\,|\*|\?|\))/$1$fmt[2]$2$fmt[3]$3/g;

      # Leading cases ("xxxx:","-") start new paragraphs & are emphasised
      # FIXME: Using bullet points for leading '-' would look nicer.
      if ($open_paragraph == 1 && $param_docs == 0)
      {
        s/^(\-)/$fmt[1]$fmt[0]$fmt[4]$1$fmt[5]/;
        s/^([[A-Za-z\-]+\:)/$fmt[1]$fmt[0]$fmt[4]$1$fmt[5]/;
      }
      else
      {
        s/^(\-)/$fmt[4]$1$fmt[5]/;
        s/^([[A-Za-z\-]+\:)/$fmt[4]$1$fmt[5]/;
      }

      if ($opt_output_format eq "h")
      {
        # Html uses links for API calls
        while ( /([A-Za-z_]+[A-Za-z_0-9-]+)(\(\))/)
        {
          my $link = $1;
          my $readable_link = $1;
          $readable_link =~ s/-/ /g;

          s/([A-Za-z_]+[A-Za-z_0-9-]+)(\(\))/<a href\=\"$link\.html\">$readable_link<\/a>/;
        }
        # Index references
        s/\{\{(.*?)\}\}\{\{(.*?)\}\}/<a href\=\"$2\.html\">$1<\/a>/g;
        s/ ([A-Z_])(\(\))/<a href\=\"$1\.html\">$1<\/a>/g;
        # And references to COM objects (hey, they'll get documented one day)
        s/ (I[A-Z]{1}[A-Za-z0-9_]+) (Object|object|Interface|interface)/ <a href\=\"$1\.html\">$1<\/a> $2/g;
        # Convert any web addresses to real links
        s/(http\:\/\/)(.+?)($| )/<a href\=\"$1$2\">$2<\/a>$3/g;
      }
      else
      {
        if ($opt_output_format eq "")
        {
          # Give the man section for API calls
          s/ ([A-Za-z_]+[A-Za-z_0-9-]+)\(\)/ $fmt[6]$1\($opt_manual_section\)$fmt[7]/g;
        }
        else
        {
          # Highlight API calls
          s/ ([A-Za-z_]+[A-Za-z_0-9-]+\(\))/ $fmt[6]$1$fmt[7]/g;
        }

        # And references to COM objects
        s/ (I[A-Z]{1}[A-Za-z0-9_]+) (Object|object|Interface|interface)/ $fmt[6]$1$fmt[7] $2/g;
      }

      if ($open_raw == 1)
      {
        # Finish the raw output
        print OUTPUT $fmt[13];
        $open_raw = 0;
      }

      if ( /^[A-Z]+$/ || /^SEE ALSO$/ )
      {
        # Start of a new section
        if ($open_paragraph == 1)
        {
          if ($param_docs == 1)
          {
            print OUTPUT $fmt[17],$fmt[15];
            $param_docs = 0;
          }
          else
          {
            print OUTPUT $fmt[1];
          }
          $open_paragraph = 0;
        }
        output_api_section_start($comment,$_);
        if ( /^PARAMS$/ || /^MEMBERS$/ )
        {
          print OUTPUT $fmt[14];
          $param_docs = 1;
        }
        else
        {
          #print OUTPUT $fmt[15];
          #$param_docs = 0;
        }
      }
      elsif ( /^$/ )
      {
        # Empty line, indicating a new paragraph
        if ($open_paragraph == 1)
        {
          if ($param_docs == 0)
          {
            print OUTPUT $fmt[1];
            $open_paragraph = 0;
          }
        }
      }
      else
      {
        if ($param_docs == 1)
        {
          if ($open_paragraph == 1)
          {
            # For parameter docs, put each parameter into a new paragraph/table row
            print OUTPUT $fmt[17];
            $open_paragraph = 0;
          }
          s/(\[.+\])( *)/$fmt[19]$fmt[10]$1$fmt[11]$fmt[19] /; # Format In/Out
        }
        else
        {
          # Within paragraph lines, prevent lines running together
          $_ = $_." ";
        }

        # Format parameter names where they appear in the comment
        for my $parameter_name (@parameter_names)
        {
          s/(^|[ \.\,\(\-\*])($parameter_name)($|[ \.\)\,\-\/]|(\=[^"]))/$1$fmt[8]$2$fmt[9]$3/g;
        }
        # Structure dereferences include the dereferenced member
        s/(\-\>[A-Za-z_]+)/$fmt[8]$1$fmt[9]/g;
        s/(\-\&gt\;[A-Za-z_]+)/$fmt[8]$1$fmt[9]/g;

        if ($open_paragraph == 0)
        {
          if ($param_docs == 1)
          {
            print OUTPUT $fmt[16];
          }
          else
          {
            print OUTPUT $fmt[0];
          }
          $open_paragraph = 1;
        }
        # Anything in all uppercase on its own gets emphasised
        s/(^|[ \.\,\(\[\|\=])([A-Z]+?[A-Z0-9_]+)($|[ \.\,\*\?\|\)\=\'])/$1$fmt[6]$2$fmt[7]$3/g;

        print OUTPUT $_;
      }
    }
  }
  if ($open_raw == 1)
  {
    print OUTPUT $fmt[13];
  }
  if ($param_docs == 1 && $open_paragraph == 1)
  {
    print OUTPUT $fmt[17];
    $open_paragraph = 0;
  }
  if ($param_docs == 1)
  {
    print OUTPUT $fmt[15];
  }
  if ($open_paragraph == 1)
  {
    print OUTPUT $fmt[1];
  }
}

# Create the master index file
sub output_master_index_files()
{
  if ($opt_output_format eq "")
  {
    return; # No master index for man pages
  }

  if ($opt_output_format eq "h")
  {
    # Append the index entries to the output db of index entries
    my $output_file = $opt_output_directory."/index.db";
    open(INDEXDB,">>$output_file") || die "Couldn't create $output_file\n";
    for (@index_entries_list)
    {
      $_ =~ s/A\,/\,/;
      print INDEXDB $_."\n";
    }
    close(INDEXDB);
  }

  # Use the comment output functions for consistency
  my $comment =
  {
    FILE => "",
    COMMENT_NAME => "The Wine API Guide",
    ALT_NAME => "The Wine API Guide",
    DLL_NAME => "",
    ORDINAL => "",
    RETURNS => "",
    PROTOTYPE => [],
    TEXT => [],
  };

  if ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    $comment->{COMMENT_NAME} = "Introduction";
    $comment->{ALT_NAME} = "Introduction",
  }
  elsif ($opt_output_format eq "h")
  {
    @{$comment->{TEXT}} = (
      "NAME",
       $comment->{COMMENT_NAME},
       "INTRODUCTION",
    );
  }

  # Create the initial comment text
  push (@{$comment->{TEXT}},
    "This document describes the API calls made available",
    "by Wine. They are grouped by the dll that exports them.",
    "",
    "Please do not edit this document, since it is generated automatically",
    "from the Wine source code tree. Details on updating this documentation",
    "are given in the \"Wine Developers Guide\".",
    "CONTRIBUTORS",
    "API documentation is generally written by the person who ",
    "implements a given API call. Authors of each dll are listed in the overview ",
    "section for that dll. Additional contributors who have updated source files ",
    "but have not entered their names in a copyright statement are noted by an ",
    "entry in the git commit logs.",
    ""
  );

  # Read in all dlls from the database of dll names
  my $input_file = $opt_output_directory."/dlls.db";
  my @dlls = `cat $input_file|sort|uniq`;

  if ($opt_output_format eq "h")
  {
    # HTML gets a list of all the dlls and an index. For docbook the index creates this for us
    push (@{$comment->{TEXT}},
      "INDEX",
      "For an alphabetical listing of the functions available, please click the ",
      "first letter of the functions name below:","",
      "[ _(), A(), B(), C(), D(), E(), F(), G(), H(), ".
      "I(), J(), K(), L(), M(), N(), O(), P(), Q(), ".
      "R(), S(), T(), U(), V(), W(), X(), Y(), Z() ]", "",
      "DLLS",
      "Each dll provided by Wine is documented individually. The following dlls are provided :",
      ""
    );
    # Add the dlls to the comment
    for (@dlls)
    {
      $_ =~ s/(\..*)?\n/\(\)/;
      push (@{$comment->{TEXT}}, $_, "");
    }
    output_open_api_file("index");
  }
  elsif ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    # Just write this as the initial blurb, with a chapter heading
    output_open_api_file("blurb");
    print OUTPUT "<chapter id =\"blurb\">\n<title>Introduction to The Wine API Guide</title>\n"
  }

  # Write out the document
  output_api_header($comment);
  output_api_comment($comment);
  output_api_footer($comment);
  if ($opt_output_format eq "s" || $opt_output_format eq "x")
  {
    print OUTPUT "</chapter>\n" # finish the chapter
  }
  output_close_api_file();

  if ($opt_output_format eq "s")
  {
    output_sgml_master_file(\@dlls);
    return;
  }
  if ($opt_output_format eq "x")
  {
    output_xml_master_file(\@dlls);
    return;
  }
  if ($opt_output_format eq "h")
  {
    output_html_index_files();
    output_html_stylesheet();
    return;
  }
}

# Write the master wine-api.xml, linking it to each dll.
sub output_xml_master_file($)
{
  my $dlls = shift;

  output_open_api_file("wine-api");
  print OUTPUT "<?xml version='1.0'?>";
  print OUTPUT "<!-- Generated file - DO NOT EDIT! -->\n";
  print OUTPUT "<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook V5.0/EN\" ";
  print OUTPUT "               \"http://www.docbook.org/xml/5.0/dtd/docbook.dtd\" [\n\n";
  print OUTPUT "<!ENTITY blurb SYSTEM \"blurb.xml\">\n";

  # List the entities
  for (@$dlls)
  {
    $_ =~ s/(\..*)?\n//;
    print OUTPUT "<!ENTITY ",$_," SYSTEM \"",$_,".xml\">\n"
  }

  print OUTPUT "]>\n\n<book id=\"index\">\n<bookinfo><title>The Wine API Guide</title></bookinfo>\n\n";
  print OUTPUT "  &blurb;\n";

  for (@$dlls)
  {
    print OUTPUT "  &",$_,";\n"
  }
  print OUTPUT "\n\n</book>\n";

  output_close_api_file();
}

# Write the master wine-api.sgml, linking it to each dll.
sub output_sgml_master_file($)
{
  my $dlls = shift;

  output_open_api_file("wine-api");
  print OUTPUT "<!-- Generated file - DO NOT EDIT! -->\n";
  print OUTPUT "<!doctype book PUBLIC \"-//OASIS//DTD DocBook V3.1//EN\" [\n\n";
  print OUTPUT "<!entity blurb SYSTEM \"blurb.sgml\">\n";

  # List the entities
  for (@$dlls)
  {
    $_ =~ s/(\..*)?\n//;
    print OUTPUT "<!entity ",$_," SYSTEM \"",$_,".sgml\">\n"
  }

  print OUTPUT "]>\n\n<book id=\"index\">\n<bookinfo><title>The Wine API Guide</title></bookinfo>\n\n";
  print OUTPUT "  &blurb;\n";

  for (@$dlls)
  {
    print OUTPUT "  &",$_,";\n"
  }
  print OUTPUT "\n\n</book>\n";

  output_close_api_file();
}

# Produce the sgml for the dll chapter from the generated files
sub output_sgml_dll_file($)
{
  my $spec_details = shift;

  # Make a list of all the documentation files to include
  my $exports = $spec_details->{EXPORTS};
  my @source_files = ();
  for (@$exports)
  {
    # @$_ => ordinal, call convention, exported name, implementation name, documented;
    if (@$_[$EXPORT_CALL] ne "forward" && @$_[$EXPORT_CALL] ne "extern" &&
        @$_[$EXPORT_CALL] ne "stub" && @$_[$EXPORT_CALL] ne "equate" &&
        @$_[$EXPORT_CALL] ne "variable" && @$_[$EXPORT_CALL] ne "fake" &&
        @$_[$EXPORT_FLAGS] & $FLAG_DOCUMENTED)
    {
      # A documented function
      push (@source_files,@$_[$EXPORT_IMPNAME]);
    }
  }

  push (@source_files,@{$spec_details->{EXTRA_COMMENTS}});

  @source_files = sort @source_files;

  # create a new chapter for this dll
  my $tmp_name = $opt_output_directory."/".$spec_details->{DLL_NAME}.".tmp";
  open(OUTPUT,">$tmp_name") || die "Couldn't create $tmp_name\n";
  print OUTPUT "<chapter>\n<title>$spec_details->{DLL_NAME}</title>\n";
  output_close_api_file();

  # Add the sorted documentation, cleaning up as we go
  `cat $opt_output_directory/$spec_details->{DLL_NAME}.sgml >>$tmp_name`;
  for (@source_files)
  {
    `cat $opt_output_directory/$_.sgml >>$tmp_name`;
    `rm -f $opt_output_directory/$_.sgml`;
  }

  # close the chapter, and overwite the dll source
  open(OUTPUT,">>$tmp_name") || die "Couldn't create $tmp_name\n";
  print OUTPUT "</chapter>\n";
  close OUTPUT;
  `mv $tmp_name $opt_output_directory/$spec_details->{DLL_NAME}.sgml`;
}

# Produce the xml for the dll chapter from the generated files
sub output_xml_dll_file($)
{
  my $spec_details = shift;

  # Make a list of all the documentation files to include
  my $exports = $spec_details->{EXPORTS};
  my @source_files = ();
  for (@$exports)
  {
    # @$_ => ordinal, call convention, exported name, implementation name, documented;
    if (@$_[$EXPORT_CALL] ne "forward" && @$_[$EXPORT_CALL] ne "extern" &&
        @$_[$EXPORT_CALL] ne "stub" && @$_[$EXPORT_CALL] ne "equate" &&
        @$_[$EXPORT_CALL] ne "variable" && @$_[$EXPORT_CALL] ne "fake" &&
        @$_[$EXPORT_FLAGS] & $FLAG_DOCUMENTED)
    {
      # A documented function
      push (@source_files,@$_[$EXPORT_IMPNAME]);
    }
  }

  push (@source_files,@{$spec_details->{EXTRA_COMMENTS}});

  @source_files = sort @source_files;

  # create a new chapter for this dll
  my $tmp_name = $opt_output_directory."/".$spec_details->{DLL_NAME}.".tmp";
  open(OUTPUT,">$tmp_name") || die "Couldn't create $tmp_name\n";
  print OUTPUT "<?xml version='1.0' encoding='UTF-8'?>\n<chapter>\n<title>$spec_details->{DLL_NAME}</title>\n";
  output_close_api_file();

  # Add the sorted documentation, cleaning up as we go
  `cat $opt_output_directory/$spec_details->{DLL_NAME}.xml >>$tmp_name`;
  for (@source_files)
  {
    `cat $opt_output_directory/$_.xml >>$tmp_name`;
    `rm -f $opt_output_directory/$_.xml`;
  }

  # close the chapter, and overwite the dll source
  open(OUTPUT,">>$tmp_name") || die "Couldn't create $tmp_name\n";
  print OUTPUT "</chapter>\n";
  close OUTPUT;
  `mv $tmp_name $opt_output_directory/$spec_details->{DLL_NAME}.xml`;
}

# Write the html index files containing the function names
sub output_html_index_files()
{
  if ($opt_output_format ne "h")
  {
    return;
  }

  my @letters = ('_', 'A' .. 'Z');

  # Read in all functions
  my $input_file = $opt_output_directory."/index.db";
  my @funcs = `cat $input_file|sort|uniq`;

  for (@letters)
  {
    my $letter = $_;
    my $comment =
    {
      FILE => "",
      COMMENT_NAME => "",
      ALT_NAME => "",
      DLL_NAME => "",
      ORDINAL => "",
      RETURNS => "",
      PROTOTYPE => [],
      TEXT => [],
    };

    $comment->{COMMENT_NAME} = $letter." Functions";
    $comment->{ALT_NAME} = $letter." Functions";

    push (@{$comment->{TEXT}},
      "NAME",
      $comment->{COMMENT_NAME},
      "FUNCTIONS"
    );

    # Add the functions to the comment
    for (@funcs)
    {
      my $first_char = substr ($_, 0, 1);
      $first_char = uc $first_char;

      if ($first_char eq $letter)
      {
        my $name = $_;
        my $file;
        $name =~ s/(^.*?)\,(.*?)\n/$1/;
        $file = $2;
        push (@{$comment->{TEXT}}, "{{".$name."}}{{".$file."}}","");
      }
    }

    # Write out the document
    output_open_api_file($letter);
    output_api_header($comment);
    output_api_comment($comment);
    output_api_footer($comment);
    output_close_api_file();
  }
}

# Output the stylesheet for HTML output
sub output_html_stylesheet()
{
  if ($opt_output_format ne "h")
  {
    return;
  }

  my $css;
  ($css = <<HERE_TARGET) =~ s/^\s+//gm;
/*
 * Default styles for Wine HTML Documentation.
 *
 * This style sheet should be altered to suit your needs/taste.
 */
BODY { /* Page body */
background-color: white;
color: black;
font-family: Tahoma,sans-serif;
font-style: normal;
font-size: 10pt;
}
a:link { color: #4444ff; } /* Links */
a:visited { color: #333377 }
a:active { color: #0000dd }
H2.section { /* Section Headers */
font-family: sans-serif;
color: #777777;
background-color: #F0F0FE;
margin-left: 0.2in;
margin-right: 1.0in;
}
b.func_name { /* Function Name */
font-size: 10pt;
font-style: bold;
}
i.dll_ord { /* Italicised DLL+ordinal */
color: #888888;
font-family: sans-serif;
font-size: 8pt;
}
p { /* Paragraphs */
margin-left: 0.5in;
margin-right: 0.5in;
}
table { /* tables */
margin-left: 0.5in;
margin-right: 0.5in;
}
pre.proto /* API Function prototype */
{
border-style: solid;
border-width: 1px;
border-color: #777777;
background-color: #F0F0BB;
color: black;
font-size: 10pt;
vertical-align: top;
margin-left: 0.5in;
margin-right: 1.0in;
}
pre.raw { /* Raw text output */
margin-left: 0.6in;
margin-right: 1.1in;
background-color: #8080DC;
}
tt.param { /* Parameter name */
font-style: italic;
color: blue;
}
tt.const { /* Constant */
color: red;
}
i.in_out { /* In/Out */
font-size: 8pt;
color: grey;
}
tt.coderef { /* Code in description text */
color: darkgreen;
}
b.emp /* Emphasis */ {
font-style: bold;
color: darkblue;
}
i.footer { /* Footer */
font-family: sans-serif;
font-size: 6pt;
color: darkgrey;
}
HERE_TARGET

  my $output_file = "$opt_output_directory/apidoc.css";
  open(CSS,">$output_file") || die "Couldn't create the file $output_file\n";
  print CSS $css;
  close(CSS);
}


sub usage()
{
  print "\nCreate API Documentation from Wine source code.\n\n",
        "Usage: c2man.pl [options] {-w <spec>} {-I <include>} {<source>}\n",
        "Where: <spec> is a .spec file giving a DLL's exports.\n",
        "       <include> is an include directory used by the DLL.\n",
        "       <source> is a source file of the DLL.\n",
        "       The above can be given multiple times on the command line, as appropriate.\n",
        "Options:\n",
        " -Th      : Output HTML instead of a man page\n",
        " -Ts      : Output SGML (DocBook source) instead of a man page\n",
        " -C <dir> : Source directory, to find source files if they are not found in the\n",
        "            current directory. Default is \"",$opt_source_dir,"\"\n",
        " -P <dir> : Parent source directory.\n",
        " -R <dir> : Root of build directory.\n",
        " -o <dir> : Create output in <dir>, default is \"",$opt_output_directory,"\"\n",
        " -s <sect>: Set manual section to <sect>, default is ",$opt_manual_section,"\n",
        " -e       : Output \"FIXME\" documentation from empty comments.\n",
        " -v       : Verbosity. Can be given more than once for more detail.\n";
}


#
# Main
#

# Print usage if we're called with no args
if( @ARGV == 0)
{
  usage();
}

# Process command line options
while(defined($_ = shift @ARGV))
{
  if( s/^-// )
  {
    # An option.
    for ($_)
    {
      /^o$/  && do { $opt_output_directory = shift @ARGV; last; };
      s/^S// && do { $opt_manual_section   = $_;          last; };
      /^Th$/ && do { $opt_output_format  = "h";           last; };
      /^Ts$/ && do { $opt_output_format  = "s";           last; };
      /^Tx$/ && do { $opt_output_format  = "x";           last; };
      /^v$/  && do { $opt_verbose++;                      last; };
      /^e$/  && do { $opt_output_empty = 1;               last; };
      /^L$/  && do { last; };
      /^w$/  && do { @opt_spec_file_list = (@opt_spec_file_list, shift @ARGV); last; };
      s/^I// && do { if ($_ ne ".") {
                       foreach my $include (`find $_/./ -type d ! -name tests`) {
                         $include =~ s/\n//;
                         $include = $include."/*.h";
                         $include =~ s/\/\//\//g;
                         my $have_headers = `ls $include >/dev/null 2>&1`;
                         if ($? >> 8 == 0) { @opt_header_file_list = (@opt_header_file_list, $include); }
                       };
                     }
                     last;
                   };
      s/^C// && do {
                     if ($_ ne "") { $opt_source_dir = $_; }
                     last;
                   };
      s/^P// && do {
                     if ($_ ne "") { $opt_parent_dir = $_; }
                     last;
                   };
      s/^R// && do { if ($_ =~ /^\//) { $opt_wine_root_dir = $_; }
                     else { $opt_wine_root_dir = `cd $pwd/$_ && pwd`; }
                     $opt_wine_root_dir =~ s/\n//;
                     $opt_wine_root_dir =~ s/\/\//\//g;
                     if (! $opt_wine_root_dir =~ /\/$/ ) { $opt_wine_root_dir = $opt_wine_root_dir."/"; };
                     last;
             };
      die "Unrecognised option $_\n";
    }
  }
  else
  {
    # A source file.
    push (@opt_source_file_list, $_);
  }
}

# Remove duplicate include directories
my %htmp;
@opt_header_file_list = grep(!$htmp{$_}++, @opt_header_file_list);

if ($opt_verbose > 3)
{
  print "Output dir:'".$opt_output_directory."'\n";
  print "Section   :'".$opt_manual_section."'\n";
  print "Format    :'".$opt_output_format."'\n";
  print "Source dir:'".$opt_source_dir."'\n";
  print "Root      :'".$opt_wine_root_dir."'\n";
  print "Spec files:'@opt_spec_file_list'\n";
  print "Includes  :'@opt_header_file_list'\n";
  print "Sources   :'@opt_source_file_list'\n";
}

if (@opt_spec_file_list == 0)
{
  exit 0; # Don't bother processing non-dll files
}

# Make sure the output directory exists
unless (-d $opt_output_directory)
{
    mkdir $opt_output_directory or die "Cannot create directory $opt_output_directory\n";
}

# Read in each .spec files exports and other details
while(my $spec_file = shift @opt_spec_file_list)
{
  process_spec_file($spec_file);
}

if ($opt_verbose > 3)
{
    foreach my $spec_file ( keys %spec_files )
    {
        print "in '$spec_file':\n";
        my $spec_details = $spec_files{$spec_file}[0];
        my $exports = $spec_details->{EXPORTS};
        for (@$exports)
        {
           print @$_[$EXPORT_ORDINAL].",".@$_[$EXPORT_CALL].", ".
                 @$_[$EXPORT_EXPNAME].",".@$_[$EXPORT_IMPNAME]."\n";
        }
    }
}

# Extract and output the comments from each source file
while(defined($_ = shift @opt_source_file_list))
{
  process_source_file($_);
}

# Write the index files for each spec
process_index_files();

# Write the master index file
output_master_index_files();

exit 0;
