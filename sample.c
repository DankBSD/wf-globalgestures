#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include "pointer-gestures-unstable-v1-client-protocol.h"
#include "wfp-global-gestures-unstable-v1-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

static bool active = false;
static uint32_t width, height;
static double pinch_scale = 0.0;
static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_seat *seat;
static struct wl_shm *shm;
static struct wl_pointer *pointer;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct zwlr_layer_surface_v1 *layer_surface;
static struct zwp_pointer_gestures_v1 *pointer_gestures;
static struct zwp_pointer_gesture_pinch_v1 *pinch;
static struct zwfp_global_gesture_manager_v1 *global_gesture_manager;
static struct zwfp_global_gesture_v1 *global_gesture;
static struct wl_output *output;
static struct wl_surface *surface;

struct buffer {
	struct wl_buffer *buffer;
	void *shm_data;
};

static void create_shm_buffer(struct buffer *buffer) {
	uint32_t stride = width * 4;
	off_t size = stride * height;

	int fd = memfd_create("weston-shared", MFD_CLOEXEC | MFD_ALLOW_SEALING);
	if (fd < 0) {
		fprintf(stderr, "memfd_create: %s\n", strerror(errno));
		exit(-1);
	}

	int ret = 0;
	do {
		ret = posix_fallocate(fd, 0, size);
	} while (ret == EINTR);
	if (ret != 0) {
		fprintf(stderr, "posix_fallocate: %s\n", strerror(errno));
		exit(-1);
	}

	buffer->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buffer->shm_data == MAP_FAILED) {
		fprintf(stderr, "mmap: %s\n", strerror(errno));
		exit(-1);
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
	buffer->buffer =
	    wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
}

static void paint(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	struct buffer buffer;
	create_shm_buffer(&buffer);
	uint8_t *buf = buffer.shm_data;
	for (size_t i = 0; i < width * height * 4; i += 4) {
		buf[i] = b;
		buf[i + 1] = g;
		buf[i + 2] = r;
		buf[i + 3] = a;
	}
	wl_surface_attach(surface, buffer.buffer, 0, 0);
	wl_surface_damage(surface, 0, 0, width, height);
	wl_surface_commit(surface);
	wl_buffer_destroy(buffer.buffer);
}

static void set_input_region(bool on) {
	if (on) {
		wl_surface_set_input_region(surface, NULL);
	} else {
		struct wl_region *reg = wl_compositor_create_region(compositor);
		wl_surface_set_input_region(surface, reg);
		wl_region_destroy(reg);
	}
}

double color = 0.69;

static void apply_active() {
	set_input_region(active);
	if (active)
		paint(color * 255, color * 255, color * 255, 0xff);
	else
		paint(0, 0, 0, 0);
}

inline double clamp(double d, double min, double max) {
	const double t = d < min ? min : d;
	return t > max ? max : t;
}

static void pinch_begin(void *data, struct zwp_pointer_gesture_pinch_v1 *gesture,
                        uint32_t serial, uint32_t time, struct wl_surface *surface,
                        uint32_t fingers) {}

static void pinch_update(void *data, struct zwp_pointer_gesture_pinch_v1 *gesture,
                         uint32_t time, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale,
                         wl_fixed_t rotation) {
	pinch_scale = wl_fixed_to_double(scale);
	// printf("%lf\n", pinch_scale);
	double alpha = active ? 1.0 - (clamp(pinch_scale, 1.0, 2.0) - 1.0)
	                      : 1.0 - clamp(pinch_scale, 0.0, 1.0);

	paint(color * alpha * 255, color * alpha * 255, color * alpha * 255, alpha * 255);
}

static void pinch_end(void *data, struct zwp_pointer_gesture_pinch_v1 *gesture,
                      uint32_t serial, uint32_t time, int32_t cancelled) {
	if (!cancelled) {
		if (active && pinch_scale > 1.3) {
			active = false;
		} else if (!active && pinch_scale < 0.69) {
			active = true;
		}
	}
	apply_active();
}

static struct zwp_pointer_gesture_pinch_v1_listener pinch_listener = {
    .begin = pinch_begin,
    .update = pinch_update,
    .end = pinch_end,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
                                     enum wl_seat_capability caps) {
	if ((caps & WL_SEAT_CAPABILITY_POINTER)) {
		pointer = wl_seat_get_pointer(wl_seat);
		// wl_pointer_add_listener(pointer, &pointer_listener, NULL);
		pinch = zwp_pointer_gestures_v1_get_pinch_gesture(pointer_gestures, pointer);
		zwp_pointer_gesture_pinch_v1_add_listener(pinch, &pinch_listener, NULL);
	} else {
		fprintf(stderr, "No pointer on seat!\n");
		exit(1);
	}
}

static void seat_handle_name(void *_, struct wl_seat *__, const char *___) {}

const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name = seat_handle_name,
};

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface,
                                    uint32_t serial, uint32_t w, uint32_t h) {
	width = w;
	height = h;
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	paint(0, 0, 0, 0);
	wl_display_roundtrip(display);
}

static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface) {
	exit(1);
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

static void handle_global(void *data, struct wl_registry *registry, uint32_t name,
                          const char *interface, uint32_t version) {
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, "wl_output") == 0) {
		if (!output) {
			output = wl_registry_bind(registry, name, &wl_output_interface, 1);
		}
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener(seat, &seat_listener, NULL);
	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface,
		                               version < 4 ? version : 4);
	} else if (strcmp(interface, zwp_pointer_gestures_v1_interface.name) == 0) {
		pointer_gestures = wl_registry_bind(
		    registry, name, &zwp_pointer_gestures_v1_interface, version < 3 ? version : 3);
	} else if (strcmp(interface, zwfp_global_gesture_manager_v1_interface.name) == 0) {
		global_gesture_manager =
		    wl_registry_bind(registry, name, &zwfp_global_gesture_manager_v1_interface, 1);
	}
}

static void handle_global_remove(void *_, struct wl_registry *__, uint32_t ___) {}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

int main(int argc, char **argv) {
	display = wl_display_connect(NULL);
	if (!display) {
		fprintf(stderr, "Failed to connect to the wayland socket!\n");
		return 1;
	}

	auto *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	assert(compositor);
	assert(output);
	assert(shm);
	if (!layer_shell) {
		fprintf(stderr, "No layer shell!\n");
		return 1;
	}
	if (!pointer_gestures) {
		fprintf(stderr, "No pointer gestures!\n");
		return 1;
	}
	if (!global_gesture_manager) {
		fprintf(stderr, "No global gesture manager!\n");
		return 1;
	}

	surface = wl_compositor_create_surface(compositor);
	assert(surface);

	layer_surface = zwlr_layer_shell_v1_get_layer_surface(layer_shell, surface, output,
	                                                      ZWLR_LAYER_SHELL_V1_LAYER_TOP,
	                                                      "global gestures sample");
	assert(layer_surface);
	zwlr_layer_surface_v1_set_size(layer_surface, 0, 0);
	zwlr_layer_surface_v1_set_anchor(
	    layer_surface,
	    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
	        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
	zwlr_layer_surface_v1_set_keyboard_interactivity(
	    layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
	zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
	                                   layer_surface);
	set_input_region(false);
	global_gesture = zwfp_global_gesture_manager_v1_request_global_gesture(
	    global_gesture_manager, ZWFP_GLOBAL_GESTURE_MANAGER_V1_GESTURE_PINCH, 4, surface);
	wl_surface_commit(surface);
	wl_display_roundtrip(display);

	fprintf(stderr, "Now do a 4-finger pinch!\n");
	while (wl_display_dispatch(display) != -1)
		;
	return 0;
}
