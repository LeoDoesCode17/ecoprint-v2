#include "restart_confirmation_page.h"
#include <Arduino.h>
#include <stdio.h>
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/network_manager.h"
#include "managers/file_system_manager.h"
#include "managers/actuator_manager.h"
#include "managers/state_manager.h"
#include "theme.h"

namespace pages
{
    RestartConfirmationPage restartConfirmationPage;
    namespace t = theme;

    namespace
    {
        // Layout (designed for 480x320) - same header as the other pages
        constexpr int kTitleY = 34;
        constexpr int kDividerY = 58;
        constexpr int kCardRadius = 12;

        // Message card
        constexpr int kMsgX = 24;
        constexpr int kMsgY = 74;
        constexpr int kMsgW = 432;
        constexpr int kMsgH = 132;
        constexpr int kMsgFirstLineY = 26; // offsets from the top of the card
        constexpr int kMsgLineStep = 26;

        // Button row: [Restart] [Cancel]
        constexpr int kBtnY = 224;
        constexpr int kBtnH = 44;
        constexpr int kBtnMarginX = 10;
        constexpr int kBtnGap = 12;

        constexpr int kHintY = 296;

        // Long enough for the "Restarting..." message and the serial log to be seen
        constexpr unsigned long kRestartDelayMs = 600;

        // Active states = a steaming run is in progress
        bool isRunActive(StateMachine state)
        {
            switch (state)
            {
            case StateMachine::PREPARATION:
            case StateMachine::FILLING_WATER:
            case StateMachine::FIRING:
            case StateMachine::HEATING:
            case StateMachine::STEAMING:
                return true;
            default:
                return false;
            }
        }

        // Writes the credentials currently held by network_manager to flash,
        // so they are still there after the restart (file_system_manager's
        // initalize() loads them back at boot).
        void saveConfigToFlash()
        {
            // store_config() takes mutable char pointers, so copy the current
            // values into local buffers. Sizes match network_manager's buffers.
            char ssid[33];
            char wifiPassword[64];
            char host[64];
            char port[6];
            char username[64];
            char mqttPassword[64];

            snprintf(ssid, sizeof(ssid), "%s", network_manager::get_wifi_ssid());
            snprintf(wifiPassword, sizeof(wifiPassword), "%s", network_manager::get_wifi_password());
            snprintf(host, sizeof(host), "%s", network_manager::get_mqtt_broker_host());
            snprintf(port, sizeof(port), "%s", network_manager::get_mqtt_port());
            snprintf(username, sizeof(username), "%s", network_manager::get_mqtt_username());
            snprintf(mqttPassword, sizeof(mqttPassword), "%s", network_manager::get_mqtt_password());

            network_config_t config = {{ssid, wifiPassword}, {host, port, username, mqttPassword}};
            file_system_manager::store_config(config);
        }
    }

    void RestartConfirmationPage::onEnter(TFT_eSPI &tft)
    {
        const int cx = tft.width() / 2;

        _focus = static_cast<int>(Button::Cancel);
        _run_active = isRunActive(state_manager::get_state_machine());

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the other pages)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Restart Required", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        drawMessage(tft);
        drawButtons(tft);

        // Static hint
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Turn to switch   |   Press to select", cx, kHintY);
    }

    void RestartConfirmationPage::onExit()
    {
    }

    void RestartConfirmationPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // The encoder moves focus between the buttons
        if (encoderDelta != 0)
        {
            const int count = static_cast<int>(Button::Count);
            const int previous = _focus;
            _focus = ((_focus + static_cast<int>(encoderDelta)) % count + count) % count;

            if (_focus != previous)
            {
                drawButton(tft, static_cast<Button>(previous));
                drawButton(tft, static_cast<Button>(_focus));
            }
        }

        if (buttonPressed)
        {
            switch (static_cast<Button>(_focus))
            {
            case Button::Restart:
                restartDevice(tft);
                return; // (does not come back on real hardware)

            case Button::Cancel:
                page_manager::navigateTo(PageId::NetworkStatus);
                return;

            default:
                break;
            }
        }
    }

    void RestartConfirmationPage::restartDevice(TFT_eSPI &tft)
    {
        // 1. Tell the user what is happening
        tft.fillScreen(t::kBg);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Restarting...", tft.width() / 2, tft.height() / 2 - 10);
        tft.setTextFont(2);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Please wait", tft.width() / 2, tft.height() / 2 + 24);

        // 2. Leave the actuators in a safe state - a restart resets the pins
        actuator_manager::turn_off_pump();
        actuator_manager::close_valve();
        actuator_manager::turn_off_lighter();

        // 3. Keep the credentials across the restart
        saveConfigToFlash();

        // 4. Give the message and the serial log a moment, then restart
        Serial.println("[UI] Restarting to apply network configuration");
        delay(kRestartDelayMs);
        ESP.restart();
    }

    void RestartConfirmationPage::drawMessage(TFT_eSPI &tft)
    {
        const int cx = kMsgX + kMsgW / 2;

        tft.fillRoundRect(kMsgX, kMsgY, kMsgW, kMsgH, kCardRadius, t::kCardIdle);
        tft.drawRoundRect(kMsgX, kMsgY, kMsgW, kMsgH, kCardRadius, t::kTerracotta);

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);

        int y = kMsgY + kMsgFirstLineY;

        tft.setTextColor(t::kCream, t::kCardIdle);
        tft.drawString("Connecting to the configured WiFi and MQTT", cx, y);
        y += kMsgLineStep;
        tft.drawString("broker requires restarting the ESP32.", cx, y);
        y += kMsgLineStep + 6;

        tft.setTextColor(t::kSage, t::kCardIdle);
        tft.drawString("Your WiFi and MQTT settings will be saved first.", cx, y);

        if (_run_active)
        {
            y += kMsgLineStep;
            tft.setTextColor(t::kTerracotta, t::kCardIdle);
            tft.drawString("A run is in progress and will be stopped!", cx, y);
        }
    }

    const char *RestartConfirmationPage::buttonLabel(Button button)
    {
        switch (button)
        {
        case Button::Restart:
            return "Restart";
        case Button::Cancel:
            return "Cancel";
        default:
            return "";
        }
    }

    void RestartConfirmationPage::drawButtons(TFT_eSPI &tft)
    {
        for (int i = 0; i < static_cast<int>(Button::Count); i++)
        {
            drawButton(tft, static_cast<Button>(i));
        }
    }

    void RestartConfirmationPage::drawButton(TFT_eSPI &tft, Button button)
    {
        const int count = static_cast<int>(Button::Count);
        const int index = static_cast<int>(button);
        const int w = (tft.width() - 2 * kBtnMarginX - (count - 1) * kBtnGap) / count;
        const int x = kBtnMarginX + index * (w + kBtnGap);
        const bool focused = (index == _focus);

        // Same look as the other pages' buttons. Restart is the destructive
        // action, so it uses the maroon "Back" color when focused.
        const uint16_t fill = focused ? (button == Button::Restart ? t::kBack : t::kLeafDark) : t::kCardIdle;
        const uint16_t border = focused ? t::kOchre : t::kLeafDark;

        tft.fillRoundRect(x, kBtnY, w, kBtnH, kCardRadius, fill);
        tft.drawRoundRect(x, kBtnY, w, kBtnH, kCardRadius, border);
        if (focused)
        {
            // Thicker border for the focused button
            tft.drawRoundRect(x + 1, kBtnY + 1, w - 2, kBtnH - 2, kCardRadius - 1, border);
        }

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(focused ? t::kCream : t::kSage, fill);
        tft.drawString(buttonLabel(button), x + w / 2, kBtnY + kBtnH / 2);
    }
}