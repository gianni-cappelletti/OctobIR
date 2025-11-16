#include "octob-ir-core/IRProcessor.hpp"

#include <WDL/ImpulseBuffer.h>
#include <WDL/convoengine.h>

namespace octob {

IRProcessor::IRProcessor()
    : impulseBuffer_(std::make_unique<WDL_ImpulseBuffer>()),
      convolutionEngine_(std::make_unique<WDL_ConvolutionEngine>()),
      irLoader_(std::make_unique<IRLoader>()) {}

IRProcessor::~IRProcessor() = default;

bool IRProcessor::loadImpulseResponse(const std::string& filepath, std::string& errorMessage) {
  IRLoadResult result = irLoader_->loadFromFile(filepath);

  if (!result.success) {
    errorMessage = result.errorMessage;
    irLoaded_ = false;
    return false;
  }

  if (!irLoader_->resampleAndInitialize(*impulseBuffer_, sampleRate_)) {
    errorMessage = "Failed to resample IR to target sample rate";
    irLoaded_ = false;
    return false;
  }

  int latency = convolutionEngine_->SetImpulse(impulseBuffer_.get());
  if (latency <= 0) {
    errorMessage = "Failed to initialize convolution engine with IR";
    irLoaded_ = false;
    return false;
  }

  currentIRPath_ = filepath;
  irLoaded_ = true;
  errorMessage.clear();

  return true;
}

void IRProcessor::setSampleRate(SampleRate sampleRate) {
  if (sampleRate_ != sampleRate) {
    sampleRate_ = sampleRate;

    if (irLoaded_) {
      irLoader_->resampleAndInitialize(*impulseBuffer_, sampleRate_);
      convolutionEngine_->Reset();
      convolutionEngine_->SetImpulse(impulseBuffer_.get());
    }
  }
}

void IRProcessor::processMono(const Sample* input, Sample* output, FrameCount numFrames) {
  if (!irLoaded_) {
    std::copy(input, input + numFrames, output);
    return;
  }

  for (size_t i = 0; i < numFrames; i++) {
    WDL_FFT_REAL sample = input[i];
    WDL_FFT_REAL* inputPtr = &sample;
    convolutionEngine_->Add(&inputPtr, 1, 1);

    if (convolutionEngine_->Avail(1) >= 1) {
      WDL_FFT_REAL** outputPtr = convolutionEngine_->Get();
      output[i] = outputPtr[0][0];
      convolutionEngine_->Advance(1);
    } else {
      output[i] = 0.0f;
    }
  }
}

void IRProcessor::reset() {
  if (convolutionEngine_) {
    convolutionEngine_->Reset();
  }
}

SampleRate IRProcessor::getIRSampleRate() const {
  return irLoader_ ? irLoader_->getIRSampleRate() : 0.0;
}

size_t IRProcessor::getIRNumSamples() const {
  return irLoader_ ? irLoader_->getNumSamples() : 0;
}

}  // namespace octob
