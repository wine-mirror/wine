## WSTĘP

Wine jest warstwą kompatybilności umożliwiającą uruchamianie aplikacji napisanych dla systemu Microsoft Windows (w tym programów DOS, Windows 3.x, Win32 oraz Win64) w systemach Unix. Składa się z programu ładującego, który wczytuje i uruchamia binaria Microsoft Windows, oraz z biblioteki (zwanej Winelib), implementującej wywołania Windows API przy użyciu ich odpowiedników w systemach Unix, X11 lub macOS. Biblioteka ta może być również wykorzystywana do portowania kodu Windows do natywnych plików wykonywalnych Unix.

Wine jest Wolnym Oprogramowaniem, rozpowszechnianym na licencji GNU LGPL. Szczegółowe informacje znajdują się w pliku LICENSE.


## QUICK START

W katalogu głównym kodu źródłowego Wine (zawierającym ten plik) wykonaj:

```
./configure
make
```

Programy uruchamia się poleceniem `wine program`. Dodatkowe informacje oraz sposoby rozwiązywania problemów znajdują się w dalszej części tego pliku, w stronach podręcznika Wine oraz w dokumentacji dostępnej na https://www.winehq.org.


## REQUIRED CONFIGURATION

Do kompilacji i uruchomienia Wine wymagany jest jeden z poniższych systemów operacyjnych:

- Linux 2.6.22 lub nowszy
- FreeBSD 12.4 lub nowszy
- Solaris x86 9 lub nowszy
- NetBSD-current
- macOS 10.12 lub nowszy

Ponieważ Wine wymaga obsługi wątków na poziomie jądra, obsługiwane są wyłącznie powyższe systemy. Inne systemy z obsługą wątków jądra mogą być wspierane w przyszłości.

**Informacje FreeBSD**:
  Wine zazwyczaj nie działa poprawnie na FreeBSD w wersjach wcześniejszych niż 8.0. Więcej informacji: https://wiki.freebsd.org/Wine

**Informacje Solaris**:
  Najczęściej konieczne jest użycie narzędzi GNU (gcc, gas itd.). Należy pamiętać, że instalacja gas nie gwarantuje jego użycia przez gcc. Może być wymagane ponowne skompilowanie gcc lub dowiązanie cc, as i ld do narzędzi GNU.

**Informacje NetBSD**:
  W jądrze muszą być włączone opcje USER_LDT, SYSVSHM, SYSVSEM oraz SYSVMSG.

**Informacje macOS**:
  Wymagane są Xcode lub narzędzia wiersza poleceń Xcode. Minimalne wymagania do kompilacji to clang 3.8 z MacOSX10.10.sdk oraz mingw-w64 v8. SDK MacOSX10.14 lub nowsze umożliwia budowę wyłącznie wine64.

**Obsługiwane systemy plików**:
  Wine powinno działać na większości systemów plików. Zgłaszano jednak problemy przy dostępie do plików przez Sambę. NTFS nie zapewnia wszystkich funkcji wymaganych przez niektóre aplikacje. Zalecany jest natywny system plików Unix.

**Wymagana konfiguracja podstawowa**:
  Muszą być zainstalowane nagłówki deweloperskie X11 (np. xorg-dev w Debianie, libX11-devel w Red Hat). Wymagany jest także program make (zwykle GNU make). Dodatkowo wymagane są flex (≥ 2.5.33) oraz bison.

**Biblioteki opcjonalne**:
  Skrypt configure wyświetla informacje o brakujących bibliotekach opcjonalnych. Wskazówki instalacyjne dostępne są na https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine. Na platformach 64-bitowych wymagane są również 32-bitowe wersje tych bibliotek.


## COMPILATION

Aby skompilować Wine, wykonaj:

```
./configure
make
```

Zostanie zbudowany program „wine” oraz biblioteki i narzędzia pomocnicze. Program „wine” ładuje i uruchamia pliki wykonywalne Windows. Biblioteka „libwine” (Winelib) może być używana do kompilowania i linkowania kodu Windows w systemach Unix.

Opcje kompilacji można wyświetlić poleceniem `./configure --help`.

Dodatkowe informacje: https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine


## INSTALLATION

Po poprawnym zbudowaniu Wine polecenie:

```
make install
```

instaluje program wine, powiązane biblioteki, strony man oraz inne wymagane pliki.

Przed instalacją należy usunąć wcześniejsze wersje Wine, które mogą powodować konflikty (np. `dpkg -r wine`, `rpm -e wine` lub `make uninstall`).

Po instalacji dostępne jest narzędzie konfiguracyjne `winecfg`. Wskazówki konfiguracyjne dostępne są w sekcji Support na https://www.winehq.org.


## RUNNING PROGRAMS

Podczas uruchamiania Wine można podać pełną ścieżkę do pliku wykonywalnego lub jedynie jego nazwę.

Przykład uruchomienia Notatnika:

```
wine notepad
wine notepad.exe
wine c:\\windows\\notepad.exe
wine ~/.wine/drive_c/windows/notepad.exe
wine notepad.exe readme.txt
```

Wine nie zapewnia pełnej kompatybilności i niektóre programy mogą ulegać awariom. W takim przypadku generowany jest log, który należy dołączyć do zgłoszenia błędu.


## ADDITIONAL INFORMATION

- **WWW**: https://www.winehq.org — dokumentacja, poradniki, baza aplikacji, system zgłoszeń błędów.
- **FAQ**: https://gitlab.winehq.org/wine/wine/-/wikis/FAQ
- **Wiki**: https://gitlab.winehq.org/wine/wine/-/wikis/
- **GitLab**: https://gitlab.winehq.org
- **Mailing lists / forums**: https://gitlab.winehq.org/wine/wine/-/wikis/Forums
- **Bug reports**: https://bugs.winehq.org (przed zgłoszeniem sprawdź istniejące raporty).
- **IRC**: kanał `#WineHQ` na irc.libera.chat.

