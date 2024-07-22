#pragma once


namespace Mouse {
    constexpr int LEFT_BUTTON{1};
    constexpr int LEFT_BUTTON_MASK{1};

    constexpr int MID_BUTTON{2};
    constexpr int MID_BUTTON_MASK{1 << (MID_BUTTON-1)};

    constexpr int RIGHT_BUTTON{3};
    constexpr int RIGHT_BUTTON_MASK{1 << (RIGHT_BUTTON-1)};

    constexpr int SCROLL_UP{4};
    constexpr int SCROLL_UP_MASK{1 << (SCROLL_UP-1)};

    constexpr int SCROLL_DOWN{5};
    constexpr int SCROLL_DOWN_MASK{1 << (SCROLL_DOWN-1)};

    constexpr int IS_CLICKED_BIT{6};
    constexpr int IS_CLICKED_MASK{1 << (IS_CLICKED_BIT-1)};

    constexpr int MOVE{7};
    constexpr int MOVE_MASK{1 << (MOVE-1)};

    constexpr int FORWARD_BUTTON{8};
    constexpr int FORWARD_BUTTON_MASK{1 << (FORWARD_BUTTON-1)};

    constexpr int GO_BACK_BUTTON{9};
    constexpr int GO_BACK_BUTTON_MASK{1 << (GO_BACK_BUTTON-1)};
    
    
    int extractButtonID(int button_mask);
    int convertToMask(std::uint8_t button_id);
    bool isClicked(int mask);
    bool isScrollMoved(int mask);
    void setClicked(int& mask);
}