project(
  'rats',
  [ 'cpp', 'c' ],
  meson_version: '>=1.10.0',
  version: '0.1.0',
  license: 'AGPL-3.0-or-later',
  default_options: [
    'warning_level=3',
    'b_ndebug=if-release',
    'b_pie=true',
  ]
)
###########
# Modules #
###########
fs = import('fs')

#########
# Tools #
#########
pymod = import('python')
python3 = pymod.find_installation('python3')
has_tools = run_command(python3, '-c',
'''
exit(0)
''', check: false)
if has_tools.returncode() != 0
  error('Please ensure that all Python requirements are installed.')
endif

git = find_program('git', required: false)
build_hpp_py = find_program('tools/build.hpp.py')
desktop_files_py = find_program('tools/desktop-files.py')
cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

###########
# Version #
###########
organization = 'megatech'
version = {
  'major': 0,
  'minor': 1,
  'patch': 0
}

###############
# Directories #
###############
dirname = '@0@-@1@'.format(organization, meson.project_name())
libexec = get_option('prefix') / get_option('libexecdir')
share = get_option('prefix') / get_option('datadir')

#########################
# Windows Compatibility #
#########################
header_only_includes = include_directories('')
if host_machine.system() == 'windows'
  header_only_includes = include_directories(meson.get_external_property('includepath', 'C:/Development/Include'))
  version_py = find_program('tools/version.py')
  deppath = meson.get_external_property('deppath', 'C:/Development')
  cpu_family = host_machine.cpu_family()
  if cpu_family == 'x86_64'
    cpu_family = 'x64'
  endif
  libsdl3 = declare_dependency(dependencies: cxx.find_library('SDL3', dirs: deppath / 'SDL3/lib' / cpu_family),
                               include_directories: deppath / 'SDL3/include',
                               version: run_command(version_py, deppath / 'SDL3/VERSION', capture: true).stdout().strip())
  meson.override_dependency('sdl3', libsdl3)
  libwebp = declare_dependency(dependencies: cxx.find_library('libwebp', dirs: deppath / 'libwebp/lib'),
                               include_directories: deppath / 'libwebp/include',
                               version: run_command(version_py, deppath / 'libwebp/VERSION', capture: true).stdout().strip())
  meson.override_dependency('libwebp', libwebp)
endif

#################
# Configuration #
#################
configuration = configuration_data()
configuration.set_quoted('CONFIG_MESON_PROJECT_NAME', meson.project_name())
if 'major' in version
  configuration.set('CONFIG_MESON_PROJECT_VERSION_MAJOR', version['major'])
endif
if 'minor' in version
  configuration.set('CONFIG_MESON_PROJECT_VERSION_MINOR', version['minor'])
endif
if 'patch' in version
  configuration.set('CONFIG_MESON_PROJECT_VERSION_PATCH', version['patch'])
endif
configuration.set_quoted('CONFIG_MESON_CPU_FAMILY_ID', host_machine.cpu_family())
if host_machine.cpu_family() == 'x86_64'
  configuration.set10('CONFIG_MESON_CPU_FAMILY_X86_64', true)
elif host_machine.cpu_family() == 'x86'
  configuration.set10('CONFIG_MESON_CPU_FAMILY_X86', true)
elif host_machine.cpu_family() == 'aarch64'
  configuration.set10('CONFIG_MESON_CPU_FAMILY_AARCH64', true)
elif host_machine.cpu_family() == 'arm'
  configuration.set10('COFNIG_MESON_CPU_FAMILY_ARM', true)
else
  configuration.set10('CONFIG_MESON_CPU_FAMILY_UNKNOWN', true)
endif
configuration.set_quoted('CONFIG_MESON_SYSTEM_ID', host_machine.system())
if host_machine.system() == 'linux'
  configuration.set10('CONFIG_MESON_SYSTEM_LINUX', true)
elif host_machine.system() == 'windows'
  configuration.set10('CONFIG_MESON_SYSTEM_WINDOWS', true)
elif host_machine.system() == 'darwin'
  configuration.set10('CONFIG_MESON_SYSTEM_DARWIN', true)
else
  configuration.set10('CONFIG_MESON_SYSTEM_UNKNOWN', true)
endif
configuration.set_quoted('CONFIG_MESON_CXX_COMPILER_ID', cxx.get_id())
if cxx.get_id() == 'gcc'
  configuration.set10('CONFIG_MESON_CXX_COMPILER_GCC', true)
elif cxx.get_id() == 'clang'
  configuration.set10('CONFIG_MESON_CXX_COMPILER_CLANG', true)
elif cxx.get_id() == 'clang-cl'
  configuration.set10('CONFIG_MESON_CXX_COMPILER_CLANG_CL', true)
elif cxx.get_id() == 'msvc'
  configuration.set10('CONFIG_MESON_CXX_COMPILER_MSVC', true)
else
  configuration.set10('CONFIG_MESON_CXX_COMPILER_UNKNOWN', true)
endif
configuration.set_quoted('CONFIG_MESON_C_COMPILER_ID', cc.get_id())
if cxx.get_id() == 'gcc'
  configuration.set10('CONFIG_MESON_C_COMPILER_GCC', true)
elif cxx.get_id() == 'clang'
  configuration.set10('CONFIG_MESON_C_COMPILER_CLANG', true)
elif cxx.get_id() == 'clang-cl'
  configuration.set10('CONFIG_MESON_C_COMPILER_CLANG_CL', true)
elif cxx.get_id() == 'msvc'
  configuration.set10('CONFIG_MESON_C_COMPILER_MSVC', true)
else
  configuration.set10('CONFIG_MESON_C_COMPILER_UNKNOWN', true)
endif
if host_machine.system() == 'linux'
  paths = [ '~/.local/share' / dirname, '/usr/local' / dirname, '/usr/share' / dirname, share / dirname, '$ORIGIN' ]
  strings = [ '' ]
  foreach path : paths
    strings += '  CONFIG_MESON_SEARCH_PATH("@0@") '.format(path)
  endforeach
  if strings.length() > 0
    configuration.set('CONFIG_MESON_SEARCH_PATHS', '\\\n'.join(strings))
    configuration.set('CONFIG_MESON_SEARCH_PATH_COUNT', paths.length())
  endif
elif host_machine.system() == 'windows'
  paths = [ '~/AppData/Roaming', '$ORIGIN' ]
  strings = [ '' ]
  foreach path : paths
    strings += '  CONFIG_MESON_SEARCH_PATH("@0@")'.format(path)
  endforeach
  if strings.length() > 0
    configuration.set('CONFIG_MESON_SEARCH_PATHS', ' \\\n'.join(strings))
    configuration.set('CONFIG_MESON_SEARCH_PATH_COUNT', paths.length())
  endif
endif

dependencies = [
  # Versions should match the version available in Debian Trixie (13)
  dependency('sdl3', version: '>=3.2.0'),
  dependency('boost', modules: [ 'iostreams' ], version: '>=1.83.0'),
  dependency('libwebp', version: '>=1.5.0'),
]
# These are header-only libraries required by the application.
# Like the proper dependencies they are required and are the versions from
# Debian Trixie
cxx.has_header('pcg_random.hpp', required: true, include_directories: header_only_includes)
cxx.has_header('pcg_extras.hpp', required: true, include_directories: header_only_includes)
cxx.has_header('pcg_uint128.hpp', required: true, include_directories: header_only_includes)
cxx.has_header('miniaudio.h', required: true, include_directories: header_only_includes)
includes = [
  header_only_includes,
  include_directories('include')
]
config = configuration_data()
config_header = configure_file(input: 'generated/include/config.hpp.in', output: '@BASENAME@',
                               configuration: configuration)
build_header = custom_target('build.hpp', output: 'build.hpp', input: 'generated/include/build.hpp.in',
                             command: [ build_hpp_py, '@INPUT@', '@OUTPUT@' ], build_always_stale: true)
sources = [
  config_header,
  build_header,
  files('src/main.cpp'),
  files('src/rats/error.cpp', 'src/rats/renderer.cpp', 'src/rats/logger.cpp', 'src/rats/timer.cpp',
        'src/rats/world.cpp', 'src/rats/assets.cpp', 'src/rats/stopwatch.cpp', 'src/rats/assets.cpp',
        'src/rats/graphics.cpp', 'src/rats/math.cpp', 'src/rats/entity.cpp', 'src/rats/sprite.cpp',
        'src/rats/geometry.cpp'),
  files('src/glad/gl.c'),
  files('src/miniaudio/miniaudio.c'),
  files('src/rats/sdl3/library.cpp', 'src/rats/sdl3/window.cpp', 'src/rats/sdl3/mixer.cpp')
]
name = '@0@.@1@.@2@.@3@.@4@'.format(meson.project_name(), host_machine.cpu_family(), version['major'],
                                    version['minor'], version['patch'])
rats_exe = executable(name, sources, include_directories: includes, dependencies: dependencies, install: true,
                      install_dir: libexec / dirname, win_subsystem: 'windows')
if host_machine.system() == 'linux'
  waitkey_exe = executable('waitkey', [ files('src/waitkey.cpp'), config_header, build_header ], install: true,
                           install_dir: libexec / dirname)
  fs.copyfile('libexec/run-and-wait.sh', 'run-and-wait.sh', install: true, install_dir: libexec / dirname)
  custom_target('rats.desktop', output: '@BASENAME@', input: files('generated/share/rats.desktop.in'),
                command: [ desktop_files_py, '@INPUT@', '@OUTPUT@', libexec / dirname / name, libexec / dirname / name,
                           '@0@.@1@.@2@'.format(version['major'], version['minor'], version['patch']) ],
                build_by_default: true, build_subdir: 'share', install: true, install_dir: share / 'applications')
  custom_target('rats-debug.desktop', output: '@BASENAME@', input: files('generated/share/rats-debug.desktop.in'),
                command: [ desktop_files_py, '@INPUT@', '@OUTPUT@',
                           libexec / dirname / 'run-and-wait.sh',
                           libexec / dirname / 'run-and-wait.sh @0@'.format(libexec / dirname / name),
                           '@0@.@1@.@2@'.format(version['major'], version['minor'], version['patch']) ],
                build_by_default: true, build_subdir: 'share', install: true, install_dir: share / 'applications')
  os_release = run_command([ 'sh', '-c', '. /etc/os-release && printf "%s" "$ID"' ], check: false)
  is_debian = os_release.returncode() == 0 and os_release.stdout().strip() == 'debian'
  if is_debian
    custom_target('changelog', output: 'changelog.Debian.gz', input: files('packaging/debian/changelog'),
                  command: [ find_program('gzip'), '-9', '-ncS', '.Debian.gz', '@INPUT@' ], capture: true,
                  install: true, build_by_default: true, install_dir: share / 'doc' / dirname)
    install_data(files('packaging/debian/copyright'), install_dir: share / 'doc' / dirname)
  endif
elif host_machine.system() == 'windows'
  name = '@0@-debug.@1@.@2@.@3@.@4@'.format(meson.project_name(), host_machine.cpu_family(), version['major'],
                                            version['minor'], version['patch'])
  executable(name, sources, include_directories: includes, dependencies: dependencies, install: true,
             install_dir: libexec / dirname, win_subsystem: 'console')
endif
subdir('assets')
