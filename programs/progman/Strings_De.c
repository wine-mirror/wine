#include <windows.h>
#include "progman.h"

LPCSTR StringTableDe[NUMBER_OF_STRINGS] =
{
  "Programm-Manager",
  "FEHLER",
  "Information",
  "Löschen",
  "Lösche Programmgruppe `%s' ?",
  "Lösche Programm `%s' ?",
  "Nicht implementiert",
  "Fehler beim Lesen von `%s'",
  "Fehler beim Schreiben von `%s'",

  "Die Programmgruppendatei `%s' kann nicht geöffnet werden.\n"
  "Soll weiterhin versucht werden, diese Datei zu laden?",

  "Zu wenig Hauptspeicher",
  "Keine Hilfe verfügbar",
  "Unbekannte Eigenschaft der `.grp' Datei",
  "Datei `%s' existiert. Sie wird nicht überschrieben.",
  "Die Programmgruppe wird als `%s' gesichert um das Überschreiben der Originaldatei zu verhindern.",
  "Keine",

  "Alle Dateien (*.*)\0"   "*.*\0"
  "Programme\0"            "*.exe;*.pif;*.com;*.bat\0",

  "Alle Dateien (*.*)\0"   "*.*\0"
  "Bibliotheken (*.dll)\0" "*.dll\0"
  "Programme\0"            "*.exe\0"
  "Symboldateien\0"        "*.ico;*.exe;*.dll\0"
  "Symbole (*.ico)\0"      "*.ico\0"
};
