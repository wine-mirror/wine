package winapi_parser;

use strict;

sub parse_c_file {
    my $options = shift;
    my $file = shift;
    my $function_found_callback = shift;

    my $level = 0;
    my $again = 0;
    my $lookahead = 0;
    my $lookahead_count = 0;

    print STDERR "Processing file '$file' ... " if $options->verbose;
    open(IN, "< $file") || die "<internal>: $file: $!\n";
    $/ = "\n";
    while($again || defined(my $line = <IN>)) {
	if(!$again) {
	    chomp $line;

	    if($lookahead) {
		$lookahead = 0;
		$_ .= "\n" . $line;
	    } else {
		$_ = $line;
		$lookahead_count = 0;
	    }
	    $lookahead_count++;
	    print "$level: $line\n" if $options->debug >= 2;
	} else {
	    $lookahead_count = 0;
	    $again = 0;
	}
       
	# remove comments
	if(s/^(.*?)\/\*.*?\*\/(.*)$/$1 $2/s) { $again = 1; next };
	if(/^(.*?)\/\*/s) {
	    $lookahead = 1;
	    next;
	}

	# remove empty rows
	if(/^\s*$/) { next; }

	# remove preprocessor directives
	if(s/^\s*\#.*$//m) { $again = 1; next; }

	if($level > 0)
	{
	    s/^[^\{\}]*//s;
	    if(/^\{/) {
		$_ = $'; $again = 1;
		print "+1: $_\n" if $options->debug >= 2;
		$level++;
	    } elsif(/^\}/) {
		$_ = $'; $again = 1;
		print "-1: $_\n" if $options->debug >= 2; 
		$level--;
	    }
	    next;
	} elsif(/((struct\s+|union\s+|enum\s+)?\w+((\s*\*)+\s*|\s+))(__cdecl|__stdcall|VFWAPIV|VFWAPI|WINAPIV|WINAPI)\s+(\w+(\(\w+\))?)\s*\(([^\)]*)\)\s*(\{|\;)/s) {
	    $_ = $'; $again = 1;

	    if($9 eq ";") {
		next;
	    } elsif($9 eq "{")  {		
		$level++;
	    }

	    my $return_type = $1;
	    my $calling_convention = $5;
	    my $name = $6;
	    my $arguments = $8;

	    $return_type =~ s/\s*$//;
	    $return_type =~ s/\s*\*\s*/*/g;
	    $return_type =~ s/(\*+)/ $1/g;

	    $name =~ s/^REGS_FUNC\((.*?)\)/$1/;

	    $arguments =~ y/\t\n/  /;
	    $arguments =~ s/^\s*(.*?)\s*$/$1/;
	    if($arguments eq "") { $arguments = "void" }
	    
	    my @arguments = split(/,/, $arguments);
	    foreach my $n (0..$#arguments) {
		my $argument = $arguments[$n];
		$argument =~ s/^\s*(.*?)\s*$/$1/;
		#print "  " . ($n + 1) . ": '$argument'\n";
		$argument =~ s/^(const(?=\s)|IN(?=\s)|OUT(?=\s)|(\s*))\s*//;
		if($argument =~ /^...$/) {
		    $argument = "...";
		} elsif($argument =~ /^((struct\s+|union\s+|enum\s+)?\w+)\s*((\*\s*?)*)\s*/) {
		    $argument = "$1";
		    if($3 ne "") {
			$argument .= " $3";
		    }
		} else {
		    die "$file: $.: syntax error: '$argument'\n";
		}
		$arguments[$n] = $argument;
		#print "  " . ($n + 1) . ": '" . $arguments[$n] . "'\n";
	    }
	    if($#arguments == 0 && $arguments[0] =~ /^void$/i) { $#arguments = -1;  } 

	    if($options->debug) {
		print "$file: $return_type $calling_convention $name(" . join(",", @arguments) . ")\n";
	    }
	    &$function_found_callback($return_type,$calling_convention,$name,\@arguments);

	} elsif(/DC_(GET_X_Y|GET_VAL_16)\s*\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    $_ = $'; $again = 1;
	    my @arguments = ("HDC16");
	    &$function_found_callback($2, "WINAPI", $3, \@arguments);
	} elsif(/DC_(GET_VAL_32)\s*\(\s*(.*?)\s*,\s*(.*?)\s*,.*?\)/s) {
	    $_ = $'; $again = 1;
	    my @arguments = ("HDC");
	    &$function_found_callback($2, "WINAPI", $3, \@arguments);
	} elsif(/DC_(GET_VAL_EX)\s*\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    $_ = $'; $again = 1;
	    my @arguments16 = ("HDC16", "LP" . $5 . "16");
	    my @arguments32 = ("HDC", "LP" . $5);
	    &$function_found_callback("BOOL16", "WINAPI", $2 . "16", \@arguments16);
	    &$function_found_callback("BOOL", "WINAPI", $2, \@arguments32);
	} elsif(/DC_(SET_MODE)\s*\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    $_ = $'; $again = 1;
	    my @arguments16 = ("HDC16", "INT16");
	    my @arguments32 = ("HDC", "INT");
	    &$function_found_callback("INT16", "WINAPI", $2 . "16", \@arguments16);
	    &$function_found_callback("INT", "WINAPI", $2, \@arguments32);
	} elsif(/WAVEOUT_SHORTCUT_(1|2)\s*\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    $_ = $'; $again = 1;
	    print "$_";
	    if($1 eq "1") {
		my @arguments16 = ("HWAVEOUT16", $4);
		my @arguments32 = ("HWAVEOUT", $4);
		&$function_found_callback("UINT16", "WINAPI", "waveOut" . $2 . "16", \@arguments16);
		&$function_found_callback("UINT", "WINAPI", "waveOut" . $2, \@arguments32);
	    } elsif($1 eq 2) {
		my @arguments16 = ("UINT16", $4);
		my @arguments32 = ("UINT", $4);
		&$function_found_callback("UINT16", "WINAPI", "waveOut". $2 . "16", \@arguments16);
		&$function_found_callback("UINT", "WINAPI", "waveOut" . $2, \@arguments32)
	    }
	} elsif(/;/s) {
	    $_ = $'; $again = 1;
	} elsif(/\{/s) {
	    $_ = $'; $again = 1;
	    print "+1: $_\n" if $options->debug >= 2;
	    $level++;
        } else {
	    $lookahead = 1;
	}
    }
    close(IN);
    print STDERR "done\n" if $options->verbose;
    print "$file: <>: not at toplevel at end of file\n" unless $level == 0;
}

1;
