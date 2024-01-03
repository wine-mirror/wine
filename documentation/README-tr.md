## GİRİŞ

Wine Microsoft Windows uygulamalarını (DOS, Windows 3.x ve Win32
uygulamaları dahil) Unix uyumlu sistemler üzerinde çalıştırmanıza izin
veren bir programdır. Microsoft Windows çalıştırılabilir dosyalarını
yükleyip çalıştıran bir program yükleyici ve Windows API çağrılarını Unix
ve X11 eşdeğer çağrılarını kullanarak gerçekleyen Winelib isimli bir
kitaplıktan oluşur. Kitaplık ayrıca Win32 kodlarını doğal Unix
çalıştırılabilir dosyalarına çevirmek için kullanılabilir.

Wine, GNU LGPL altında dağıtılan özgür bir yazılımdır; ayrıntılar için
LICENSE dosyasına bakabilirsiniz.

## HIZLI BAŞLANGIÇ

Eğer kaynaktan derlemek istiyorsanız, Wine'yi derlemek ve kurmak için Wine
Kur'u kullanmanız önerilir. Wine kaynak kodu içerisinde en üst dizine
(README dosyasını içerir) geçiniz ve şu komutu veriniz:

```
./configure
make
```

Programları `wine program` şeklinde çalıştırabilirsiniz.
Daha ayrıntılı bilgi ve sorun çözümü için, bu dosyanın geri kalanını,
Wine kullanım kılavuzunu ve özellikle https://www.winehq.org sitesindeki
zengin bilgi arşivini tarayabilirsiniz.

## GEREKSİNİMLER

Wine'ı derlemek ve çalıştırmak için, aşağıdakilerden en az birine sahip
olmanız gerekir:

- Linux 2.0.36 veya sonrası
- FreeBSD 12.4 veya sonrası
- Solaris x86 9 veya sonrası
- NetBSD-current
- macOS 10.8 veya sonrası

Wine çalışmak için çekirdek düzeyinde iş parçacığı desteğine ihtiyaç
duyduğundan sadece yukarıda söz edilen işletim sistemleri
desteklenmektedir.
Çekirdek düzeyinde iş parçacıklarını destekleyen diğer işletim sistemleri
de gelecekte desteklenebilir.

**FreeBSD hakkında bilgi**:
  Ayrıntılı bilgi için https://wiki.freebsd.org/Wine adresini ziyaret
  ediniz.

**Solaris hakkında bilgi**:
  Wine'ı derlemek için muhtemelen GNU araçlarına (gcc, gas, vb.) ihtiyaç
  duyacaksınız. Uyarı: gas kurmuş olmak onun gcc tarafından kullanılacağını
  temin etmez. gas kurduktan sonra yeniden gcc derlemeniz veya cc, as ve ld
  sembolik bağlantılarını gnu araçlarına ayarlamanız gerekmektedir.

**NetBSD hakkında bilgi**:
  USER_LDT, SYSVSHM, SYSVSEM ve SYSVMSG seçeneklerinin çekirdeğinizde etkin
  olduğundan emin olunuz.

**macOS hakkında bilgi**:
  x86 üzerinde düzgün bir derleme yapabilmeniz için Xcode 2.4 veya daha yeni
  bir sürüm gereklidir.
  Mac sürücüsü için 10.6 veya daha üzeri gereklidir, sürücü 10.5'te derlenmez.

**Desteklenen dosya sistemleri**:
  Wine birçok dosya sisteminde çalışabilir. Bununla beraber, Samba üzerinden
  yapılan dosya erişimlerinde de birkaç uyumluluk sorunu bildirilmiştir.
  Ayrıca, NTFS bazı uygulamalar için gereken bütün dosya sistemi özelliklerini
  sağlamamaktadır. Unix için geliştirilen dosya sistemlerini kullanmanız önerilir.

**Temel gereksinimler**:
  X11 geliştirme dosyalarının kurulu olması gereklidir.
  (Debian'da xorg-dev, Red Hat'da libX11-devel adlı paketler).
  make komutuna kesinlikle ihtiyacınız vardır. (çoğunlukla GNU make)
  Ayrıca, flex 2.5.33 ve bison araçları da gereklidir.

**Seçeneğe bağlı destek kitaplıkları**:
  Yapılandırılırken, eğer seçeneğe bağlı paketler bulunamadıysa, bildirim
  alacaksınız. Yüklemeniz gereken paketler hakkında ipuçları için,
  https://wiki.winehq.org/Recommended_Packages adresini ziyaret ediniz.
  64 bit üzerinde, eğer Wine'yi 32 bit (varsayılan) olarak derleyecekseniz,
  bazı kitaplıklarin 32 bit sürümlerinin yüklü olduğundan emin olunuz;
  ayrıntılar için https://wiki.winehq.org/WineOn64bit adresini ziyaret
  ediniz. Eğer gerçek bir 64 bit Wine kurulumu (veya 32-64 bit karışık)
  isterseniz, bilgi için https://wiki.winehq.org/Wine64 adresini ziyaret
  ediniz.

## DERLEME

Eğer wineinstall kullanmamayı seçtiyseniz, Wine'yi derlemek için aşağıdaki
komutları çalıştırınız:

```
./configure
make
```

Bu, "wine" programını ve destek kitaplıklarını/ikili dosyalarını
derleyecektir.  "wine" programı Windows uygulamalarını yükleyecek ve
çalıştıracaktır.  "libwine" ("Winelib") kitaplığı Windows programlarının
kaynak kodunu Unix altında derlemek ve bağlamak için kullanılabilir.

Derleme yapılandırma seçeneklerini görmek için `./configure --help` komutunu giriniz.

## KURULUM

Wine düzgün bir şekilde derlendiğinde, `make install` komutunu
verebilirsiniz; bu komut Wine çalıştırılabilir dosyalarını, kullanım
kılavuzunu ve gerekli birkaç dosyayı kuracaktır.

Önce, çakışan eski bir Wine kurulumunu kaldırmayı unutmayınız. Kurmadan
önce `dpkg -r wine` veya `rpm -e wine` veya `pisi rm wine` ya da `make
uninstall` komutlarından birini deneyiniz.

Yüklemeden sonra, `winecfg` yapılandıma aracını
çalıştırabilirsiniz. Yapılandırma ipuçları için https://www.winehq.org/
adresinde Destek bölümüne bakınız.

## UYGULAMALARIN ÇALIŞTIRILMASI

Wine'yi çağırırken çalışabilir dosyanın tam yolunu verebilir veya sadece
bir dosya adı belirtebilirsiniz.

Örneğin: Not Defteri'ni çalıştırmak için:

```
wine notepad		   (dosyaları konumlandırmak için config
wine notepad.exe	    dosyasındaki Path arama yolunu kullanarak)

wine c:\\windows\\notepad.exe  (DOS dosya adı sözdizimi ile)

wine ~/.wine/drive_c/windows/notepad.exe (Unix dosya adı sözdizimi ile)

wine notepad.exe readme.txt
			   (programı parametre vererek çağırma)
```

Wine tamamen bitmiş değildir; bu nedenle bazı programlar çökebilir. Bir
sorun oluştuğunda, bir çökme günlük dosyası alacaksınız, bu dosyayı hata
bildirirken eklemelisiniz.

## DAHA FAZLA BİLGİYE ERİŞİM

- **WWW**: Wine hakkında geniş bilgiyi WineHQ sitesine https://www.winehq.org/
	adresinden ulaşarak edinebilirsiniz: çeşitli Wine kılavuzları,
	uygulama veritabanı, hata izleme gibi. Burası muhtemelen en iyi
	başlangıç noktasıdır.

- **SSS** Wine hakkında sıkça sorulan sorulara buradan ulaşabilirsiniz:
              https://www.winehq.org/FAQ

- **Wiki**: https://wiki.winehq.org

- **Gitlab**: https://gitlab.winehq.org

- **E-posta listeleri**:
	Wine geliştiricileri için birtakım e-posta listeleri bulunmaktadır.
	Daha fazla bilgi için https://www.winehq.org/forums adresine gidiniz.

- **Hatalar**:
	Karşılaştığınız hataları https://bugs.winehq.org adresinden Wine
	Bugzilla ile bildirebilirsiniz. Lütfen bir hata bildirmeden önce
	hatanın önceden girilip girilmediğini öğrenmek için Bugzilla'da
	arama yapınız.

- **IRC**: irc.libera.chat sunucusundan `#WineHQ` kanalı ile çevrimiçi yardım
	alabilirsiniz.
