## BEVEZETÉS

A Wine egy program amivel lehetõség nyílik a Microsoft Windows
programok futtatására (beleértve a  DOS, Windows 3.x és Win32
futtatható állományokat) Unix alatt.  Tartalmaz egy program betöltõt,
ami betölt és futtat egy Microsoft Windows binárist, és egy
függvénykönyvtárat (nevén Winelib), ami implementálja a Windows
API hívásokat azok Unix, vagy X11 megfelelõinek használatával.
Ez a függvénykönyvtár használható a Win32 kód natív Unix futtatható
állománnyá portlásához.

A Wine egy szabad szoftver, a GNU LGPL alatt kiadva; nézze meg a
LICENSE fájlt a részletekért.

## GYORS INDÍTÁS

Ha ön forrásból fordította, ajánlott a Wine telepítõ használata a
a Wine lefordításához és telepítéséhez.  A Wine forráskód szülõ-
könyvtárából (ami tartalmazza ezt a fájlt), futtassa:

```
./configure
make
```

A programok futtatása `wine program`.  A további információkhoz
és a probléma felvilágosításhoz olvassa el ennek a fájlnak a további részét,
a Wine man oldalát, és különösen gazdag információ található itt:
https://www.winehq.org.

## KÖVETELMÉNYEK

A Wine-nak a fordításához és futtatásához önnek szüksége lesz a következõkbõl
egynek:

- Linux 2.0.36-os, vagy feletti verzió
- FreeBSD 12.4, vagy késõbbi
- Solaris x86 9, vagy késõbbi
- NetBSD-current
- Mac OS X 10.8, vagy késõbbi

Mivel a Wine-nak kükséges kernelszintû futási szál támogatás a futtatáshoz, csak
a fent említett operációs rendszerek támogatottak.
Más operációs rendszerek, amik támogatják a kernel futási szálakat, támogatva
lesznek a jövõben.

**FreeBSD információ**:
  Több információ található a FreeBSD ports fában itt:
  <ftp://ftp.freebsd.org/pub/FreeBSD/ports/ports/emulators/wine/>.

**Solaris információ**:
  A Wine fordításához kell a GNU toolchain (gcc, gas, etc.).
  Figyelem : A gas telepítése *nemt* biztosíték, hogy a gcc fogja is
  használni. A gcc újrafordítása a gas telepítése után, vagy a cc
  szimbolikus linkelése, és ld-zése a gnu tools-hoz szükséges.

**NetBSD információ**:
  ellenõrizze, hogy a USER_LDT, SYSVSHM, SYSVSEM, és SYSVMSG opciók be vannak-e
  kapcsolva a kernelben.

**Támogatott fájlrendszerek**:
  A Wine fut a legtöbb fájlrendszeren. Habár a Wine nem fog elindulni, ha
  umsdos-t használunk a /tmp könyvtárban. Néhány kompatibilitási gondot
  is bejelentettek a Sambán keresztül elért fájlok esetében. Az NTFS-t
  lehet használni biztonságban írásvédett hozzáféréssel, de az NTFS ellen
  szól, hogy a Windows programoknak majdnem mindenhova írási jog kell.
  NTFS fájlok esetén másoljuk át õket egy írható helyre.

**Alap követelmények**:
  Önnek fel kell telepítenie az X11 fejlesztõi fájlokat
  (xorg-dev néven van a Debianban és libX11-devel néven a Red Hat-ben).

**Fordítási eszköz követelmények**:
  x86 rendszereken gcc >= 2.7.2 szükséges.
  A 2.7.2.3-nál régebbi verziókban problémák lehetnek különféle fájlokkal,
  amik optimalizációval lettek fordítva, gyakran a fejléc fájlok kezelésének
  problémái miatt. A pgcc jelenelg nem mûködik s Wine-sl. A probléma oka
  ismeretlen.
  Természetesen kell a make is (leginkább a GNU make).
  Kell még a flex 2.5 verzió, vagy késõbbi, és a bison.

**Opciónális támogatási függvénykönyvtárak**:
  Ha szeretne CUPS nyomtató támogatást, telepítse fel a cups és a cups-devel
  csomagot.
  Telepítse fel a libxml2 csomagot, ha szeretné hogy mûködjön az msxml
  implementáció.

## FORDÍTÁS

Ha ön nem használja a wineinstall-t, futtassa a következõ parancsokat s
Wine fordításához:

```
./configure
make
```

Ez le fogja fordítani a "wine" programot és számos függvénykönyvtárat/binárist.
A "wine" program be fogja tölteni és futtatni fogja a Windows futtatható
állományokat.
A "libwine" függvénykönyvtár ("Winelib") használható a Windows forráskód Unix
alatt történõ fordításához és linkeléséhez.

A fordítási konfigurációs opciók megtekinétéséhez nézze használja a `./configure --help`
parancsot.

## TELEPÍTÉS

Ha a Wine egyszer helyesen lefordult, használhatja a `make install`
parancsot; ez telepíteni fogja a wine futtatható fájlt, a Wine man
oldalát, és néhány egyéb szükséges fájlt.

Elõször ne felejtse eltávolítani bármely elõzõ Wine telepítést.
Próbálja ki a `dpkg -r wine`, és az `rpm -e wine`, vagy a `make uninstall`
parancsot telepítés elõtt.

Látogassa meg a támogatási oldalt itt: https://www.winehq.org/ a konfigurációs
tippekhez.

## PROGRAMOK FUTTATÁSA

Ha segítségül hívja a Wine-t, megadhatja a teljes útvonalát a futtatható
állománynak, vagy csak a fájlnevet.

Például:

```
wine notepad		   (a konfigfájlban megadott keresési útvonal
wine notepad.exe		    használatával keressük meg a fájlt)

wine c:\\windows\\notepad.exe  (a DOS fájlnév szintaxis használatával)

wine ~/.wine/drive_c/windows/notepad.exe  (a Unix-os fájlnév szintaxis használatával)

wine notepad.exe readme.txt          (program hívása paraméterekkel)
```

Felhívás: a fájl eléséi útja is hozzá lesz adva a path-hez, ha a teljes név
          meg lett adva a parancssorban.

## TÖBB INFORMÁCIÓ BESZERZÉSE

- **WWW**: A Wine-ról hatalmas mennyiségû információ érhetõ el a WineHQ-n ezen
	a címen: https://www.winehq.org/ : különbözõ Wine útmutatók,
	alkalmazás adatbázis, és hibakövetés.
	Ez talán a legjobb kiindulási pont.

- **GYIK**: A Wine GYIK itt található: https://www.winehq.org/FAQ

- **Wiki**: https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **Levelezõlisták**:
	Elérhetõ néhány levelezõlista a Wine fejlesztõk számára; nézze meg a
	https://www.winehq.org/forums címet a további információhoz.

- **Hibák**: Wine hibabejelentés a Bugzilla-n keresztül itt: https://bugs.winehq.org
	Kérem hibabejelentés küldése elõtt ellenõrizze, hogy az ön problémája
	már megtalálható-e az adatbázisban.  Küldhet hibabejelentéseket a
	comp.emulators.ms-windows.wine címre is.

- **IRC**: Azonnali segítség elérhetõ a `#WineHQ` csatornán a irc.libera.chat-en.
