SUBDIRS = libnotify tools tests

pcdata_DATA = libnotify.pc
pcdatadir = $(libdir)/pkgconfig

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	libnotify.pc.in \
	libnotify.spec.in

dist-hook: libnotify.spec
	cp libnotify.spec $(distdir)

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

doxygen:
	@echo "Running doxygen..."
	@doxygen Doxyfile
