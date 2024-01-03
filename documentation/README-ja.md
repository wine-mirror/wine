## はじめに

Wineは（DOS、Windows 3.x、Win32 や Win64 実行可能ファイルを含む）Microsoft
WindowsプログラムをUnix上で実行できるようにするプログラムです。
Microsoft Windowsバイナリを読み込んで実行するプログラムローダと、
UnixやX11同等物を使ってWindows APIの呼び出しを実装する（Winelibと呼ばれる）
ライブラリから成ります。ライブラリはWindowsのコードをネイティブな
Unix実行可能ファイルに移植するのにも使えます。

Wineはフリーソフトウェアで、GNU LGPLのもとでリリースされています。
詳細についてはLICENSEというファイルを参照してください。


## クイックスタート

Wineソースのトップレベルディレクトリ（このファイル[訳注:README.jaではなく
READMEというファイル]を含むディレクトリ）から、以下を実行してください:

```
./configure
make
```

それから次のいずれか、Wineをインストールするか:

```
make install
```

または、Wineをビルドディレクトリから直接実行してください:

```
./wine notepad
```

`wine program`のようにプログラムを実行してください。更なる情報や
問題解決については、このファイルの残りの部分、Wineのmanページや、
特に<https://www.winehq.org>で見つかる豊富な情報を読んでください。


## 要件

Wineをコンパイルし実行するには、以下のうち一つを持っていなければなりません:

- Linux バージョン2.0.36以上
- FreeBSD 12.4以降
- Solaris x86 9以降
- NetBSD-current
- Mac OS X 10.8以降

動作するためにWineにはカーネルレベルのスレッドのサポートが必要なので、
以上で触れたオペレーティングシステムだけがサポートされます。
カーネルスレッドをサポートする他のオペレーティングシステムは
将来サポートされるかもしれません。

**FreeBSD情報**:
  詳細については<https://wiki.freebsd.org/Wine>を参照してください。

**Solaris情報**:
  GNUツールチェーン（gcc、gasなど）でWineをビルドする必要がある可能性が
  最も高いでしょう。警告 : gccがgasを使うことが、gasをインストールすることに
  よって保証されるわけでは*ありません*。gasのインストール後にgccを
  再コンパイルするか、cc、asやldをgnuツールにシンボリックリンクすることが
  必要だと言われています。

**NetBSD情報**:
  USER_LDT、SYSVSHM、SYSVSEMやSYSVMSGオプションがカーネルで有効になっている
  かどうかを確認してください。

**Mac OS X情報**:
  Xcode/Xcode Command Line ToolsまたはApple cctoolsが必要です。Wineを
  コンパイルするための最小要件はclang 3.8とMacOSX10.10.sdkおよびmingw-w64
  v8の組み合わせです。MacOSX10.14.sdkまたはそれ以降はwine64だけをビルドでき
  ます。

**サポートされたファイルシステム**:
  Wineはほとんどのファイルシステム上で動作するはずです。Sambaを通して
  アクセスしたファイルを使っていくつかの互換性問題が報告されています。同様に、
  NTFSはいくつかのアプリケーションで必要なファイルシステム機能すべてを提供し
  ていません。ネイティブなUnixファイルシステムを使うことが推奨されます。

**基本的な要件**:
  X11開発includeファイルをインストールする必要があります。
  （Debianではxorg-devでRed HatではlibX11-develと呼ばれます。[訳注: 最近の
    ディストリビューションでは別のパッケージで置き換えられています]）
  もちろんmakeも必要です（大概はGNU make）。
  flexバージョン2.5.33以降とbisonも必要です。

**オプションのサポートライブラリ**:
  configureはオプションのライブラリがシステム上に見つからなかったときに通知を
  表示します。インストールすべきパッケージについてのヒントについては
  <https://wiki.winehq.org/Recommended_Packages>を参照してください。64ビットプ
  ラットフォームでは、これらライブラリの32ビットバージョンをインストールした
  ことをよく確認してください。


## コンパイル

Wineをビルドするには以下のコマンドを実行してください:

```
./configure
make
```

これによって"wine"というプログラムと多数のサポートライブラリやバイナリが
ビルドされます。"wine"というプログラムはWindows実行可能ファイルを読み込み
実行します。"libwine" ("Winelib") というライブラリはUnixのもとでWindowsの
ソースコードをコンパイルしリンクするのに使えます。

コンパイル設定オプションを見るには、`./configure --help`を行なってください。

更なる情報は<https://wiki.winehq.org/Building_Wine>を参照してください。


## 設定

いったんWineが正しくビルドされると、`make install`を行なえます。
これによりwine実行可能ファイルとライブラリ、Wine manページやいくつかの必要な
ファイルがインストールされます。

まず、衝突するあらゆる前のWineインストールをアンインストールするのを
忘れないでください。インストール前に`dpkg -r wine`または`rpm -e wine`
または`make uninstall`を試してください。

いったんインストールされると、`winecfg`設定ツールを実行できます。
設定のヒントについては<https://www.winehq.org/>におけるサポート領域を
参照してください。


## プログラムの実行

Wineを起動するとき、実行可能ファイルのパス全体またはファイル名のみを
指定できます。

例えば、メモ帳を実行するには:

```
wine notepad            （レジストリで指定された、ファイルを検索
wine notepad.exe          するための検索パスを使う）

wine c:\\windows\\notepad.exe  （DOSファイル名の文法を使う）

wine ~/.wine/drive_c/windows/notepad.exe  （Unixファイル名の文法を使う）

wine notepad.exe readme.txt    （パラメータを付けてプログラムを呼ぶ）
```

Wineは完璧ではないので、いくつかのプログラムはクラッシュするかもしれません。
そのような場合はクラッシュログを得られるでしょう。クラッシュログはバグを報告
するときにレポートに添付するべきです。


## 更なる情報の取得

- **WWW**: Wineについてのたくさんの情報が<https://www.winehq.org/>にある
        WineHQから入手できます。多様なWineガイド、アプリケーションデータベース、
	バグ追跡。これはおそらく最良の出発点です。

- **FAQ**: Wine FAQは<https://www.winehq.org/FAQ>にあります

- **Wiki**: Wine Wikiは<https://wiki.winehq.org>にあります

- **Gitlab**: <https://gitlab.winehq.org>

- **メーリングリスト**:
	Wineユーザと開発者のためのいくつかのメーリングリストがあります。
	詳細については<https://www.winehq.org/forums>を参照してください。

- **バグ**: <https://bugs.winehq.org>にあるWine Bugzillaでバグを報告してください。
        バグ報告を投稿する前に問題が既知や修正済みかどうかを調べるために
	bugzillaデータベースを検索してください。

- **IRC**: irc.libera.chat上のチャンネル`#WineHQ`でオンラインヘルプを利用できます。
