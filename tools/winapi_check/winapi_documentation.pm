package winapi_documentation;

use strict;

my %comment_width;
my %comment_indent;
my %comment_spacing;

sub check_documentation {
    my $options = shift;
    my $output = shift;
    my $win16api = shift;
    my $win32api = shift;
    my $function = shift;

    my $module16 = $function->module16;
    my $module32 = $function->module32;
    my $external_name16 = $function->external_name16;
    my $external_name32 = $function->external_name32;
    my $internal_name = $function->internal_name;
    my $documentation = $function->documentation;
    my @argument_documentations = @{$function->argument_documentations};

    my $external_name;
    my $name1;
    my $name2;

    if(defined($module16) && !defined($module32)) {
	my @uc_modules16 = split(/\s*\&\s*/, uc($module16));
	push @uc_modules16, "WIN16";

	$name1 = $internal_name;
	foreach my $uc_module16 (@uc_modules16) {
	    if($name1 =~ s/^$uc_module16\_//) { last; }
	}

	$name2 = $name1;
	$name2 =~ s/(?:_)?16$//;
	$name2 =~ s/16_fn/16_/;

	$external_name = $external_name16;
    } elsif(!defined($module16) && defined($module32)) {
	my @uc_modules32 = split(/\s*\&\s*/, uc($module32));
	push @uc_modules32, "wine";

	foreach my $uc_module32 (@uc_modules32) {
	    if($uc_module32 =~ /^WS2_32$/) {
		push @uc_modules32, "WSOCK32";
	    }
	}

	$name1 = $internal_name;
	foreach my $uc_module32 (@uc_modules32) {
	    if($name1 =~ s/^$uc_module32\_//) { last; }
	}

	$name2 = $name1;
	$name2 =~ s/AW$//;

	$external_name = $external_name32;
    } else {
	my @uc_modules = split(/\s*\&\s*/, uc($module16));
	push @uc_modules, split(/\s*\&\s*/, uc($module32));

	$name1 = $internal_name;
	foreach my $uc_module (@uc_modules) {
	    if($name1 =~ s/^$uc_module\_//) { last; }
	}

	$name2 = $name1;
	$external_name = $external_name32;
    }

    if(!defined($external_name)) {
	$external_name = $internal_name;
    }

    if($options->documentation_name) {
	my $n = 0;
	if((++$n && defined($module16) && defined($external_name16) &&
	    $external_name16 ne "@" && $documentation !~ /\b\Q$external_name16\E\b/) ||
	   (++$n && defined($module16) && defined($external_name16) &&
	    $external_name16 eq "@" && $documentation !~ /\@/) ||
	   (++$n && defined($module32) && defined($external_name32) &&
	    $external_name32 ne "@" && $documentation !~ /\b\Q$external_name32\E\b/) ||
	   (++$n && defined($module32) && defined($external_name32) &&
	    $external_name32 eq "@" && $documentation !~ /\@/))
	{
	    my $external_name = ($external_name16, $external_name32)[($n-1)/2];
	    if($options->documentation_pedantic || $documentation !~ /\b(?:$internal_name|$name1|$name2)\b/) {
		$output->write("documentation: wrong or missing name ($external_name) \\\n$documentation\n");
	    }
	}
    }

    if($options->documentation_ordinal) {
	if(defined($module16)) {
	    my $ordinal16 = $win16api->function_internal_ordinal($internal_name);

	    if(!defined($ordinal16)) {
		$output->write("function have no ordinal\n");
	    } else {
		my @uc_modules16 = split(/\s*\&\s*/, uc($module16));
		foreach my $uc_module16 (@uc_modules16) {
		    if($documentation !~ /\b$uc_module16\.\Q$ordinal16\E/) {
			$output->write("documentation: wrong or missing ordinal " .
				       "expected ($uc_module16.$ordinal16) \\\n$documentation\n");
		    }
		}
	    }
	}
	if(defined($module32)) {
	    my $ordinal32 = $win32api->function_internal_ordinal($internal_name);

	    if(!defined($ordinal32)) {
		$output->write("function have no ordinal\n");
	    } else {
		my @uc_modules32 = split(/\s*\&\s*/, uc($module32));
		foreach my $uc_module32 (@uc_modules32) {
		    if($documentation !~ /\b$uc_module32\.\Q$ordinal32\E/) {
			$output->write("documentation: wrong or missing ordinal " .
						   "expected ($uc_module32.$ordinal32) \\\n$documentation\n");
		    }
		}
	    }
	}
    }

    if($options->documentation_pedantic) {
	my $ordinal = $win16api->function_internal_ordinal($internal_name);
	if(defined($ordinal) && $documentation !~ /^ \*\s+(?:\@|\w+)(?:\s+[\(\[]\w+\.(?:\@|\d+)[\)\]])+/m) {
	    $output->write("documentation: pedantic check failed \\\n$documentation\n");
	}
    }

    if($options->documentation_comment_indent) {
	if($documentation =~ /^ \*(\s*)\w+(\s*)([\(\[])\s*\w+\.\s*(?:\@|\d+)\s*([\)\]])/m) {
	    my $indent = $1;
	    my $spacing = $2;
	    my $left = $3;
	    my $right = $4;

	    $indent =~ s/\t/        /g;
	    $indent = length($indent);

	    $spacing =~ s/\t/        /g;
	    $spacing = length($spacing);

	    $comment_indent{$indent}++;
	    if($indent >= 20) {
		$output->write("documentation: comment indent is $indent\n");
	    }

	    $comment_spacing{$spacing}++;
	}
    }

    if($options->documentation_comment_width) {
	if($documentation =~ /(^\/\*\*+)/) {
	    my $width = length($1);

	    $comment_width{$width}++;
	    if($width <= 65 || $width >= 81) {
		$output->write("comment is $width columns wide\n");
	    }
	}
    }

    if($options->documentation_arguments) {
	my $n = 0;
	for my $argument_documentation (@argument_documentations) {
	    $n++;
	    if($argument_documentation ne "") {
		if($argument_documentation !~ /^\/\*\s+\[(?:in|out|in\/out|\?\?\?)\].*?\*\/$/s) {
		    $output->write("argument $n documentation: \\\n$argument_documentation\n");
		}
	    }
	}
    }
}

sub report_documentation {
    my $options = shift;
    my $output = shift;

    if($options->documentation_comment_indent) {
	foreach my $indent (sort(keys(%comment_indent))) {
	    my $count = $comment_indent{$indent};
	    $output->write("*.c: $count functions have comment that is indented $indent\n");
	}
    }

    if($options->documentation_comment_width) {
	foreach my $width (sort(keys(%comment_width))) {
	    my $count = $comment_width{$width};
	    $output->write("*.c: $count functions have comments of width $width\n");
	}
    }
}

1;
