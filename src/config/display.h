#pragma once
#include <Arduino.h>

namespace display_config
{
    // Colors (RGB565)
    constexpr uint16_t COLOR_BACKGROUND      = 0x0000; // black
    constexpr uint16_t COLOR_TITLE           = 0xFFFF; // white
    constexpr uint16_t COLOR_BUTTON          = 0x39C7; // slate gray-blue
    constexpr uint16_t COLOR_BUTTON_SELECTED = 0x0668; // eco green
    constexpr uint16_t COLOR_BUTTON_BORDER   = 0xFFFF; // white
    constexpr uint16_t COLOR_BUTTON_TEXT     = 0xFFFF; // white

    // Screen rotation passed to tft.setRotation()
    // 0/2 = portrait, 1/3 = landscape (flip 1<->3 if orientation is upside down)
    constexpr uint8_t SCREEN_ROTATION = 1;

    // Title
    constexpr uint16_t TITLE_Y = 24;
    constexpr uint8_t TITLE_TEXT_SIZE = 2;

    // Buttons
    constexpr uint8_t BUTTON_COUNT       = 2;
    constexpr uint16_t BUTTON_WIDTH      = 220;
    constexpr uint16_t BUTTON_HEIGHT     = 55;
    constexpr uint16_t BUTTON_SPACING    = 18;
    constexpr uint16_t BUTTON_TOP_MARGIN = 70; // vertical space reserved for the title
    constexpr uint8_t BUTTON_CORNER_RADIUS = 8;
    constexpr uint8_t BUTTON_TEXT_SIZE = 2;

    // Sub-screen (UI 1 / UI 2 / UI 3) chrome
    constexpr uint16_t SUBSCREEN_TITLE_Y = 30;
    constexpr uint8_t SUBSCREEN_TITLE_TEXT_SIZE = 3;

    constexpr uint16_t SUBSCREEN_PLACEHOLDER_Y = 110;
    constexpr uint8_t SUBSCREEN_PLACEHOLDER_TEXT_SIZE = 2;
    constexpr uint16_t COLOR_SUBSCREEN_PLACEHOLDER = 0x8410; // mid gray

    // Back button (shown on every sub-screen, anchored near the bottom)
    constexpr uint16_t BACK_BUTTON_WIDTH  = 140;
    constexpr uint16_t BACK_BUTTON_HEIGHT = 50;
    constexpr uint16_t BACK_BUTTON_BOTTOM_MARGIN = 30; // gap from bottom edge of screen
    constexpr uint16_t COLOR_BACK_BUTTON = 0x7800; // dark red-ish, visually distinct from menu buttons
}