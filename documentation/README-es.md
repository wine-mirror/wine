## INTRODUCCIÓN

Wine es un programa que permite la ejecución de programas de Microsoft Windows
(incluyendo ejecutables de DOS, Windows 3.x y Win32) sobre Unix. Consiste en un
programa cargador que carga y ejecuta un binario de Microsoft Windows, y una
librería (llamada Winelib) que implementa las llamadas a la API de Windows
usando sus equivalentes Unix o X11. La librería puede también utilizarse para
portar código Win32 a ejecutables Unix nativos.

Wine es software libre, publicado bajo la licencia GNU LGPL; vea el fichero
LICENSE para los detalles.


## INICIO RÁPIDO

Cuando compile desde el código fuente, se recomienda utilizar el Instalador de
Wine para compilar e instalar Wine. Desde el directorio base del código de Wine
(el cual contiene este fichero), ejecute:

```
./configure
make
```

Ejecute aplicaciones con `wine programa`. Para más información y resolución de
problemas, continúe leyendo este archivo, la página man de Wine, o más
específicamente, la numerosa información que puede encontrar en
https://www.winehq.org.


## REQUISITOS

Para compilar y ejecutar Wine, deberá tener uno de los siguientes:

- Linux versión 2.0.36 o superior
- FreeBSD 12.4 o superior
- Solaris x86 9 o superior
- NetBSD-current
- Mac OS X 10.8 o superior

Ya que Wine requiere soporte de hilos de ejecución a nivel de núcleo para
ejecutarse, sólo se soportan los sistemas operativos ya mencionados.
Otros sistemas operativos que soporten hilos de ejecución a nivel de núcleo
podrían obtener soporte en el futuro.

**Información sobre FreeBSD**:
  Visite https://wiki.freebsd.org/Wine para más información.

**Información sobre Solaris**:
  Lo más probable es que necesite compilar con el conjunto de herramientas GNU
  (gcc, gas, etc.). Advertencia: instalar gas *no* asegura que sea utilizado
  por gcc. Un comentario ampliamente extendido es que es necesario recompilar
  gcc tras la instalación de gas o enlazar simbólicamente cc, as y ld a las
  herramientas gnu.

**Información de NetBSD**:
  Asegúrese de que tiene las opciones USER_LDT, SYSVSHM, SYSVSEM, y SYSVMSG
  activadas en el núcleo.

**Información de Mac OS X**:
  Necesitará Xcode 2.4 o posterior para compilar correctamente en x86.

**Sistemas de ficheros soportados**:
  Wine puede ejecutarse en la mayoría de los sistemas de ficheros. Sin embargo,
  se han reportado unos cuantos problemas de compatibilidad usando archivos
  accedidos a través de Samba. Además, NTFS no proporciona todas las
  funcionalidades necesitadas por algunas aplicaciones. Se recomienda el uso de
  un sistema de ficheros Unix.

**Requisitos básicos**:
  Necesitará tener instalados los archivos include X11 para desarrollo
  (llamados xorg-dev en Debian y libX11-devel en RedHat).
  Además necesitará make (preferiblemente GNU make).
  También necesitará flex versión 2.5.33 o posterior y bison.

**Librerías de soporte adicionales**:
  Configure mostrará advertencias cuando las librerías opcionales no se
  encuentren disponibles en su sistema.
  Visite https://wiki.winehq.org/Recommended_Packages para recomendaciones
  acerca de que paquetes debe instalar.
  En plataformas de 64-bit, si se está compilando Wine 32-bits (es la
  compilación por defecto), debe estar seguro de tener instaladas las librerías
  de desarrollo de 32-bits; visite https://wiki.winehq.org/WineOn64bit para más
  detalles. Si usted desea un entorno Wine 64-bits puro (o una mezcla de 32-bits
  y 64-bits), vaya a https://wiki.winehq.org/Wine64 para más detalles.


## COMPILACIÓN

En el caso de que elija no utilizar wineinstall, ejecute los siguientes
comandos para construir Wine:

```
./configure
make
```

Esto construirá el programa "wine" y numerosas librerías/binarios de soporte.
El programa "wine" cargará y ejecutará ejecutables de Windows.
La librería "libwine" ("Winelib") puede utilizarse para compilar y enlazar
código fuente de Windows bajo Unix.

Para ver las opciones de compilación, haga: `./configure --help`.


## CONFIGURACIÓN

Una vez que Wine se ha compilado correctamente, puede hacer `make install`;
esto instalará el ejecutable y librerías de wine, el manual de Wine, y el
resto de ficheros necesarios.

Recuerde desinstalar antes cualquier instalación anterior de Wine que pueda
crear conflictos. Realice un `dpkg -r wine` o `rpm -e wine` o `make uninstall`
antes de instalar.

Una vez instalado, puede ejecutar la herramienta de configuración `winecfg`.
Visite la zona de Soporte en https://www.winehq.org/ para consejos sobre la
configuración.


## EJECUTANDO PROGRAMAS

Cuando invoque Wine, puede especificar la ruta completa al ejecutable, o sólo
el nombre del fichero.

Por ejemplo:

```
wine notepad                   (usando la ruta de búsqueda indicada en el
wine notepad.exe                el registro para encontrar el archivo)

wine c:\\windows\\notepad.exe  (usando la sintaxis de DOS)

wine ~/.wine/drive_c/windows/notepad.exe  (usando la sintaxis de Unix)

wine notepad.exe readme.txt (ejecutando el programa con parámetros)
```

Nota: la ruta del fichero también se añadirá a la ruta cuando se proporcione un
      nombre completo en la línea de comandos.

Wine no es perfecto, algunos programas pueden fallar. Si esto le ocurre usted
recibirá un log de error que debe adjuntar en caso de reportar un fallo.


## OBTENIENDO MÁS INFORMACIÓN

- **WWW**: Una gran cantidad de información sobre Wine está disponible en WineHQ
        en https://www.winehq.org/ : varias guías de Wine, base de datos de
        aplicaciones, registro de bugs. Este es probablemente el mejor punto de
        partida.

- **FAQ**: Las preguntas frecuentes de Wine se encuentran en
        https://www.winehq.org/FAQ

- **Wiki**: https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **Listas de correo**:
        Hay varias listas de correo, tanto para usuarios como desarrolladores;
        Vaya a https://www.winehq.org/forums para más información.

- **Bugs**: Reporte fallos en el Bugzilla de Wine, https://bugs.winehq.org
        Por favor, antes de enviar un informe de fallo busque en la base de
        datos de bugzilla para comprobar si su problema es un fallo conocido
        o existe una solución.

- **IRC**: Se puede obtener ayuda online en el canal `#WineHQ` de irc.libera.chat.
