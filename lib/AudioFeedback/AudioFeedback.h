#pragma once
class AudioFeedback {
private:
    int buzzPin;
    int ledPin;
    int pwmChannel;
public:
    AudioFeedback(int buzzer_pin, int led_pin);
    void init();
    void playDetectionBeep(int duration_ms = 200);
    void playCommandConfirm();
    void setLED(bool state);
};