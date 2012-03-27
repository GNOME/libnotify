dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT([libnotify],
        [0.7.5],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=libnotify],
        [libnotify])
AC_PREREQ([2.63])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

dnl ################################################################
dnl # Version information
dnl ################################################################
LIBNOTIFY_MAJOR_VERSION=0
LIBNOTIFY_MINOR_VERSION=7
LIBNOTIFY_MICRO_VERSION=5
LIBNOTIFY_DEVEL_VERSION=0

AC_SUBST(LIBNOTIFY_MAJOR_VERSION)
AC_SUBST(LIBNOTIFY_MINOR_VERSION)
AC_SUBST(LIBNOTIFY_MICRO_VERSION)

LIBNOTIFY_VERSION=$LIBNOTIFY_MAJOR_VERSION.$LIBNOTIFY_MINOR_VERSION.$LIBNOTIFY_MICRO_VERSION

if test "x$LIBNOTIFY_DEVEL_VERSION" != "x0"; then
	LIBNOTIFY_VERSION=$LIBNOTIFY_VERSION.$LIBNOTIFY_DEVEL_VERSION
fi

AC_DEFINE_UNQUOTED(LIBNOTIFY_MAJOR_VERSION, $LIBNOTIFY_MAJOR_VERSION,
	[libnotify major version.])
AC_DEFINE_UNQUOTED(LIBNOTIFY_MINOR_VERSION, $LIBNOTIFY_MINOR_VERSION,
	[libnotify minor version.])
AC_DEFINE_UNQUOTED(LIBNOTIFY_MICRO_VERSION, $LIBNOTIFY_MICRO_VERSION,
	[libnotify micro version.])
AC_DEFINE_UNQUOTED(LIBNOTIFY_VERSION, "$LIBNOTIFY_VERSION",
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
LT_CURRENT=4
LT_REVISION=0
LT_AGE=0

LT_RELEASE=$LIBNOTIFY_MAJOR_VERSION.$LIBNOTIFY_MINOR_VERSION.$LIBNOTIFY_MICRO_VERSION

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl ################################################################
dnl # Initialize automake
dnl ################################################################
AM_INIT_AUTOMAKE([1.10 no-dist-gzip dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ################################################################
dnl # Check for some standard stuff.
dnl ################################################################
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

AC_C_CONST
AC_TYPE_SIZE_T
AC_EXEEXT

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

REQ_GTK_VERSION=2.90
REQ_GLIB_VERSION=2.26.0

pkg_modules="gdk-pixbuf-2.0 glib-2.0 >= $REQ_GLIB_VERSION gio-2.0 >= $REQ_GLIB_VERSION"
AC_SUBST([pkg_modules])
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])

tests_modules="gtk+-3.0 >= $REQ_GTK_VERSION"
PKG_CHECK_MODULES(TESTS, [$tests_modules])

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

GOBJECT_INTROSPECTION_CHECK([0.9.12])

dnl ################################################################
dnl # Set up gtk-doc
dnl ################################################################
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

dnl ---------------------------------------------------------------------------
dnl - DocBook Documentation
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE(docbook-docs,
	[AS_HELP_STRING([--enable-docbook-docs],
	[build documentation (requires xmlto)])],
	enable_docbook_docs=$enableval,enable_docbook_docs=no)
AC_PATH_PROG(XMLTO, xmlto, no)
AC_MSG_CHECKING([whether to build DocBook documentation])
if test x$XMLTO = xno ; then
	have_docbook=no
else
	have_docbook=yes
fi
if test x$enable_docbook_docs = xauto ; then
	 if test x$have_docbook = xno ; then
		enable_docbook_docs=no
	else
		enable_docbook_docs=yes
	fi
fi
if test x$enable_docbook_docs = xyes; then
	if test x$have_docbook = xno; then
		AC_MSG_ERROR([Building DocBook docs explicitly required, but DocBook not found])
	fi
fi
AM_CONDITIONAL(DOCBOOK_DOCS_ENABLED, test x$enable_docbook_docs = xyes)
AC_MSG_RESULT(yes)

AC_ARG_VAR([XMLTO],[Define/override the `xmlto' location.])
AC_ARG_VAR([XMLTO_FLAGS],[Define/override `xmlto' options, like `--skip-validation'.])


# Turn on the additional warnings last

AC_ARG_ENABLE(more-warnings,
              AS_HELP_STRING([--enable-more-warnings],
                             [Maximum compiler warnings]),
              set_more_warnings="$enableval",[
	      if test -d $srcdir/.git; then
	        set_more_warnings=yes
	      else
	        set_more_warnings=no
              fi])

AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
        AC_MSG_RESULT(yes)
        CFLAGS="\
        -Wall \
        -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wnested-externs -Wpointer-arith \
        -Wcast-align -Wsign-compare -Wp,-D_FORTIFY_SOURCE=2 \
        $CFLAGS"

        for option in -Wno-strict-aliasing -Wno-sign-compare; do
                SAVE_CFLAGS="$CFLAGS"
                CFLAGS="$CFLAGS $option"
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
                                  [has_option=yes],
                                  [has_option=no])
                if test $has_option = no; then
                        CFLAGS="$SAVE_CFLAGS"
                fi
                AC_MSG_RESULT($has_option)
                unset has_option
                unset SAVE_CFLAGS
        done
        unset option
else
        AC_MSG_RESULT(no)
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
libnotify/notify-features.h
tests/Makefile
tools/Makefile
])

AC_OUTPUT

echo "
                    $PACKAGE $VERSION
                    ========================

	prefix:                   ${prefix}
	exec_prefix:              ${exec_prefix}
        bindir:                   ${bindir}
        datadir:                  ${datadir}
	source code location:	  ${srcdir}
	cflags:		          ${CFLAGS}

        Build docs:               ${enable_docbook_docs}

"
