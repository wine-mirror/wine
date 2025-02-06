## INTRODUÇÃO

Wine é um programa que permite executar programas Microsoft Windows
(incluindo executáveis DOS, Windows 3.x, Win32 e Win64) no Unix.
Constituído de um programa carregador (loader) que carrega e executa
um binário Microsoft Windows, e uma biblioteca (chamada Winelib) que
implementa chamadas da API do Windows usando os equivalentes do Unix
ou do X11. A biblioteca também pode ser usada para portar código
Win32 em executáveis nativos Unix.

Wine é software livre, liberado segundo a GNU LGPL; veja o arquivo
LICENSE para detalhes.


## INÍCIO RÁPIDO

Sempre que se compilam os fontes, é recomendado usar o Wine Installer
para construir e instalar o Wine. Estando no diretório de mais alto
nível do código fonte do Wine, execute:

```
./configure
make
```

Execute programas conforme `wine programa`. Para maiores informações
e resolução de problemas, leia o restante deste arquivo, a página
"man" do Wine (man wine), e especialmente a abundância de informação
encontrada em https://www.winehq.org.


## REQUERIMENTOS

Para compilar e executar o Wine, você deve ter o seguinte:

- Linux versão 2.6.22 ou posterior
- FreeBSD 12.4 ou posterior
- Solaris x86 9 ou posterior
- NetBSD-atual
- macOS 10.12 ou posterior

Como o Wine requer suporte a tarefas no nível de kernel para executar,
somente os sistemas operacionais acima mencionados são suportados.
Outros sistemas operacionais que suportarem tarefas do kernel poderão
ter suporte no futuro.

**Informações para o FreeBSD**:
  Veja https://wiki.freebsd.org/Wine para mais informações.

**Informações para o Solaris**:
  Você provavelmente necessitará construir o Wine com as ferramentas GNU
  (gcc, gas, etc.). Aviso: instalar gas NÃO assegura que será usado pelo
  gcc. Recompilar o gcc depois de instalar o gas ou criar uma ligação ao
  cc, as e ld para as ferramentas gnu é dito ser necessário.

**Informações para o NetBSD**:
  Certifique-se de ter as opções USER_LDT, SYSVSHM, SYSVSEM, e SYSVMSG
  ligadas no kernel.

**Informações para Mac OS X**:
  Será necessário o Xcode 2.4 ou superior para compilar corretamente no x86.
  O driver gráfico Mac requer OS X 10.6 ou superior e não será usado no 10.5.

**Sistemas de arquivo suportados**:
  O Wine deve rodar na maioria dos sistemas de arquivos. Alguns problemas de
  compatibilidade foram reportados quando usado via Samba. Além disso, o NTFS
  não provê todas as funcionalidades necessárias para alguns aplicativos.
  Usar uma partição nativa Unix é recomendado.

**Requisitos básicos**:
  Você necessita ter instalados os arquivos de inclusão para desenvolvimento
  do X11 (chamados de xorg-dev no Debian e libX11-devel no RedHat).
  Obviamente você também irá precisar do make (comumente o GNU make).
  Também será necessário o flex versão 2.5.33 ou superior e o bison.

**Bibliotecas de suporte opcionais**:
  O script configure irá mostrar diversas mensagens quando bibliotecas
  opcionais não forem encontradas no seu sistema.
  Veja https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine para
  dicas sobre pacotes que você pode instalar.
  Em plataformas de 64 bits, se compilar Wine como 32 bits (padrão), você
  precisa assegurar que as versões 32 bits das bibliotecas estão instaladas.


## COMPILAÇÃO

No caso de escolher não usar o wineinstall, execute os comandos a seguir
para construir o Wine:

```
./configure
make
```

Isto construirá o programa "wine" e vários binários/bibliotecas de suporte.
O programa "wine" carregará e executará executáveis do Windows.
A biblioteca "libwine" ("Winelib") pode ser usada para compilar e ligar
código-fonte do Windows sob o Unix.

Para ver as opções de compilação da configuração, rode `./configure --help`.


## CONFIGURAÇÃO

Uma vez que o Wine esteja construído corretamente, você pode executar
`make install`; assim irá instalar o executável do wine, as páginas
"man" do Wine, e outros arquivos necessários.

Não esqueça de desinstalar primeiramente qualquer instalação prévia do
Wine que possa ser conflitante. Tente tanto `dpkg -r wine`, `rpm -e wine`
ou `make uninstall` antes de instalar.

Depois de instalado, pode-se executar o programa de configuração `winecfg`.
Veja a área de suporte em https://www.winehq.org/ para dicas de configuração.


## EXECUTANDO PROGRAMAS

Ao invocar o Wine, você pode especificar o caminho completo do executável,
ou somente um nome de arquivo.

Por exemplo, para executar o bloco de notas:

```
wine notepad                    (usando o caminho de pesquisa como
wine notepad.exe                 especificado no registro para
                                 encontrar o arquivo)

wine c:\\windows\\notepad.exe   (usando um nome de arquivo DOS)

wine ~/.wine/drive_c/windows/notepad.exe   (usando um nome de arquivo Unix)

wine notepad.exe leiame.txt     (chamando o programa com parâmetros)
```

O Wine não é perfeito, então alguns programas podem travar. Se isso
acontecer você verá um registro do travamento (crash log) que você
poderá anexar ao bug que for criar.


## OBTENDO MAIS INFORMAÇÃO

- **WWW**: Uma grande quantidade de informação sobre o Wine está disponível
	no WineHQ em https://www.winehq.org/ : vários guias do Wine, base
	de dados de aplicações, rastreamento de erros. Este é provavelmente
	o melhor ponto para começar.

- **FAQ**: O FAQ (perguntas frequentes) do Wine está em https://gitlab.winehq.org/wine/wine/-/wikis/FAQ

- **Wiki**: O wiki do Wine está disponível em https://gitlab.winehq.org/wine/wine/-/wikis/

- **Gitlab**: https://gitlab.winehq.org

- **Listas de discussão**:
	Há diversas listas de mensagens para usuários e colaboradores no
	desenvolvimento do Wine; veja
	https://gitlab.winehq.org/wine/wine/-/wikis/Forums para mais
	informação.

- **Bugs**: Relate erros ao Bugzilla do Wine em https://bugs.winehq.org
	Por favor, pesquise a base de dados do bugzilla para verificar
	se seu problema já foi encontrado ou resolvido antes de enviar
	um relatório do erro.

- **IRC**: A ajuda online está disponível em `#WineHQ` em irc.libera.chat.
