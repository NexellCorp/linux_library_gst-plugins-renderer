Name:    gst-plugins-renderer
Version: 0.0.1
Release: 0
License: Apache 2.0
Summary: gstreamer plugin renderer
Group: Development/Libraries
Source:  %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:	gstreamer1-devel
BuildRequires:	glibc-devel
BuildRequires:	gstreamer1-plugins-base-devel
BuildRequires:	nx-renderer
BuildRequires:	libdrm
BuildRequires:	nx-drm-allocator
BuildRequires:	nx-v4l2

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
gstreamer plugin renderer

%package devel
Summary: gstreamer plugin renderer
Group: Development/Libraries
License: Apache 2.0
Requires: %{name} = %{version}-%{release}

%description devel
gstreamer plugin renderer (devel)

%prep
%setup -q

%build
make

%postun -p /sbin/ldconfig

%install
rm -rf %{buildroot}

mkdir -p %{buildroot}/usr/include
cp %{_builddir}/%{name}-%{version}/mm_types.h %{buildroot}/usr/include

mkdir -p %{buildroot}/usr/lib/gstreamer-1.0
cp %{_builddir}/%{name}-%{version}/libgstnxrenderer.so  %{buildroot}/usr/lib/gstreamer-1.0

%files
%attr (0644, root, root) %{_libdir}/gstreamer-1.0/libgstnxrenderer.so

%files devel
%attr (0644, root, root) %{_includedir}/mm_types.h
