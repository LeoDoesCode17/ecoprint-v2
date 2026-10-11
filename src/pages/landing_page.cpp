#include <Arduino.h>
#include "landing_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    LandingPage landing_page;

    namespace
    {
        // RGB888 -> RGB565 at compile time
        constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b)
        {
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Palette inspired by natural dyes and plant material
        constexpr uint16_t kBg = rgb(20, 36, 28);       // deep forest
        constexpr uint16_t kCream = rgb(240, 230, 205); // raw cotton / silk
        constexpr uint16_t kSage = rgb(150, 185, 140);
        constexpr uint16_t kLeafDark = rgb(46, 94, 60);
        constexpr uint16_t kLeafMid = rgb(84, 140, 82);
        constexpr uint16_t kTerracotta = rgb(204, 112, 70); // eucalyptus / madder tone
        constexpr uint16_t kOchre = rgb(214, 170, 80);      // marigold / turmeric tone
        constexpr uint16_t kSteam = rgb(190, 205, 200);

        constexpr int kPromptY = 276;
        constexpr unsigned long kBlinkMs = 700;

        // Draws a leaf whose base is at (bx, by), pointing along angleDeg
        // (0 = right, -90 = up). The body is a strip of triangles following a
        // leaf-shaped width profile, with a midrib and optional side veins.
        void drawLeaf(TFT_eSPI &tft, int bx, int by, float length, float width,
                      float angleDeg, uint16_t fill, uint16_t vein)
        {
            const float a = angleDeg * DEG_TO_RAD;
            const float ca = cosf(a), sa = sinf(a);
            constexpr int kSegments = 10;

            // (distance along leaf, offset from midrib) -> screen coordinates
            auto toScreen = [&](float along, float side, int &x, int &y)
            {
                x = bx + (int)lroundf(along * ca - side * sa);
                y = by + (int)lroundf(along * sa + side * ca);
            };
            // Widest near the base, tapering to a point at the tip
            auto halfWidth = [&](float t)
            {
                return 0.5f * width * sinf(PI * powf(t, 0.7f));
            };

            int lx0, ly0, rx0, ry0;
            toScreen(0, 0, lx0, ly0);
            rx0 = lx0;
            ry0 = ly0;

            for (int i = 1; i <= kSegments; i++)
            {
                float t = (float)i / kSegments;
                float hw = halfWidth(t);
                int lx1, ly1, rx1, ry1;
                toScreen(t * length, -hw, lx1, ly1);
                toScreen(t * length, hw, rx1, ry1);

                tft.fillTriangle(lx0, ly0, rx0, ry0, lx1, ly1, fill);
                tft.fillTriangle(rx0, ry0, lx1, ly1, rx1, ry1, fill);

                lx0 = lx1;
                ly0 = ly1;
                rx0 = rx1;
                ry0 = ry1;
            }

            // Midrib
            int sx, sy, ex, ey;
            toScreen(0, 0, sx, sy);
            toScreen(length * 0.95f, 0, ex, ey);
            tft.drawLine(sx, sy, ex, ey, vein);

            // Side veins (only on larger leaves)
            if (length >= 40)
            {
                for (int k = 1; k <= 4; k++)
                {
                    float t = 0.15f + 0.17f * k;
                    float t2 = min(t + 0.12f, 0.95f);
                    int mx, my, vx, vy;
                    toScreen(t * length, 0, mx, my);

                    toScreen(t2 * length, -halfWidth(t2) * 0.8f, vx, vy);
                    tft.drawLine(mx, my, vx, vy, vein);
                    toScreen(t2 * length, halfWidth(t2) * 0.8f, vx, vy);
                    tft.drawLine(mx, my, vx, vy, vein);
                }
            }
        }

        // A wavy wisp of steam that fades out as it rises
        void drawSteam(TFT_eSPI &tft, int x, int yBase, int height, float phase)
        {
            for (int j = 0; j < height; j++)
            {
                float p = (float)j / height;
                uint8_t alpha = (uint8_t)(200.0f * (1.0f - p));
                uint16_t c = tft.alphaBlend(alpha, kSteam, kBg);
                int dx = (int)lroundf(6.0f * sinf(j * 0.28f + phase));
                tft.fillCircle(x + dx, yBase - j, 1, c);
            }
        }

        // Horizontal vine with small alternating leaves, used as top/bottom border
        void drawVine(TFT_eSPI &tft, int y)
        {
            const uint16_t palette[] = {kLeafMid, kOchre, kTerracotta, kLeafMid, kSage};
            const int w = tft.width();

            tft.drawFastHLine(24, y, w - 48, kLeafDark);
            tft.drawFastHLine(24, y + 1, w - 48, kLeafDark);

            int i = 0;
            for (int x = 40; x < w - 30; x += 36, i++)
            {
                float angle = (i % 2 == 0) ? -50.0f : 50.0f;
                drawLeaf(tft, x, y, 20, 10, angle, palette[i % 5], kBg);
            }
        }

        struct LeafSpec
        {
            float angle, length, width;
            uint16_t color;
        };
    } // namespace

    void LandingPage::onEnter(TFT_eSPI &tft)
    {
        const int cx = tft.width() / 2;
        const int h = tft.height();

        tft.fillScreen(kBg);

        // Border vines
        drawVine(tft, 16);
        drawVine(tft, h - 16);

        // Steam rising from the leaves
        drawSteam(tft, cx - 18, 70, 34, 0.0f);
        drawSteam(tft, cx, 66, 42, 2.0f);
        drawSteam(tft, cx + 18, 70, 34, 4.0f);

        // Leaf fan emblem (outer leaves first, centre leaf last)
        const LeafSpec fan[] = {
            {-166, 40, 18, kTerracotta},
            {-14, 40, 18, kOchre},
            {-128, 52, 22, kLeafDark},
            {-52, 52, 22, kLeafDark},
            {-90, 64, 26, kLeafMid},
        };
        for (const LeafSpec &l : fan)
        {
            drawLeaf(tft, cx, 134, l.length, l.width, l.angle, l.color, kBg);
        }

        // Title
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(kTerracotta, kBg);
        tft.drawString("H I L I R I S E T", cx, 160);

        tft.setTextFont(4);
        tft.setTextSize(2);
        tft.setTextColor(kCream, kBg);
        tft.drawString("ECOPRINT", cx, 198);
        tft.setTextSize(1);

        // Divider with an ochre dot
        tft.drawFastHLine(cx - 70, 232, 60, kSage);
        tft.drawFastHLine(cx + 10, 232, 60, kSage);
        tft.fillCircle(cx, 232, 3, kOchre);

        // Subtitle
        tft.setTextFont(2);
        tft.setTextColor(kSage, kBg);
        tft.drawString("Natural Dye Steaming System", cx, 250);

        // Prompt (blinks in update())
        promptVisible_ = true;
        lastBlinkMs_ = millis();
        drawPrompt(tft, true);
    }

    void LandingPage::onExit() {}

    void LandingPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        (void)encoderDelta;

        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
            return;
        }

        unsigned long now = millis();
        if (now - lastBlinkMs_ >= kBlinkMs)
        {
            lastBlinkMs_ = now;
            promptVisible_ = !promptVisible_;
            drawPrompt(tft, promptVisible_);
        }
    }

    void LandingPage::drawPrompt(TFT_eSPI &tft, bool visible)
    {
        // "Hiding" = redrawing the same text in the background color,
        // so there's no fillRect/fillScreen flicker.
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(visible ? kCream : kBg, kBg);
        tft.drawString("Press button to start", tft.width() / 2, kPromptY);
    }
}