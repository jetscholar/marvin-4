#include "VoiceCommands.h"
#include "env.h"
VoiceCommands::VoiceCommands(AudioProcessor* proc) : processor(proc), interpreter(nullptr), initialized(false) {}
bool VoiceCommands::init() {
    static tflite::AllOpsResolver resolver;
    const tflite::Model* model = tflite::GetModel(g_command_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) return false;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, sizeof(tensor_arena));
    interpreter = &static_interpreter;
    if (interpreter->AllocateTensors() != kTfLiteOk) return false;
    input = interpreter->input(0);
    output = interpreter->output(0);
    initialized = true;
    return true;
}
int VoiceCommands::detect() {
    if (!initialized) return -1;
    float* mfcc = processor->getMFCC();
    for (int i = 0; i < MFCC_NUM_COEFFS * MFCC_NUM_FRAMES; i++) {
        input->data.f[i] = mfcc[i];
    }
    if (interpreter->Invoke() != kTfLiteOk) return -1;
    float* output_data = output->data.f;
    int max_idx = 0;
    float max_conf = output_data[0];
    for (int i = 1; i < 3; i++) {
        if (output_data[i] > max_conf) {
            max_conf = output_data[i];
            max_idx = i;
        }
    }
    return (max_conf > COMMAND_CONFIDENCE_THRESHOLD) ? max_idx : -1;
}