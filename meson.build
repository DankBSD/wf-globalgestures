project('wf-globalgestures', ['c', 'cpp'], version: '0.0.1', license: 'Unlicense',
	default_options: ['c_std=c11', 'cpp_std=c++17', 'warning_level=2'])

wayfire = dependency('wayfire')
wlroots = dependency('wlroots', fallback: ['wlroots', 'wlroots'])
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.14')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')

gen_code = generator(wayland_scanner,
	output: '@BASENAME@-protocol.c',
	arguments: ['private-code', '@INPUT@', '@OUTPUT@'])

gen_client_header = generator(wayland_scanner,
	output: '@BASENAME@-client-protocol.h',
	arguments: ['client-header', '@INPUT@', '@OUTPUT@'])

gen_server_header = generator(wayland_scanner,
	output: '@BASENAME@-server-protocol.h',
	arguments: ['server-header', '@INPUT@', '@OUTPUT@'])

pointer_gestures_code = gen_code.process(join_paths([wl_protocol_dir, 'unstable/pointer-gestures/pointer-gestures-unstable-v1.xml']))
global_gestures_code = gen_code.process('wfp-global-gestures-unstable-v1.xml')

plugin_globalgestures = shared_module('globalgestures',
	'globalgestures.cpp',
	pointer_gestures_code,
	gen_server_header.process(join_paths([wl_protocol_dir, 'unstable/pointer-gestures/pointer-gestures-unstable-v1.xml'])),
	global_gestures_code,
	gen_server_header.process('wfp-global-gestures-unstable-v1.xml'),
	dependencies: [wayfire, wlroots],
	install: true,
	install_dir: 'lib/wayfire')

sample = executable('global-gestures-sample',
	'sample.c',
	pointer_gestures_code,
	gen_client_header.process(join_paths([wl_protocol_dir, 'unstable/pointer-gestures/pointer-gestures-unstable-v1.xml'])),
	global_gestures_code,
	gen_client_header.process('wfp-global-gestures-unstable-v1.xml'),
	gen_code.process(join_paths([wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'])),
	gen_code.process('wlr-layer-shell-unstable-v1.xml'),
	gen_client_header.process('wlr-layer-shell-unstable-v1.xml'),
	dependencies: [wayland_client],
	install: false)
