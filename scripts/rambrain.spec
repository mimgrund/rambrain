Name:		rambrain
Version:	0.1
Release:	1%{?dist}
Summary:	rambrain dynamically linked library

Group:		D Development Tools and Libraries
License:	GPLv3+
URL:		https://github.com/mimgrund/rambrain/
Source0:	rambrain.tar.gz

BuildRequires:	libaio-devel git
Requires:	libaio-devel

%description
Rambrain is a versatile memory manager for C++11 which lets you extend your physical RAM size to the size of your harddisk

%package devel
Summary:	Development headers for rambrain
%description devel
This package provides necessary header files to develop programs using rambrain

%package doc
Summary:        Developer documentation
%description doc
This package provides the developer documentation for rambrain

%prep
rm -rv rambrain
git clone https://github.com/mimgrund/rambrain.git


%build
cd rambrain/build
cmake .. -DOPTIMISE_COMPILATION=on -DCMAKE_INSTALL_PREFIX=%{buildroot}/usr -DLOGSTATS=off
make -j5
make doc


%install
cd rambrain/build
make install
mkdir -p %{buildroot}/usr/share/doc/rambrain/
cp -r ../doc/* %{buildroot}/usr/share/doc/rambrain/
rm -rf rambrain

%files
/usr/lib64/*
%files devel
/usr/include/rambrain/*
%files doc
/usr/share/doc/rambrain/*

%changelog

