package winapi_global;

use strict;
 
sub check {
    my $options = shift;
    my $winapi = shift;
    my $nativeapi = shift;

    my $winver = $winapi->name;

    if($options->argument) {
	foreach my $type ($winapi->all_declared_types) {
	    if(!$winapi->type_found($type) && $type ne "CONTEXT86 *") {
		print "*.c: $winver: $type: ";
		print "type not used\n";
	    }
	}
    }

    if($options->declared) {
	foreach my $name ($winapi->all_functions) {
	    if(!$winapi->function_found($name) && !$nativeapi->is_function($name)) {
		print "*.c: $winver: $name: ";
		print "function declared but not implemented: " . $winapi->function_arguments($name) . "\n";
	    }
	}
    }

    if($options->implemented) {
	foreach my $name ($winapi->all_functions_found) {
	    if($winapi->function_stub($name)) {
		print "*.c: $winver: $name: ";
		print "function implemented but not declared\n";
	    }
	}
    }
}

1;

