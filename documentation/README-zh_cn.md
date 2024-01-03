## 简介

Wine 是能够使 Microsoft Windows 程序（包括 DOS、Windows 3.x、Win32
以及 Win64 可执行程序）在 Unix 上运行的应用程序。
它包含一个用来加载并执行 Microsoft Windows 二进制程序的程序加载器，
以及一个使用 Unix 或 X11 的等价功能实现 Windows API 调用的函数库
（名称为 Winelib）。这个库也可用来将 Windows 代码移植为原生 Unix
可执行程序。

Wine 是自由软件，在 GNU LGPL 协议下发布；请查看 LICENSE 文件以了解
详情。


## 快速开始

当您从源代码编译时，我们建议您使用 Wine Installer 来构建、安装 Wine。
从 Wine 源代码的顶层目录开始（即包含本文件的目录[译注：指顶层的 README
文件]），运行：

```
./configure
make
```

运行程序时，请使用`wine [程序名]`的语法。如需了解更多信息或解决遇到的
问题，您可以继续阅读本文件的剩余部分，或阅读 Wine 的手册页。
需特别指出的是，您可以在网站：https://www.winehq.org 上找到十分丰富
的信息。


## 系统需求

要想成功编译并运行 Wine，您必须使用以下列出的操作系统之一：

- Linux 2.0.36 或更新版本
- FreeBSD 12.4 或更新版本
- Solaris x86 9 或更新版本
- NetBSD-current
- Mac OS X 10.8 或更新版本

鉴于运行 Wine 需要内核级别的线程支持，仅以上提及的操作系统能被支持。
其它拥有内核线程的操作系统可能在未来得到支持。

**FreeBSD 信息**：
  请查看 https://wiki.freebsd.org/Wine 以了解更多信息。

**Solaris 信息**：
  您大部分情况下可能需要使用 GNU 工具链（gcc、gas 等）来构建 Wine。
  警告：安装 gas *不能* 确保它被 gcc 所使用。您可能需要在安装 gas
  之后重编译 gcc，或是将 cc、as 和 ld 符号链接至 GNU 版的对应工具。

**NetBSD 信息**：
  请确保您在内核中启用了 USER_LDT、SYSVSHM、SYSVSEM 和 SYSVMSG 选项。

**Mac OS X 信息**:
  您需要 Xcode/Xcode Command Line Tools 或者 Apple cctools。
  编译 Wine 的最低要求是 clang 3.8 以及 MacOSX10.10.sdk 和 mingw-w64 v8。
  MacOSX10.14.sdk 或者更新版本只能编译 wine64。

**支持的文件系统**：
  Wine 应当能在大多数文件系统上工作。有一些被报告的兼容性问题，
  主要与使用 Samba 访问文件有关。另外，NTFS 不能提供一些应用所
  需要的某些文件系统的特定功能。综上所述，建议您使用原生的 Unix
  文件系统。

**基本要求**：
  您需要安装 X11 开发用头文件（在 Debian 中可能被称为 xorg-dev
  ，在 Red Hat 中可能被称为 libX11-devel）。
  当然您同样需要“make”工具（一般应当为 GNU make）。
  您也需要 flex 的 2.5.33 版或更新版本，以及 bison 工具。

**可选的支持库**：
  当可选的函数库无法在您的系统中找到时，configure 会显示对应的提示信息。
  请查看 https://wiki.winehq.org/Recommended_Packages 以了解您应当安装的
  软件包的信息。
  在 64 位平台上，如果以 32 位的方式编译 Wine（这是默认情况），您必须确保
  对应函数库的 32 位版本已被安装；请查看 https://wiki.winehq.org/WineOn64bit
  以了解详情。如果您想安装一个真正的 64 位 Wine（或者一个混合 32 位 Wine 和
  64 位 Wine 的版本），请参阅 https://wiki.winehq.org/Wine64 以了解具体内容。

## 编译

如果您选择不使用 wineinstall 工具，请运行下列的命令以构建 Wine：

```
./configure
make
```

这样将会构建“wine”程序和大量的支持库/二进制文件。
“wine”程序可以加载并运行 Windows 可执行文件。
“libwine”（“Winelib”）函数库可被用来在 Unix 下编译并链接 Windows
程序的源码。

如需查看编译配置选项，请使用`./configure --help`。

## 安装

一旦 Wine 已被正确构建，您可以进行`make install`；该操作将安装 wine
可执行文件、函数库、Wine 手册页以及其它需要使用的文件。

不要忘记事先卸载先前任何安装过的、会引起冲突的 Wine 软件集。
请在安装前尝试`dpkg -r wine`、`rpm -e wine`或`make uninstall`。

一旦安装完成，您可以运行`winecfg`配置工具。请参考 https://www.winehq.org/
网站中的支持页面以了解配置技巧。


## 运行程序

当您调用 Wine 时，您可以指定指向可执行文件的整个路径，或是仅指定文件名
信息。

例如，如需运行 Notepad：

```
wine notepad            (使用注册表中指定的搜索路径来
wine notepad.exe         定位文件)

wine c:\\windows\\notepad.exe      (使用 DOS 文件名语法)

wine ~/.wine/drive_c/windows/notepad.exe  (使用 Unix 文件名语法)

wine notepad.exe readme.txt          (带参数调用程序)
```

Wine 不是十全十美的，所以某些程序可能会崩溃。如果这样的情况发生，
您将会得到一份崩溃日志。您应当在提交程序漏洞时附上这份日志。


## 获取更多信息

- **WWW**: 有关 Wine 的很多信息可以在 WineHQ 网站：https://www.winehq.org/
	上面找到。包括：各类 Wine 相关的向导、应用程序数据库（AppDB）、
	漏洞跟踪系统等等。这个网站可能是您最佳的起始之处。

- **FAQ**: Wine 常见问题位于 https://www.winehq.org/FAQ

- **维基**： Wine 维基位于 https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **邮件列表**：
	存在数个服务于 Wine 用户和开发者的邮件列表；
	请查看 https://www.winehq.org/forums 以了解更多信息。

- **Bugs**: 请向位于 https://bugs.winehq.org 的 Wine Bugzilla 提交 bug。
	请在提交漏洞报告前先在 bugzilla 数据库中进行搜索，检查
	先前是否已有类似的已知问题以避免重复，并跟踪这些问题的修复情况。

- **IRC**: 您可以在 irc.libera.chat 的 `#WineHQ` 频道获取到在线帮助。
