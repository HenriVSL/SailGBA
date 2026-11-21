Name:       harbour-sailgba

Summary:    Game Boy Advance emulator for Sailfish OS
Version:    0.1.0
Release:    1
License:    MPL-2.0
URL:        https://github.com/HenriVSL/SailGBA
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  desktop-file-utils

%description
SailGBA plays Game Boy Advance, Game Boy and Game Boy Color games on
Sailfish OS. It is built on the mGBA emulator core and has a native Silica
interface with on-screen touch controls, save states, fast forward and a
game library.


%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build


%install
%qmake5_install

# The SDK's %%qmake5 sets QMAKE_STRIP=: and the SDK skips rpm's own stripping,
# so the binary would ship with symbols (rpmlint: unstripped-binary-or-object).
# For a debuggable build: sfdev build emu -- --define "sailgba_keep_symbols 1"
%if 0%{!?sailgba_keep_symbols:1}
%{__strip} %{buildroot}%{_bindir}/%{name}
%endif

# License texts next to the app (inih's BSD license and blip_buf's LGPL ask
# for them in binary distributions). %%{_sourcedir} is <source tree>/rpm in
# both in-tree and shadow builds.
install -m 644 %{_sourcedir}/../LICENSE %{_sourcedir}/../THIRD_PARTY_NOTICES.md \
    %{buildroot}%{_datadir}/%{name}/

desktop-file-install --delete-original         --dir %{buildroot}%{_datadir}/applications                %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
