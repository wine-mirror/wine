#!/bin/bash

# This script is the receipe to generate the key that have to be created like 
# if an applicaiton was installed by its installer.  It processes using a 
# registry based on the picture of the registry before the application is 
# installed and the picture of the registry after the application is installed.
#
# Copyright 1999 Sylvain St-Germain
#

if [ $# -ne 2 ]; then
  echo "$0 Usage: "
  echo "  You must provide 2 arguments."
  echo "  1 - Registry output before the application's installation."
  echo "  2 - Registry output after the application's installation."
  echo 
  exit 1
fi

echo "Assuming that $1 is the \"before\" file..."
echo "Assuming that $2 is the \"after\" file..."

#
# do not attempt to regFix.pl /dev/null ...
#
echo "Fixing exported registry files..."
if [ $1 != "/dev/null" ]; then  
  cat $1 | ./regFixer.pl > $1.fix
fi

cat $2 | ./regFixer.pl > $2.fix

#
# diff accordingly depending on /dev/null
#
echo "Diffing..."
if [ $1 != "/dev/null" ]; then  
  diff $1.fix $2.fix > $2.diff
else
  diff /dev/null $2.fix > $2.diff
fi
#
# Keep only added lines
# 
echo "Grepping keys to add and generating cleaned fixed registry file."
cat $2.diff | grep '^> ' | sed -e 's/^> //' > $2.toAdd

# 
# Restore the file format to the regedit export 'like' format
#
echo "Restoring key's in the regedit export format..."
cat $2.toAdd | ./regRestorer.pl > $2.toAdd.final

echo "Cleaning..."
rm $1.fix $2.fix >/dev/null 2>&1
rm $2.diff       >/dev/null 2>&1
rm $2.toAdd      >/dev/null 2>&1
mv $2.toAdd.final $2.toAdd 

echo "Operation completed, result file is $2.toAdd"

exit 0
