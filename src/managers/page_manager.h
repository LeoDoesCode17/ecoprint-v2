#pragma once
#include <TFT_eSPI.h>
#include "pages/page_id.h"

namespace page_manager
{
    // Call once from setup(), after tft.init() / tft.setRotation().
    void initialize(TFT_eSPI &tft);

    // Call once from loop(). Reads the encoder/button through
    // sensor_manager and forwards them to whichever page is active.
    void update();

    // Switch the active page. Resets the encoder reference frame (via
    // sensor_manager::set_encoder_positioon) so the new page always
    // starts counting encoder movement from 0.
    void navigateTo(pages::PageId id);

    int get_temperature_celcius();
    int get_timer_minutes();
    void set_temperature_celcius(int value);
    void set_timer_minutes(int value);
}
