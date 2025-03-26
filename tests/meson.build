test_progs = [
  'replace',
  'replace-widget',
  'server-info',
  'default-action',
  'multi-actions',
  'action-icons',
  'image',
  'basic',
  'error',
  'markup',
  'persistence',
  'removal',
  'resident',
  'rtl',
  'size-changes',
  'transient',
  'urgency',
  'xy',
  'xy-actions',
  'xy-stress',
]

foreach tprog: test_progs
  test('test-' + tprog, executable('test-' + tprog, ['test-' + tprog + '.c'],
    dependencies: [libnotify_dep, tests_deps],
  ))
endforeach
