## EINFÜHRUNG

Wine ist ein Programm, das es ermöglicht, Programme, die für Microsoft
Windows geschrieben wurden (inklusive DOS-, Windows 3.x-, Win32- und
Win64-Binärdateien), unter Unix auszuführen. Es besteht aus einem Programm-
Lader, der Microsoft-Windows-Binärdateien lädt und ausführt, sowie
einer Programmbibliothek (Winelib genannt), die Aufrufe der Windows API unter
Verwendung der entsprechenden Unix- oder X11-Gegenstücke implementiert.
Winelib kann auch benutzt werden, um Windows-Code nativ nach Unix
zu portieren.

Wine ist Freie Software, die unter der GNU LGPL veröffentlicht wird;
Bitte lesen Sie die Details in der Datei LICENSE nach.


## SCHNELLSTART

Rufen Sie aus dem Stammverzeichnis des Wine-Quelltextes (das die Datei README
enthält) folgende Befehle auf:

```
./configure
make
```

Im Anschluss können Sie Wine entweder installieren:

```
make install
```

Oder direkt aus dem Quellverzeichnis ausführen:

```
./wine notepad
```

Starten Sie Programme mit `wine Programmname`. Weitere
Informationen und Problemlösungen entnehmen Sie bitte dem Rest dieser
Datei, der Man-Page von Wine und insbesondere der Vielzahl an 
Informationen, die Sie auf https://www.winehq.org finden.


## VORAUSSETZUNGEN

Um Wine kompilieren und ausführen zu können, benötigen Sie eines der
folgenden Betriebssysteme:

- Linux version 2.0.36 oder neuer
- FreeBSD 12.4 oder neuer
- Solaris x86 9 oder neuer
- NetBSD-current
- Mac OS X 10.8 oder neuer

Da Wine Thread-Unterstützung auf Kernelebene benötigt, werden nur die oben
aufgeführten Betriebssysteme unterstützt.
Andere Betriebssysteme, die Kernel-Threads unterstützen, werden unter
Umständen in der Zukunft unterstützt.

**FreeBSD-Info**:
  Wine wird generell nicht korrekt unter FreeBSD vor 8.0 funktionieren. Für
  weitere Informationen überprüfen Sie auch https://wiki.freebsd.org/Wine

**Solaris-Info**:
  Höchstwahrscheinlich werden Sie Wine mit der GNU-Toolchain (gcc, gas etc.)
  kompilieren müssen. Warnung: Die Installation von gas stellt *nicht*
  sicher, dass es auch von gcc verwendet wird. Erneutes Kompilieren von gcc,
  nach der Installation von gas, oder das symbolische Verlinken von cc, as und
  ld auf die GNU-Tools ist vermutlich notwendig.

**NetBSD-Info**:
  Stellen Sie sicher, dass Sie die Optionen USER_LDT, SYSVSHM, SYSVSEM und
  SYSVMSG in Ihrem Kernel aktiviert haben.

**Mac OS X Info**:
  Sie benötigen Xcode/Xcode-Kommandozeilentools oder Apple cctools. Die
  Mindestanforderungen um Wine zu kompilieren sind clang 3.8 mit dem
  MacOSX10.10.sdk und mingw-w64 v8. Das MacOSX10.14.sdk oder neuer kann
  nur wine64 erzeugen.

**Unterstützte Dateisysteme**:
  Wine sollte auf den meisten Dateisystemen laufen. Kompatibilitätsprobleme
  wurden allerdings beim Dateizugriff über Samba gemeldet.
  Weiterhin bietet NTFS nicht alle Dateisystemfunktionen, die von einigen
  Programmen benötigt werden. Ein natives Unix-Dateisystem ist empfohlen.

**Grundsätzliche Voraussetzungen**:
  Sie müssen die Entwicklungsdateien für X11 installiert haben
  (Debian nennt diese xorg-dev, Red Hat libX11-devel).
  Selbstverständlich benötigen Sie auch make (höchstwahrscheinlich GNU make).
  Des Weiteren benötigen Sie flex (Version 2.5.33 oder höher) und bison.

**Optionale Bibliotheken**:
  Der Configure-Aufruf zeigt am Ende optionale Bibliotheken an,
  die von Wine benutzt werden können, aber auf Ihrem System nicht gefunden
  wurden. Tipps zum Installieren fehlender Pakete finden Sie unter:
  https://wiki.winehq.org/Recommended_Packages
  Unter 64-Bit-Plattformen benötigen Sie auch die 32-Bit-Versionen dieser
  Bibliotheken.


## KOMPILIEREN

Um Wine zu kompilieren, führen Sie aus:

```
./configure
make
```

Damit werden das Programm "wine" und diverse unterstützende Bibliotheken und
Binärdateien erstellt. Das Programm "wine" lädt ausführbare Windows-Dateien
und führt diese aus.
Die Bibliothek "libwine" ("Winelib") kann benutzt werden, um Windows-Quelltexte
unter Unix zu kompilieren und zu verlinken.

Mit `./configure --help` können Sie sich Konfigurations-Optionen für die
Kompilierung anzeigen lassen.

Weitere Informationen finden Sie unter https://wiki.winehq.org/Building_Wine


## SETUP

Nachdem Wine korrekt erstellt wurde, können Sie `make install` aufrufen;
Dadurch werden Wine-Programme und Bibliotheken, die Man-Page und andere nötige
Dateien installiert.

Vergessen Sie nicht, zuerst frühere Wine-Installationen zu entfernen, die mit
der neuen in Konflikt stehen könnten. Versuchen Sie entweder `dpkg -r wine`,
`rpm -e wine` oder `make uninstall` vor der Installation auszuführen.

Nach der Installation können Sie das Konfigurationswerkzeug `winecfg` starten.
Hinweise dazu finden Sie im Support-Bereich auf https://www.winehq.org/


## AUSFÜHREN VON PROGRAMMEN

Wenn Sie Wine aufrufen, können Sie den vollständigen Pfad zur ausführbaren
Datei angeben oder nur einen Dateinamen.

Beispiel: Um Notepad auszuführen:

```
wine notepad            (wobei der Suchpfad, wie in der Registrierung
wine notepad.exe         angegeben, benutzt wird)

wine c:\\windows\\notepad.exe         (mit Dateinamen-Syntax von DOS)

wine ~/.wine/drive_c/windows/notepad.exe            (mit Unix-Syntax)

wine notepad.exe liesmich.txt         (Programmaufruf mit Parametern)
```

Wine ist nicht perfekt, manche Programme könnten abstürzen.
In solchen Fällen bekommen Sie einen Log vom Absturz, den Sie beifügen sollten,
wenn Sie den Fehler melden.


## WEITERFÜHRENDE INFORMATIONEN

- **WWW**: Eine große Menge an Informationen findet sich im WineHQ unter
        https://www.winehq.org/ : Verschiedene Wine Guides,
        Applikations-Datenbank, Bugtracker.
        Das ist vermutlich der beste Ausgangspunkt.

- **FAQ**: Die FAQ zu Wine befindet sich unter https://www.winehq.org/FAQ

- **Wiki**: Das Wine-Wiki finden Sie unter https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **Mailing-Listen**:
    Es gibt mehrere Mailing-Listen für Wine-Anwender und -Entwickler;
    Schauen Sie sich dazu bitte https://www.winehq.org/forums an.

- **Fehler**: Melden Sie Fehler bitte an Wine-Bugzilla unter https://bugs.winehq.org
    Schauen Sie bitte erst in der Bugzilla-Datenbank nach, ob Ihr Problem
    bereits bekannt ist oder sogar behoben wurde, bevor Sie eine Fehlermeldung
    verfassen.

- **IRC**: Online-Hilfe ist im IRC-Kanal `#WineHQ` unter irc.libera.chat verfügbar.
