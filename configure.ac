dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT(libnotify, 0.4.5, chipx86@chipx86.com)
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR(config.h.in)
AC_COPYRIGHT([Copyright 2004-2008 Christian Hammond])


dnl ################################################################
dnl # Version information
dnl ################################################################
LIBGALAGO_MAJOR_VERSION=0
LIBGALAGO_MINOR_VERSION=4
LIBGALAGO_MICRO_VERSION=5
LIBGALAGO_DEVEL_VERSION=0

LIBGALAGO_VERSION=$LIBGALAGO_MAJOR_VERSION.$LIBGALAGO_MINOR_VERSION.$LIBGALAGO_MICRO_VERSION

if test "x$LIBGALAGO_DEVEL_VERSION" != "x0"; then
	LIBGALAGO_VERSION=$LIBGALAGO_VERSION.$LIBGALAGO_DEVEL_VERSION
fi

AC_DEFINE_UNQUOTED(LIBGALAGO_MAJOR_VERSION, $LIBGALAGO_MAJOR_VERSION,
	[libnotify major version.])
AC_DEFINE_UNQUOTED(LIBGALAGO_MINOR_VERSION, $LIBGALAGO_MINOR_VERSION,
	[libnotify minor version.])
AC_DEFINE_UNQUOTED(LIBGALAGO_MICRO_VERSION, $LIBGALAGO_MICRO_VERSION,
	[libnotify micro version.])
AC_DEFINE_UNQUOTED(LIBGALAGO_VERSION, "$LIBGALAGO_VERSION",
	[libnotify version.])

dnl ################################################################
dnl # libtool versioning
dnl ################################################################
dnl #
dnl # +1 :  0 : +1   == new interface that does not break old one.
dnl # +1 :  0 :  0   == removed an interface. Breaks old apps.
dnl #  ? : +1 :  ?   == internal changes that doesn't break anything.
dnl #
dnl # CURRENT : REVISION : AGE
dnl #
LT_CURRENT=2
LT_REVISION=3
LT_AGE=1

LT_RELEASE=$LIBGALAGO_MAJOR_VERSION.$LIBGALAGO_MINOR_VERSION.$LIBGALAGO_MICRO_VERSION

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl ################################################################
dnl # Initialize automake
dnl ################################################################
VERSION=$LIBGALAGO_VERSION
PACKAGE=libnotify

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

dnl ################################################################
dnl # Specify a header configuration file
dnl ################################################################
AM_CONFIG_HEADER(config.h)

dnl ################################################################
dnl # Check for some standard stuff.
dnl ################################################################
AC_PROG_CC
#AC_ARG_PROGRAM
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

AC_C_CONST
AC_TYPE_SIZE_T
AC_EXEEXT

AM_PROG_LIBTOOL

REQ_DBUS_VERSION=0.76
REQ_GTK_VERSION=2.6
REQ_GLIB_VERSION=2.6
pkg_modules="gtk+-2.0 >= $REQ_GTK_VERSION, glib-2.0 >= $REQ_GLIB_VERSION, dbus-1 >= $REQ_DBUS_VERSION, dbus-glib-1 >= $REQ_DBUS_VERSION"
AC_SUBST(pkg_modules)
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

dnl
dnl Check the D-BUS version.
dnl

AC_MSG_CHECKING([dbus version])
DBUS_VERSION=`$PKG_CONFIG --modversion dbus-1`
DBUS_MAJOR_VER=`echo $DBUS_VERSION | cut -d. -f 1`
DBUS_MINOR_VER=`echo $DBUS_VERSION | cut -d. -f 2`
DBUS_MICRO_VER=`echo $DBUS_VERSION | cut -d. -f 3`

AC_MSG_RESULT($DBUS_VERSION)
AC_DEFINE_UNQUOTED(DBUS_MAJOR_VER, $DBUS_MAJOR_VER, [D-BUS major version.])
AC_DEFINE_UNQUOTED(DBUS_MINOR_VER, $DBUS_MINOR_VER, [D-BUS minor version.])
AC_DEFINE_UNQUOTED(DBUS_MICRO_VER, $DBUS_MICRO_VER, [D-BUS micro version.])

AC_DEFINE_UNQUOTED(DBUS_API_SUBJECT_TO_CHANGE, ,DBUS API is subject to change)


dnl ################################################################
dnl # Set up gtk-doc
dnl ################################################################
GTK_DOC_CHECK([1.4])


dnl # Use wall if we have GCC
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall"
fi

AC_SUBST(CFLAGS)

dnl ################################################################
dnl # Output the Makefiles
dnl ################################################################
AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
libnotify.pc
libnotify/Makefile
tests/Makefile
tools/Makefile
])

AC_OUTPUT

echo
echo $PACKAGE v$VERSION

echo
echo "Now type make to compile"
echo "Then su to root and type: make install"
echo
