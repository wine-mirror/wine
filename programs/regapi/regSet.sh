#!/bin/sh

# This script is the receipe to generate the key that have to be created like
# if an application was installed by its installer.  It processes using a
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

if [ ! -f $1 ]; then echo "$1 does not exist."; exit 1; fi
if [ ! -f $2 ]; then echo "$2 does not exist."; exit 1; fi

echo "Assuming that $1 is the \"before\" file..."
echo "Assuming that $2 is the \"after\" file..."

#
# do not attempt to regFix.pl /dev/null ...
#
echo "Fixing exported registry files..."

FIX1_FILE=`mktemp -q /tmp/file1_fix.XXXXXXXXX`
FIX2_FILE=`mktemp -q /tmp/file2_fix.XXXXXXXXX`
DIFF_FILE=`mktemp -q /tmp/file2_diff.XXXXXXXXX`
FILE_TOADD_CLEAN=`mktemp -q /tmp/file_toAdd_clean.XXXXXXXXX`
FILE_TOADD=`mktemp -q /tmp/file_toAdd.XXXXXXXXX`

if [ $1 != "/dev/null" ]; then
  cat $1 | ./regFixer.pl > $FIX1_FILE
fi

cat $2 | ./regFixer.pl > $FIX2_FILE


#
# diff accordingly depending on /dev/null
#
echo "Diffing..."
if [ $1 != "/dev/null" ]; then
  diff $FIX1_FILE $FIX2_FILE > $DIFF_FILE
else
  diff /dev/null  $FIX2_FILE > $DIFF_FILE
fi

#
# Keep only added lines
#
echo "Grepping keys to add and generating cleaned fixed registry file."
cat $DIFF_FILE | grep '^> ' | sed -e 's/^> //' > $FILE_TOADD_CLEAN

#
# Restore the file format to the regedit export 'like' format
#
echo "Restoring key's in the regedit export format..."
cat $FILE_TOADD_CLEAN | ./regRestorer.pl > $FILE_TOADD

echo "Cleaning..."
rm $FIX1_FILE $FIX2_FILE    >/dev/null 2>&1
rm $DIFF_FILE              >/dev/null 2>&1
rm $FILE_TOADD_CLEAN       >/dev/null 2>&1

if mv $FILE_TOADD $2.toAdd
then
  FILE_TOADD=$2.toAdd
fi

echo "Operation completed, result file is '$FILE_TOADD'"

exit 0
