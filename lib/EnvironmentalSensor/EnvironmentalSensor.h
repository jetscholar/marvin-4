#pragma once
#include <Adafruit_AHTX0.h>
class EnvironmentalSensor {
private:
    Adafruit_AHTX0 aht;
    bool initialized;
public:
    bool init();
    float getTemperature();
    float getHumidity();
    bool isReady();
};