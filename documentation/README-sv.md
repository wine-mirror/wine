## INLEDNING

Wine är ett program som gör det möjligt att köra Windows-program
(inkluderat DOS, Windows 3.x, Win32 och Win64) i Unix. Det består av en
programstartare som startar och kör Windows-programfiler, samt ett
bibliotek (kallat Winelib) som implementerar Windows API-anrop med hjälp
av motsvarande Unix-, X11- eller Mac-anrop. Biblioteket kan också användas
till att porta Windows-kod till vanliga Unix-program.

Wine är fri programvara, utgett under villkoren i GNU LGPL; se
filen LICENSE för detaljer.


## KOM IGÅNG

Kör följande kommandon i rotkatalogen för Wines källkod:

```
./configure
make
```

Efteråt antingen installera Wine:

```
make install
```

Eller kör Wine direkt från dess rotkatalog:

```
./wine notepad
```

Kör program med `wine program`. Se resten av denna fil,
Wines man-sidor samt sist men inte minst https://www.winehq.org/ för mer
information och tips om hur problem kan lösas.


## SYSTEMKRAV

För att kompilera och köra Wine krävs ett av följande:

- Linux version 2.0.36 eller senare
- FreeBSD 12.4 eller senare
- Solaris x86 9 eller senare
- NetBSD-current
- Mac OS X 10.8 eller senare

Wine kräver stöd för trådar på kernelnivå, och därför är det bara
operativsystemen ovan som stöds. Andra operativsystem som
stöder kerneltrådar kommer eventuellt att stödjas i framtiden.

**Information för FreeBSD**:
  Se https://wiki.freebsd.org/Wine för mer information.

**Information för Solaris**:
  Wine måste antagligen byggas med GNU toolchain (gcc, gas etc.).
  Varning: även om gas installeras så är det inte säkert att det används av
  gcc. Det sägs att det är nödvändigt att antingen bygga gcc på nytt, eller
  skapa symboliska länkar från "cc", "as" och "ld" till GNU toolchain.

**Information för NetBSD**:
  USER_LDT, SYSVSHM, SYSVSEM och SYSVMSG måste vara aktiverade i kerneln.

**Information för Mac OS X**:
  Du behöver Xcode 2.4 eller senare för att korrekt kunna bygga Wine på x86.
  Mac-drivrutinen kräver OS X 10.6 eller senare och kommer inte att byggas på 10.5.

**Stödda filsystem**:
  Wine kan köra på de flesta filsystem, men det har rapporterats problem vad
  gäller kompatibilitet då samba används för att ansluta till filer. NTFS
  tillhandahåller inte heller alla filsystemsfunktioner som behövs av alla
  program. Det rekommenderas att använda ett riktigt Unix-filsystem.

**Grundläggande krav**:
  Utvecklingsfilerna för X11 måste vara installerade (de kallas xorg-dev i
  Debian och libX11-devel i Red Hat).
  Du måste givetvis också ha make (mest troligt GNU make).
  Det är också nödvändigt att ha flex 2.5.33 eller senare samt bison.

**Valfria stödbibliotek**:
  configure-skriptet visar varningar när valfria bibliotek inte hittats.
  Se https://wiki.winehq.org/Recommended_Packages för information om vilka
  paket du bör installera. På 64-bitars system måste du säkerställa att du
  installerar 32-bitars versionerna av dessa bibliotek.


## KOMPILERING

Kör följande kommandon för att bygga Wine:

```
./configure
make
```

Detta bygger programmet "wine" och diverse stödbibliotek/programfiler.
Programfilen "wine" laddar och kör Windows-program.
Biblioteket "libwine" ("Winelib") kan användas till att bygga och länka
Windows-källkod i Unix.

Kör `./configure --help` för att se inställningar och val vid kompilering.

För mer information se https://wiki.winehq.org/Building_Wine


## INSTALLATION

När Wine är byggt kan du köra `make install` för att installera det;
detta installerar också man-sidorna och några fler nödvändiga filer.

Glöm inte att först avinstallera gamla Wine-versioner. Pröva antingen
`dpkg -r wine`, `rpm -e wine` eller `make uninstall` före installationen.

När Wine är installerat kan du använda inställningsprogrammet `winecfg`.
Se hjälpavdelningen på https://www.winehq.org/ för tips om inställningar.


## KÖRNING AV PROGRAM

När du använder Wine kan du uppge hela sökvägen till programfilen, eller
enbart ett filnamn.

Exempel: för att köra Notepad:

```
wine notepad               (använder sökvägen angiven i Wines
wine notepad.exe            konfigurationsfil för att finna filen)

wine c:\\windows\\notepad.exe  (användning av DOS-filnamnssyntax)

wine ~/.wine/drive_c/windows/notepad.exe  (användning av Unix-filvägar)

wine notepad.exe readme.txt  (köra program med parametrar)
```

Wine är inte perfekt, så det är möjligt att vissa program kraschar.
I så fall får du en kraschlogg som du bör bifoga till din rapport då du
rapporterar ett fel.


## MER INFORMATION

- **Internet**: Mycket information om Wine finns samlat på WineHQ på
           https://www.winehq.org/ : diverse guider, en programdatabas samt
           felspårning. Detta är antagligen det bästa stället att börja.

- **Frågor**: Frågor och svar om Wine finns samlade på https://www.winehq.org/FAQ

- **Wiki**: Wines Wiki finns på https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **E-postlistor**:
           Det finns flera e-postlistor för Wine-användare och -utvecklare; se
           https://www.winehq.org/forums för mer information.

- **Fel**: Rapportera fel till Wines Bugzilla på https://bugs.winehq.org
           Sök i Bugzilla-databasen för att se om problemet redan finns
           rapporterat innan du sänder en felrapport.

- **IRC**: Hjälp finns tillgänglig online på kanalen `#WineHQ` på
           irc.libera.chat.
