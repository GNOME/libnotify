project('libnotify',
  'c',
  version: '0.7.7',
  meson_version: '>= 0.47.0')

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
EXTRA_DEPS = []

pkg_deps = [
  {'name': 'gdk-pixbuf-2.0' },
  {'name': 'gio-2.0', 'version': '>= 2.26.0' },
  {'name': 'glib-2.0', 'version': '>= 2.26.0' },
]

extra_deps = [
  {'name': 'gtk+-3.0', 'version': '>= 2.90', 'required': get_option('tests') },
]

foreach p: pkg_deps + extra_deps
  pkg = p.get('name')
  required = p.get('required', true)
  dep = dependency(pkg, version: p.get('version', []), required: required)

  if (pkg_deps.contains(p))
    LIBNOTIFY_DEPS += dep
  else
    EXTRA_DEPS += dep
  endif
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

subdir('libnotify')
subdir('tools')
subdir('docs')

if get_option('tests')
  subdir('tests')
endif
