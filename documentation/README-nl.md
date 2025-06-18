## INTRODUCTIE

Wine is een programma wat het mogelijk maakt om Microsoft Windows
programma's (waaronder uitvoerbare DOS, Windows 3.x, Win32 en Win64
bestanden) op Unix uit te voeren. Het bestaat uit een programma-
lader die het Microsoft Windows binaire bestand laadt en uitvoert.
Een bibliotheek (Winelib genaamd) die de Windows API aanroepen laat
uitvoeren door overeenkomstige Unix, X11 of Mac varianten. Deze
bibliotheek kan ook worden gebruikt om Windows code om te zetten in
echte Unix uitvoerbare bestanden.

Wine is vrije software, uitgebracht onder de GNU LGPL. Zie het
LICENCE bestand voor meer informatie.


## SNEL AAN DE SLAG

Voer in de hoofdmap van de Wine broncode (waar het README bestand staat),
het volgende uit:

```
./configure
make
```

Installeer daarna Wine met:

```
make install
```

Of voer Wine uit in de map waarin die gebouwd is:

```
./wine notepad
```

Programma's kunnen uitgevoerd worden met `wine programma`. Lees voor meer
informatie en het oplossingen van problemen de rest van dit bestand, de
man pagina's van Wine en vooral de overvloed aan informatie op
https://winehq.org .


## BENODIGDHEDEN

Om Wine te kunnen compileren en uit te voeren, is één van het volgende
besturingssystemen nodig:

- Linux versie 2.6.22 of nieuwer
- FreeBSD 12.4 of nieuwer
- Solaris x86 9 of nieuwer
- NetBSD-current
- macOS 10.12 of nieuwer

Omdat Wine threadondersteuning op kernelniveau vereist, worden alleen de
bovenstaande besturingssystemen ondersteund. Andere besturingssystemen
die threadondersteuning op kernelniveau hebben, kunnen mogelijk in de
toekomst ook ondersteund worden.

**FreeBSD informatie**:
  Zie https://wiki.freebsd.org/Wine voor meer informatie.

**Solaris informatie**:
  Hoogst waarschijnlijk moet Wine gebouwd worden met de GNU toolchain
  (gcc, gas, enz.). Waarschuwing: het installeren van gas houdt *niet*
  in dat het gebruikt wordt door gcc. Hercompileren van gcc nadat gas
  is geïnstalleerd of symbolische verwijzingen maken voor cc, as en
  ld naar de gnu-tools is waarschijnlijk noodzakelijk.

**NetBSD informatie**:
  Zorg er voor dat de volgende opties aan staan in de kernel: USER_LDT,
  SYSVSHM, SYSVSEM, en SYSVMSG.

**Mac OS X informatie**:
  Xcode/Xcode Command Line Tools of Apple cctools zijn nodig. De minimale
  vereisten om Wine te kunnen compileren zijn clang 3.8 met MacOSX10.10.sdk
  en mingw-w64 v8. MacOSX10.14.sdk en nieuwer kunnen alleen wine64 bouwen.

**Ondersteunde bestandssystemen**:
  Wine zou op de meeste bestandssystemen uitgevoerd moeten kunnen worden.
  Er zijn enkele compatibiliteitsproblemen bekend met bestanden die via
  Samba worden benaderd. Ook heeft NTFS niet alle bestandssysteemopties die
  nodig zijn met sommige programma's. Een oorspronkelijk Unix
  bestandssysteem wordt aangeraden.

**Basis benodigdheden**:
  De X11-ontwikkel-bestanden moeten geïnstalleerd zijn. (Voor Debian is dat
  pakket xorg-dev in RedHat is dit libX11-devel).
  Natuurlijk is ook make nodig. (Hoogst waarschijnlijk GNU make.)
  Ook zijn bison en flex versie 2.5.33 of nieuwer nodig.

**Optionele bibliotheken**:
  Tijdens het uitvoeren van ./configure wordt er aangegeven of de optionele
  bibliotheken zijn gevonden op het systeem. Bekijk
  https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine voor meer
  informatie over welke pakketten er geïnstalleerd zouden moeten
  worden. Op 64-bit systemen moeten ook de 32-bit versies van deze
  pakketten geïnstalleerd worden.


## COMPILEREN

Om Wine te bouwen, voer het volgende uit:

```
./configure
make
```

Hiermee wordt het programma "wine" en talrijke ondersteunende bibliotheken
en uitvoerbare bestanden gemaakt. Het programma "wine" laadt en voert de
uitvoerbare Windows bestanden uit.
De bibliotheek "libwine" ("Winelib") kan worden gebruikt om Windows
broncode te compileren en te linken in Unix.

Voor alle opties tijdens het compileren, voer `./configure --help` uit.

Voor meer informatie bekijk: https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine


## SETUP

Wanneer Wine goed in elkaar is gezet, kan Wine geïnstalleerd worden met
`make install`. Deze opdracht installeert het uitvoerbare wine bestand,
de bibliotheken, de Wine man pagina's en alle andere benodigde bestanden.

Vergeet niet om eerst elke tegenstrijdige Wine installatie te verwijderen.
Dit kan voor de installatie met `dpkg -r wine`, `rpm -e wine` of met
`make uninstall`

Eenmaal geïnstalleerd, kan het configuratie programma `winecfg` worden
uitgevoerd. Bekijk de Support pagina op https://www.winehq.org/ voor
configuratiehints.


## PROGRAMMA'S UITVOEREN

Bij het gebruiken van Wine kan het gehele pad naar het uitvoerbare bestand
worden gebruikt of alleen de bestandsnaam.

Voorbeeld: het Kladblok (Notepad) uitvoeren:

```
wine notepad           (gebruikt het zoek-pad, zoals in het register is
wine notepad.exe        opgegeven, om het bestand te vinden)

wine c:\\windows\\notepad.exe    (met een DOS bestandsnaam constructie)

wine ~/.wine/drive_c/windows/notepad.exe         (een Unix constructie)

wine notepad.exe readme.txt              (een programma met parameters)
```

Wine is niet perfect. Dus sommige programma's kunnen crashen. Als dat
gebeurd komt er een logboek van de crash. Deze kan bijgevoegd worden als de
fout wordt gerapporteerd.


## MEER INFORMATIE

- **WWW**: Een grote verscheidenheid aan informatie voor Wine is beschikbaar
        gemaakt door WineHQ op https://www.winehq.org/ : verschillende
        handleidingen, programma database, bug tracking. Dit is
        waarschijnlijk de beste plek om te beginnen.

- **FAQ**: Veel vragen over Wine zijn te vinden op https://gitlab.winehq.org/wine/wine/-/wikis/FAQ

- **Wiki**: De Wine-Wiki staat op https://gitlab.winehq.org/wine/wine/-/wikis/

- **Gitlab**: https://gitlab.winehq.org

- **Mail-lijsten**:
        Er zijn meerdere mail-lijsten voor gebruikers en ontwikkelaars van
        Wine. Bekijk https://gitlab.winehq.org/wine/wine/-/wikis/Forums voor meer informatie.

- **Fouten**: Op https://bugs.winehq.org kunnen fouten gemeld worden aan de Wine
        Bugzilla. Doorzoek eerst de database van bugzilla om te kijken of
        de fout al bekend of gerepareerd is voordat de fout gemeld wordt.

- **IRC**: Online hulp is beschikbaar in kanaal `#WineHQ` op irc.libera.chat
