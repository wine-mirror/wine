package output;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress = "";
    $$last_progress = "";
    $progress_count = 0;

    return $self;
}


sub show_progress {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress_count++;

    if($$progress_count > 0 && $$progress) {
	print STDERR $$progress;
	$$last_progress = $$progress;
    }
}

sub hide_progress  {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress_count--;

    if($$last_progress) {
	my $message;
	for (1..length($$last_progress)) {
	    $message .= " ";
	}
	print STDERR $message;
	undef $$last_progress;
    }
}

sub update_progress {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    
    my $prefix = "";
    my $suffix = "";
    if($$last_progress) {
	for (1..length($$last_progress)) {
	    $prefix .= "";
	}
	
	my $diff = length($$last_progress)-length($$progress);
	if($diff > 0) {
	    for (1..$diff) {
		$suffix .= " ";
	    }
	    for (1..$diff) {
		$suffix .= "";
	    }
	}
    }
    print STDERR $prefix . $$progress . $suffix;
    $$last_progress = $$progress;
}

sub progress {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};

    $$progress = shift;

    $self->update_progress;
}

sub write {
    my $self = shift;

    my $message = shift;


    $self->hide_progress;
    print $message;
    $self->show_progress;
}

1;
