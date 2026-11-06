#include "network_status_page.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/network_manager.h"
#include "theme.h"

namespace pages
{
    NetworkStatusPage networkStatusPage;
    namespace t = theme;

    namespace
    {
        // Layout (designed for 480x320) - same compact header as the
        // sensor dashboard so both panels fit on one screen
        constexpr int kTitleY = 22;
        constexpr int kDividerY = 44;
        constexpr int kCardRadius = 12;

        constexpr int kPanelTop = 58;
        constexpr int kPanelH = 180;
        constexpr int kPanelMarginX = 10;
        constexpr int kPanelGap = 10;

        // Inside a panel (offsets from its top-left corner)
        constexpr int kPanelTitleY = 22;
        constexpr int kPillCenterY = 58;
        constexpr int kPillW = 150;
        constexpr int kPillH = 28;
        constexpr int kFirstLineY = 96;
        constexpr int kLineStep = 22;
        constexpr int kLabelX = 12;
        constexpr int kValueX = 58; // where the value column starts

        // Back button (the page's only action, so it is always highlighted)
        constexpr int kBtnY = 250;
        constexpr int kBtnH = 44;
        constexpr int kBtnW = 240;

        constexpr int kHintY = 306;

        constexpr unsigned long kRefreshMs = 500;

        // Same green as the other pages' "active" color
        constexpr uint16_t kOk = t::rgb(110, 200, 100);

        int panelW(TFT_eSPI &tft)
        {
            return (tft.width() - 2 * kPanelMarginX - kPanelGap) / 2;
        }

        int panelX(TFT_eSPI &tft, int index)
        {
            return kPanelMarginX + index * (panelW(tft) + kPanelGap);
        }

        // Copies src into out, shortened with ".." so it fits maxW pixels in
        // the current font. outSize must be at least 4.
        void fitText(TFT_eSPI &tft, const char *src, int maxW, char *out, size_t outSize)
        {
            const size_t srcLen = strlen(src);
            if (srcLen < outSize && tft.textWidth(src) <= maxW)
            {
                memcpy(out, src, srcLen + 1);
                return;
            }

            size_t n = (srcLen < outSize - 3) ? srcLen : outSize - 3;
            for (; n > 0; n--)
            {
                memcpy(out, src, n);
                out[n] = '.';
                out[n + 1] = '.';
                out[n + 2] = '\0';
                if (tft.textWidth(out) <= maxW)
                {
                    return;
                }
            }
            out[0] = '\0';
        }

        // One "label   value" row inside a panel. Empty values show
        // "(not set)"; masked values (passwords) show one '*' per character.
        void drawLine(TFT_eSPI &tft, int px, int pw, int lineIndex,
                      const char *label, const char *raw, bool masked)
        {
            if (raw == nullptr)
            {
                raw = "";
            }
            const bool isEmpty = (raw[0] == '\0');
            const int y = kPanelTop + kFirstLineY + lineIndex * kLineStep;

            char display[72];
            if (isEmpty)
            {
                snprintf(display, sizeof(display), "(not set)");
            }
            else if (masked)
            {
                size_t n = strlen(raw);
                if (n > sizeof(display) - 1)
                {
                    n = sizeof(display) - 1;
                }
                memset(display, '*', n);
                display[n] = '\0';
            }
            else
            {
                snprintf(display, sizeof(display), "%s", raw);
            }

            tft.setTextDatum(ML_DATUM);
            tft.setTextFont(2);
            tft.setTextSize(1);

            tft.setTextColor(t::kSage, t::kCardIdle);
            tft.drawString(label, px + kLabelX, y);

            char fitted[72];
            fitText(tft, display, pw - kValueX - kLabelX, fitted, sizeof(fitted));
            tft.setTextColor(isEmpty ? t::kSage : t::kCream, t::kCardIdle);
            tft.drawString(fitted, px + kValueX, y);
        }
    }

    void NetworkStatusPage::onEnter(TFT_eSPI &tft)
    {
        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the other pages)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Network Status", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        for (int i = 0; i < static_cast<int>(Panel::Count); i++)
        {
            drawPanel(tft, static_cast<Panel>(i));
        }

        // Draw the connection indicators right away
        _last_wifi_state = -1;
        _last_mqtt_state = -1;
        refreshStatus(tft);
        _last_refresh_ms = millis();

        drawBackButton(tft);

        // Static hint
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Press button to go back", cx, kHintY);
    }

    void NetworkStatusPage::onExit()
    {
    }

    void NetworkStatusPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // Nothing to scroll or edit: the encoder is unused, a click goes back.
        (void)encoderDelta;

        if (buttonPressed)
        {
            // page_manager::navigateTo(PageId::NetworkConfiguration);
            ESP.restart();
            return;
        }

        // The connection can drop or come back while this page is open
        const unsigned long now = millis();
        if (now - _last_refresh_ms >= kRefreshMs)
        {
            _last_refresh_ms = now;
            refreshStatus(tft);
        }
    }

    void NetworkStatusPage::refreshStatus(TFT_eSPI &tft)
    {
        const int wifi = network_manager::is_wifi_connected() ? 1 : 0;
        if (wifi != _last_wifi_state)
        {
            _last_wifi_state = wifi;
            drawStatus(tft, Panel::Wifi, wifi == 1);
        }

        const int mqtt = network_manager::is_mqtt_connected() ? 1 : 0;
        if (mqtt != _last_mqtt_state)
        {
            _last_mqtt_state = mqtt;
            drawStatus(tft, Panel::Mqtt, mqtt == 1);
        }
    }

    void NetworkStatusPage::drawPanel(TFT_eSPI &tft, Panel panel)
    {
        const int w = panelW(tft);
        const int x = panelX(tft, static_cast<int>(panel));

        tft.fillRoundRect(x, kPanelTop, w, kPanelH, kCardRadius, t::kCardIdle);
        tft.drawRoundRect(x, kPanelTop, w, kPanelH, kCardRadius, t::kLeafDark);

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kCardIdle);
        tft.drawString(panel == Panel::Wifi ? "WiFi" : "MQTT", x + w / 2, kPanelTop + kPanelTitleY);

        if (panel == Panel::Wifi)
        {
            drawLine(tft, x, w, 0, "SSID", network_manager::get_wifi_ssid(), false);
            drawLine(tft, x, w, 1, "Pass", network_manager::get_wifi_password(), true);
        }
        else
        {
            drawLine(tft, x, w, 0, "Host", network_manager::get_mqtt_broker_host(), false);
            drawLine(tft, x, w, 1, "Port", network_manager::get_mqtt_port(), false);
            drawLine(tft, x, w, 2, "User", network_manager::get_mqtt_username(), false);
            drawLine(tft, x, w, 3, "Pass", network_manager::get_mqtt_password(), true);
        }
    }

    void NetworkStatusPage::drawStatus(TFT_eSPI &tft, Panel panel, bool connected)
    {
        const int w = panelW(tft);
        const int cx = panelX(tft, static_cast<int>(panel)) + w / 2;
        const int cy = kPanelTop + kPillCenterY;

        // The pill always covers the same rectangle, so a redraw fully
        // replaces the previous state - no clearing needed.
        const uint16_t fill = connected ? kOk : t::kTerracotta;
        const uint16_t text = connected ? t::kBg : t::kCream;

        tft.fillRoundRect(cx - kPillW / 2, cy - kPillH / 2, kPillW, kPillH, kPillH / 2, fill);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(text, fill);
        tft.drawString(connected ? "Connected" : "Disconnected", cx, cy);
    }

    void NetworkStatusPage::drawBackButton(TFT_eSPI &tft)
    {
        // Same look as a selected Back card on the carousel pages
        const int x = (tft.width() - kBtnW) / 2;

        tft.fillRoundRect(x, kBtnY, kBtnW, kBtnH, kCardRadius, t::kBack);
        tft.drawRoundRect(x, kBtnY, kBtnW, kBtnH, kCardRadius, t::kOchre);
        tft.drawRoundRect(x + 1, kBtnY + 1, kBtnW - 2, kBtnH - 2, kCardRadius - 1, t::kOchre);

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBack);
        tft.drawString("CONNECT", tft.width() / 2, kBtnY + kBtnH / 2);
    }
}