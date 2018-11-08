gnome.gtkdoc(meson.project_name(),
  mode: 'sgml',
  main_sgml: meson.project_name() + '-docs.sgml',
  src_dir: join_paths(meson.source_root(), meson.project_name()),
  dependencies: declare_dependency(
    dependencies: libnotify_dep,
    link_with: libnotify_lib
  ),
  gobject_typesfile: meson.project_name() + '.types',
  ignore_headers: ['notify-enum-types.h', 'notify-marshal.h', 'internal.h'],
  scan_args: [ '--deprecated-guards=LIBNOTIFY_DISABLE_DEPRECATED' ],
  install: true,
)
