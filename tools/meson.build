progs = ['notify-send']

foreach prog: progs
  target = executable(prog, [prog + '.c'],
    dependencies: libnotify_dep,
    install: true)
  set_variable('tool_@0@'.format(prog.underscorify()), target)
endforeach
