## JOHDANTO

Wine on ohjelma, jonka avulla Windows-ohjelmia (mukaan luettuna DOS-,
Windows 3.x, Win32 ja Win64 -ohjelmat) voi ajaa Unix-järjestelmissä.
Wine koostuu ohjelmanlataajasta, joka lataa ja käynnistää Windowsin
ohjelmatiedostoja, sekä kirjastosta nimeltä Winelib, joka toteuttaa
Windowsin API-kutsuja niiden Unix- ja X11-vastineiden avulla. Kirjaston
avulla voidaan myös kääntää Windows-koodia natiiveiksi Unix-ohjelmiksi.

Wine on vapaa ohjelma, ja se on julkaistu GNU LGPL:n alaisena; lisätietoja
lisenssistä on englanniksi tiedostossa LICENSE.


## PIKAOPAS

Aja Winen lähdekoodin juurihakemistossa seuraavat komennot:

```
./configure
make
```

Sitten joko asenna Wine:

```
make install
```

Tai aja se käännöshakemistosta:

```
./wine notepad
```

Ohjelmat ajetaan komennolla `wine ohjelma`. Lisätietoja sekä apua ongelmien
ratkaisemiseen on jäljempänä tässä tiedostossa, Winen man-sivuilla sekä
ennen kaikkea Internetissä osoitteessa https://www.winehq.org/.


## JÄRJESTELMÄVAATIMUKSET

Winen kääntämiseen tarvitaan jokin seuraavista:

- Linuxin versio 2.6.22 tai uudempi
- FreeBSD 12.4 tai uudempi
- Solaris x86 9 tai uudempi
- NetBSD-current
- macOS 10.12 tai uudempi

Wine vaatii kerneliltä tuen säikeille. Tämän takia toistaiseksi vain yllä
mainittuja käyttöjärjestelmiä tuetaan; tulevaisuudessa saatetaan lisätä tuki
muillekin käyttöjärjestelmille, joissa on tarvittava tuki säikeille.

**Tietoa FreeBSD:lle**:
  Osoitteessa https://wiki.freebsd.org/Wine on lisätietoa.

**Tietoa Solarikselle**:
  Wine täytyy luultavasti kääntää GNU-työkaluilla (gcc, gas jne.).
  Varoitus: vaikka gas olisi asennettu, ei ole varmaa, että gcc käyttää sitä;
  voi olla tarpeen joko kääntää gcc uudestaan tai luoda symboliset linkit
  ohjelmista "cc", "as" ja "ld" vastaaviin GNU-työkaluihin.

**Tietoa NetBSD:lle**:
  USER_LDT, SYSVSHM, SYSVSEM ja SYSVMSG täytyy aktivoida kernelistä.

**Tietoa Mac OS X:lle**:
  Winen kääntämiseen tarvitaan Xcode Command Line Tools tai Apple cctools.
  Vähimmäisversiot ovat clang 3.8, MacOSX10.10.sdk ja mingw-w64 v8.
  MacOSX10.14.sdk ja myöhemmät sopivat vain wine64:n kääntämiseen.

**Tuetut tiedostojärjestelmät**:
  Wine toimii useimmilla tiedostojärjestelmillä, mutta Samban kanssa on
  ilmoitettu ilmenevän ongelmia. Myöskään NTFS ei tue kaikkia ominaisuuksia,
  joita jotkin ohjelmat vaativat. Natiivin Unix-tiedostojärjestelmän käyttö
  on suotavaa.

**Perusvaatimukset**:
  Koneella täytyy olla X11:n kehitystiedostot (Debianissa xorg-dev,
  Red Hatissa libX11-devel).
  Luonnollisesti myös make (yleensä GNU make) on tarpeen.
  Lisäksi tarvitaan flex 2.5.33 tai uudempi sekä bison.

**Valinnaisia tukikirjastoja**:
  configure-skripti näyttää varoituksia, kun valinnaisia kirjastoja puuttuu.
  Osoitteessa https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine on tietoa, mitkä
  paketit ovat hyödyksi. 64-bittisissä järjestelmissä täytyy varmistaa, että
  kirjastoista on asennettu 32-bittiset versiot.


## KÄÄNTÄMINEN

Winen voi kääntää seuraavilla komennoilla:

```
./configure
make
```

Tämä kääntää ohjelman "wine" sekä lukuisia tukikirjastoja ja -ohjelmia.
Ohjelma "wine" lataa ja käynnistää Windows-ohjelmia.
Kirjastoa "libwine" ("Winelib") voidaan käyttää Windows-lähdekoodin
kääntämiseen Unixissa.

Komento `./configure --help` näyttää asetuksia ja valintoja, joita
käännösprosessiin voi lisätä.

Lisätietoja on osoitteessa https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine

## ASENNUS

Kun Wine on käännetty, komento `make install` asentaa Winen sekä sen man-sivut
ja joitakin muita hyödyllisiä tiedostoja.

Ennen asennusta pitää muistaa poistaa aiemmat Winen versiot. Poistamista
voi yrittää komennolla `dpkg -r wine`, `rpm -e wine` tai `make uninstall`.

Kun Wine on asennettu, voidaan ajaa asetusohjelma `winecfg`. Sivustolla
https://www.winehq.org/ kohdassa Support on englanninkielisiä lisäohjeita.


## OHJELMIEN AJAMINEN

Winelle voi antaa joko ohjelmatiedoston koko polun tai pelkän nimen.

Esimerkiksi Notepad eli Muistio voitaisiin ajaa näin:

```
wine notepad               (ohjelma yritetään löytää Winen
wine notepad.exe            rekisterissä luetelluista paikoista)

wine c:\\windows\\notepad.exe  (kokonainen DOS-polku)

wine ~/.wine/drive_c/windows/notepad.exe  (kokonainen Unix-polku)

wine notepad.exe readme.txt  (ajetaan ohjelma parametrin kanssa)
```

Wine ei ole täydellinen, joten on mahdollista, että jotkin ohjelmat kaatuvat.
Siinä tapauksessa komentoriville tulostuu virheloki, joka on syytä liittää
mukaan, jos raportoi virheestä.


## LISÄTIETOJA

- **WWW**: Winestä on paljon tietoa WineHQ:ssa, https://www.winehq.org/.
	Oppaita, ohjelmatietokanta sekä Bugzilla vikojen listaamiseen.
	Täältä kannattaa yleensä aloittaa.

- **Kysymyksiä**:
	Sivulle https://gitlab.winehq.org/wine/wine/-/wikis/FAQ on koottu kysymyksiä ja vastauksia.

- **Wiki**: Wine Wiki on osoitteessa https://gitlab.winehq.org/wine/wine/-/wikis/

- **Gitlab**: Winen kehitykseen voi osallistua sivustolla https://gitlab.winehq.org

- **Postituslistat**:
	Winen käyttäjille ja kehittäjille on joitakin postituslistoja,
	sivulla https://gitlab.winehq.org/wine/wine/-/wikis/Forums kerrotaan niistä lisää.

- **Virheet**:
	Ilmoita virheistä Winen Bugzillaan, https://bugs.winehq.org/.
	Katso kuitenkin ensin Bugzilla-tietokannasta, onko samasta asiasta
	ilmoitettu jo aiemmin.

- **IRC**: Online-apua voi saada kanavalta `#WineHQ` palvelimella irc.libera.chat.
