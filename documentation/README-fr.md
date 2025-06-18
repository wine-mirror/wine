## INTRODUCTION

Wine est un programme qui permet d'exécuter les logiciels écrits pour
Microsoft Windows (comprenant les exécutables DOS, Windows 3.x, Win32 et
Win64) sur un système Unix. Il est composé d'un chargeur qui charge et
exécute un binaire Microsoft Windows, ainsi que d'une bibliothèque (appelée
Winelib) qui implémente les appels de l'API de Windows à l'aide de leurs
équivalents Unix, X11 ou Mac. Cette bibliothèque peut également être
utilisée pour porter du code Windows vers un exécutable Unix natif.

Wine est un logiciel libre, distribué sous GNU LGPL ; lisez le fichier
LICENSE pour plus de détails.


## DÉMARRAGE RAPIDE

Dans le dossier racine du code source de Wine (qui contient ce fichier),
lancez :

```
./configure
make
```

Pour exécuter un programme, tapez `wine programme`. Pour des informations
complémentaires et la résolutions de problèmes, lisez la suite de ce
fichier, les pages de manuel de Wine, et surtout les nombreuses
informations que vous trouverez sur https://www.winehq.org.


## CONFIGURATION NÉCESSAIRE

Pour compiler et exécuter Wine, vous devez disposer d'un des systèmes
d'exploitation suivants :

- Linux version 2.6.22 ou ultérieur
- FreeBSD 12.4 ou ultérieur
- Solaris x86 9 ou ultérieur
- NetBSD-current
- macOS 10.12 ou ultérieur

Étant donné que Wine nécessite une implémentation des « threads »
(processus légers) au niveau du noyau, seuls les systèmes d'exploitation
mentionnés ci-dessus sont supportés. D'autres systèmes d'exploitation
implémentant les threads noyau seront peut-être pris en charge dans le
futur.

**Informations FreeBSD** :
  Wine ne fonctionnera généralement pas bien avec les versions FreeBSD
  antérieures à 8.0.  Voyez https://wiki.freebsd.org/Wine pour plus
  d'informations.

**Informations Solaris** :
  Il est plus que probable que vous deviez construire Wine avec la chaîne
  d'outils GNU (gcc, gas, etc.). Attention : installer gas n'assure pas
  qu'il sera utilisé par gcc.  Recompiler gcc après l'installation de gas
  ou créer un lien symbolique de cc, as et ld vers les outils GNU
  correspondants semble nécessaire.

**Informations NetBSD** :
  Assurez-vous que les options USER_LDT, SYSVSHM, SYSVSEM et SYSVMSG sont
  activées dans votre noyau.

**Informations Mac OS X** :
  Xcode 2.4 ou ultérieur est nécessaire pour compiler Wine sous x86.
  Le pilote Mac requiert OS X 10.6 ou ultérieur et ne pourra être
  compilé sous 10.5.

**Systèmes de fichiers pris en charge** :
  Wine devrait fonctionner sur la plupart des systèmes de fichiers.
  Certains problèmes de compatibilité ont été rapportés lors de
  l'utilisation de fichiers accédés via Samba. De plus, NTFS ne fournit pas
  toutes les fonctionnalités de système de fichiers nécessaires pour
  certains applications. L'utilisation d'un système de fichiers Unix natif
  est recommandée.

**Configuration de base requise** :
  Les fichiers d'en-tête de X11 (appelés xorg-dev sous Debian et
  libX11-devel sous Red Hat) doivent être installés.
  Bien entendu, vous aurez besoin du programme « make » (très probablement
  GNU make).
  flex 2.5.33 ou ultérieur, ainsi que bison, sont également requis.

**Bibliothèques optionnelles** :
  « configure » affiche des messages quand des bibliothèques optionnelles
  ne sont pas détectées sur votre système.
  Consultez https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine
  (en anglais) pour des indications sur les paquets logiciels que vous
  devriez installer.  Sur les plates-formes 64 bits, si vous compilez
  Wine pour le mode 32 bits (mode par défaut), les versions 32 bits de
  ces bibliothèques doivent être installées.


## COMPILATION

Pour compiler Wine, lancez :

```
./configure
make
```

Cela va construire le programme « wine », ainsi que nombreux binaires et
bibliothèques de support.
Le programme « wine » charge et exécute les exécutables Windows.
La bibliothèque « libwine » (alias « Winelib ») peut être utilisée pour
compiler et lier du code source Windows sous Unix.

Pour voir les options de compilation, tapez `./configure --help`.

Pour plus d'information consultez https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine

## INSTALLATION

Une fois Wine construit correctement, `make install` installe
l'exécutable wine, les bibliothèques associées, les pages de manuel de
Wine et quelques autres fichiers nécessaires.

N'oubliez pas de désinstaller toutes les installations précédentes :
essayez `dpkg -r wine`, `rpm -e wine` ou `make uninstall` avant
d'installer une nouvelle version.

Une fois l'installation terminée, vous pouvez lancer l'outil de
configuration `winecfg`. Consultez la section Support de
https://www.winehq.org/ pour obtenir des astuces de configuration.


## EXÉCUTER DES PROGRAMMES

Au démarrage de Wine, vous pouvez spécifier le chemin entier vers
l'exécutable ou seulement le nom de fichier.

Par exemple pour exécuter le bloc-notes :

```
wine notepad               (en utilisant le chemin d'accès spécifié
wine notepad.exe            dans la base de registre pour localiser
                            le fichier)

wine c:\\windows\\notepad.exe
        (en utilisant la syntaxe de fichiers DOS)

wine ~/.wine/drive_c/windows/notepad.exe
        (en utilisant la syntaxe Unix)

wine notepad.exe lisezmoi.txt
        (en appelant le programme avec des paramètres)
```

Wine n'est pas parfait, et certains programmes peuvent donc planter. Si
cela se produit, vous obtiendrez un journal du crash qu'il est recommandé
d'attacher à un éventuel rapport de bogue.


## SOURCES D'INFORMATIONS COMPLÉMENTAIRES

- **WWW** :	Beaucoup d'informations à propos de Wine sont disponibles sur
        WineHQ (https://www.winehq.org) : divers guides, base de données
        d'applications, suivi de bogues. C'est probablement le meilleur
        point de départ.

- **FAQ** :   La Foire aux Questions de Wine se trouve sur
        https://gitlab.winehq.org/wine/wine/-/wikis/FAQ

- **Wiki** :  Le wiki Wine est situé sur https://gitlab.winehq.org/wine/wine/-/wikis/

- **Gitlab**: Le développement de Wine est hébergé sur https://gitlab.winehq.org

- **Listes de diffusion** :
        Il existe plusieurs listes de diffusion pour les utilisateurs et
        les développeurs Wine ; voyez https://gitlab.winehq.org/wine/wine/-/wikis/Forums pour de
        plus amples informations.

- **Bogues** :
        Signalez les bogues sur le Bugzilla Wine à https://bugs.winehq.org
        Merci de vérifier au préalable dans la base de données de bugzilla
        que le problème n'est pas déjà connu, ou corrigé.

- **IRC** :   L'aide en ligne est disponible via le canal `#WineHQ` sur
        irc.libera.chat.
