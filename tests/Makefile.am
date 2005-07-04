if HAVE_GDK
gdk_tests = \
	test-animation \
	test-xy-stress

test_animation_SOURCES = test-animation.c
test_xy_stress_SOURCES = test-xy-stress.c
else
gdk_tests =
endif

noinst_PROGRAMS = \
	test-replace \
	test-default-action \
	test-multi-actions \
	test-image \
	test-basic \
	test-error \
	test-markup \
	test-xy \
	$(gdk_tests)

common_ldflags = \
	$(top_builddir)/libnotify/libnotify.la \
	$(PACKAGE_LIBS) \
	$(GDK_LIBS)

test_replace_SOURCES = test-replace.c
test_replace_LDADD  = $(common_ldflags)
test_default_action_SOURCES = test-default-action.c
test_default_action_LDADD  = $(common_ldflags)
test_multi_actions_SOURCES = test-multi-actions.c
test_multi_actions_LDADD  = $(common_ldflags)
test_image_SOURCES = test-image.c
test_image_LDADD  = $(common_ldflags)
test_basic_SOURCES = test-basic.c
test_basic_LDADD = $(common_ldflags)
test_error_SOURCES = test-error.c
test_error_LDADD = $(common_ldflags)

test_animation_LDADD = $(common_ldflags)

test_markup_SOURCES = test-markup.c
test_markup_LDADD = $(common_ldflags)

test_xy_SOURCES = test-xy.c
test_xy_LDADD = $(common_ldflags)

test_xy_stress_LDADD = $(common_ldflags)

INCLUDES = $(PACKAGE_CFLAGS) $(GDK_CFLAGS)
