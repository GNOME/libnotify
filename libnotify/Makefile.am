notifyincdir = $(includedir)/libnotify

MARSHAL_PREFIX = notify_marshal
MARSHAL_FILE = notify-marshal
ENUM_FILE = notify-enum-types

AM_CPPFLAGS = \
	-I.				\
	-I..				\
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	-DG_LOG_DOMAIN=\"libnotify\"	\
	$(PACKAGE_CFLAGS)

lib_LTLIBRARIES = libnotify.la

notify_headers = \
	notify.h \
	notify-features.h \
	notification.h

notifyinc_HEADERS = \
	$(notify_headers) \
	$(ENUM_FILE).h

noinst_HEADERS = \
	notify-marshal.h \
	internal.h

BUILT_SOURCES = \
	$(ENUM_FILE).h \
	$(ENUM_FILE).c \
	notify-marshal.c \
	notify-marshal.h

libnotify_la_SOURCES = \
	notify.c \
	notification.c \
	notify-enum-types.c \
	notify-marshal.c

libnotify_la_LIBADD = \
	$(PACKAGE_LIBS)

libnotify_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-symbols-regex "^notify_.*"

notify-marshal.h: notify-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=$(MARSHAL_PREFIX) $(srcdir)/notify-marshal.list \
		--header > $@.tmp && mv $@.tmp $@

notify-marshal.c: notify-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=$(MARSHAL_PREFIX) $(srcdir)/notify-marshal.list \
		--header --body > $@.tmp && mv $@.tmp $@

$(ENUM_FILE).h: s-enum-types-h
	@true
s-enum-types-h: $(notify_headers) Makefile
	$(AM_V_GEN) ( cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef _NOTIFY_ENUM_TYPES_H_\n#define _NOTIFY_ENUM_TYPES_H_\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type(void);\n#define NOTIFY_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n"	\
			--ftail "G_END_DECLS\n\n#endif /* _NOTIFY_ENUM_TYPES_H_ */" \
		$(notify_headers) ) > tmp-$(ENUM_FILE).h \
	&& (cmp -s tmp-$(ENUM_FILE).h $(ENUM_FILE).h || cp tmp-$(ENUM_FILE).h $(ENUM_FILE).h ) \
	&& rm -f tmp-$(ENUM_FILE).h	\
	&& echo timestamp > $(@F)

$(ENUM_FILE).c: s-enum-types-c
	@true
s-enum-types-c: $(notify_headers) Makefile
	$(AM_V_GEN) ( cd $(srcdir) && glib-mkenums \
			--fhead "#include <libnotify/notify.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(notify_headers) ) > tmp-$(ENUM_FILE).c \
	&& (cmp -s tmp-$(ENUM_FILE).c $(ENUM_FILE).c || cp tmp-$(ENUM_FILE).c $(ENUM_FILE).c ) \
	&& rm -f tmp-$(ENUM_FILE).c \
	&& echo timestamp > $(@F)


if HAVE_INTROSPECTION

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Notify-0.7.gir
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_srcdir) --add-include-path=$(top_builddir) --warn-all -I$(top_srcdir) -I$(top_builddir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir)

Notify-0.7.gir: libnotify.la
Notify_0_7_gir_INCLUDES = GLib-2.0 GdkPixbuf-2.0
Notify_0_7_gir_LIBS = libnotify.la
Notify_0_7_gir_FILES = $(notify_headers) $(libnotify_la_SOURCES)
Notify_0_7_gir_PACKAGES = glib-2.0 gobject-2.0 gdk-pixbuf-2.0

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

endif

EXTRA_DIST = \
	notify-marshal.list \
	$(ENUM_FILE).h

CLEANFILES = \
	notify-marshal.c \
	notify-marshal.h \
	$(ENUM_FILE).c \
	$(ENUM_FILE).h \
	s-enum-types-c \
	s-enum-types-h

if HAVE_INTROSPECTION
CLEANFILES += \
	$(dist_gir_DATA) \
	$(typelib_DATA)
endif

-include $(top_srcdir)/git.mk
