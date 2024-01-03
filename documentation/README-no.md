## INTRODUKSJON

Wine er et program som gjør det mulig å kjøre Windows-programmer
(inkludert DOS, Windows 3.x, Win32 og Win64) i Unix. Det består av en
programstarter som starter og kjører en Windows-programfil, og et
bibliotek kalt «Winelib»; en uavhengig utgave av Windows' API som
bruker Unix- eller X11-funksjoner. Biblioteket kan også brukes til å putte
Windows-kode i vanlige Unix-programmer.

Wine er fri programvare, utgitt under vilkårene i GNU LGPL; se
filen «LICENSE» for detaljer.


## KOM I GANG

Det anbefales å bruke Wines installasjonsprogram for å bygge og
installere Wine når det bygges fra kildekode. Kjør følgende i
rotkatalogen til Wine-kildekoden

```
./configure
make
```

Kjør programmer som `wine program`. Se resten av denne filen,
Wines man-side og https://www.winehq.org/ for mer informasjon og
problemløsing.


## SYSTEMKRAV

Følgende kreves for å bygge og kjøre Wine:

- Linux versjon 2.0.36 eller nyere
- FreeBSD 12.4 eller nyere
- Solaris x86 9 eller nyere
- NetBSD-current
- Mac OS X 10.8 eller nyere

Wine krever støtte for tråder på kjernenivå, og derfor er det bare
operativsystemene ovenfor som støttes. Andre operativsystemer som
støtter kjernetråder støttes kanskje i framtiden.

**Informasjon for FreeBSD**:
  Wine vil som regel ikke virke på FreeBSD-versjoner eldre enn 8.0.
  Se <https://wiki.freebsd.org/Wine> for mer informasjon.

**Informsjon for Solaris**:
  Wine må antakelig bygges med GNU-verktøyene (gcc, gas etc.).
  Advarsel: selv om gas installeres er det ikke sikkert det brukes av
  gcc. Det sies at gcc må bygges på nytt, eller at symbolske
  koblinger for «cc», «as» og «ld» må legges til GNU-verktøyene.

**Informasjon for NetBSD**:
  USER_LDT, SYSVSHM, SYSVSEM og SYSVMSG må være aktivert i kjernen.

**Informasjon for Mac OS X**:
  Du må ha Xcode/Xcode Command Line Tools eller Apple cctools.
  Minimumskravet for å bygge Wine er clang 3.8 med MacOSX10.10.sdk og
  mingw-w64 v8. MacOSX10.14.sdk eller nyere kan bare bygge wine64.

**Støttede filsystemer**:
  Wine kan kjøre på de fleste filsystemer. Det har imidlertid vært
  rapportert om problemer med filtilgang gjennom Samba. Vi anbefaler
  ikke bruk av NTFS, siden dette ikke støtter funksjoner som noen
  programmer trenger. Det anbefales å bruke et ekte Unix-filsystem.

**Basiskrav**:
  Inkluderingsfilene for X11 må være installert (kalt «xorg-dev» i
  Debian og «libX11-devel» i RedHat).
  Du må selvfølgelig også ha «make», sannsynligvis «GNU make».
  flex 2.5.33 eller nyere og bison er også nødvendig.

**Valgfrie støttebiblioteker**:
  configure-skriptet viser meldinger når valgfrie biblioteker ikke blir
  funnet. Se https://wiki.winehq.org/Recommended_Packages for info om
  hvilke pakker du bør installere.
  På 64 bit-systemer trenger du 32 bit-versjoner av disse bibliotekene
  hvis du skal bygge Wine som et 32 bit-program (standard);
  se https://wiki.winehq.org/WineOn64bit for flere detaljer.
  Hvis du ønsker å bygge Wine som et 64 bit-program (eller et oppsett
  med blandet 32 bit og 64 bit) kan du lese mer på
  https://wiki.winehq.org/Wine64

## BYGGING

Kjør følgende kommandoer for å bygge Wine hvis du ikke bruker wineinstall:

```
./configure
make
```

Dette bygger programmet «wine» og diverse støttebiblioteker/programfiler.
Programfilen «wine» laster og kjører Windows-programmer.
Biblioteket «libwine» («Winelib») kan brukes til å bygge og koble
Windows-kildekode i Unix.

Kjør `./configure --help` for å se valg for bygging.

## INSTALLASJON

Når Wine er bygget kan du kjøre `make install` for å installere det;
dette installerer også man-siden og noen andre nødvendigheter.

Ikke glem å avinstallere tidligere Wine-versjoner først. Prøv enten
`dpkg -r wine`, `rpm -e wine` eller `make uninstall` før installsjonen.

Når Wine er installert kan du bruke oppsettsverktøyet `winecfg`.
Se støtteområdet på https://www.winehq.org/ for hint om oppsett.


## KJØRING AV PROGRAMMER

Når du bruker Wine kan du oppgi hele stien til programfilen, eller bare
et filnavn.

Eksempel: for å kjøre Notisblokk:

```
wine notepad                   (ved å bruke søkestien oppgitt i
wine notepad.exe                Wine-registeret for å finne filen)

wine c:\\windows\\notepad.exe  (bruk av DOS-filbaner)

wine ~/.wine/drive_c/windows/notepad.exe  (bruk av Unix-filbaner)

wine notepad.exe readme.txt  (kjøre programmer med parametere)
```

Wine er ikke helt ferdig ennå, så det er mulig at noen programmer klikker.
Hvis dette skjer vil Wine lage en krasjlogg som du bør vedlegge til rapporten
når du rapporterer en feil.


## MER INFORMASJON

- **Internett**: En god del informasjon om Wine finnes hos WineHQ på
           https://www.winehq.org/ : diverse veiledere, en programdatabase
           og feilsporing. Dette er antakelig det beste stedet å begynnne.

- **Svar**: Wines spørsmål og svar finnes på https://www.winehq.org/FAQ

- **Wiki**: Wines Wiki er tilgjengelig på https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **E-postlister**:
           Det finnes flere e-postlister for Wine-utviklere og -brukere;
           besøk https://www.winehq.org/forums for mer informasjon.

- **Feil**: Rapporter feil til Wines Bugzilla på https://bugs.winehq.org
           Søk i Bugzilla-databasen og se om probelmet allerede er funnet
           før du sender en feilrapport.

- **IRC**: Nødhjelp er tilgjengelig på kanalen `#WineHQ` på
           irc.libera.chat.
