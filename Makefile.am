SUBDIRS = libnotify docs tools tests

pcdata_DATA = libnotify.pc
pcdatadir = $(libdir)/pkgconfig

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	libnotify.pc.in

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

doxygen:
	@echo "Running doxygen..."
	@doxygen Doxyfile

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
