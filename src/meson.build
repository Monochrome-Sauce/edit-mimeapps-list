momuma_sources = files(
	'main.cpp',
	'widget.cpp',
)

if not is_debug_build
	extra_flags += [
		'-DG_DISABLE_ASSERT=1',
		'-DG_DISABLE_CHECKS=1',
		'-DNDEBUG',
	]
endif
