test_env = environment()
test_env.set('G_DEBUG', 'fatal-criticals')

dbus_run_session = find_program('dbus-run-session', required: false)
py = python.find_installation('python3',
  modules: ['dbus', 'dbusmock'],
  required: false)
xvfb_run = find_program('xvfb-run', required: false)

if not dbus_run_session.found() or not py.found()
  subdir_done()
endif

add_test_setup(
  'default',
  is_default: true,
  exclude_suites: ['interactive'] + (xvfb_run.found() ? [] : ['graphical']),
  exe_wrapper: [
    xvfb_run.found() ? [xvfb_run, '-a'] : [],
    dbus_run_session, '--',
    py, '-m', 'dbusmock', '--session', '-t', 'notification_daemon', '-e',
  ],
  env: test_env,
)

test_progs = {
  'replace': {},
  'replace-widget': {'suites': 'interactive'},
  'server-info': {},
  'default-action': {'suites': 'interactive'},
  'multi-actions': {'suites': 'interactive'},
  'action-icons': {'suites': 'interactive'},
  'image': {
    'suites': 'graphical',
    'depends': [
      fs.copyfile(files('applet-critical.png'), 'applet-critical.png'),
      fs.copyfile(files('dewdop_leaf.jpg'), 'dewdop_leaf.jpg'),
    ],
  },
  'basic': {},
  'error': {},
  'markup': {},
  'persistence': {'suites': 'graphical'},
  'removal': {'suites': 'interactive'},
  'resident': {'suites': 'interactive'},
  'rtl': {},
  'size-changes': {},
  'transient': {'suites': 'interactive'},
  'urgency': {},
  'xy': {},
  'xy-actions': {'suites': 'interactive'},
  'xy-stress': {'suites': 'interactive'},
}

foreach tprog, tprop: test_progs
  t = executable('test-' + tprog, ['test-' + tprog + '.c'],
    dependencies: [libnotify_dep, tests_deps],
  )

  test('test-' + tprog, t,
    suite: tprop.get('suites', []),
    depends: tprop.get('depends', []),
    workdir: meson.current_build_dir(),
  )
endforeach


test('test-notify-send', py,
  args: files('test_notify_send.py'),
  depends: tool_notify_send,
  env: {
    'NOTIFY_SEND': tool_notify_send.full_path(),
  },
)
