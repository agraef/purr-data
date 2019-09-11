# This is a sample spec file for packaging Pd libraries

#%define _topdir	 	/home/strike/mywget
%define name		template 
%define release		1
%define version 	0.1
%define buildroot %{_topdir}/%{name}-%{version}-root

BuildRoot:	%{buildroot}
Summary: 		replace me with a summary of your library
License: 		GPL
Name: 			%{name}
Version: 		%{version}
Release: 		%{release}
Source: 		%{name}-%{version}.tar.gz
Prefix: 		/usr
Group: 			Development/Tools

%description
Replace me with a description of your library.

%prep
%setup -q

%build
make

%install
make install prefix=$RPM_BUILD_ROOT/usr
