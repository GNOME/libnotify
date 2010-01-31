bin_PROGRAMS = notify-send

common_ldflags = \
	$(top_builddir)/libnotify/libnotify.la \
	$(PACKAGE_LIBS)

notify_send_SOURCES = notify-send.c
notify_send_LDADD   = $(common_ldflags)

INCLUDES = $(PACKAGE_CFLAGS)	\
	-I$(top_srcdir)

-include $(top_srcdir)/git.mk
