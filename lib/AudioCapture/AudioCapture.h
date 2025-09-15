#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

class AudioCapture {
public:
    bool begin();
    int16_t readSample();

private:
    void configureI2S();
};

#endif