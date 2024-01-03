## INTRODUÇÃO

Wine é um programa que permite correr programas Microsoft Windows
(incluindo DOS, Windows 3.x e Win32 executáveis) em Unix.
Consiste num carregador (loader), que carrega e executa um binário
Microsoft Windows, e uma livraria (chamada Winelib) que implementa
Windows API calls usando o Unix ou X11 equivalentes. A livraria também
pode ser usada para "porting" Win32 código para executáveis do nativo Unix .

Wine é software grátis,autorizado segundo a GNU LGPL; ver o ficheiro
LICENÇA para os detalhes.

## ARRANQUE RÁPIDO

Sempre que se compila da source, é recomendado que se use o Wine
Installer para construir e instalar o Wine. Desde a primeira directoria
do Wine source (que contém este ficheiro),corre:

```
./configure
make
```

Corre os programas conforme as `wine program`. Para mais
informações e resoçução de problemas. Lê o resto deste ficheiro, a Wine
man page, e especialmente a abundância de informação  encontrada em
https://www.winehq.org.

## REQUERIMENTOS

Para compilar e correr o Wine, deves ter o seguinte:

- Linux version 2.0.36 ou superior
- FreeBSD 12.4 ou seguinte
- Solaris x86 9 ou seguinte
- NetBSD-current
- Mac OS X 10.8 ou seguinte

Como o Wine requer sequências kernel-level para poder correr, apenas
os sistemas operativos acima mencionados são suportados.
Outros sistemas operativos que suportam sequências kernel, talvez
venham a ser suportados no futuro.

**FreeBSD info**:
Mais informações encontra-se em:
ftp://ftp.freebsd.org/pub/FreeBSD/ports/ports/emulators/wine/

**Solaris info**:
Tu irás provavelmente ter que construir o Wine com as ferramentas GNU
(gcc, gas, etc.). Aviso: ao instalar gas *não* assegura que será usado
pelo gcc. Recompilar o gcc depois de instalar o gas ou symking cc, as e
ld para as ferramentas gnu é dito que é necessário.

**NetBSD info**:
Certifica-te que tens as opções USER_LDT, SYSVSHM, SYSVSEM, e SYSVMSG
ligadas ao teu kernel.

**File systems info**:
O Wine deverá correr na maioria dos file systems. Contudo, o Wine falha
no aranque se umsdos é usado para a directoria /tmp. Alguns problemas de
compatibilidades foram relatados estando a usar ficheiros acessados
através do Samba. Também, como o NTFS apenas pode ser usado com
segurança com acesso readonly por agora, nós não recomendamos o uso de
NTFS. Como os programas de Windows precisam de acesso write em quase tudo.
No caso de NTFS files, copia por cima para uma localização em que se
possa escrever.

**Requisitos básicos**:
Tu precisas de ter instalados os fucheiros de include do X11 development
(chamados xorg-dev em Debian e libX11-devel no RedHat).

**Requisitos para as ferramentas de construção**:
Em sistemas x86 o gcc >= 2.7.2 é requerido.
Versões anteriores à 2.7.2.3 poderão ter problemas quando certos
ficheiros são compilados com optimização, frequentemente devido a
problemas relacionados com as gestôes dos cabeçalhos dos ficheiros.
Actualmente o pgcc não trabalha com o Wine. A causa deste problema é
desconhecida.
Claro que também precisas fazer make (geralmente como o GNU make).
Tu também necessitas do flex versao 2.5 ou superior e o bison.
Se estiveres a usar RedHat ou Debian, instala os pacotes do flex e do bison.

**Bibliotecas de suporte opcionais**:
Se desejares suporte de impressão do CUPS, por favor instala os pacotes
cups e cups-devel.

## COMPILAÇÃO

No caso de escolheres não usar wineinstall, corre os seguintes comandos
para contruir o Wine:

```
./configure
make
```

Isto irá contruir o programa "wine" e numerosos suportes livraris/binarios.
O programa "wine" irá carregar e correr executaveis do Windows.
A livraria "libwine" ("winelib") pode ser usada para compilar e ligar
Windows source code sob o Unix.

Para ver as opções de compilação da configuração, faz `./configure -help`.

## SETUP

Uma vez o Wine contruido correctamente, tu podes entao fazer o `make
install`; isto irá instalar o wine executavel, o Wine man page, e alguns
outros ficheiros necessários.

Não esquecer de primeiro desinstalar qualquer previo conflito relativo a
instalação do Wine.
Tenta outro `dpkg -r wine` ou `rpm -e wine` ou `make uninstall` antes de
installar.

Ver https://www.winehq.org/support/ para informação sobre a
configuraçao.

## CORRER PROGRAMAS

Quando e invoca o Wine, tens que especificar o caminho (patch) complecto
do executavel, ou apenas o nome do ficheiro.

Por exemplo:

```
wine notepad			(usando o searchpatch para lozalizar o ficheiro)
wine notepad.exe

wine c:\\windows\\notepad.exe	(usando um nome de ficheiro DOS)

wine ~/.wine/drive_c/windows/notepad.exe  (usando um nome de ficheiro Unix)
```

Nota: o caminho do ficheiro também irá ser adicionado ao caminho(patch)
quando um nome complecto é fornecido na linha de comando.

O Wine ainda não está complecto.então poderão vários programas
quebrar(crash). Providenciamos-te bem para que o winedbg esteja
correctamente e de acordo com a documentation/debugger.sgml, sera-te
dado um detector de erros (debugger) para que possas investigar e
corrigir os problemas.
Para mais informação como em fazer isto ou aquilo, por favor lê o
ficheiro documentation/debugging.sgml.

Tu deves fazer um backup de todos os teus ficheiros importantes em que
destes acesso ao Wine, ou usa uma especial cópia deles.tem havido casos
de certos users que têm feito relatos de ficheiros corronpidos. Não
corrar o Explorer, por exemplo, se não tiveres um backup próprio, que
por vezes renomeia e estraga algumas directorias. Nem todos os MS apps
como o e.g. Messenger são seguros, ao correrem o Explorer de alguma
maneira. Este caso particular de corrupeçao (!$!$!$!$.pfr) podem ao
menos parcialmente podem ser corrigidos usando
http://home.nexgo.de/andi.mohr/download/decorrupt_explorer

## ARRANJAR MAIS INFORMAÇÃO

- **www**: Uma grande quantidade de informação acerca do Wine está disponivel
        pelo WineHQ em https://www.winehq.org/ : varios guias Wine, base de
        dados de aplicações, localizaçao de erros. Isto é provavelmente o
        melhor ponto de começo.

- **FAQ**: A Wine FAQ está localizada em https://www.winehq.org/FAQ

- **Wiki**: https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **Mailing Lists**:
	Há algumas mailing list para responsaveis pelo desenvolvimento Wine; ver em
	https://www.winehq.org/forums para mais informação.

- **IRC** Ajuda online está disponivel em `#WineHQ` on irc.libera.chat.
