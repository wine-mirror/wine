package setup;

use strict;

BEGIN {
    use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
    require Exporter;

    @ISA = qw(Exporter);
    @EXPORT = qw();
    @EXPORT_OK = qw($current_dir $wine_dir $winapi_dir $winapi_check_dir);

    use vars qw($current_dir $wine_dir $winapi_dir $winapi_check_dir);

    my $dir;
    my $tool;

    if($0 =~ m%^((.*?)/?tools/([^/]+))/([^/]+)$%)
    {
	$winapi_dir = $1;
	$winapi_check_dir = $1;
	$dir = $3;
	$tool = $4;
	
	if(defined($2) && $2 ne "")
	{
	    $wine_dir = $2;
	} else {
	    $wine_dir = ".";
	    
	} 
	
	if($wine_dir =~ /^\./) {
	    $current_dir = ".";
	    my $pwd; chomp($pwd = `pwd`);
	    foreach my $n (1..((length($wine_dir) + 1) / 3)) {
		$pwd =~ s/\/([^\/]*)$//;
		$current_dir = "$1/$current_dir";
	    }
	    $current_dir =~ s%/\.$%%;
	}
	
	$winapi_dir =~ s%^\./%%;
	$winapi_dir =~ s/$dir/winapi/g;
	
	$winapi_check_dir =~ s%^\./%%;
	$winapi_check_dir =~ s/$dir/winapi_check/g; 
    } else {
	print STDERR "$tool: You must run this tool in the main Wine directory or a sub directory\n";
	exit 1;
    }

    push @INC, ($winapi_dir, $winapi_check_dir);
}

1;
