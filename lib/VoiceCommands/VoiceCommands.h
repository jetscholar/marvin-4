#pragma once
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "AudioProcessor.h"
#include "command_model.h"
class VoiceCommands {
private:
    AudioProcessor* processor;
    tflite::MicroInterpreter* interpreter;
    TfLiteTensor* input;
    TfLiteTensor* output;
    uint8_t tensor_arena[32 * 1024]; // 32KB arena
    bool initialized;
public:
    VoiceCommands(AudioProcessor* proc);
    bool init();
    int detect();
};