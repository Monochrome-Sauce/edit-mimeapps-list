project('momuma-gtk', 'cpp',
	default_options: [
		'b_sanitize=address,undefined',
		'buildtype=debug',
		'c_std=gnu17',
		'cpp_std=gnu++20',
		'optimization=s',
		'warning_level=2',
		'werror=true'
	],
	license : 'GPL-3.0-or-later',
	meson_version: '>= 0.61',
	version: '0.1.1',
)


################################################################################
# Version

version_array = meson.project_version().split('.')
version_major = version_array[0].to_int()
version_minor = version_array[1].to_int()
version_patch = version_array[2].to_int()


################################################################################
# Default paths

sys_prefix = get_option('prefix')
sys_bin_dir = sys_prefix / get_option('bindir')
sys_locale_dir = sys_prefix / get_option('localedir')
sys_data_dir = sys_prefix / get_option('datadir')
source_code_root = meson.current_source_dir()

message(f'System prefix = @sys_prefix@')
message(f'System locale dir = @sys_locale_dir@')
message(f'System data dir = @sys_data_dir@')
message(f'Source code dir = @source_code_root@')


################################################################################
# Options

cxx = meson.get_compiler('cpp')
is_debug_build = (get_option('buildtype') == 'debug')

message(f'Debug build = @is_debug_build@')


################################################################################
# Dependencies

asan_dep = cxx.find_library('asan')
ubsan_dep = cxx.find_library('ubsan')

#adwaita_dep = dependency('libadwaita-1', include_type: 'system', version: '>= 1.1')
giomm_dep = dependency('giomm-2.4', include_type: 'system', version: '>= 2.60')
glibmm_dep = dependency('glibmm-2.4', include_type: 'system', version: '>= 2.60')
gtkmm_dep = dependency('gtkmm-3.0', include_type: 'system', version: '>= 3.24')


################################################################################
# Compiler flags

extra_flags = [
	'-DFMT_USE_CONSTEXPR', # let fmt/spdlog functions be type-checked at compile-time
	'-DGDK_DISABLE_DEPRECATED',
	'-DGDKMM_DISABLE_DEPRECATED',
	'-DGIOMM_DISABLE_DEPRECATED',
	'-DGLIBMM_DISABLE_DEPRECATED',
	'-DGTK_DISABLE_DEPRECATED',
	'-DGTKMM_DISABLE_DEPRECATED',
	'-Wcast-align',
	'-Wcast-qual',
	'-Wchar-subscripts',
	'-Wconversion',
	'-Wdouble-promotion',
	'-Wformat=2',
	'-Wmissing-declarations',
	'-Wnull-dereference',
	'-Wold-style-cast',
	'-Woverflow',
	'-Wpointer-arith',
	'-Wshadow',
	'-Wsign-compare',
	'-Wundef',
	'-Wunused-result',
	'-Wunused-variable',
]

cxx_flags = extra_flags
add_project_arguments(cxx.get_supported_arguments(cxx_flags), language: 'cpp')


################################################################################
# Subdirectories

root_directory = include_directories('./')
include_directory = include_directories('./include/')

subdir('src')
executable(meson.project_name(),
	cpp_args: cxx_flags + extra_flags,
	dependencies: [
		asan_dep, ubsan_dep,
		giomm_dep, glibmm_dep, gtkmm_dep,
	],
	implicit_include_directories: false,
	include_directories: [ include_directory, root_directory ],
	install: true,
	sources: momuma_sources,
)
