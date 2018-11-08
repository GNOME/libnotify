progs = ['notify-send']

foreach prog: progs
  executable(prog, [prog + '.c'],
    dependencies: libnotify_dep,
    link_with: libnotify_lib,
    install: true,
    install_dir: bindir)
endforeach
