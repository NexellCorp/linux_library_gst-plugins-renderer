ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I${includedir} \
	-I${includedir}/libdrm \
	-I./ \
	-I${GST_PATH}/staging/usr/include \
	-I${GST_PATH}/staging/usr/lib/glib-2.0/include \
	-I${GST_PATH}/staging/usr/include/glib-2.0 \
	-I${GST_PATH}/staging/usr/include/gstreamer-1.0

libgstnxrenderer_la_LTLIBRARIES = libgstnxrenderer.la
libgstnxrenderer_ladir = ${libdir}
libgstnxrenderer_la_LDFLAGS = -L${libdir} -L${GST_PATH}/target/usr/lib 
libgstnxrenderer_la_LIBADD = -lgstapp-1.0 -lgstallocators-1.0 -lgstbase-1.0 -lgmodule-2.0 -lgobject-2.0 -lglib-2.0
libgstnxrenderer_la_SOURCES = \
	gstnxrenderer.c

gstnxrendererincludedir = ${includedir}
gstnxrendererinclude_HEADERS = \
	gstnxrenderer.h


