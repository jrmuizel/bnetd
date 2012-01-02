%define name bnetd
%define version 0.4.25pre3
%define release 5mdk

Summary: Emulates a Battle.net server for various Blizzard games
Url: http://www.bnetd.org/
Name: %{name}
Version: %{version}
Release: %{release}
Source0: http://www.bnetd.org/files/%{name}-%{version}.tar.bz2
Patch0: ftp://ftp.iconsult.com/pub/patches/bnetd-%{version}-diffs
License: GPL
Group: Networking/Other
BuildRoot: %{_tmppath}/%{name}-buildroot
BuildRequires: libpcap0-devel
Prefix: %{_prefix}
Packager: Hakan Tandogan <hakan@gurkensalat.com>
Prereq: chkconfig


%description
Bnetd is a program that will eventually emulate a Starcraft Battle.net
server.  It is distributed under the GPL so that others may contribute
to the development.  It currently runs on Unix and Unix like systems.
The code is based on the original bnetd-0.3 by Mark Baysinger which was
distributed on http://www.starhack.ml.org/ until that domain went away.
Mark's work also spawned several versions for MS-Windows, most notably
FSGS.


%package devel
Summary: Development-only tools for the Battle.Net server emulator
Group: Development/Other


%description devel
Bnpcap shows a developer-readable dump of Battle.net packets recorded
via tcpdump(1). This is useful to debug / reverse-engineer the protocol
used on Battle.Net


%prep

%setup
%patch -p1

%build
%serverbuild
cd src
export DEFINES="-DBNETD_VERSION=\\\"%{version}-%{release}\\\""
./configure \
    --prefix=/usr \
    --sysconfdir=/etc/bnetd \
    --localstatedir=/var/games/bnetd
make
cd bnpcap
make


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/bnetd
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
mkdir -p $RPM_BUILD_ROOT%{_initrddir}
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man5
mkdir -p $RPM_BUILD_ROOT/var/log/bnetd
mkdir -p $RPM_BUILD_ROOT/var/games/bnetd
mkdir -p $RPM_BUILD_ROOT/var/games/bnetd/bnmail
mkdir -p $RPM_BUILD_ROOT/var/games/bnetd/files
mkdir -p $RPM_BUILD_ROOT/var/games/bnetd/users
mkdir -p $RPM_BUILD_ROOT/var/games/bnetd/reports
mkdir -p $RPM_BUILD_ROOT/var/games/bnetd/chanlogs

cp ./sbin/bnetd    $RPM_BUILD_ROOT%{_sbindir}
cp ./sbin/bnproxy  $RPM_BUILD_ROOT%{_sbindir}
cp ./sbin/bntrackd $RPM_BUILD_ROOT%{_sbindir}

cp ./bin/bnchat   $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bnpass   $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bnftp    $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bnbot    $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bnstat   $RPM_BUILD_ROOT%{_bindir}

cp ./src/bnpcap/bnpcap $RPM_BUILD_ROOT%{_bindir}

cp ./bin/bnilist    $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bni2tga    $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bniextract $RPM_BUILD_ROOT%{_bindir}
cp ./bin/bnibuild   $RPM_BUILD_ROOT%{_bindir}
cp ./bin/tgainfo    $RPM_BUILD_ROOT%{_bindir}

cp man/*.1          $RPM_BUILD_ROOT%{_mandir}/man1
cp man/*.5          $RPM_BUILD_ROOT%{_mandir}/man5

cp ./files/*.pcx     $RPM_BUILD_ROOT/var/games/bnetd/files
cp ./files/icons.bni $RPM_BUILD_ROOT/var/games/bnetd/files
cp ./files/tos*.txt  $RPM_BUILD_ROOT/var/games/bnetd/files
cp ./files/*.ini     $RPM_BUILD_ROOT/var/games/bnetd/files

(cd ${RPM_BUILD_ROOT}/var/games/bnetd/files && ln -s tos_DEU.txt tos-unicode_DEU.txt)
(cd ${RPM_BUILD_ROOT}/var/games/bnetd/files && ln -s tos_USA.txt tos-unicode_USA.txt)

cp ./conf/ad.list            $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/autoupdate         $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bits_motd          $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bits_passwd        $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bnban              $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bnetd.conf.in      $RPM_BUILD_ROOT/etc/bnetd/bnetd.conf
cp ./conf/bnetd_default_user $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bnhelp             $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bnissue.txt        $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/bnmotd.txt         $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/channel.list       $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/gametrans          $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/news.txt           $RPM_BUILD_ROOT/etc/bnetd
cp ./conf/realm.list         $RPM_BUILD_ROOT/etc/bnetd

cp ./scripts/bnetd.init      $RPM_BUILD_ROOT%{_initrddir}/bnetd
cp ./scripts/bnetd.logrotate $RPM_BUILD_ROOT/etc/logrotate.d/bnetd


%clean
if [ ! "$RPM_BUILD_ROOT" = "/" ];
then
    rm -rf $RPM_BUILD_ROOT
fi


# After installation
%post
%_post_service bnetd


# Just before removing the package
%preun
%_preun_service bnetd



%files
%defattr(-,games,games,0755)
%doc %attr(-,root,root) CHANGELOG COPYING CREDITS INSTALL README TODO packaging/README.packaging docs/*
%doc %attr(-,root,root) %{_mandir}/man1/*
%doc %attr(-,root,root) %{_mandir}/man5/*
%attr(-,root,root) %{_sbindir}/*
%attr(-,root,root) %{_bindir}/*
%dir    /etc/bnetd
%config(noreplace) /etc/bnetd/*
%attr(-,root,root) %dir    %{_initrddir}
%attr(-,root,root) %config(noreplace) %{_initrddir}/bnetd
%attr(-,root,root) %dir    /etc/logrotate.d
%attr(-,root,root) %config(noreplace) /etc/logrotate.d/bnetd
%dir    /var/log/bnetd
%dir    /var/games/bnetd
%dir    /var/games/bnetd/bnmail
%dir    /var/games/bnetd/files
%config(noreplace) /var/games/bnetd/files/tos*.txt
%config(noreplace) /var/games/bnetd/files/bnserver-*.ini
        /var/games/bnetd/files/*.pcx
        /var/games/bnetd/files/*.bni
%dir    /var/games/bnetd/users
%dir    /var/games/bnetd/reports
%dir    /var/games/bnetd/chanlogs


%files devel
%doc %attr(-,root,root) %{_mandir}/man1/bnpcap.1*
%doc %attr(-,root,root) %{_bindir}/bnpcap



%changelog
* Thu Aug 20 2001 Hakan Tandogan <hakan@gurkensalat.com> snapshot-20010820mdk

- Small Summary change

* Thu Jul 26 2001 Hakan Tandogan <hakan@gurkensalat.com> snapshot-20010726mdk

- Added a copy of bnserver-D2DV.ini from europe.battle.net

* Thu Jul 11 2001 Hakan Tandogan <hakan@gurkensalat.com> snapshot-20010711mdk

- Split bnpcap into its own "devel" package

* Thu Jul 9 2001 Hakan Tandogan <hakan@gurkensalat.com> snapshot-20010709mdk

- Support for bnet internal "mail"

* Thu Jul 4 2001 Hakan Tandogan <hakan@gurkensalat.com> snapshot-20010704mdk

- Use more macros
- Use the serverbuild macro
- rpmlint'ify the spec

* Thu Jun 26 2001 Hakan Tandogan <hakan@gurkensalat.com>

- Don't forget the manpages ;-)
- Add "unicode" Terms Of Service for Diablo II
- FHS 2.2 wants games data in /var/games
- add/remove the service during installation
- The data and log directories belong to games.games

* Thu Jun 25 2001 Hakan Tandogan <hakan@gurkensalat.com>

- Compiling, installing & packaging 0.4.25pre3 finished.

* Thu Jun 21 2001 Hakan Tandogan <hakan@gurkensalat.com>

- Compiling, installing & packaging 0.4.25pre2 finished.

* Thu Jun 21 2001 Hakan Tandogan <hakan@gurkensalat.com>

- Compiling, installing & packaging 0.4.25pre1 finished.

* Thu Jun 21 2001 Hakan Tandogan <hakan@gurkensalat.com>

- Compiling, installing & packaging 0.4.24 finished.

* Thu Jun 21 2001 Hakan Tandogan <hakan@gurkensalat.com>

- Compiling, installing & packaging 0.4 finished.
- First spec file for Mandrake distribution.

# end of file
