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

//static void demo_handle_xcb_event(struct demo *demo, const xcb_generic_event_t *event);
//static void demo_run_xcb(struct demo *demo);
//static void demo_create_xcb_window(struct demo *demo);

#include "cube.c"

/*
void init(demo *demo) {
	demo->connection = xcb_connect(NULL, &wm->mainscreen);
	if(xcb_connection_has_error(wm->conn)) exit(1);
	wm->screen = xcb_setup_roots_iterator(xcb_get_setup(wm->conn)).data;
	wm->rootwin = wm->screen->root;

	VkResult U_ASSERT_ONLY err;

	VkXcbSurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.connection = wm->conn;
	createInfo.window = wm->rootwin;

	err = vkCreateXcbSurfaceKHR(
		demo->inst, &createInfo, NULL, &wm->surface);

}
*/


static void demo_init_connection(struct demo *demo) {
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    const char *display_envar = getenv("DISPLAY");
    if (display_envar == NULL || display_envar[0] == '\0') {
        printf("Environment variable DISPLAY requires a valid value.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    demo->connection = xcb_connect(NULL, &scr);
    if (xcb_connection_has_error(demo->connection) > 0) {
        printf("Cannot find a compatible Vulkan installable client driver (ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&iter);

    demo->screen = iter.data;
}

static void demo_handle_xcb_event(struct demo *demo, const xcb_generic_event_t *event) {
    uint8_t event_code = event->response_type & 0x7f;
    switch (event_code) {
        case XCB_EXPOSE:
            // TODO: Resize window
            break;
        case XCB_CLIENT_MESSAGE:
            if ((*(xcb_client_message_event_t *)event).data.data32[0] == (*demo->atom_wm_delete_window).atom) {
                demo->quit = true;
            }
            break;
        case XCB_KEY_RELEASE: {
            const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)event;

            switch (key->detail) {
                case 0x9:  // Escape
                    demo->quit = true;
                    break;
                case 0x71:  // left arrow key
                    demo->spin_angle -= demo->spin_increment;
                    break;
                case 0x72:  // right arrow key
                    demo->spin_angle += demo->spin_increment;
                    break;
                case 0x41:  // space bar
                    demo->pause = !demo->pause;
                    break;
            }
        } break;
        case XCB_CONFIGURE_NOTIFY: {
            const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)event;
            if ((demo->width != cfg->width) || (demo->height != cfg->height)) {
                demo->width = cfg->width;
                demo->height = cfg->height;
                demo_resize(demo);
            }
        } break;
        default:
            break;
    }
}

static void demo_run_xcb(struct demo *demo) {
    xcb_flush(demo->connection);

    while (!demo->quit) {
        xcb_generic_event_t *event;

        if (demo->pause) {
            event = xcb_wait_for_event(demo->connection);
        } else {
            event = xcb_poll_for_event(demo->connection);
        }
        while (event) {
            demo_handle_xcb_event(demo, event);
            free(event);
            event = xcb_poll_for_event(demo->connection);
        }

        demo_draw(demo);
        demo->curFrame++;
        if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount) demo->quit = true;
    }
}


static void demo_create_xcb_window(struct demo *demo) {
    uint32_t value_mask, value_list[32];

	uint32_t evmask = XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_ENTER_WINDOW |
		XCB_EVENT_MASK_LEAVE_WINDOW |
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_PROPERTY_CHANGE |
		XCB_EVENT_MASK_VISIBILITY_CHANGE |
		XCB_EVENT_MASK_EXPOSURE;
	xcb_change_window_attributes(demo->connection,
			demo->screen->root, XCB_CW_EVENT_MASK, &evmask);

	// get overlay window
	xcb_composite_get_overlay_window_cookie_t cookie;
	xcb_composite_get_overlay_window_reply_t *reply;
	xcb_generic_error_t *error = NULL;
	cookie = xcb_composite_get_overlay_window(
				demo->connection, demo->screen->root);
	if(reply = xcb_composite_get_overlay_window_reply(demo->connection,
				cookie, &error)) {
		demo->xcb_window = reply->overlay_win;

		// disable input for the overlay window
		xcb_rectangle_t rectangle;
		rectangle.x = 0;
		rectangle.y = 0;
		rectangle.width = 0;
		rectangle.height = 0;
		xcb_xfixes_region_t xfixesRegion = xcb_generate_id(
			demo->connection);
		xcb_xfixes_create_region(demo->connection,
				xfixesRegion, 1, &rectangle);
		xcb_xfixes_set_window_shape_region (demo->connection,
				reply->overlay_win, 2, 0, 0, xfixesRegion);
		xcb_xfixes_destroy_region(demo->connection, xfixesRegion);
		xcb_flush(demo->connection);

		free(reply);
		free(error);
	} else {
		exit(-25);
	}

//	demo->xcb_window = demo->screen->root;


#if 0
    demo->xcb_window = xcb_generate_id(demo->connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = demo->screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(demo->connection, XCB_COPY_FROM_PARENT, demo->xcb_window, demo->screen->root, 0, 0, demo->width, demo->height,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, demo->screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(demo->connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(demo->connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(demo->connection, 0, 16, "WM_DELETE_WINDOW");
    demo->atom_wm_delete_window = xcb_intern_atom_reply(demo->connection, cookie2, 0);

    xcb_change_property(demo->connection, XCB_PROP_MODE_REPLACE, demo->xcb_window, (*reply).atom, 4, 32, 1,
                        &(*demo->atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(demo->connection, demo->xcb_window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(demo->connection, demo->xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
#endif
}

int main(int argc, char **argv, char **envp) {
	struct demo demo;
	demo_init(&demo, argc, argv);

	demo_create_xcb_window(&demo);

	demo_init_vk_swapchain(&demo);

	demo_prepare(&demo);

	demo_run_xcb(&demo);

	demo_cleanup(&demo);

	return validation_error;
}
