package parser;

BEGIN {
    use Exporter   ();
    use vars       qw(@ISA @EXPORT);

    @ISA         = qw(Exporter);
    @EXPORT      = qw(&init
		      &transact &commit &rollback &token
		      &dump_state
		      &either &filter &many &many1 &separate &separate1 &sequence);
}

my @stack;
my $current;
my $next;

sub init {
    @stack = ();
    $current = [];
    $next = shift;

    @$next = grep {
	$_->{type} !~ /^comment|preprocessor$/;
    } @$next;
}

sub dump_state {
    print "stack: [\n";
    for my $tokens (@stack) {
	print "  [\n";
	for my $token (@$tokens) {
	    print "    " . $token->{type} . ": " . $token->{data} . "\n";
	}
	print "  ]\n";
    }
    print "]\n";
    print "current: [\n";
    for my $token (@$current) {
	print "  " . $token->{type} . ": " . $token->{data} . "\n";
    }
    print "]\n";
    print "next: [\n";
    for my $token (@$next) {
	print "  " . $token->{type} . ": " . $token->{data} . "\n";
    }
    print "]\n";
}

sub token {
    my $token = shift @$next;
    push @$current, $token;
    return $token;
};

sub transact {
    push @stack, $current; 
    $current = [];
}

sub commit {
    my $oldcurrent = $current;
    $current = pop @stack;
    push @$current, @$oldcurrent;
}

sub rollback {
    unshift @$next, @$current;
    $current = pop @stack;
}

sub filter {
    my $parser = shift;
    my $filter = shift;

    transact;
    my $r1 = &$parser;
    if(defined($r1)) {
	my $r2 = &$filter($r1);
	if($r2) {
	    commit;
	    return $r1;
	} else {
	    rollback;
	    return undef;
	}
    } else {
	rollback;
	return undef;
    }
}

sub either {
    for my $parser (@_) {
	transact;
	my $r = &$parser;
	if(defined($r)) {
	    commit;
	    return $r;
	} else {
	    rollback;
	}
    }
    return undef;
}

sub sequence {
    transact;
    my $rs = [];
    for my $parser (@_) {
	my $r = &$parser;
	if(defined($r)) {
	    push @$rs, $r;
	} else {
	    rollback;
	    return undef;
	}	
    }
    commit;
    return $rs;
}

sub separate {
    my $parser = shift;
    my $separator = shift;
    my $rs = [];
    while(1) {
	my $r = &$parser;
	if(defined($r)) {
	    push @$rs, $r;
	} else {
	    last;
	}
	my $s = &$separator;
	if(!defined($r)) {
	    last;
	}
    
    }
    return $rs;
}

sub separate1 {
    my $parser = shift;
    my $separator = shift;
    transact;
    my $rs = separate($parser,$separator);
    if($#$rs != -1) {
	commit;
	return $rs;
    } else {
	rollback;
	return undef;
    }
}

sub many {
    my $parser = shift;
    my $rs = [];
    while(1) {
	my $r = &$parser;
	if(defined($r)) {
	    push @$rs, $r;
	} else {
	    last;
	}
    }
    return $rs;
}

sub many1 {
    my $parser = shift;
    transact;
    my $rs = many($parser);
    if($#$rs != -1) {
	commit;
	return $rs;
    } else {
	rollback;
	return undef;
    }
}

1;
