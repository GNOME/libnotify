notifyincdir = $(includedir)/libnotify

lib_LTLIBRARIES = libnotify.la

notifyinc_HEADERS = \
	notify.h

libnotify_la_SOURCES = \
	notify.c

libnotify_la_LIBADD = \
	$(PACKAGE_LIBS)

libnotify_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

INCLUDES = \
	$(PACKAGE_CFLAGS)
