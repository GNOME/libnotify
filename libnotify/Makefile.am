notifyincdir = $(includedir)/libnotify

MARSHAL_PREFIX = notify_marshal
MARSHAL_FILE = notify-marshal

lib_LTLIBRARIES = libnotify.la

notifyinc_HEADERS = \
	notify.h \
	notification.h

noinst_HEADERS = \
	notify-marshal.h \
	internal.h

libnotify_la_SOURCES = \
	notify.c \
	notification.c \
	notify-marshal.c

libnotify_la_LIBADD = \
	$(PACKAGE_LIBS)

libnotify_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

BUILT_SOURCES = \
	notify-marshal.c \
	notify-marshal.h

notify-marshal.h: notify-marshal.list
	$(GLIB_GENMARSHAL) --prefix=$(MARSHAL_PREFIX) notify-marshal.list \
		--header > notify-marshal.h

notify-marshal.c: notify-marshal.list
	$(GLIB_GENMARSHAL) --prefix=$(MARSHAL_PREFIX) notify-marshal.list \
		--body > notify-marshal.c

EXTRA_DIST = \
	notify-marshal.list

CLEANFILES = \
	notify-marshal.c \
	notify-marshal.h

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libnotify\" \
	$(PACKAGE_CFLAGS)
