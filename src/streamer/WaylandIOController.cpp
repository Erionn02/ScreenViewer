#include "streamer/WaylandIOController.hpp"

#include "LamdaToFuncPtr.hpp"

#include <iostream>


WaylandIOController::WaylandIOController(): seat_listener(createListener()) {

}

void WaylandIOController::handleKeyboardEvent(KeyboardEventData event) {
    struct wl_display *display;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);

    if (!keyboard) {
        std::cerr << "Failed to get keyboard from seat" << std::endl;
        return;
    }

    uint32_t state = event.down ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED;

    // Simulate key press/release
    wl_keyboard_send_key(keyboard, 0, 0, event.key, state);
    wl_display_flush(display);

    // Cleanup
    wl_keyboard_release(keyboard);}

void WaylandIOController::handleMouseEvent(MouseEventData event) {
    (void) event;
}

cv::Mat WaylandIOController::captureScreenshot() {
    return cv::Mat();
}

wl_seat_listener WaylandIOController::createListener() {
    wl_seat_listener listener_{};
    listener_.capabilities = lambdaToPointer([this](void *data, struct wl_seat *seat, uint32_t caps){
        seat_handle_capabilities(data, seat, caps);
    });
    return listener_;
}

void WaylandIOController::seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps) {
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        // Simulate key press and release events
        struct wl_display *display = (struct wl_display *)data;

        KeyboardEventData press_event{true, KEY_A}; // Simulate 'A' key press
        wl_pointer_send_motion(display, seat, press_event);

        sleep(1); // Wait for 1 second

        KeyboardEventData release_event{false, KEY_A}; // Simulate 'A' key release
        wl_pointer_send_motion(display, seat, release_event);
    }

}
