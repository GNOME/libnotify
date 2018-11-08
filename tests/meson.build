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

foreach test: test_progs
  executable('test-' + test, ['test-' + test + '.c'],
    dependencies: [libnotify_dep, EXTRA_DEPS],
    link_with: libnotify_lib,
    install: false,
  )
endforeach
