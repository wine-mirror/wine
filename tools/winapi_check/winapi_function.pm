package winapi_function;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    return $self;
}

sub file {
    my $self = shift;
    my $file = \${$self->{FILE}};

    local $_ = shift;

    if(defined($_)) { $$file = $_; }
    
    return $$file;
}

sub documentation {
    my $self = shift;
    my $documentation = \${$self->{DOCUMENTATION}};

    local $_ = shift;

    if(defined($_)) { $$documentation = $_; }
    
    return $$documentation;
}

sub return_type {
    my $self = shift;
    my $return_type = \${$self->{RETURN_TYPE}};

    local $_ = shift;

    if(defined($_)) { $$return_type = $_; }
    
    return $$return_type;
}

sub calling_convention {
    my $self = shift;
    my $calling_convention = \${$self->{CALLING_CONVENTION}};

    local $_ = shift;

    if(defined($_)) { $$calling_convention = $_; }
    
    return $$calling_convention;
}

sub name {
    my $self = shift;
    my $name = \${$self->{NAME}};

    local $_ = shift;

    if(defined($_)) { $$name = $_; }
    
    return $$name;
}

sub arguments {
    my $self = shift;
    my $arguments = \${$self->{ARGUMENTS}};

    local $_ = shift;

    if(defined($_)) { $$arguments = $_; }
    
    return $$arguments;
}

sub module16 {
    my $self = shift;
    my $module16 = \${$self->{MODULE16}};

    local $_ = shift;

    if(defined($_)) { $$module16 = $_; }
    return $$module16;
}

sub module32 {
    my $self = shift;
    my $module32 = \${$self->{MODULE32}};

    local $_ = shift;
    
    if(defined($_)) { $$module32 = $_; }	
    return $$module32;
}

sub statements {
    my $self = shift;
    my $statements = \${$self->{STATEMENTS}};

    local $_ = shift;

    if(defined($_)) { $$statements = $_; }
    
    return $$statements;
}

sub module {
    my $self = shift;
    my $module16 = \${$self->{MODULE16}};
    my $module32 = \${$self->{MODULE32}};

    my $module;
    if(defined($$module16) && defined($$module32)) {
	$module = "$$module16 & $$module32";
    } elsif(defined($$module16)) {
	$module = $$module16;
    } elsif(defined($$module32)) {
	$module = $$module32;
    } else {
	$module = "";
    }
}

sub function_called {    
    my $self = shift;
    my $called_function_names = \%{$self->{CALLED_FUNCTION_NAMES}};

    my $name = shift;

    $$called_function_names{$name}++;
}

sub function_called_by { 
   my $self = shift;
   my $called_by_function_names = \%{$self->{CALLED_BY_FUNCTION_NAMES}};

   my $name = shift;

   $$called_by_function_names{$name}++;
}

sub called_function_names {    
    my $self = shift;
    my $called_function_names = \%{$self->{CALLED_FUNCTION_NAMES}};

    return sort(keys(%$called_function_names));
}

sub called_by_function_names {    
    my $self = shift;
    my $called_by_function_names = \%{$self->{CALLED_BY_FUNCTION_NAMES}};

    return sort(keys(%$called_by_function_names));
}


1;
