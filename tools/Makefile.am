bin_PROGRAMS = notify-send test-replace

common_ldflags = \
	$(top_builddir)/libnotify/libnotify.la \
	$(PACKAGE_LIBS) \
	-lpopt

notify_send_SOURCES = notify-send.c
notify_send_LDADD   = $(common_ldflags)

test_replace_SOURCES = test-replace.c
test_replace_LDADD  = $(common_ldflags)

INCLUDES = $(PACKAGE_CFLAGS)
