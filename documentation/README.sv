1. INLEDNING

Wine är ett program som gör det möjligt att köra Windows-program
(inkluderat DOS, Windows 3.x och Win32) i Unix. Det består av en
programstartare som startar och kör Windows-programfiler, samt ett
bibliotek (kallat Winelib) som implementerar Windows API-anrop med hjälp
av deras Unix- eller X11-motsvarigheter. Biblioteket kan också användas
till att porta Win32-kod till vanliga Unix-program.

Wine är fri programvara, utgett under villkåren i GNU LGPL; se
filen LICENSE för detaljer.

2. KOM IGÅNG

När du bygger Wine från källkod så rekommenderas du använda Wines
installationsprogram. Kör följande i rotkatalogen för Wines källkod:

./tools/wineinstall

Kör program med "wine [val] program". Se resten av denna fil,
Wines man-sidor samt sist men inte minst http://www.winehq.org/ för mer
information och tips om hur problem kan lösas.


3. SYSTEMKRAV

För att kompilera och köra Wine krävs ett av följande:

  Linux version 2.0.36 eller senare
  FreeBSD 6.3 eller senare
  Solaris x86 9 eller senare
  NetBSD-current
  Mac OS X 10.4 eller senare

Wine kräver stöd för trådar på kernelnivå, och därför är det bara
operativsystemen ovan som stöds. Andra operativsystem som
stöder kerneltrådar kommer eventuellt att stödjas i framtiden.

Information för Linux
  Även om Linux 2.2.x antagligen fortfarande fungerar, och Linux 2.0.x kanske
  fungerar (tidiga 2.0.x-versioner uppvisade trådrelaterade problem), så är
  det bäst att ha en nuvarande kernel som 2.4.x eller 2.6.x.

Information för FreeBSD
  Wine kommer i regel inte fungera på FreeBSD-versioner äldre än 6.3 eller 7.0.
  FreeBSD 6.3 kan uppdateras for att stödja Wine. Se
  <http://wiki.freebsd.org/Wine> för mer information.

Information för Solaris
  Wine måste antagligen byggas med GNU toolchain (gcc, gas etc.).
  Varning: även om gas installeras så är det inte säkert att det används av
  gcc. Det sägs att det är nödvändigt att antingen bygga gcc på nytt, eller
  skapa symboliska länkar från "cc", "as" och "ld" till GNU toolchain.

Information för NetBSD
  USER_LDT, SYSVSHM, SYSVSEM och SYSVMSG måste vara aktiverade i kerneln.

Information för Mac OS X:
  Du behöver Xcode 2.4 eller senare för att korrekt kunna bygga Wine på x86.


Stödda filsystem
  Wine kan köra på de flesta filsystem, men det har rapporterats problem vad
  gäller kompatibilitet då samba används för att ansluta till filer. NTFS
  tillhandahåller inte heller alla filsystemsfunktioner som behövs av alla
  program. Det rekommenderas att använda ett Linux-filsystem som exempelvis
  ext3.

Grundläggande krav:
  Utvecklingsfilerna för X11 måste vara installerade (de kallas xlib6g-dev i
  Debian och XFree86-devel i Red Hat).

  Du måste givetvis också ha "make" (mest troligt "GNU make").

  Det är också nödvändigt att ha flex 2.5 eller senare samt bison.

Valfria stödbibliotek:
  configure-skriptet visar varningar när valfria bibliotek inte hittats.
  Se http://wiki.winehq.org/Recommended_Packages för information om
  vilka paket du bör installera.

  På 64 bit-system måste du säkerställa att 32 bit-versionerna av
  ovannämnda bibliotek installerats; se
  http://wiki.winehq.org/WineOn64bit för närmare detaljer.

4. KOMPILERING

Kör följande kommandon för att bygga Wine om du inte använder wineinstall:

./configure
make depend
make

Detta bygger programmet "wine" och diverse stödbibliotek/programfiler.
Programfilen "wine" laddar och kör Windows-program.
Biblioteket "libwine" ("Winelib") kan användas till att bygga och länka
Windows-källkod i Unix.

Kör './configure --help' för att se inställningar och val vid kompilering.

Gör följande för att uppgradera till en ny utgåva med hjälp av en
uppdateringsfil:
Gå in i utgåvans rotkatalog och kör kommandot "make clean".
Därefter uppdaterar du utgåvan med

    bunzip -c uppdateringsfil | patch -p1

där "uppdateringsfil" är namnet på uppdateringsfilen (något i stil med
wine-1.0.x.diff.bz2). Därefter kan du köra "./configure" och
"make depend && make".


5. INSTALLATION

När Wine är byggt kan du köra "make install" för att installera det;
detta installerar också man-sidorna och några fler nödvändiga filer.

Glöm inte att först avinstallera gamla Wine-versioner. Pröva antingen
"dpkg -r wine", "rpm -e wine" eller "make uninstall" före installationen.

När Wine är installerat kan du använda inställningsprogrammet "winecfg".
Se hjälpavdelningen på http://www.winehq.org/ för tips om inställningar.


6. KÖRNING AV PROGRAM

När du använder Wine kan du uppge hela sökvägen till programfilen, eller
enbart ett filnamn.

Exempel: för att köra Notepad:

        wine notepad               (använder sökvägen angiven i Wines
        wine notepad.exe            konfigurationsfil för att finna filen)

        wine c:\\windows\\notepad.exe  (användning av DOS-filnamnssyntax)

        wine ~/.wine/drive_c/windows/notepad.exe  (användning av Unix-filvägar)

        wine notepad.exe /parameter1 -parameter2 parameter3
                                   (köra program med parametrar)

Wine är inte ännu färdigutvecklat, så det är möjligt att åtskilliga program
kraschar. I så fall öppnas Wines felsökare, där du kan undersöka och fixa
problemet. Läs delen "debugging" i Wines utvecklarmanual för mer information
om hur detta kan göras.


7. MER INFORMATION

Internet:  Mycket information om Wine finns samlat på WineHQ på
           http://www.winehq.org/ : diverse guider, en programdatabas samt
           felspårning. Detta är antagligen det bästa stället att börja.

Frågor:    Frågor och svar om Wine finns samlade på http://www.winehq.org/FAQ

Usenet:    Du kan diskutera problem med Wine och få hjälp på
           comp.emulators.ms-windows.wine.

Fel:       Rapportera fel till Wines Bugzilla på http://bugs.winehq.org
           Sök i Bugzilla-databasen för att se om problemet redan finns
           rapporterat innan du sänder en felrapport. Du kan också rapportera
           fel till comp.emulators.ms-windows.wine.

IRC:       Hjälp finns tillgänglig online på kanalen #WineHQ på
           irc.freenode.net.

GIT:       Wines nuvarande utvecklingsversion finns tillgänglig genom GIT.
           Gå till http://www.winehq.org/git för mer information.

E-postlistor:
        Det finns flera e-postlistor för Wine-utvecklare; se
        http://www.winehq.org/forums för mer information.

Wiki:   Wines Wiki finns på http://wiki.winehq.org

Om du lägger till något eller fixar ett fel, är det bra om du sänder
en patch (i 'diff -u'-format) till listan wine-patches@winehq.org för
inkludering i nästa utgåva av Wine.

--
Originalet till denna fil skrevs av
Alexandre Julliard
julliard@winehq.org

Översatt till svenska av
Anders Jonsson
anders.jonsson@norsjonet.se
