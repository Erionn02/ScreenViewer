#include "MouseConfig.hpp"


namespace Mouse {
    int extractButtonID(int button_mask) {
        [[likely]] if (button_mask & LEFT_BUTTON_MASK) return LEFT_BUTTON;
        if (button_mask & MID_BUTTON_MASK) return MID_BUTTON;
        if (button_mask & RIGHT_BUTTON_MASK) return RIGHT_BUTTON;
        if (button_mask & SCROLL_UP_MASK) return SCROLL_UP;
        if (button_mask & SCROLL_DOWN_MASK) return SCROLL_DOWN;
        if (button_mask & FORWARD_BUTTON_MASK) return FORWARD_BUTTON;
        if (button_mask & BACK_BUTTON_MASK) return BACK_BUTTON;
        return 0;
    }

    int convertToMask(std::uint8_t sdl_button_id) {
        switch (sdl_button_id) {
            case 4:
                return BACK_BUTTON_MASK;
            case 5:
                return FORWARD_BUTTON_MASK;
            [[likely]] default:
                return 1 << (sdl_button_id - 1);
        }
    }

    bool isClicked(int mask) {
        return mask & IS_CLICKED_MASK;
    }

    bool isScrollMoved(int mask) {
        return mask & SCROLL_UP_MASK || mask & SCROLL_DOWN_MASK;
    }

    void setClicked(int &mask) {
        mask |= IS_CLICKED_MASK;
    }
}

