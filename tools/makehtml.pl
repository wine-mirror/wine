#!/usr/bin/perl
open(APIW,"./apiw.index") or die "Can't find ./apiw.index";
while(<APIW>)
{
  ($func,$link)=split /:/;
  chop $link;
  $link=~m/(\d*)/;
  $apiw{$func}="http://www.willows.com/apiw/chapter$1/p$link.html";
}
close(APIW);

open(WINDOWS,"../include/windows.h") or die "Can't find ../include/windows.h";
while(<WINDOWS>) { add_func($_) if /AccessResource/../wvsprintf/; }
close(WINDOWS);
open(TOOLHELP,"../include/toolhelp.h") or die "Can't find ../include/toolhelp.h";
while(<TOOLHELP>) { add_func($_) if /GlobalInfo/../MemoryWrite/; }
close(TOOLHELP);
open(COMMDLG,"../include/commdlg.h") or die "Can't find ../include/commdlg.h";
while(<COMMDLG>) { add_func($_) if /ChooseColor/../ReplaceText/; }
close(COMMDLG);

print "<html><body>\n";

print "<h2>Windows API Functions</h2>\n";
print "The following API functions were found by searching windows.h,\n";
print "toolhelp.h, and commdlg.h.  Where possible, help links pointing\n";
print "to www.willows.com are included.<p>\n";
print "<table>\n";
print "<th>Help-link</th><th></th><th></th><th align=left>Function</th>\n";
foreach $func (sort(keys %funcs))
{
  $funcs{$func}=~m/(.*) +(\w*)(\(.*)/;
  print "<tr>\n<td>";
  if($apiw{$2})
  {
    print "<center><a href=\"$apiw{$2}\">APIW</a></center>";
    $impl{$2}=1;
    $impl++;
  }
  $numfuncs++;
  print "</td>\n";
  print "<td></td>\n";
  print "<td>$1</td>\n";
  print "<td>$2$3</td>\n";
  print "</tr>\n";
}
print "</table><p>\n";
print "(Approximately ",sprintf("%3.1f",$impl/(1.0*$numfuncs)*100.0),
      "% of the functions above are in the APIW standard.)<p>\n";

print "<hr>\n";
print "<h2>Unimplemented APIW functions</h2><p>\n";
print "Here's a list of the API functions in the APIW standard which were <b>not</b> found\n";
print "in windows.h, commdlg.h, or toolhelp.h:<p>\n";
foreach $func (sort (keys %apiw))
{
  if(!$impl{$func})
  {
    print "<a href=\"$apiw{$func}\">$func</a>\n";
    $unimpl++;
  }
  $numapiw++;
}
print "<p>(This comprises approximately ",sprintf("%3.1f",$unimpl/(1.0*$numapiw)*100.0),
      "% of the APIW.)\n";

print "</body></html>\n";

sub add_func
{
  $line=shift;
  chop $line; 
  $line=~s/\s+/ /g;
  ($func)=$line=~m/ (\w*)\(/;
  if($func)
  {
    while($funcs{$func}) { $func.=" "; }
    $funcs{$func}=$line;
  }
}
