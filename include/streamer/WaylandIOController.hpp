#pragma once

#include "IOController.hpp"

#include <wayland-client.h>
#include <linux/input-event-codes.h>
#include <unistd.h>
#include <wayland-server-protocol.h>


class WaylandIOController: public IOController {
public:
    WaylandIOController();

    void handleKeyboardEvent(KeyboardEventData event) override;
    void handleMouseEvent(MouseEventData event) override;
    cv::Mat captureScreenshot() override;

private:
    void seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps);
    wl_seat_listener createListener();

    const struct wl_seat_listener seat_listener;
};