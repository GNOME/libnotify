gnome = import('gnome')

libnotify_includedir = join_paths(includedir, meson.project_name())

headers = [
  'notify.h',
  'notification.h',
]

sources = [
  'notify.c',
  'notification.c',
]

generated_files = []

features_data = configuration_data()
features_data.set('LIBNOTIFY_MAJOR_VERSION', VERSION_ARRAY[0])
features_data.set('LIBNOTIFY_MINOR_VERSION', VERSION_ARRAY[1])
features_data.set('LIBNOTIFY_MICRO_VERSION', VERSION_ARRAY[2])

headers += configure_file(
  input: 'notify-features.h.in',
  output: 'notify-features.h',
  configuration: features_data)

enum_types = gnome.mkenums_simple('@0@-enum-types'.format(LIBNAME),
  sources : headers,
  install_header: true,
  install_dir: libnotify_includedir
)

marshal = gnome.genmarshal('@0@-marshal'.format(LIBNAME),
  prefix: '@0@_marshal'.format(LIBNAME),
  sources: '@0@-marshal.list'.format(LIBNAME),
)

libnotify_cflags = [
  '-DG_LOG_DOMAIN="libnotify"',
]

mapfile = meson.project_name() + '.map'
libnotify_ldflags = [
  '-Wl,--version-script,'+join_paths(meson.current_source_dir(), mapfile),
]

libnotify_dep = declare_dependency(
  include_directories: default_includes,
  dependencies: LIBNOTIFY_DEPS,
  compile_args: libnotify_cflags,
  sources: headers + [enum_types[1]] + [marshal[1]],
  link_args: libnotify_ldflags,
)

libnotify_lib = shared_library(LIBNAME,
  dependencies: libnotify_dep,
  sources: sources + enum_types + marshal,
  version: '@0@.@1@.@2@'.format(LT_CURRENT, LT_REVISION, LT_AGE),
  soversion: LT_CURRENT,
  install: true,
)

introspection = get_option('introspection')
if not introspection.disabled()
  find_program('g-ir-scanner', required: introspection.enabled())
  gnome.generate_gir(libnotify_lib,
    sources: headers + sources + enum_types,
    namespace: 'Notify',
    nsversion: MODULE_VERSION,
    export_packages: meson.project_name(),
    includes: ['GLib-2.0', 'GdkPixbuf-2.0'],
    extra_args: '--c-include=@0@/@1@.h'.format(meson.project_name(), LIBNAME),
    install: true
  )
endif

install_headers(headers, subdir: libnotify_includedir)
