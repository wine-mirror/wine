#!/bin/bash

# This script is the receipe to generate the key that have to be created like 
# if an applicaiton was installed by its installer.  It processes using a 
# registry based on the picture of the registry before the application is 
# installed and the picture of the registry after the application is installed.
#
# Copyright 1999 Sylvain St-Germain
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
cat $2.diff | grep '^> ' | sed -e 's/^> //' > $2.toAdd.clean

# 
# Restore the file format to the regedit export 'like' format
#
echo "Restoring key's in the regedit export format..."
cat $2.toAdd.clean | ./regRestorer.pl > $2.toAdd

echo "Cleaning..."
rm $1.fix $2.fix    >/dev/null 2>&1
rm $2.diff          >/dev/null 2>&1
rm $2.toAdd.clean   >/dev/null 2>&1

echo "Operation completed, result file is $2.toAdd"

exit 0
