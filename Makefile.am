SUBDIRS = libnotify docs tools tests

pcdata_DATA = libnotify.pc
pcdatadir = $(libdir)/pkgconfig

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	Doxyfile.in \
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

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
