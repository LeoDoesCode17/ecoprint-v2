#include "test_actuator_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include <Arduino.h>
#include "managers/actuator_manager.h"

namespace pages
{
    TestActuatorPage test_actuator_page;

    namespace
    {
        struct ActuatorItem
        {
            const char *label;
            const int actuator_id;
            bool is_active;
        };

        ActuatorItem items[] = {
            {"Test Pump", 0, false},
            {"Test Lighter", 1, false},
            {"Test Valve", 2, false},
            {"Back", 3, false}};

        const int item_count = sizeof(items) / sizeof(items[0]);

        const int button_height = 40;
        const int button_gap = 15;
        const int margin_x = 10;
        const int margin_top = 100;

        const char *on_label = "ON";
        const char *off_label = "OFF";

        char label_buffer[100];
    }

    void TestActuatorPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("TEST ACTUATOR", tft.width() / 2, 60);

        for (size_t i = 0; i < item_count; i++)
        {
            drawButton(tft, i);
        }
    }

    void TestActuatorPage::onExit() {}

    void TestActuatorPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (encoderDelta != 0)
        {
            int previous = _selectedIndex;
            _selectedIndex = (_selectedIndex + static_cast<int>(encoderDelta)) % item_count;
            if (_selectedIndex < 0)
                _selectedIndex += item_count;
            if (_selectedIndex != previous)
            {
                drawButton(tft, previous);
                drawButton(tft, _selectedIndex);
            }
        }
        if (buttonPressed)
        {
            switch (items[_selectedIndex].actuator_id)
            {
            case 0: // pump
                items[_selectedIndex].is_active = !items[_selectedIndex].is_active;
                if (items[_selectedIndex].is_active)
                {
                    Serial.println("[ACTUATOR] Turn on pump");
                    actuator_manager::turn_on_pump();
                }
                else
                {
                    Serial.println("[ACTUATOR] Turn off pump");
                    actuator_manager::turn_off_pump();
                }
                drawButton(tft, _selectedIndex); // <-- now reflects the NEW state
                break;
            case 1: // Lighter
                items[_selectedIndex].is_active = !items[_selectedIndex].is_active;
                if (items[_selectedIndex].is_active)
                {
                    Serial.println("[ACTUATOR] Turn on lighter");
                    actuator_manager::turn_on_lighter();
                }
                else
                {
                    Serial.println("[ACTUATOR] Turn off lighter");
                    actuator_manager::turn_off_lighter();
                }
                drawButton(tft, _selectedIndex); // <-- now reflects the NEW state
                break;
            case 2: // Servo valve
                items[_selectedIndex].is_active = !items[_selectedIndex].is_active;
                if (items[_selectedIndex].is_active)
                {
                    Serial.println("[ACTUATOR] Turn on servo valve");
                    actuator_manager::open_valve_by_degree(90);
                }
                else
                {
                    Serial.println("[ACTUATOR] Turn off servo valve");
                    actuator_manager::close_valve();
                }
                drawButton(tft, _selectedIndex); // <-- now reflects the NEW state
                break;
            case 3: // back
                Serial.println("[UI] Back to menu page");
                page_manager::navigateTo(PageId::Menu);
                break;
            }
        }
    }

    void TestActuatorPage::drawButton(TFT_eSPI &tft, int index)
    {
        snprintf(label_buffer, sizeof(label_buffer), "%s (%s)", items[index].label, items[index].is_active ? on_label : off_label);
        int y = margin_top + index * (button_height + button_gap);
        bool selected = (index == _selectedIndex);
        uint16_t fillColor = selected ? ((index == 3) ? TFT_RED : TFT_BLUE) : TFT_DARKGREY;
        tft.fillRoundRect(margin_x, y, tft.width() - 2 * margin_x, button_height, 8, fillColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, fillColor);
        tft.setTextFont(4);
        tft.drawString(index == 3 ? items[index].label : label_buffer, tft.width() / 2, y + button_height / 2);
    }
}