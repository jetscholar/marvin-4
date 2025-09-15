#include "EnvironmentalSensor.h"
#include "env.h"
bool EnvironmentalSensor::init() {
    initialized = aht.begin(&Wire, I2C_SDA_PIN, I2C_SCL_PIN);
    return initialized;
}
float EnvironmentalSensor::getTemperature() {
    if (!initialized) return -1.0f;
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    return temp.temperature;
}
float EnvironmentalSensor::getHumidity() {
    if (!initialized) return -1.0f;
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    return humidity.relative_humidity;
}
bool EnvironmentalSensor::isReady() {
    return initialized;
}