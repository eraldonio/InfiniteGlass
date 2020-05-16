#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/composite.h>
#include <xcb/xfixes.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h> // exit
#include <stdio.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#ifndef NDEBUG
FILE *dlog_fp; // the file to save the debug log records to
#define dlog(...) fprintf(dlog_fp, __VA_ARGS__); fflush(dlog_fp)
#define dassert(expr)							\
  ((void) sizeof ((expr) ? 1 : 0), __extension__ ({			\
      if (expr)								\
        ;							\
      else { \
	dlog("Assert failed\n"); \
        assert(expr);	\
	} \
    }))
#else
#define dlog(...) ((void)0)
#define dassert(ignore) ((void)0)
#endif // NDEBUG

#include "cube.c"

static void wm_init_connection(struct Wm *wm) {
	const xcb_setup_t *setup;
	xcb_screen_iterator_t iter;
	int scr;

	const char *display_envar = getenv("DISPLAY");
	if(display_envar == NULL || display_envar[0] == '\0') {
			printf("Environment variable DISPLAY"
				" requires a valid value.\nExiting ...\n");
		fflush(stdout);
		exit(1);
	}

	wm->connection = xcb_connect(NULL, &scr);
	if(xcb_connection_has_error(wm->connection) > 0) {
		printf("Cannot find a compatible Vulkan installable"
			" client driver (ICD).\nExiting ...\n");
		fflush(stdout);
		exit(1);
	}

	setup = xcb_get_setup(wm->connection);
	iter = xcb_setup_roots_iterator(setup);
	while(scr-- > 0) xcb_screen_next(&iter);

	wm->screen = iter.data;
}

static void wm_handle_xcb_event(struct Wm *wm,
					const xcb_generic_event_t *event) {
	uint8_t event_code = event->response_type &0x7f;
	switch(event_code) {
	case XCB_EXPOSE:
		dlog("XCB_EXPOSE event\n");
		// TODO: Resize window
		break;
	case XCB_CLIENT_MESSAGE:
		dlog("XCB_CLIENT_MESSAGE event\n");
		break;
//		if((*(xcb_client_message_event_t *)event).data.data32[0] ==
//				(*wm->atom_wm_delete_window).atom) {
//			wm->quit = true;
//		}
//		break;
	case XCB_KEY_RELEASE: {
		dlog("XCB_KEY_RELEASE event\n");
		const xcb_key_release_event_t *key =
				(const xcb_key_release_event_t *)event;

		switch (key->detail) {
		case 0x9:  // Escape
			wm->quit = true;
			break;
		case 0x71:  // left arrow key
			wm->spin_angle -= wm->spin_increment;
			break;
		case 0x72:  // right arrow key
			wm->spin_angle += wm->spin_increment;
			break;
		case 0x41:  // space bar
			wm->pause = !wm->pause;
			break;
		}
	} break;
	case XCB_CONFIGURE_NOTIFY: {
		dlog("XCB_CONFIGURE_NOTIFY event\n");
		break;
		const xcb_configure_notify_event_t *cfg =
				(const xcb_configure_notify_event_t *)event;
		if((wm->width != cfg->width) ||
				(wm->height != cfg->height)) {
			wm->width = cfg->width;
			wm->height = cfg->height;
			wm_resize(wm);
		}
	} break;
	default:
		break;
	}
}

static void wm_run_xcb(struct Wm *wm) {
	xcb_flush(wm->connection);

	while(!wm->quit) {
		xcb_generic_event_t *event;

		if(wm->pause) {
			event = xcb_wait_for_event(wm->connection);
		} else {
			event = xcb_poll_for_event(wm->connection);
		}
		while(event) {
			wm_handle_xcb_event(wm, event);
			free(event);
			event = xcb_poll_for_event(wm->connection);
		}

		wm_draw(wm);
		++wm->curFrame;
	}
}


static void wm_create_xcb_window(struct Wm *wm) {
	uint32_t value_mask, value_list[32];

	uint32_t evmask = XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_ENTER_WINDOW |
		XCB_EVENT_MASK_LEAVE_WINDOW |
//		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_PROPERTY_CHANGE |
		XCB_EVENT_MASK_VISIBILITY_CHANGE |
		XCB_EVENT_MASK_EXPOSURE;
	xcb_change_window_attributes(wm->connection,
			wm->screen->root, XCB_CW_EVENT_MASK, &evmask);

	// get overlay window
	xcb_composite_get_overlay_window_cookie_t cookie;
	xcb_composite_get_overlay_window_reply_t *reply;
	xcb_generic_error_t *error = NULL;
	cookie = xcb_composite_get_overlay_window(
			wm->connection, wm->screen->root);
	if(reply = xcb_composite_get_overlay_window_reply(wm->connection,
			cookie, &error)) {
		wm->xcb_window = reply->overlay_win;

		// disable input for the overlay window
		xcb_rectangle_t rectangle;
		rectangle.x = 0;
		rectangle.y = 0;
		rectangle.width = 0;
		rectangle.height = 0;
		xcb_xfixes_region_t xfixesRegion = xcb_generate_id(
				wm->connection);
		xcb_xfixes_create_region(wm->connection,
				xfixesRegion, 1, &rectangle);
		xcb_xfixes_set_window_shape_region(wm->connection,
				reply->overlay_win, 2, 0, 0, xfixesRegion);
		xcb_xfixes_destroy_region(wm->connection, xfixesRegion);
		xcb_flush(wm->connection);

		free(reply);
		free(error);
	} else {
		dlog("Could not get overlay window. Exiting.\n");
		exit(-25);
	}
}

int main(int argc, char **argv, char **envp) {
#ifndef NDEBUG
	dlog_fp = fopen("/tmp/wm.log", "w"); // open the debug log file
	assert(dlog_fp);
#endif //NDEBUG

	dlog("Starting glass-vulkan renderer\n");

	struct Wm wm;
	system("xterm -geometry +0+18 &");
	signal(SIGCHLD, SIG_IGN);

	dlog("entering wm_init\n");

	wm_init(&wm, argc, argv);

	dlog("entering wm_create_xcb_window\n");

	wm_create_xcb_window(&wm);

	dlog("entering wm_init_vk_swapchain\n");

	wm_init_vk_swapchain(&wm);

	dlog("entering wm_prepare\n");

	wm_prepare(&wm);

	dlog("entering wm_run_xcb\n");

	wm_run_xcb(&wm);

	dlog("entering wm_cleanup\n");

	wm_cleanup(&wm);

#ifndef NDEBUG
	fclose(dlog_fp); // close the debug log file
#endif //NDEBUG

	return validation_error;
}
