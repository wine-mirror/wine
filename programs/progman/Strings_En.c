#include <windows.h>
#include "progman.h"

LPCSTR StringTableEn[NUMBER_OF_STRINGS] =
{
  "Program Manager",
  "ERROR",
  "Information",
  "Delete",
  "Delete group `%s' ?",
  "Delete program `%s' ?",
  "Not implemented",
  "Error reading `%s'",
  "Error writeing `%s'",

  "The group file `%s' cannot be opened.\n"
  "Should it be tried further on?",

  "Out of memory",
  "Help not available",
  "Unknown feature in `.grp' file",
  "File `%s' exists. Not overwritten.",
  "Save group as `%s' to prevent overwriting original files",
  "None",

  "All files (*.*)\0"   "*.*\0"
  "Programs\0"          "*.exe;*.pif;*.com;*.bat\0",

  "All files (*.*)\0"   "*.*\0"
  "Libraries (*.dll)\0" "*.dll\0"
  "Programs\0"          "*.exe\0"
  "Symbol files\0"      "*.ico;*.exe;*.dll\0"
  "Symbols (*.ico)\0"   "*.ico\0"
};
