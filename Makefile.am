ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I${includedir} \
	-I${includedir}/libdrm \
	-I./ \
	-I${EXTRAPATH_INCLUDE} \
	-I${EXTRAPATH}/lib/glib-2.0/include \
	-I${EXTRAPATH_INCLUDE}/glib-2.0 \
	-I${EXTRAPATH_INCLUDE}/gstreamer-1.0

libgstnxrenderer_la_LTLIBRARIES = libgstnxrenderer.la
libgstnxrenderer_ladir = ${libdir}
libgstnxrenderer_la_LDFLAGS = -L${libdir} -L${EXTRAPATH_LIB}
libgstnxrenderer_la_LIBADD = -lgstapp-1.0 -lgstallocators-1.0 -lgstbase-1.0 -lgmodule-2.0 -lgobject-2.0 -lglib-2.0
libgstnxrenderer_la_SOURCES = \
	gstnxrenderer.c

gstnxrendererincludedir = ${includedir}
gstnxrendererinclude_HEADERS = \
	gstnxrenderer.h \
	mm_types.h


