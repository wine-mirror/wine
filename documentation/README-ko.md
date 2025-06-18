## 소개

Wine은 Microsoft Windows 프로그램(DOS, Windows 3.x, Win32 및 Win64 실행
파일 포함)을 유닉스 상에서 실행할 수 있게 해 주는 프로그램입니다.  Wine은
Microsoft Windows 바이너리를 로드하고 실행하는 프로그램 로더와 Unix, X11
또는 Mac API를 써서 Windows API를 구현한 라이브러리(Winelib)로 이루어져
있습니다.  이 라이브러리는 Windows 코드를 유닉스 실행 파일로 이식하는
용도로도 사용할 수 있습니다.

Wine은 자유 소프트웨어이며 GNU LGPL 하에 배포됩니다.  자세한 내용은
LICENSE 파일을 참조하십시오.


## 간편 설치 및 실행

Wine 소스의 최상위 디렉토리에서 다음 명령을 실행합니다.

```
./configure
make
```

그런 다음 Wine을 설치하거나

```
make install
```

또는 빌드 디렉터리에서 직접 Wine을 실행합니다.

```
./wine noetepad
```

프로그램을 실행할 때는 `wine 프로그램명` 처럼 합니다.  더 자세한 정보 및
문제 해결 방법은 이 파일의 나머지 부분과 Wine 맨페이지를 참조하십시오.
특히 https://www.winehq.org 사이트에 방대한 정보가 집약되어 있습니다.


## 빌드에 필요한 조건

Wine을 컴파일하고 실행하려면 다음 중 하나가 필요합니다.

- 리눅스 2.6.22 또는 그 이후
- FreeBSD 12.4 또는 그 이후
- Solaris x86 9 또는 그 이후
- NetBSD-current
- macOS 10.12 또는 그 이후

Wine을 실행하려면 커널 차원의 스레드 지원이 필요하기 때문에 위에서 언급한
운영 체제만 지원합니다.  커널 스레드를 지원하는 다른 운영 체제는 향후
지원될 수 있습니다.

**FreeBSD 정보**:
  더 자세한 정보는 https://wiki.freebsd.org/Wine 페이지를 참조하십시오.

**Solaris 정보**:
  거의 대부분의 경우 Wine을 빌드하려면 GNU 툴체인(gcc, gas 등)을 사용해야
  합니다.  경고: gas를 단순히 설치하기만 하면 gcc에서 사용되지 않을 수도
  있습니다.  gas를 설치한 다음 gcc를 재컴파일하거나 cc, as, ld 명령을 GNU
  바이너리로 심볼릭 링크할 필요가 있다고 합니다.

**NetBSD 정보**:
  USER_LDT, SYSVSHM, SYSVSEM, SYSVMSG 옵션이 커널에 포함되어야 합니다.

**Mac OS X 정보**:
  Xcode/Xcode 명령 줄 도구 또는 Apple cctools가 필요합니다.  Wine을
  컴파일하려면 최소한 MacOSX10.10.sdk 포함 clang 3.8 및 mingw-w64 v8 이
  필요합니다.  MacOSX10.14.sdk 이상에서는 wine64만 빌드할 수 있습니다.

**지원되는 파일 시스템**:
  Wine은 대부분의 파일 시스템에서 문제없이 실행됩니다.  Samba를 이용해서
  파일을 액세스하는 경우 몇 가지 호환성 문제가 보고되기도 했습니다.  또한,
  NTFS는 일부 프로그램에서 필요로 하는 파일 시스템 기능 전부를 제공하지는
  않습니다.  기본 Unix 파일 시스템을 사용하는 것이 좋습니다.

**기본 요구 사항**:
  X11 개발용 헤더 파일이 설치되어 있어야 합니다 (데비안에서는 xorg-dev,
  레드햇에서는 libX11-devel이란 패키지명으로 되어 있습니다).
  당연히 make 프로그램도 필요합니다 (대부분 GNU make 사용).
  flex 버전 2.5 이상과 bison이 필요합니다.

**선택적 지원 라이브러리**:
  configure는 빌드 시스템에서 찾을 수 없는 선택적 라이브러리를 표시해
  줍니다.  https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine 에서 설치해야 하는
  패키지에 대한 힌트를 얻을 수 있습니다.  64비트 플랫폼에서는 이러한
  라이브러리의 32비트 버전을 설치해야 합니다.


## 컴파일하기

Wine을 빌드하려면 다음 명령을 실행합니다.

```
./configure
make
```

이렇게 하면 "wine" 프로그램과 다수의 라이브러리/바이너리가 빌드됩니다.
"wine" 프로그램은 Windows 실행 파일을 로드하고 실행하는 데 사용됩니다.
"libwine" 라이브러리("Winelib")는 Windows 소스 코드를 유닉스 상에서
컴파일하고 링크하는 용도로 사용할 수 있습니다.

컴파일 설정 옵션을 보려면 `./configure --help` 명령을 사용합니다.

자세한 내용은 https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine 을 참조하십시오.


## 설정

Wine을 정상적으로 빌드한 다음에는 `make install` 명령으로 실행 파일과
라이브러리, Wine 맨페이지, 기타 필요한 파일을 시스템에 설치할 수 있습니다.

설치하기 전 이전 버전의 Wine을 제거해야 한다는 점을 잊지 마시기 바랍니다.
`dpkg -r wine` 또는 `rpm -e wine` 또는 `make uninstall` 등의 명령으로 이전
버전을 제거할 수 있습니다.

설치가 끝나면 `winecfg` 설정 도구를 실행할 수 있습니다.  설정 관련 힌트는
https://www.winehq.org/ 의 Support 페이지를 참조하십시오


## 프로그램 실행

Wine을 실행할 때 실행 파일의 전체 경로를 지정할 수도 있고 파일명만 지정할
수도 있습니다.

예를 들어 메모장을 실행하려면:

```
wine notepad                     (파일을 찾기 위해 레지스트리에
wine notepad.exe                  지정된 검색 경로를 이용)

wine c:\\windows\\notepad.exe                 (DOS 파일명 사용)

wine ~/.wine/drive_c/windows/notepad.exe   (유닉스 파일명 사용)

wine notepad.exe readme.txt      (프로그램에 매개변수를 넘겨줌)
```

Wine은 아직 완성 단계에 도달하지 못했기 때문에 프로그램이 제대로 실행되지
않을 수 있습니다.  프로그램이 죽었을 경우 버그 보고서에 첨부해야 하는 충돌
로그가 생성됩니다.


## 더 자세한 정보

- **WWW**: 여러 종류의 Wine 가이드, 애플리케이션 데이터베이스, 버그 추적 등
        Wine에 대한 많은 정보를 https://www.winehq.org/ 에 있는 WineHQ
        사이트에서 얻을 수 있습니다.  정보를 찾을 때 대개 이 사이트가
        최고의 출발점이 됩니다.

- **FAQ**: Wine FAQ는 https://gitlab.winehq.org/wine/wine/-/wikis/FAQ 에 있습니다.

- **위키**: Wine 위키는 https://gitlab.winehq.org/wine/wine/-/wikis/ 에 있습니다.

- **Gitlab**: https://gitlab.winehq.org

- **메일링 리스트**:
        Wine 사용자와 개발자를 위해 메일링 리스트가 운영되고 있습니다.  더
        자세한 정보는 https://gitlab.winehq.org/wine/wine/-/wikis/Forums 를 참조하기 바랍니다.

- **Bugs**: https://bugs.winehq.org 의 Wine Bugzilla에 버그를 보고하십시오.
        버그 보고서를 제출하기 전에 같은 문제가 이미 제출된 적이 있는지
        Bugzilla 데이터베이스를 검색해 보시기 바랍니다.

- **IRC**: irc.libera.chat 의 `#WineHQ` 채널에서 도움을 받을 수 있습니다.
