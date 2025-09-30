project('libnotify',
  'c',
  version: '0.8.7',
  meson_version: '>= 0.64.0')

fs = import('fs')
gnome = import('gnome')
pkgconfig = import('pkgconfig')
python = import('python')

cc = meson.get_compiler('c')

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

API_VERSION = 7

VERSION_ARRAY = meson.project_version().split('.')
# Minor version is hardcoded until we have a real API break
MODULE_VERSION = '@0@.@1@'.format(VERSION_ARRAY[0], API_VERSION)
LIBNAME = meson.project_name().split('lib')[1]

default_includes = include_directories('.')

prefix = get_option('prefix')
includedir = join_paths(prefix, get_option('includedir'))
docdir = join_paths(prefix, get_option('datadir'), 'doc', meson.project_name())
man1dir = join_paths(prefix, get_option('mandir'), 'man1')

libnotify_deps = []
extra_deps = []

glib_req_version = '>= 2.62.0'

gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0')
glib_dep = dependency('glib-2.0', version: glib_req_version)
gio_dep = dependency('gio-2.0', version: glib_req_version)
gio_unix_dep = dependency('gio-unix-2.0', version: glib_req_version)
gtk_dep = dependency('gtk4', version: '>= 4.0', required: get_option('tests'))

have_gio_desktop_app_info = cc.check_header('gio/gdesktopappinfo.h',
  dependencies: gio_unix_dep,
  required: host_machine.system() == 'linux',
)

libnotify_deps = [gdk_pixbuf_dep, gio_dep, glib_dep, gio_unix_dep]
tests_deps = [gtk_dep]

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('HAVE_GIO_DESKTOP_APP_INFO', have_gio_desktop_app_info)
configure_file(input: 'config.h.meson',
  output : 'config.h',
  configuration : conf)

if get_option('man')
  xsltproc = find_program('xsltproc', required: true)
  stylesheet = 'http://docbook.sourceforge.net/release/xsl-ns/current/manpages/docbook.xsl'
  xsltproc_command = [
    xsltproc,
    '--nonet',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'man.th.extra1.suppress', '1',
    '-o', '@OUTPUT@',
    stylesheet,
    '@INPUT@',
  ]

  testrun = run_command(xsltproc, '--nonet', stylesheet, check: false)

  if testrun.returncode() != 0
    error('DocBook stylesheet for generating man pages not found, you need to install docbook-xsl-ns or similar package.')
  endif
endif

subdir('libnotify')
subdir('tools')
subdir('docs')

if get_option('tests')
  subdir('tests')
endif
