%define pdver    0.38.4
%define pdpkgver 0.38-4
 
%define	desktop_vendor planetccrma
%define desktop_utils  %(test -x /usr/bin/desktop-file-install && echo "yes")

Summary:      Real-time patchable audio and multimedia processor.
Name:         pd
Version:      %{pdver}
Release:      1
License:      free for any use
Group:        Applications/Multimedia
Source:       pd-%{pdpkgver}.src.tar.gz
Source1:      pd-get-cvs
BuildRoot:    %{_tmppath}/%{name}-%{version}-root
Obsoletes:    pd-alsa0.9 pd-alsa0.5
Requires:     jack-audio-connection-kit >= 0.66.3
Packager:     Fernando Lopez-Lezcano
Vendor: Planet CCRMA
Distribution: Planet CCRMA

BuildRequires: tcl-devel, tk-devel, XFree86-devel
BuildRequires: alsa-lib-devel, jack-audio-connection-kit-devel

%description
Pd gives you a canvas for patching together modules that analyze, process,
and synthesize sounds, together with a rich palette of real-time control
and I/O possibilities.  Similar to Max (Cycling74) and JMAX (IRCAM).  A
related software package named Gem extends Pd's capabilities to include
graphical rendering.

%prep
%setup -q -n pd-%{pdpkgver}

%build
# remove all cvs directories
# /usr/bin/find ./ -type d -name CVS | xargs rm -r

# disable warnings as errors for now
/usr/bin/find ./ -name makefile\* -exec perl -p -i -e "s/ -Werror//g" {} \;
/usr/bin/find ./ -name configure\* -exec perl -p -i -e "s/ -Werror//g" {} \;

cd src
%configure --enable-alsa --enable-jack

%{__make} depend
%{__make}

%install
%{__rm} -rf %{buildroot}

cd src
%{__make} DESTDIR=%{buildroot} MANDIR=share/man install

# add include files (they are apparently needed to build externals)
%{__mkdir} -p %{buildroot}%{_libdir}/pd/include
%{__install} *.h %{buildroot}%{_libdir}/pd/include

# redhat menus
%{__cat} << EOF > %{desktop_vendor}-%{name}.desktop
[Desktop Entry]
Name=Pd
Comment=Real-time patchable audio and multimedia processor
Icon=
Exec=%{_bindir}/%{name}
Terminal=false
Type=Application
EOF

%if "%{desktop_utils}" == "yes"
  %{__mkdir} -p %{buildroot}%{_datadir}/applications
  desktop-file-install --vendor %{desktop_vendor} \
    --dir %{buildroot}%{_datadir}/applications    \
    --add-category X-Red-Hat-Base                 \
    --add-category Application                    \
    --add-category AudioVideo                     \
    %{desktop_vendor}-%{name}.desktop
%else
  %{__mkdir} -p %{buildroot}%{_sysconfdir}/X11/applnk/Multimedia
  %{__cp} %{desktop_vendor}-%{name}.desktop \
     %{buildroot}%{_sysconfdir}/X11/applnk/Multimedia/%{desktop_vendor}-%{name}.desktop
%endif

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root)

%doc *.txt
%{_bindir}/pd
%{_bindir}/pdsend
%{_bindir}/pdreceive
%{_libdir}/pd
%{_includedir}/m_pd.h
%{_mandir}/man1/pd.1*
%{_mandir}/man1/pdsend.1*
%{_mandir}/man1/pdreceive.1*
%if "%{desktop_utils}" == "yes"
%{_datadir}/applications/*%{name}.desktop
%else
%{_sysconfdir}/X11/applnk/Multimedia/%{desktop_vendor}-%{name}.desktop
%endif

%changelog

* Tue Jul  5 2005 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.37.4-1
- updated to Miller's 0.38-4
* Fri Feb 11 2005 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.37.2-1
- updated to Miller's 0.38-2
* Wed Dec 29 2004 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 
- spec file cleanup
* Fri Feb 20 2004 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.37.0-3.cvs
- updated to current cvs
* Fri Dec  5 2003 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.37.0-2.cvs
- enabled alsa
- added patche for building with old ALSA API (patch1)
- default audio api is now alsa (patch2)
* Thu Dec  4 2003 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.37.0-1.cvs
- cvs: 2003/12/04
* Wed Apr  2 2003 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.36.0-3.cvs
- rebuild for jack 0.66.3, added explicit requires for it
* Wed Jan 22 2003 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.36.0-1
- switched to using devel_0_36 branch on df cvs, includes jack and other nice
  things
- added files from the pure data documentation project, some objects do not
  load cleanly so don't include for now
* Tue Nov 12 2002 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.35.0-3
- menu in 7.2/7.3 was in wrong group
* Sun Nov 10 2002 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.35.0-2
- change jack alsa_pcm port names to match jack >= 0.40
- added explicit dependency for jack because of the port name change
- added redhat menus
* Mon Oct 14 2002 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.35.0-1.1
- fix extras/fiddle to compile under gcc 3.2 and redhat 8.0
* Sun Aug 11 2002 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.35.0-1
- update to 0.35
- make the alsa api the default if building for alsa, add a command line
  switch to be able to force the oss driver if starting the alsa enabled pd
- added Guenter Gieger's jack patch at:
  ftp://xdv.org/pub/gige/pd/
* Sun Jun 23 2002 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.34.4-2
- alsa 0.5 no longer supported, change names of packages
* Wed Nov 28 2001 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.34.4-1
- updated to 0.34-4
- added sound api to rpm name
- adjusted file list
- added "provides pd" so that other packages can depend on the pd name without
  the sound api (ie: they will match any of them)
* Wed Oct 17 2001 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.34.2-1
- updated to 0.34-2
- used pd's install make target
* Tue Apr 14 2001 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu>
- added %{prefix}, added %{_mandir} so that the man pages go into the 
  correct man directory for redhat
- added %{alsa} for automatic detection of the installed alsa library
- decoupled pd release (ie: PATCH2) from the rpm release
