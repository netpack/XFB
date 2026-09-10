# XFB — radio automation, playout and library management.
#
# Written for Fedora, and here because a blind operator asked for it: Fedora is
# where the screen-reader work is happening, and a .deb was no use to them.
#
# Epoch is 1 and must never be lowered. RPM compares the digit runs of a
# version numerically, exactly as dpkg and pacman do, so 3.14159 outranks
# 3.1416 and every desk that installed the older number would be told it was
# already up to date. The epoch is what puts the ordering back under our
# control; see the same note in PKGBUILD and in the Debian control file.
Epoch:          1
Name:           xfb
Version:        %{xfb_version}
Release:        1%{?dist}
Summary:        Radio automation, playout and music library management

License:        GPL-3.0-or-later
URL:            https://github.com/netpack/XFB
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  pkgconf-pkg-config
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtwebengine-devel
BuildRequires:  qt6-qtwebchannel-devel
BuildRequires:  qt6-qtpositioning-devel
BuildRequires:  qt6-qtquick3d-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  speech-dispatcher-devel
BuildRequires:  at-spi2-core-devel
BuildRequires:  alsa-lib-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  sqlite-devel
BuildRequires:  libcurl-devel
BuildRequires:  mesa-libGL-devel
BuildRequires:  desktop-file-utils

# The shared libraries are picked up by RPM's own dependency generator, so only
# what it cannot see is named here.
#
# The SQLite driver is a plugin XFB loads at runtime rather than links against,
# and without it the library is simply always empty — a miserable way to find
# out. On Fedora that plugin lives in qt6-qtbase itself; it is mysql, odbc and
# postgresql that are split into subpackages, so there is no qt6-qtbase-sqlite
# to ask for, and asking for one makes the package refuse to install. Named
# anyway rather than left to arrive with libQt6Core: it is the reason the
# requirement exists, and a future split would otherwise take the library with
# it silently.
Requires:       qt6-qtbase%{?_isa}
Requires:       python3
Requires:       curl

# The accessibility bus. Qt speaks AT-SPI over D-Bus rather than linking
# against it, so RPM's dependency generator cannot see this one at all — and
# without it XFB starts up saying "AT-SPI is not available on this system" and
# a screen reader is told nothing whatsoever. That is the single thing this
# package exists to get right, so it is a hard requirement and not a weak one.
Requires:       at-spi2-core

# Everything XFB shells out to. None of it is needed to start, and each missing
# one costs a specific feature, so they are recommendations and not hard
# requirements: a desk that only plays its library needs none of them.
Recommends:     ffmpeg-free
Recommends:     yt-dlp
Recommends:     perl-Image-ExifTool
Recommends:     mediainfo
Recommends:     tor
Recommends:     speech-dispatcher

%description
XFB is a radio automation and playout system: two decks with crossfades and
performance FX, a scheduled programme, a music library with loudness and BPM
analysis, jingles and commercials, streaming to Icecast, and a companion app
for Android that plays a set off the desk.

It is built to be used without sight — the whole interface is reachable from
the keyboard and reported to a screen reader through AT-SPI and
speech-dispatcher.

%prep
%autosetup -n %{name}-%{version}

# LTO off. Fedora turns it on by default with fat objects, and a Qt +
# WebEngine C++ tree this size then needs several GB per compiler process;
# the build is killed part-way through player.cpp on any ordinary builder
# (and inside Docker, whatever the host has). Nothing here is hot enough for
# link-time optimisation to be worth that.
%global _lto_cflags %{nil}

# Two jobs, not one per core, for the same reason: each g++ over this tree
# peaks well past a gigabyte, and a builder with plenty of cores and ordinary
# memory is exactly where that goes wrong.
%global _smp_mflags -j2

%build
%cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
%cmake_build --target XFB

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/XFB.desktop || :

%files
%license LICENSE
%doc README.md
%{_bindir}/XFB
%{_datadir}/applications/XFB.desktop
%{_datadir}/pixmaps/xfb_icon.png
%{_datadir}/icons/hicolor/*/apps/xfb_icon.png
# share/xfb holds the server-sync script templates and, when the Android
# toolchain has staged one, the companion APK the desk hands out. The APK is
# an OPTIONAL install in CMake, so this has to be a whole-directory glob
# rather than a list — a build without it must still package.
%{_datadir}/xfb/

%changelog
* Thu Sep 10 2026 Netpack <info@netpack.pt> - 1:4.0-1
- Operator accounts: XFB asks who is at the desk, and every menu entry sits
  behind a permission that person's role has to hold.
- A production computer works on the station's own files over the share.
- Watched folders file what lands in them; a folder named after a category
  files its songs under it.
- The national music quota, marked on the library and counted off the as-run.
- Time signals on the hour, and What Is Scheduled to show what is booked.
- Themed icons, two more themes, and a rearranged Options window.
- Portuguese and French throughout.

* Wed Sep 02 2026 Netpack <info@netpack.pt> - 1:3.1423-1
- Cover art can be fetched for downloads that predate XFB keeping one.
- The desk tells a paired phone what companion app it has, on every answer.
- Requires at-spi2-core: Qt speaks AT-SPI over D-Bus rather than linking it,
  so nothing else would have pulled it in and a screen reader heard nothing.

* Mon Aug 31 2026 Netpack <info@netpack.pt> - 1:3.1422-1
- First RPM build, for Fedora. Asked for by an operator who moved to Fedora
  for its accessibility and could not use the .deb.
