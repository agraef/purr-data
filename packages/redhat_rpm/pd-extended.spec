%define build_pdp 1
%define build_gem 1

%define cvsdate 06.07.23

Summary: Pd extended
Name: pd-extended
Version: 0.39.2
Release: 0.1.cvs.%{cvsdate}
License: GPL
Group: Applications/Multimedia
URL: http://pure-data.sourceforge.net/
Source0: pure-data-%{cvsdate}.tar.gz
# from ./pure-data/scripts/checkout-developer-layout.sh
Source1: checkout-developer-layout.sh
Source2: pd-externals-list.sh
Source3: pd-make-specfile.sh
Source4: pd-descriptions
Patch0: pd-extended-0.39.2-externalsMakefile.patch
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Packager: Fernando Lopez-Lezcano
Vendor: Planet CCRMA
Distribution: Planet CCRMA

# requires to install all of pd-extended
Requires: pd-boids = %{version}
Requires: pd-bsaylor = %{version}
Requires: pd-buildsrc = %{version}
Requires: pd-corelibs = %{version}
Requires: pd-creb = %{version}
Requires: pd-cxc = %{version}
Requires: pd-cyclone = %{version}
Requires: pd-ext13 = %{version}
Requires: pd-flib = %{version}
Requires: pd-freeverb = %{version}
Requires: pd-gem = %{version}
Requires: pd-ggee = %{version}
Requires: pd-gyre = %{version}
Requires: pd-hardware = %{version}
Requires: pd-iem_ambi = %{version}
Requires: pd-iem_bin_ambi = %{version}
Requires: pd-iemlib = %{version}
Requires: pd-iemmatrix = %{version}
Requires: pd-keyboardkeys = %{version}
Requires: pd-la-kitchen = %{version}
Requires: pd-list-abs = %{version}
Requires: pd-loaders = %{version}
Requires: pd-mapping = %{version}
Requires: pd-markex = %{version}
Requires: pd-maxlib = %{version}
Requires: pd-memento = %{version}
Requires: pd-mjlib = %{version}
Requires: pd-motex = %{version}
Requires: pd-nqpoly = %{version}
Requires: pd-nusmuk = %{version}
Requires: pd-oscx = %{version}
Requires: pd-parazit = %{version}
Requires: pd-pddp = %{version}
Requires: pd-pdogg = %{version}
Requires: pd-pdp = %{version}
Requires: pd-pidip = %{version}
Requires: pd-pixeltango = %{version}
Requires: pd-pmpd = %{version}
Requires: pd-purepd = %{version}
Requires: pd-rradical = %{version}
Requires: pd-sigpack = %{version}
Requires: pd-smlib = %{version}
Requires: pd-toxy = %{version}
Requires: pd-unauthorized = %{version}
Requires: pd-vbap = %{version}
Requires: pd-zexy = %{version}

BuildRequires: automake autoconf libtool python-devel pd
BuildRequires: alsa-lib-devel jack-audio-connection-kit-devel
BuildRequires: libogg-devel libvorbis-devel ladspa-devel speex-devel 
BuildRequires: libpng-devel tk-devel lame-devel imlib2-devel
%{?build_pdp:BuildRequires: libquicktime-devel gsl-devel}
%{?fc4:%{?build_pdp:BuildRequires: XFree86-devel fftw3-devel}}
%{?fc5:%{?build_pdp:BuildRequires: libXv-devel mesa-libGLU-devel libXext-devel fftw-devel}}

%{?build_gem:BuildRequires: ImageMagick-devel gltt-devel glut-devel libdv-devel ffmpeg-devel}
%{?build_gem:BuildRequires: libjpeg-devel libpng-devel libtiff-devel avifile-devel}
%{?build_gem:BuildRequires: freetype-devel libmpeg3}

# if we include curl it it tries to download this:
# http://iem.kug.ac.at/pd/externals-HOWTO/pd-externals-HOWTO.pdf
# BuildRequires: curl

%description
The Pd developer community have added some extensions to Pd, like
colored audio cords, GUI glitch prevention, and more. The pd-extended
distribtution includes these patches.

%package -n pd
Summary: Real-time patchable audio and multimedia processor.
Group: Applications/Multimedia

%description -n pd
Pd gives you a canvas for patching together modules that analyze, process,
and synthesize sounds, together with a rich palette of real-time control
and I/O possibilities.  Similar to Max (Cycling74) and JMAX (IRCAM).  A
related software package named Gem extends Pd's capabilities to include
graphical rendering.

# automatically created subpackages
# (see pd-make-specfile.sh)

%package -n pd-boids
Summary: 2D and 3D boids flocking algorithm
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-boids
Summary: 2D and 3D boids flocking algorithm

%package -n pd-bsaylor
Summary: signal objects
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-bsaylor
Summary: signal objects

%package -n pd-buildsrc
Summary: buildsrc
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-buildsrc
Summary: buildsrc

%package -n pd-corelibs
Summary: core libraries stripped out of Pd
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-corelibs
Summary: core libraries stripped out of Pd

%package -n pd-creb
Summary: creb
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-creb
Summary: creb

%package -n pd-cxc
Summary: cxc
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-cxc
Summary: cxc

%package -n pd-cyclone
Summary: a library for porting and running Max/MSP patches in Pd
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-cyclone
Summary: a library for porting and running Max/MSP patches in Pd

%package -n pd-ext13
Summary: ext13
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-ext13
Summary: ext13

%package -n pd-flib
Summary: library for feature extraction
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-flib
Summary: library for feature extraction

%package -n pd-freeverb
Summary: freeverb
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-freeverb
Summary: freeverb

%package -n pd-gem
Summary: gem
Group: Applications/Multimedia
Requires: pd = %{version}
Requires: avifile

%description -n pd-gem
Summary: gem

%package -n pd-ggee
Summary: ggee
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-ggee
Summary: ggee

%package -n pd-gyre
Summary: gyre
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-gyre
Summary: gyre

%package -n pd-hardware
Summary: objects for working with hardware sensor boxes
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-hardware
Summary: objects for working with hardware sensor boxes

%package -n pd-iem_ambi
Summary: calculate ambisonic encoder matrices rotation matrices and decoder matrices from 1st to 4th order in 2D or 3D.
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-iem_ambi
Summary: calculate ambisonic encoder matrices rotation matrices and decoder matrices from 1st to 4th order in 2D or 3D.

%package -n pd-iem_bin_ambi
Summary: calculate the product of an ambisonic decoder-matrix and the binaural HRIR's (in frequency and in time domain)
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-iem_bin_ambi
Summary: calculate the product of an ambisonic decoder-matrix and the binaural HRIR's (in frequency and in time domain)

%package -n pd-iemlib
Summary: a collection of objects written at IEM/KUG
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-iemlib
Summary: a collection of objects written at IEM/KUG

%package -n pd-iemmatrix
Summary: objects for matrix operations and math
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-iemmatrix
Summary: objects for matrix operations and math

%package -n pd-keyboardkeys
Summary: objects for using keyboard keys for scrolling and selecting
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-keyboardkeys
Summary: objects for using keyboard keys for scrolling and selecting

%package -n pd-la-kitchen
Summary: a collection of objects working with sensors
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-la-kitchen
Summary: a collection of objects working with sensors

%package -n pd-list-abs
Summary: a collection of objects for manipulating lists. Requires pd>
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-list-abs
Summary: a collection of objects for manipulating lists. Requires pd>

%package -n pd-loaders
Summary: loaders
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-loaders
Summary: loaders

%package -n pd-mapping
Summary: objects for mapping data to control
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-mapping
Summary: objects for mapping data to control

%package -n pd-markex
Summary: markex
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-markex
Summary: markex

%package -n pd-maxlib
Summary: maxlib
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-maxlib
Summary: maxlib

%package -n pd-memento
Summary: a collection of objects for managing state saving
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-memento
Summary: a collection of objects for managing state saving

%package -n pd-mjlib
Summary: mjlib
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-mjlib
Summary: mjlib

%package -n pd-motex
Summary: motex
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-motex
Summary: motex

%package -n pd-nqpoly
Summary: nqpoly
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-nqpoly
Summary: nqpoly

%package -n pd-nusmuk
Summary: a collection of objects for physical modelling
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-nusmuk
Summary: a collection of objects for physical modelling

%package -n pd-oscx
Summary: objects for working with OpenSoundControl
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-oscx
Summary: objects for working with OpenSoundControl

%package -n pd-parazit
Summary: parazit
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-parazit
Summary: parazit

%package -n pd-pddp
Summary: support objects for the Pure Data Documentation Project
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-pddp
Summary: support objects for the Pure Data Documentation Project

%package -n pd-pdogg
Summary: objects for reading, writing, and streaming ogg
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-pdogg
Summary: objects for reading, writing, and streaming ogg

%package -n pd-pdp
Summary: pdp
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-pdp
Summary: pdp

%package -n pd-pidip
Summary: PiDiP is Definitely in Pieces
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-pidip
Summary: PiDiP is Definitely in Pieces

%package -n pd-pixeltango
Summary: objects for creating visuals in a live performance setting
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-pixeltango
Summary: objects for creating visuals in a live performance setting

%package -n pd-pmpd
Summary: Physical Modelling for Pd
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-pmpd
Summary: Physical Modelling for Pd

%package -n pd-purepd
Summary: existing objects reimplemented in Pd
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-purepd
Summary: existing objects reimplemented in Pd

%package -n pd-rradical
Summary: rradical
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-rradical
Summary: rradical

%package -n pd-sigpack
Summary: sigpack
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-sigpack
Summary: sigpack

%package -n pd-smlib
Summary: vector processing, vector analysis, vector synthesis, number stream analysis, number stream filters
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-smlib
Summary: vector processing, vector analysis, vector synthesis, number stream analysis, number stream filters

%package -n pd-toxy
Summary: objects for working with Tcl and Pd's Tk GUI
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-toxy
Summary: objects for working with Tcl and Pd's Tk GUI

%package -n pd-unauthorized
Summary: GUI and streaming objects
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-unauthorized
Summary: GUI and streaming objects

%package -n pd-vbap
Summary: Vector Based Amplitude Panning
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-vbap
Summary: Vector Based Amplitude Panning

%package -n pd-zexy
Summary: GUI and streaming objects
Group: Applications/Multimedia
Requires: pd = %{version}

%description -n pd-zexy
Summary: GUI and streaming objects

# end automatically created

%prep
%setup -q -n pure-data
%patch0 -p1

%install
%{__rm} -rf %{buildroot}
# remember base dir
buildhome=`pwd`
# remove all CVS directories
# find . -name CVS -exec rm -rf {} \;

# OSCx: had to remove "-lpd" from Makefiles
find externals/OSCx -name Makefile\* -exec perl -p -i -e "s|-lpd||g" {} \; 

%if 0%{?build_pdp}
# fix quicktime includes in pdp and pidip
find externals/pdp externals/pidip -type f -exec perl -p -i -e "s|<quicktime/|<lqt/|g" {} \; 
%else
# remove pdp from Makefile if not building it
perl -p -i -e "s|pdp pidip||g" externals/Makefile
%endif

# patch the official pd source
cd packages
make patch_pd

# make and install pd and externals
cd linux_make
CC=gcc %{__make} \
    OPT_CFLAGS="-O3 -funroll-loops -fomit-frame-pointer $RPM_OPT_FLAGS" \
    install prefix=%{_prefix} \
    DESTDIR=%{buildroot} install

# move pd man pages to proper destination
%{__mkdir} -p %{buildroot}%{_mandir}/man1/
%{__mv} %{buildroot}%{_prefix}/man/man1/* %{buildroot}%{_mandir}/man1/

#### build Gem
cd ${buildhome}/Gem

# make auxilliary libraries
(cd ${buildhome}/GemLibs/; ./makeauxlibs)

# make gem
cd src
%configure --with-glut --with-mpeg3 --with-quicktime

%{__make} 
%{__make} DESTDIR=%{buildroot} install

#### create lists of files installed for each subpackage
cd ${buildhome}

# read in all externals and abstractions
source %{SOURCE2}

# install packages again into separate subdirectories
DESTDIR_ROOT=`pwd`/root
prefix=%{_prefix}
# create lists of all installed externals
for library in ${EXTERNALS} ; do
    echo $library
    DESTDIR=${DESTDIR_ROOT}/${library}
    mkdir -p $DESTDIR
    libdir=${DESTDIR}${prefix}/lib
    libpddir=${libdir}/pd
    libpdbindir=${libpddir}/bin
    if [ "$library" == "iemmatrix" ] ; then
        # special case, some of the iemmatrix externals are in buildsrc...
        # so move them where the iemmattrix install can find them
        mkdir -p ${DESTDIR_ROOT}/${library}%{_prefix}/lib/pd/extra/
        mv ${DESTDIR_ROOT}/buildsrc%{_prefix}/lib/pd/extra/mtx* \
           ${DESTDIR_ROOT}/${library}%{_prefix}/lib/pd/extra/
    fi
    make -C externals DESTDIR=${DESTDIR} prefix=${prefix} libpddir=${libpddir} libpdbindir=${libpdbindir} ${library}_install
    if [ -d "${libpddir}/doc/5.reference/" ] ; then
        (cd ${libpddir}/doc/5.reference/ ; ${buildhome}/scripts/convert-help-to-standard.sh)
    fi
    find ${DESTDIR_ROOT}/${library} -type f | sed "s|${DESTDIR_ROOT}/${library}||g" > ${buildhome}/files-subpackage-${library}	
done

# create lists of all installed abstractions
for abstraction in ${ABSTRACTIONS} ; do
    echo $abstraction
    DESTDIR=${DESTDIR_ROOT}/${abstraction}
    mkdir -p $DESTDIR
    libdir=${DESTDIR}${prefix}/lib
    libpddir=${libdir}/pd
    libpdbindir=${libpddir}/bin
    make -C abstractions DESTDIR=${DESTDIR} prefix=${prefix} libpddir=${libpddir} libpdbindir=${libpdbindir} ${abstraction}_install
    if [ -d "${libpddir}/doc/5.reference/" ] ; then
        (cd ${libpddir}/doc/5.reference/ ; ${buildhome}/scripts/convert-help-to-standard.sh)
    fi
    find ${DESTDIR_ROOT}/${abstraction} -type f | sed "s|${DESTDIR_ROOT}/${abstraction}||g" > ${buildhome}/files-subpackage-${abstraction}	
done

# redo libsrc file list to account for stuff that other libraries might have moved away
find ${DESTDIR_ROOT}/buildsrc -type f | sed "s|${DESTDIR_ROOT}/buildsrc||g" > ${buildhome}/files-subpackage-buildsrc	

# Gem is different...
DESTDIR=${DESTDIR_ROOT}/gem
mkdir -p ${DESTDIR}
make -C ${buildhome}/Gem/src DESTDIR=${DESTDIR} install
find ${DESTDIR_ROOT}/gem -type f | sed "s|${DESTDIR_ROOT}/gem||g" > ${buildhome}/files-subpackage-gem

# and finally pd itself as well
DESTDIR=${DESTDIR_ROOT}/pd
mkdir -p ${DESTDIR}/usr/lib/pd/doc/manuals/Pd
make -C ${buildhome}/pd/src prefix=${DESTDIR}%{_prefix} DESTDIR=${DESTDIR} install
install -p ${buildhome}/pd/src/notes.txt ${DESTDIR}%{_libdir}/pd/doc/manuals/Pd
find ${DESTDIR_ROOT}/pd -type f | sed "s|${DESTDIR_ROOT}/pd||g" > ${buildhome}/files-subpackage-pd

# find all descriptions written to the install tree
find %{buildroot} -name \*.pd -exec grep -q "PDDP_META: " {} \; -print > pddp-meta-files
for meta in `cat pddp-meta-files` ; do
    NAME=`grep "NAME: " ${meta} | awk -F: '{print $2}' | sed "s/;//g" | sed "s/^ //g" | sed "s/ $//"`
    DESC=`grep "DESCRIPTION: " ${meta} | awk -F: '{print $2}' | sed "s/;//g" | sed "s/^ //g" | sed "s/ $//"`
    echo "${NAME}=${DESC}" >> pd-descriptions
done

# fix broken stuff in pd install (man pages)
cat files-subpackage-pd | sed "s|%{_prefix}/man|%{_datadir}/man|g" > files-tmp
mv -f files-tmp files-subpackage-pd

# all the files already packaged
cat files-subpackage-* | sort | uniq > already-packaged

# all installed files
find %{buildroot} -type f | sed "s|%{buildroot}||g" | sort | uniq > all-installed

# all files not already in file lists
comm -2 -3 all-installed already-packaged > files-basepackage

%clean
# %{__rm} -rf %{buildroot}

%files -f files-basepackage
%defattr(-,root,root,-)
%exclude /usr/lib/pd/extra/expr.pd_linux
%exclude /usr/lib/pd/extra/expr~.pd_linux
%exclude /usr/lib/pd/extra/expr~/expr~.pd_linux
%exclude /usr/lib/pd/extra/expr~/fexpr~.pd_linux
%exclude /usr/lib/pd/extra/fexpr~.pd_linux

%if 0%{?fc5}
%exclude /usr/lib/pd/doc/examples/py/scripts/buffer.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/buffer.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/pak.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/pak.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/script.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/script.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/sendrecv.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/sendrecv.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/sig.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/sig.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/simple.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/simple.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/tcltk.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/tcltk.pyo
%exclude /usr/lib/pd/doc/examples/py/scripts/threads.pyc
%exclude /usr/lib/pd/doc/examples/py/scripts/threads.pyo
%endif

%files -n pd -f files-subpackage-pd

# automatically created subpackage files

%files -n pd-boids -f files-subpackage-boids
%defattr(-,root,root,-)

%files -n pd-bsaylor -f files-subpackage-bsaylor
%defattr(-,root,root,-)

%files -n pd-buildsrc -f files-subpackage-buildsrc
%defattr(-,root,root,-)

%files -n pd-corelibs -f files-subpackage-corelibs
%defattr(-,root,root,-)

%files -n pd-creb -f files-subpackage-creb
%defattr(-,root,root,-)

%files -n pd-cxc -f files-subpackage-cxc
%defattr(-,root,root,-)

%files -n pd-cyclone -f files-subpackage-cyclone
%defattr(-,root,root,-)

%files -n pd-ext13 -f files-subpackage-ext13
%defattr(-,root,root,-)

%files -n pd-flib -f files-subpackage-flib
%defattr(-,root,root,-)

%files -n pd-freeverb -f files-subpackage-freeverb
%defattr(-,root,root,-)

%files -n pd-gem -f files-subpackage-gem
%defattr(-,root,root,-)

%files -n pd-ggee -f files-subpackage-ggee
%defattr(-,root,root,-)

%files -n pd-gyre -f files-subpackage-gyre
%defattr(-,root,root,-)

%files -n pd-hardware -f files-subpackage-hardware
%defattr(-,root,root,-)

%files -n pd-iem_ambi -f files-subpackage-iem_ambi
%defattr(-,root,root,-)

%files -n pd-iem_bin_ambi -f files-subpackage-iem_bin_ambi
%defattr(-,root,root,-)

%files -n pd-iemlib -f files-subpackage-iemlib
%defattr(-,root,root,-)

%files -n pd-iemmatrix -f files-subpackage-iemmatrix
%defattr(-,root,root,-)

%files -n pd-keyboardkeys -f files-subpackage-keyboardkeys
%defattr(-,root,root,-)

%files -n pd-la-kitchen -f files-subpackage-la-kitchen
%defattr(-,root,root,-)

%files -n pd-list-abs -f files-subpackage-list-abs
%defattr(-,root,root,-)

%files -n pd-loaders -f files-subpackage-loaders
%defattr(-,root,root,-)

%files -n pd-mapping -f files-subpackage-mapping
%defattr(-,root,root,-)

%files -n pd-markex -f files-subpackage-markex
%defattr(-,root,root,-)

%files -n pd-maxlib -f files-subpackage-maxlib
%defattr(-,root,root,-)

%files -n pd-memento -f files-subpackage-memento
%defattr(-,root,root,-)

%files -n pd-mjlib -f files-subpackage-mjlib
%defattr(-,root,root,-)

%files -n pd-motex -f files-subpackage-motex
%defattr(-,root,root,-)

%files -n pd-nqpoly -f files-subpackage-nqpoly
%defattr(-,root,root,-)

%files -n pd-nusmuk -f files-subpackage-nusmuk
%defattr(-,root,root,-)

%files -n pd-oscx -f files-subpackage-oscx
%defattr(-,root,root,-)

%files -n pd-parazit -f files-subpackage-parazit
%defattr(-,root,root,-)

%files -n pd-pddp -f files-subpackage-pddp
%defattr(-,root,root,-)

%files -n pd-pdogg -f files-subpackage-pdogg
%defattr(-,root,root,-)

%files -n pd-pdp -f files-subpackage-pdp
%defattr(-,root,root,-)

%files -n pd-pidip -f files-subpackage-pidip
%defattr(-,root,root,-)

%files -n pd-pixeltango -f files-subpackage-pixeltango
%defattr(-,root,root,-)

%files -n pd-pmpd -f files-subpackage-pmpd
%defattr(-,root,root,-)

%files -n pd-purepd -f files-subpackage-purepd
%defattr(-,root,root,-)

%files -n pd-rradical -f files-subpackage-rradical
%defattr(-,root,root,-)

%files -n pd-sigpack -f files-subpackage-sigpack
%defattr(-,root,root,-)

%files -n pd-smlib -f files-subpackage-smlib
%defattr(-,root,root,-)

%files -n pd-toxy -f files-subpackage-toxy
%defattr(-,root,root,-)

%files -n pd-unauthorized -f files-subpackage-unauthorized
%defattr(-,root,root,-)

%files -n pd-vbap -f files-subpackage-vbap
%defattr(-,root,root,-)

%files -n pd-zexy -f files-subpackage-zexy
%defattr(-,root,root,-)

%changelog
* Mon Jul 31 2006 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.39.2
- added imlib2-devel to build requirements (pidip needs it)

- pending problems:
  . too many files in the catch all pd-extended package
  . pidip: needs imlib2, can't find it

install: cannot stat `/usr/src/rpm/BUILD/pure-data/packages/linux_make/../../externals/pmpd/*.pd_linux': No such file or directory

install: cannot stat `/usr/src/rpm/BUILD/pure-data/packages/linux_make/../../externals/pidip/*.pd_linux': No such file or directory

install: cannot stat `/usr/src/rpm/BUILD/pure-data/packages/linux_make/../../packages/noncvs/linux/bin/*.*': No such file or directory

install: cannot stat `/usr/src/rpm/BUILD/pure-data/packages/linux_make/../../packages/noncvs/linux/doc/5.reference/*.*': No such file or directory

install: cannot stat `/usr/src/rpm/BUILD/pure-data/packages/linux_make/../../packages/noncvs/linux/extra/*.*': No such file or directory

install: cannot stat `/usr/src/rpm/BUILD/pure-data/packages/linux_make/../../packages/noncvs/linux/gripd/*.*': No such file or directory

* Tue Jul 25 2006 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.39.2
- changed package name to pd-extended, build all of it, both pd and
  the external collection

* Sun Jul 23 2006 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 
- cvs snapshot of 2006.07.23
- add build requirements for pdp/pidip
- remove pddp help files that conflict with pd
- ignore the pd install

* Sat Jun 10 2006 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 
- initial build.
- do not build pdp/pidip
