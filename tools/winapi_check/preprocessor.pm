package preprocessor;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $state = \%{$self->{STATE}};
    my $stack = \@{$self->{STACK}};
    my $include_found = \${$self->{INCLUDE_FOUND}};
    my $conditional_found = \${$self->{CONDITIONAL_FOUND}};
  
    $$include_found = shift;
    $$conditional_found = shift;

    return $self;
}

sub include {
    my $self = shift;
    my $include_found = \${$self->{INCLUDE_FOUND}};

    my $argument = shift;

    &$$include_found($argument);
}

sub define {
    my $self = shift;
    my $state = \%{$self->{STATE}};
    my $conditional_found = \${$self->{CONDITIONAL_FOUND}};

    my $name = shift;

    $$state{$name} = "def";

    &$$conditional_found($name);
}

sub undefine {
    my $self = shift;
    my $state = \%{$self->{STATE}};
    my $conditional_found = \${$self->{CONDITIONAL_FOUND}};

    my $name = shift;

    $$state{$name} = "undef";

    &$$conditional_found($name);
}

sub begin_if {
    my $self = shift;
    my $state = \%{$self->{STATE}};
    my $stack = \@{$self->{STACK}};

    my $directive = shift;
    local $_ = shift;

    while(!/^$/) {
	if(/^0\s*\&\&/) {
	    $_ = "0";
	} elsif(/^1\s*\|\|/) {
	    $_ = "1";
	}

	if(/^(!)?defined\s*\(\s*(.+?)\s*\)\s*((\&\&|\|\|)\s*)?/){
	    $_ = $';
	    if(defined($1) && $1 eq "!") {
		$self->undefine($2);
		push @$stack, $2;
	    } else {
		$self->define($2);
		push @$stack, $2;
	    }
	} elsif(/^(\w+)\s*(<|<=|==|!=|>=|>)\s*(\w+)\s*((\&\&|\|\|)\s*)?/) {
	    $_ = $';
	} elsif(/^(\w+)\s*$/) {
	    $_ = $';
	} elsif(/^\(|\)/) {
	    $_ = $';
	} else {
	    print "*** Can't parse '#$directive $_' ***\n";
	    $_ = "";
	}
    }
}

sub else_if {
    my $self = shift;
    my $state = \%{$self->{STATE}};
    my $stack = \@{$self->{STACK}};

    my $argument = shift;

    $self->end_if;

    if(defined($argument)) {
	$self->begin_if("elif", $argument);
    }
}

sub end_if {
    my $self = shift;
    my $state = \%{$self->{STATE}};
    my $stack = \@{$self->{STACK}};

    my $macro = pop @$stack;
    delete $$state{$macro} if defined($macro);
}

sub directive {
    my $self = shift;
    my $state = \%{$self->{STATE}};
    my $stack = \@{$self->{STACK}};

    my $directive = shift;
    my $argument = shift;

    local $_ = $directive;
    if(/^if$/) {
	$self->begin_if("if",$argument);
    } elsif(/^ifdef$/) {
	$self->begin_if("if", "defined($argument)");
    } elsif(/^ifndef$/) {
	$self->begin_if("if", "!defined($argument)");
	push @$stack, $argument;
    } elsif(/^elif$/) {
	$self->else_if($argument);
    } elsif(/^else$/) {
	$self->else_if;
    } elsif(/^endif$/) {
	$self->end_if;
    } elsif(/^include/) {
	$self->include($argument);
    }
}

sub is_def {
    my $self = shift;
    my $state = \%{$self->{STATE}};

    my $name = shift;
    
    my $status = $$state{$name};

    return defined($status) && $status eq "def";
}

sub is_undef {
    my $self = shift;
    my $state = \%{$self->{STATE}};

    my $name = shift;
    
    my $status = $$state{$name};

    return defined($status) && $status eq "undef";
}

sub is_unknown {
    my $self = shift;
    my $state = \%{$self->{STATE}};

    my $name = shift;
    
    my $status = $$state{$name};

    return !defined($status);
}

1;
