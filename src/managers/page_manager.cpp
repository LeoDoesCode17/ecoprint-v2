#include "page_manager.h"
#include "sensor_manager.h"
#include "pages/i_page.h"
#include "pages/menu_page.h"
#include "pages/page1.h"
#include "pages/page2.h"
#include "pages/page3.h"
#include "pages/test_actuator_page.h"
#include "pages/modify_servo_page.h"
#include "pages/collect_data_page.h"
#include "pages/sensor_dashboard_page.h"
#include "pages/setting_page.h"
#include "pages/sensor_calibration_page.h"
#include "pages/process_calibration_page.h"
#include "pages/thermocouple_calibration_page.h"
#include "pages/hysteresis_calibration_page.h"

namespace
{
    TFT_eSPI *tftPtr = nullptr;
    pages::IPage *currentPage = nullptr;
    long lastEncoderPosition = 0;

    int _temperature_celcius = 0, _timer_minutes = 0;

    // The one place that maps a PageId to its concrete page object.
    // Adding a new page = one new case here + one new PageId enum value.
    pages::IPage *pageFor(pages::PageId id)
    {
        switch (id)
        {
        case pages::PageId::Menu:
            return &pages::menuPage;
        case pages::PageId::Page1:
            return &pages::page1;
        case pages::PageId::Page2:
            return &pages::page2;
        case pages::PageId::Page3:
            return &pages::page3;
        case pages::PageId::TestActuator:
            return &pages::test_actuator_page;
        case pages::PageId::ModifyServo:
            return &pages::modify_servo_page;
        case pages::PageId::CollectData:
            return &pages::collect_data_page;
        case pages::PageId::SensorDashboard:
            return &pages::sensor_dashboard_page;
        case pages::PageId::Setting:
            return &pages::settingPage;
        case pages::PageId::SensorCalibration:
            return &pages::sensorCalibrationPage;
        case pages::PageId::ProcessCalibration:
            return &pages::processCalibrationPage;
        case pages::PageId::ThermocoupleCalibration:
            return &pages::thermocoupleCalibrationPage;
        case pages::PageId::HysteresisCalibration:
            return &pages::hysteresisCalibrationPage;
        }
        return &pages::menuPage;
    }
}

namespace page_manager
{
    void initialize(TFT_eSPI &tft)
    {
        // pointer to global single tft
        tftPtr = &tft;

        // Navigate to page by id
        navigateTo(pages::PageId::Menu);
    }

    void update()
    {
        if (currentPage == nullptr || tftPtr == nullptr)
        {
            return;
        }

        long position = sensor_manager::getEncoderPosition();
        long delta = position - lastEncoderPosition;
        lastEncoderPosition = position;

        bool buttonPressed = sensor_manager::wasEncoderButtonPressed();

        currentPage->update(*tftPtr, delta, buttonPressed);
    }

    void navigateTo(pages::PageId id)
    {
        if (currentPage != nullptr)
        {
            // Clean up states/variables from current page before navigating
            currentPage->onExit();
        }

        // Zero the underlying encoder count so the incoming page always
        // starts from a clean reference frame, regardless of how far the
        // encoder was turned on the previous page.
        sensor_manager::setEncoderPosition(0);
        lastEncoderPosition = 0;

        currentPage = pageFor(id);
        currentPage->onEnter(*tftPtr);
    }

    int get_temperature_celcius()
    {
        return _temperature_celcius;
    }
    int get_timer_minutes()
    {
        return _timer_minutes;
    }
    void set_temperature_celcius(int value)
    {
        _temperature_celcius = value;
    }
    void set_timer_minutes(int value)
    {
        _timer_minutes = value;
    }
}
