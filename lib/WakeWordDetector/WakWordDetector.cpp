#include "WakeWordDetector.h"
#include "AudioProcessor.h"
#include "env.h"

// TFLM headers
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Bring real types in with local aliases to match header forward decls
using TfLiteTensorTag = TfLiteTensor;
using tfliteModelTag = tflite::Model;
using tfliteMicroInterpreterTag = tflite::MicroInterpreter;

// Your trained model (path as you provided)
#include "ManuallDSCNN/model_int8.cc"

// Static arena
alignas(16) uint8_t WakeWordDetector::tensor_arena_[WakeWordDetector::kTensorArenaSize];

WakeWordDetector::WakeWordDetector(AudioProcessor* proc) : ap_(proc) {
  for (int i = 0; i < CONF_HISTORY; i++) conf_hist_[i] = 0.0f;
}

void WakeWordDetector::pushConf_(float c) {
  conf_hist_[conf_idx_] = c;
  conf_idx_ = (conf_idx_ + 1) % CONF_HISTORY;
  if (conf_count_ < CONF_HISTORY) conf_count_++;
}

float WakeWordDetector::getAverageConfidence() const {
  if (conf_count_ == 0) return 0.0f;
  float sum = 0.0f;
  for (int i = 0; i < conf_count_; i++) sum += conf_hist_[i];
  return sum / conf_count_;
}

bool WakeWordDetector::init() {
  if (!ap_) {
    Serial.println("❌ WakeWordDetector: AudioProcessor is null");
    return false;
  }

  const tflite::Model* model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("❌ Model schema mismatch: %d vs %d\n", model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }

  static tflite::AllOpsResolver resolver;

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena_, kTensorArenaSize);

  interpreter_ = &static_interpreter;

  if (interpreter_->AllocateTensors() != kTfLiteOk) {
    Serial.println("❌ AllocateTensors failed");
    return false;
  }

  input_  = interpreter_->input(0);
  output_ = interpreter_->output(0);

  // Cache input quantization
  in_scale_ = input_->params.scale;
  in_zero_  = input_->params.zero_point;
  Serial.printf("🔢 Input quant: scale=%g zero_point=%d\n", (double)in_scale_, (int)in_zero_);

  // Log input/output tensor shapes (helps confirm MFCC_NUM_FRAMES/COEFFS)
  Serial.print("📐 Input dims: ");
  for (int i = 0; i < input_->dims->size; i++) {
    Serial.printf("%d ", input_->dims->data[i]);
  }
  Serial.println();

  Serial.print("📐 Output dims: ");
  for (int i = 0; i < output_->dims->size; i++) {
    Serial.printf("%d ", output_->dims->data[i]);
  }
  Serial.println();

  initialized_ = true;
  return true;
}

float WakeWordDetector::detect() {
  if (!initialized_) return 0.0f;

  // 1) Get current MFCC window as float [frames * coeffs]
  static float mfcc_window[MFCC_NUM_FRAMES * MFCC_NUM_COEFFS];
  ap_->copyMfccWindowFloat(mfcc_window);

  // 2) Quantize into input tensor using model's own scale/zero_point
  //    Assume input layout is [1, frames, coeffs, 1] or flat equivalent
  const int N = MFCC_NUM_FRAMES * MFCC_NUM_COEFFS;
  for (int i = 0; i < N; i++) {
    // int8 = round(float / scale) + zero_point
    const float x = mfcc_window[i];
    int32_t q = (int32_t)lrintf(x / in_scale_) + in_zero_;
    if (q < -128) q = -128;
    if (q > 127)  q = 127;
    input->data.int8[i] = (int8_t)q;
  }

  // 3) Inference
  if (interpreter_->Invoke() != kTfLiteOk) {
    Serial.println("❌ TFLM Invoke failed");
    pushConf_(0.0f);
    return 0.0f;
  }

  // 4) Read "marvin" confidence. Typical convention is output[0]=background, output[1]=wake.
  //    If yours is different, just adjust the index here.
  const int MARVIN_INDEX = 1; // <-- change if your training puts "marvin" elsewhere
  const int8_t qscore = output_->data.int8[MARVIN_INDEX];
  const float confidence = (qscore - output_->params.zero_point) * output_->params.scale;

  // 5) Bookkeeping for smoothing + threshold/cooldown
  pushConf_(confidence);

  if (confidence > threshold_) {
    const unsigned long now = millis();
    if (now - last_detection_ms_ > DETECTION_COOLDOWN_MS) {
      detection_count_++;
      last_detection_ms_ = now;
    }
  }

  return confidence;
}



