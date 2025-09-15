#include "AudioFeedback.h"
#include <Arduino.h>
AudioFeedback::AudioFeedback(int buzzer_pin, int led_pin) : buzzPin(buzzer_pin), ledPin(led_pin), pwmChannel(0) {}
void AudioFeedback::init() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    ledcSetup(pwmChannel, 1000, 8); // 1000Hz, 8-bit
    ledcAttachPin(buzzPin, pwmChannel);
}
void AudioFeedback::playDetectionBeep(int duration_ms) {
    ledcWrite(pwmChannel, 127); // 50% duty cycle
    delay(duration_ms);
    ledcWrite(pwmChannel, 0);
}
void AudioFeedback::playCommandConfirm() {
    ledcWrite(pwmChannel, 127);
    delay(100);
    ledcWrite(pwmChannel, 0);
    delay(50);
    ledcWrite(pwmChannel, 127);
    delay(100);
    ledcWrite(pwmChannel, 0);
}
void AudioFeedback::setLED(bool state) {
    digitalWrite(ledPin, state ? HIGH : LOW);
}