package nativeapi;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $functions = \%{$self->{FUNCTIONS}};

    my $file = shift;

    open(IN, "< $file");
    $/ = "\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begin and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines   
	
	$$functions{$_} = 1;
    }
    close(IN);

    return $self;
}

sub is_function {
    my $self = shift;
    my $functions = \%{$self->{FUNCTIONS}};

    my $name = shift;

    return $$functions{$name};
}

1;
