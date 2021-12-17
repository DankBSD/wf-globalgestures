#define WAYFIRE_PLUGIN
#define WLR_USE_UNSTABLE

#include <wayfire/core.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>
extern "C" {
#include <wlr/types/wlr_seat.h>
}
#include <wayfire/output.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/singleton-plugin.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/workspace-manager.hpp>
#include "pointer-gestures-unstable-v1-server-protocol.h"
#include "wfp-global-gestures-unstable-v1-server-protocol.h"

struct global_gesture_handler {
	uint32_t fingers;
	struct wl_resource *surface;
	struct wl_client *client;
	bool started = false;

	template <class wlr_event>
	using event = wf::input_event_signal<wlr_event>;

	inline uint32_t serial() {
		return wlr_seat_client_next_serial(
		    wlr_seat_client_for_wl_client(wf::get_core().get_current_seat(), client));
	}

	wf::signal_connection_t on_swipe_begin = [=](wf::signal_data_t *data) {
		auto ev = static_cast<event<wlr_event_pointer_swipe_begin> *>(data)->event;
		if (static_cast<int>(ev->fingers) != fingers) return;
		started = true;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->swipes) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_swipe_v1_send_begin(gesture, serial(), ev->time_msec, surface,
			                                        ev->fingers);
		}
	};

	wf::signal_connection_t on_swipe_update = [=](wf::signal_data_t *data) {
		if (!started) return;
		auto ev = static_cast<event<wlr_event_pointer_swipe_update> *>(data)->event;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->swipes) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_swipe_v1_send_update(gesture, ev->time_msec,
			                                         wl_fixed_from_double(ev->dx),
			                                         wl_fixed_from_double(ev->dy));
		}
	};

	wf::signal_connection_t on_swipe_end = [=](wf::signal_data_t *data) {
		if (!started) return;
		auto ev = static_cast<event<wlr_event_pointer_swipe_end> *>(data)->event;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->swipes) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_swipe_v1_send_end(gesture, serial(), ev->time_msec,
			                                      ev->cancelled);
		}
	};

	wf::signal_connection_t on_pinch_begin = [=](wf::signal_data_t *data) {
		auto ev = static_cast<event<wlr_event_pointer_pinch_begin> *>(data)->event;
		if (static_cast<int>(ev->fingers) != fingers) return;
		started = true;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->pinches) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_pinch_v1_send_begin(gesture, serial(), ev->time_msec, surface,
			                                        ev->fingers);
		}
	};

	wf::signal_connection_t on_pinch_update = [=](wf::signal_data_t *data) {
		if (!started) return;
		auto ev = static_cast<event<wlr_event_pointer_pinch_update> *>(data)->event;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->pinches) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_pinch_v1_send_update(
			    gesture, ev->time_msec, wl_fixed_from_double(ev->dx),
			    wl_fixed_from_double(ev->dy), wl_fixed_from_double(ev->scale),
			    wl_fixed_from_double(ev->rotation));
		}
	};

	wf::signal_connection_t on_pinch_end = [=](wf::signal_data_t *data) {
		if (!started) return;
		auto ev = static_cast<event<wlr_event_pointer_pinch_end> *>(data)->event;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->pinches) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_pinch_v1_send_end(gesture, serial(), ev->time_msec,
			                                      ev->cancelled);
		}
	};

	wf::signal_connection_t on_hold_begin = [=](wf::signal_data_t *data) {
		auto ev = static_cast<event<wlr_event_pointer_hold_begin> *>(data)->event;
		if (static_cast<int>(ev->fingers) != fingers) return;
		started = true;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->holds) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_hold_v1_send_begin(gesture, serial(), ev->time_msec, surface,
			                                       ev->fingers);
		}
	};

	wf::signal_connection_t on_hold_end = [=](wf::signal_data_t *data) {
		if (!started) return;
		auto ev = static_cast<event<wlr_event_pointer_hold_end> *>(data)->event;
		struct wl_resource *gesture;
		wl_resource_for_each(gesture, &wf::get_core().protocols.pointer_gestures->holds) {
			if (wl_resource_get_client(gesture) != client) continue;
			zwp_pointer_gesture_hold_v1_send_end(gesture, serial(), ev->time_msec,
			                                     ev->cancelled);
		}
	};

	global_gesture_handler(uint32_t gesture, uint32_t _fingers,
	                       struct wl_resource *_surface)
	    : fingers(_fingers), surface(_surface) {
		client = wl_resource_get_client(surface);
		switch (gesture) {
			case ZWFP_GLOBAL_GESTURE_MANAGER_V1_GESTURE_SWIPE:
				wf::get_core().connect_signal("pointer_swipe_begin", &on_swipe_begin);
				wf::get_core().connect_signal("pointer_swipe_update", &on_swipe_update);
				wf::get_core().connect_signal("pointer_swipe_end", &on_swipe_end);
				break;
			case ZWFP_GLOBAL_GESTURE_MANAGER_V1_GESTURE_PINCH:
				wf::get_core().connect_signal("pointer_pinch_begin", &on_pinch_begin);
				wf::get_core().connect_signal("pointer_pinch_update", &on_pinch_update);
				wf::get_core().connect_signal("pointer_pinch_end", &on_pinch_end);
				break;
			case ZWFP_GLOBAL_GESTURE_MANAGER_V1_GESTURE_HOLD:
				wf::get_core().connect_signal("pointer_hold_begin", &on_hold_begin);
				wf::get_core().connect_signal("pointer_hold_end", &on_hold_end);
				break;
			default:
				LOGE("Unknown gesture ", gesture);
				break;
		};
	};
};

static void handle_destroy(wl_resource *resource) {
	auto *handler =
	    static_cast<global_gesture_handler *>(wl_resource_get_user_data(resource));
	if (!handler) return;
	delete handler;
	wl_resource_set_user_data(resource, nullptr);
}

static void destroy_impl(wl_client *client, wl_resource *resource) {
	wl_resource_destroy(resource);
};

const struct zwfp_global_gesture_v1_interface zwfp_global_gesture_v1_impl = {
    .destroy = destroy_impl};

static void request_global_gesture_impl(wl_client *client, wl_resource *resource,
                                        uint32_t gesture, uint32_t fingers,
                                        wl_resource *surface, uint32_t global_gesture) {
	auto handler_res =
	    wl_resource_create(client, &zwfp_global_gesture_v1_interface, 1, global_gesture);
	auto *handler = new global_gesture_handler(gesture, fingers, surface);
	wl_resource_set_implementation(handler_res, &zwfp_global_gesture_v1_impl, handler,
	                               handle_destroy);
};

const struct zwfp_global_gesture_manager_v1_interface
    zwfp_global_gesture_manager_v1_impl = {.request_global_gesture =
                                               request_global_gesture_impl};

void bind_zwfp_global_gesture_manager_v1(wl_client *client, void *data, uint32_t version,
                                         uint32_t id) {
	auto resource =
	    wl_resource_create(client, &zwfp_global_gesture_manager_v1_interface, 1, id);
	wl_resource_set_implementation(resource, &zwfp_global_gesture_manager_v1_impl, NULL,
	                               NULL);
}

struct globalgestures_manager_owner : public wf::custom_data_t {
	wl_global *manager;
	globalgestures_manager_owner() {
		manager = wl_global_create(wf::get_core().display,
		                           &zwfp_global_gesture_manager_v1_interface, 1, nullptr,
		                           bind_zwfp_global_gesture_manager_v1);

		if (!manager) LOGE("Failed to create interface");
	}

	// TODO: gather all objects, destroy here, then we can be unloadable
	~globalgestures_manager_owner() {
		if (manager) wl_global_destroy(manager);
	}
};

struct wayfire_globalgestures
    : public wf::singleton_plugin_t<globalgestures_manager_owner, false> {};

DECLARE_WAYFIRE_PLUGIN(wayfire_globalgestures);
