project('libnotify',
  'c',
  version: '0.7.7',
  meson_version: '>= 0.48.1')

################################################################
# libtool versioning
################################################################
#
# +1 :  0 : +1   == new interface that does not break old one.
# +1 :  0 :  0   == removed an interface. Breaks old apps.
#  ? : +1 :  ?   == internal changes that doesn't break anything.
#
# CURRENT : REVISION : AGE
#
LT_CURRENT=4
LT_REVISION=0
LT_AGE=0

VERSION_ARRAY = meson.project_version().split('.')
MODULE_VERSION = '@0@.@1@'.format(VERSION_ARRAY[0], VERSION_ARRAY[1])
LIBNAME = meson.project_name().split('lib')[1]

prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
libdir = join_paths(prefix, get_option('libdir'))
includedir = join_paths(prefix, get_option('includedir'))
docdir = join_paths(prefix, get_option('datadir'), 'doc', meson.project_name())

default_includes = include_directories('.')
conf = configuration_data()
cc = meson.get_compiler('c')

check_headers = [
  'dlfcn.h',
  'inttypes.h',
  'memory.h',
  'stdint.h',
  'stdlib.h',
  'strings.h',
  'string.h',
  'sys/stat.h',
  'sys/types.h',
  'unistd.h',
]

foreach h : check_headers
  conf.set('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

LIBNOTIFY_DEPS = []

pkg_deps = [
  {'name': 'gdk-pixbuf-2.0' },
  {'name': 'gio-2.0', 'version': '>= 2.26.0' },
  {'name': 'glib-2.0', 'version': '>= 2.26.0' },
]

extra_deps = [
  {'name': 'gtk+-3.0', 'version': '>= 2.90', 'required': not get_option('disable-tests') },
]

foreach p: pkg_deps + extra_deps
  pkg = p.get('name')
  required = p.get('required', true)
  dep = dependency(pkg, version: p.get('version', []), required: required)
  LIBNOTIFY_DEPS += dep
endforeach

conf.set('LIBNOTIFY_MAJOR_VERSION', VERSION_ARRAY[0])
conf.set('LIBNOTIFY_MINOR_VERSION', VERSION_ARRAY[1])
conf.set('LIBNOTIFY_MICRO_VERSION', VERSION_ARRAY[2])
conf.set_quoted('LIBNOTIFY_VERSION', meson.project_version())
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_STRING', meson.project_name() + ' ' + meson.project_version())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('VERSION', meson.project_version())

configure_file(input: 'config.h.meson',
  output : 'config.h',
  configuration : conf)

pkg_modules = ''
foreach p: pkg_deps
  pkg_modules += p.get('name') + ' ' + p.get('version', '') + ' '
endforeach

pc_conf = configuration_data()
pc_conf.set('prefix', get_option('prefix'))
pc_conf.set('exec_prefix', '${prefix}')
pc_conf.set('libdir', '${exec_prefix}/' + get_option('libdir'))
pc_conf.set('includedir', '${prefix}/' + get_option('includedir'))
pc_conf.set('pkg_modules', pkg_modules)
pc_conf.set('VERSION', meson.project_version())

configure_file(input: meson.project_name() + '.pc.in',
  output: meson.project_name() + '.pc',
  configuration: pc_conf,
  install_dir: join_paths(libdir, 'pkgconfig'))

subdir('libnotify')
subdir('tools')
subdir('docs')

if not get_option('disable-tests')
  subdir('tests')
endif
