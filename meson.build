project('deep_scan', 'cpp')

gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0')

# Retrieve the value of the environment variable
mvcam_runenv = '/opt/MVS/lib'

# Add include directory and library paths using the environment variable
inc_dir = join_paths(mvcam_runenv, '../include')
lib_dir = join_paths(mvcam_runenv, '64')

executable('ds', 
    ['main.cpp', 'mainwindow.cpp', 'framequeue.cpp', 'drawwindow.cpp'],
    install: true,
    install_dir: join_paths(get_option('prefix'), 'bin', 'deep-scan'),
    dependencies: gtkmm_dep,
    include_directories: inc_dir,
    link_args: [
        '-Wl,-rpath=' + lib_dir,
        '-L' + lib_dir,
        '-lMvCameraControl'
    ]
)

# Install data files
install_data(
    'ui.glade', 
    install_dir: join_paths(get_option('prefix'), 'share', 'deep-scan')
)