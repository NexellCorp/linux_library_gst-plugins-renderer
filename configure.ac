#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.


# checks for Argument Enable
AC_ARG_WITH(
		[gst_path],
		AS_HELP_STRING([--with-gst_path],[with a extra dependency path for gstreamer]),
		[GST_PATH=$withval],
		[GST_PATH=no]
)
AC_SUBST([GST_PATH])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
