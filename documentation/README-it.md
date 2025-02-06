## INTRODUZIONE

Wine è un programma che permette di eseguire programmi sviluppati per
Microsoft Windows (compresi eseguibili DOS, Windows 3.x, Win32, and
Win64) on Unix. Consiste di un caricatore di programmi che carica ed
esegue un binario Microsoft Windows, e di una libreria (chiamata Winelib)
che implementa le chiamate alle API Windows usando i loro equivalenti
Unix o X11. La libreria può essere usata anche per portare codice Windows
in eseguibili nativi Unix.

Wine è un software libero, rilasciato sotto la GNU LGPL; leggere il file
LICENSE per maggiori dettagli.


## QUICK START

Compilando da sorgente, si raccomanda di usare l'Installer di Wine per
compilare ed installare Wine. Dalla cartella principale del sorgente di
Wine, eseguire:

```
./configure
make
```

Eseguire i programmi com `wine programma`. Per maggiori informazioni e
risoluzioni di problemi, leggere il resto di questo file, la pagina man
di Wine, ed in modo particolare la notevole quantità di informazioni che
si trova all'indirizzo https://www.winehq.org.


## REQUISITI

Per compilare ed eseguire Wine, è necessario avere uno dei seguenti sistemi:

- Linux versione 2.6.22 o successiva
- FreeBSD 12.4 o successiva
- Solaris x86 9 o successiva
- NetBSD-current
- macOS 10.12 o successiva

Poiché Wine richiede il supporto dei thread a livello kernel per funzionare,
solo i sistemi operativi sopra mentionati sono supportati. Altri sistemi
che forniscono kernel threads potrebbero essere supportati in futuro.

**Informazioni per FreeBSD**:
  Leggere https://wiki.freebsd.org/Wine per maggiori informazioni.

**Informazioni per Solaris**:
  Sarà molto probabilmente necessario compilare Wine con i tool GNU
  (gcc, gas, etc). Attenzione: installare gas *non* assicura che
  sia usato da gcc. Sembra che sia necessario ricompilare gcc dopo
  l'installazione di gas o il symlink di cc, as e ld per i tool GNU.

**Informazioni per NetBSD**:
  Assicurarsi che le opzioni USER_LDT, SYSVSHM, SYSVSEM, e SYSVMSG siano
  abilitate nel kernel.

**Informazioni per Mac OS X**:
  È richiesto Xcode 2.4 o superiore per compilare correttamente su x86.

**File system supportati**:
  Wine dovrebbe funzionare sulla maggior parte dei file system. Qualche
  problema di compatibilità è stato riportato usando file acceduti
  tramite Samba. Inoltre, NTFS non fornisce tutte le funzionalità di
  file system necessarie per alcune applicazioni. Si raccomanda di usare
  un file system nativo di Unix.

**Requisiti basilari**:
  Devono essere installati i file include di sviluppo di X11
  (chiamato xorg-dev in Debian e libX11-devel in Red Hat).
  Ovviamente necessario anche make (possibilmente GNU make).
  È richiesto anche flex versione 2.5.33 o superiore e bison.

**Librerie opzionali di supporto**:
  Configure notificherà a video quando le librerie opzionali non sono
  trovate sul sistema. Leggere https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine
  per suggerimenti sui pacchetti da installare.
  Su piattaforme a 64-bit, per compilare Wine a 32-bit (predefinito),
  assicurarsi di installare la versione a 32-bit di queste librerie.


## COMPILAZIONE

Nel caso in cui si scegliesse di non utilizzare wineinstall, eseguire
i seguenti comandi per compilare Wine:

```
./configure
make
```

Questa sequenza compilerà il programma "wine" e numerose librerie e
binari di supporto.
Il programma "wine" caricherà ed eseguirà eseguibili Windows.
La libreria "libwine" ("Winelib") può essere usata per compilare e
linkare codice sorgente Windows sotto Unix.

Per vedere le opzioni di configurazione della compilazione, eseguire
`./configure -help`.

## SETUP

Una volta che Wine è stato compilato correttamente, puoi eseguire
`make install`; questo installerà l'eseguibile wine, le librerie, la
pagina man di Wine, e altri file necessari.

Non dimenticarti di disinstallare qualsiasi precedente installazione
di Wine in conflitto. Prova sia `dpkg -r wine` o `rpm -e wine` o
`make uninstall` prima di installare.

Una volta installato, esegui lo strumento di configurazione `winecfg`.
Leggi l'area Support su https://www.winehq.org/ per suggerimenti sulla
configurazione.


## ESEGUIRE PROGRAMMI

Quando si esegue Wine, si può specificare l'intero percorso
dell'eseguibile o solo il nome del file.

Per esempio, per eseguire Blocco Note:

```
wine notepad            (usando il percorso di ricerca come specificato
wine notepad.exe         nel registro per trovare il file)

wine c:\\windows\\notepad.exe      (usando la sintassi DOS)

wine ~/.wine/drive_c/windows/notepad.exe  (usando la sintassi Unix)

wine notepad.exe readme.txt     (chiamando il programma con dei parametri)
```

Wine non è perfetto, quindi alcuni programmi potrebbero andare in crash.
Se ciò accadesse, sarà creato un log del crash da aggiungere al rapporto
di segnalazione del problema.


## PER OTTENERE PIÙ INFORMAZIONI

- **WWW**: Un gran quantitativo di informazioni su Wine è disponibile al
	WineHQ, https://www.winehq.org/: varie guide su Wine, database
	delle applicazioni, rintracciamento di bug. Questo è probabilmente
	il miglior punto di partenza.

- **FAQ**: Le FAQ di Wine si trovano all'indirizzo https://gitlab.winehq.org/wine/wine/-/wikis/FAQ

- **Wiki**: Il Wiki di Wine si trova all'indirizzo https://gitlab.winehq.org/wine/wine/-/wikis/

- **Gitlab**: https://gitlab.winehq.org

- **Mailing list**:
	Esistono diverse mailing list per utenti e sviluppatori di Wine;
	visitare https://gitlab.winehq.org/wine/wine/-/wikis/Forums per
	ulteriori informazioni.

- **Bugs**: Segnalare i bug su Wine Bugzilla all'indirizzo https://bugs.winehq.org
	Si prega di controllare il database di Bugzilla per verificare che
	il problema non sia già conosciuto o risolto prima di creare un
	rapporto su di esso.

- **IRC**: Aiuto online disponibile nel canale `#WineHQ` su irc.libera.chat.
