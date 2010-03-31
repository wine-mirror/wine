1. INTRODUCCIÓN

Wine es un programa que permite la ejecución de programas de Microsoft Windows
(incluyendo ejecutables de DOS, Windows 3.x y Win32) sobre Unix. Consiste en un
programa cargador que carga y ejecuta un binario de Microsoft Windows, y una
librería (llamada Winelib) que implementa las llamadas a la API de Windows
usando sus equivalentes Unix o X11. La librería puede también utilizarse para
portar código Win32 a ejecutables Unix nativos.

Wine es software libre, publicado bajo la licencia GNU LGPL; vea el fichero
LICENSE para los detalles.

2. INICIO RÁPIDO

Cuando compile desde el código fuente, se recomienda utilizar el Instalador de
Wine para contruir e instalar Wine. Desde el directorio superior del código de
Wine (el cual contiene este fichero), ejecute:

./tools/wineinstall

Ejecute programas con "wine [opciones] programa". Para más información y
resolución de problemas, lea el resto de este fichero, la página de manual de
Wine  y, específicamente, la numerosa información que se encuentra en 
http://www.winehq.org.

3. REQUISITOS

Para compilar y ejecutar Wine, deberá tener uno de los siguientes:

  Linux versión 2.0.36 o superior
  FreeBSD 5.3 o superior
  Solaris x86 2.5 o superior
  NetBSD-current

Ya que Wine requiere soporte de hilos a nivel de núcleo para ejecutarse, sólo
se soportan los sistemas operativos arriba mencionados.
Otros sistemas operativos que soportan hilos de núcleo pueden ser soportados en
el futuro.

Información de Linux:
  A pesar de que Linux 2.2.x debería funcionar todavía y Linux 2.0.x aún podría
  funcionar (versiones antiguas de 2.0.x tenían problemas relacionados con los
  hilos), es mejor tener un núcleo actual como los 2.4.x.

Información de FreeBSD:
  Wine debería construirse sobre FreeBSD 4.x y FreeBSD 5.x, pero versiones
  anteriores a FreeBSD 5.3 generalmente no funcionarán adecuadamente.

  Más información se puede encontrar en el árbol de portes de FreeBSD en
  <ftp://ftp.freebsd.org/pub/FreeBSD/ports/ports/emulators/wine/>.

Información de Solaris:
  Lo más probable es que necesite construir con el conjunto de herramientas GNU
  (gcc, gas, etc.). Advertencia: el instalar gas *no* asegura que sea utilizado
  por gcc. Se dice que recompilar gcc tras la instalación de gas o enlazar
  simbólicamente cc, as y ld a las herramientas gnu es necesario.

Información de NetBSD:
  Asegúrese de que posee las opciones USER_LDT, SYSVSHM, SYSVSEM, y SYSVMSG
  activadas en su núcleo.



Sistemas de ficheros soportados:
  Wine debería ejecutarse en la mayoría de los sistemas de ficheros. Sin
  embargo, Wine no conseguirá iniciarse si umsdos es utilizado para el
  directorio /tmp. Unos cuantos problemas de compatibilidad se han reportado
  también al utilizar ficheros accedidos a través de Samba. Además, como de
  momento NTFS sólo puede ser utilizado con seguridad con acceso de sólo
  lectura, recomendamos no utilizar NTFS, ya que los programas Windows
  necesitan acceso de escritura en casi cualquier sitio. En el caso de ficheros
  NTFS, cópielos a una localización escribible.

Requisitos básicos:
  Necesita tener los ficheros de inclusión de desarrollo de X11 instalados
  (llamados xlib6g-dev en Debian y XFree86-devel en RedHat).

Requisitos de herramienta de construcción:
  Sobre sistemas x86 se requiere gcc >= 2.7.2.
  Versiones más antiguas que la 2.7.2.3 pueden tener problemas cuando ciertos
  ficheros sean compilados con optimización, a menudo debido a problemas con el
  manejo de ficheros de cabecera. pgcc actualmente no funciona con Wine. La
  causa de este problema se desconoce.

  Por supuesto también necesita "make" (preferiblemente GNU make).

  También necesita flex versión 2.5 o superior y bison. Si está utilizando
  RedHat o Debian, instale los paquetes flex y bison.

Librerías de soporte adicionales:
  Si desea soporte de impresión CUPS, por favor instale los paquetes cups y
  cups-devel.

4. COMPILACIÓN

En el caso de que elija no utilizar wineinstall, ejecute los siguientes
comandos para construir Wine:

./configure
make depend
make

Esto construirá el programa "wine" y numerosas librerías/binarios de soporte.
El programa "wine" cargará y ejecutará ejecutables de Windows.
La librería "libwine" ("Winelib") puede utilizarse para compilar y enlazar
código fuente de Windows bajo Unix.

Para ver las opciones de configuración para la compilación, haga ./configure
--help.

Para actualizar a nueva versión usando un fichero de parches, primero haga cd
al directorio superior de la versión (el que contiene este fichero README).
Entonces haga un "make clean", y parchee la versión con:

    gunzip -c fichero-parche | patch -p1

donde "fichero-parche" es el nombre del fichero de parches (algo como
Wine-aammdd.diff.gz). Entonces puede volver a ejecutar "./configure", y luego
"make depend && make".

5. CONFIGURACIÓN

Una vez que Wine ha sido construido correctamente, puede hacer "make install";
esto instalará el ejecutable de wine, la página de manual de Wine, y otros
cuantos ficheros necesarios.

No olvide desinstalar antes cualquier instalación anterior de Wine conflictiva.
Intente "dpkg -r wine" o "rpm -e wine" o "make uninstall" antes de instalar.

Vea la zona de Soporte en http://www.winehq.org/ para consejos de
configuración.

En el caso de que tenga problemas de carga de librerías (p. ej. "Error while
loading shared libraries: libntdll.so"), asegúrese de añadir la ruta de las
librerías a /etc/ld.so.conf y ejecutar ldconfig como root.

6. EJECUTANDO PROGRAMAS

Cuando invoque Wine, puede especificar la ruta completa al ejecutable, o sólo
el nombre del fichero.

Por ejemplo: para ejecutar el Solitario:

  wine sol                   (usando la ruta de búsqueda indicada en el fichero
  wine sol.exe                de configuración para encontrar el fichero)

  wine c:\\windows\\sol.exe  (usando la sintaxis de nombre de fichero de DOS)

  wine /usr/windows/sol.exe  (usando la sintaxis de nombre de fichero de Unix)

  wine sol.exe /parametro1 -parametro2 parametro3
                             (llamando al programa con parámetros)

Nota: la ruta del fichero también se añadirá a la ruta cuando se proporcione un
      nombre completo en la línea de comandos.

Wine todavía no está completo, por lo que algunos programas pueden fallar. Si
configura winedbg correctamente de acuerdo con documentation/debugger.sgml,
entrará en un depurador para que pueda investigar y corregir el problema.
Para más información sobre cómo hacer esto, por favor lea el fichero
documentation/debugging.sgml.

Debería hacer copia de seguridad de todos sus ficheros importantes a los dé
acceso desde Wine, o utilizar una copia especial para Wine de ellos, ya que ha
habido algunos casos de usuarios reportando corrupción de ficheros. NO ejecute
Explorer, por lo tanto, si no posee una copia de seguridad adecuada, ya que
renombra/corrompe a veces algunos directorios. Tampoco otras aplicaciones MS
como p. ej. Messenger son seguras, ya que lanzan de algún modo Explorer. Esta
corrupción particular (!$!$!$!$.pfr) puede corregirse al menos parcialmente
utilizando http://home.nexgo.de/andi.mohr/download/decorrupt_explorer

7. OBTENIENDO MÁS INFORMACIÓN

WWW:    Una gran cantidad de información sobre Wine está disponible en WineHQ
        en http://www.winehq.org/ : varias guías de Wine, base de datos de
        aplicaciones, registro de bugs. Este es probablemente el mejor punto de
        partida.

FAQ:    La FAQ de Wine se encuentra en http://www.winehq.org/FAQ

Usenet: Puede discutir sobre temas relacionados con Wine y obtener ayuda en
        comp.emulators.ms-windows.wine.

Bugs:   Reporte bugs al Bugzilla de Wine en http://bugs.winehq.org
        Por favor, busque en la base de datos de bugzilla para comprobar si su
        problema ya se encuentra antes de enviar un informe de bug. Puede
        también enviar informes de bugs a comp.emulators.ms-windows.wine.

IRC:    Hay disponoble ayuda online en el canal #WineHQ de irc.freenode.net.

CVS:    El árbol actual de desarrollo de Wine está disponible a través de CVS.
        Vaya a http://www.winehq.org/cvs para más información.

Listas de correo:
        Hay varias listas de correo para desarrolladores de Wine; vea
        http://www.winehq.org/forums para más información.

Si añade algo, o corrige algún bug, por favor envíe un parche (en formato
'diff -u') a la lista wine-patches@winehq.org para su inclusión en la siguiente
versión.

--
Alexandre Julliard
julliard@winehq.org
