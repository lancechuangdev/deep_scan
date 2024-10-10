project('deep_scan', 'cpp')

# Import gnome module for resource compilation
gnome = import('gnome')

gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0')

# Retrieve the value of the environment variable
mvcam_runenv = '/opt/MVS/lib'

# Add include directory and library paths using the environment variable
inc_dir = join_paths(mvcam_runenv, '../include')
lib_dir = join_paths(mvcam_runenv, '64')

# Generate the binary resource file, no need to install it directly
resource_target = gnome.compile_resources(
    'deep_scan_resources',
    'deep_scan.gresource.xml',
    source_dir: 'icons',  # Directory where your icons are located
)

executable('ds', 
    [resource_target, 'main.cpp', 'mainwindow.cpp', 'framequeue.cpp', 'drawwindow.cpp', 'preprocesswindow.cpp', 'trainmodelwindow.cpp', 'fileutils.cpp', 'pyscript.cpp', 'logger.cpp'],
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