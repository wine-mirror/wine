package modules;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $options = \${$self->{OPTIONS}};
    my $output = \${$self->{OUTPUT}};
    my $modules = \%{$self->{MODULES}};

    $$options = shift;
    $$output = shift;
    my $module_file = shift;

    $module_file =~ s/^\.\///;

    if($$options->progress) {
	$$output->progress("$module_file");
    }

    my $allowed_dir;
    my $spec_file;

    open(IN, "< $module_file");
    local $/ = "\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begining and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines

	if(/^%\s+(.*?)$/) {
	    $spec_file = $1;
	    next;
	} else {
	    $allowed_dir = $1;
	}

	$$modules{$allowed_dir}{$spec_file}++;
    }
    close(IN);

    return $self;
}

1;
