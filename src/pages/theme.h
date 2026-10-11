#pragma once
#include <stdint.h>

namespace pages
{
    namespace theme
    {
        // RGB888 -> RGB565 at compile time
        constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b)
        {
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        constexpr uint16_t kBg = rgb(20, 36, 28);       // deep forest
        constexpr uint16_t kCream = rgb(240, 230, 205); // raw cotton / silk
        constexpr uint16_t kSage = rgb(150, 185, 140);
        constexpr uint16_t kLeafDark = rgb(46, 94, 60);
        constexpr uint16_t kLeafMid = rgb(84, 140, 82);
        constexpr uint16_t kTerracotta = rgb(204, 112, 70);
        constexpr uint16_t kOchre = rgb(214, 170, 80);
        constexpr uint16_t kCardIdle = rgb(30, 52, 40); // unselected card fill
    } // namespace theme
} // namespace pages