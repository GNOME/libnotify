noinst_PROGRAMS =
	send-notification

common_ldflags = \
	$(top_builddir)/libnotify/libnotify.la \
	$(PACKAGE_LIBS)

send_notification_SOURCES = send-notification.c
send_notification_LDADD   = $(common_ldflags)

INCLUDES = $(PACKAGE_CFLAGS)
