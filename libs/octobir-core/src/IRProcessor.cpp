#include "octobir-core/IRProcessor.hpp"

#include <convoengine.h>

namespace octob {

IRProcessor::IRProcessor()
    : impulseBuffer_(new WDL_ImpulseBuffer()),
      convolutionEngine_(new WDL_ConvolutionEngine_Div()),
      irLoader_(new IRLoader()) {}

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

  int irLength = impulseBuffer_->GetLength();
  int irChannels = impulseBuffer_->GetNumChannels();
  double irSampleRate = impulseBuffer_->samplerate;

  if (irLength <= 0) {
    errorMessage = "IR buffer length is invalid: " + std::to_string(irLength);
    irLoaded_ = false;
    return false;
  }

  if (irChannels <= 0) {
    errorMessage = "IR buffer channels is invalid: " + std::to_string(irChannels);
    irLoaded_ = false;
    return false;
  }

  if (irSampleRate <= 0) {
    errorMessage = "IR sample rate is invalid: " + std::to_string(irSampleRate);
    irLoaded_ = false;
    return false;
  }

  latencySamples_ = convolutionEngine_->SetImpulse(impulseBuffer_.get(), 64);
  if (latencySamples_ < 0) {
    errorMessage = "Failed to initialize convolution engine with IR (returned " +
                   std::to_string(latencySamples_) + "). IR: " + std::to_string(irLength) +
                   " samples, " + std::to_string(irChannels) + " channels, " +
                   std::to_string(irSampleRate) + " Hz";
    irLoaded_ = false;
    latencySamples_ = 0;
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
      latencySamples_ = convolutionEngine_->SetImpulse(impulseBuffer_.get(), 64);
    }
  }
}

void IRProcessor::processMono(const Sample* input, Sample* output, FrameCount numFrames) {
  if (!irLoaded_) {
    std::copy(input, input + numFrames, output);
    return;
  }

  WDL_FFT_REAL* inputPtr = const_cast<WDL_FFT_REAL*>(input);
  convolutionEngine_->Add(&inputPtr, static_cast<int>(numFrames), 1);

  int available = convolutionEngine_->Avail(static_cast<int>(numFrames));
  if (available >= static_cast<int>(numFrames)) {
    WDL_FFT_REAL** outputPtr = convolutionEngine_->Get();
    std::copy(outputPtr[0], outputPtr[0] + numFrames, output);
    convolutionEngine_->Advance(static_cast<int>(numFrames));
  } else {
    std::fill(output, output + numFrames, 0.0f);
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

int IRProcessor::getNumIRChannels() const {
  return irLoader_ ? irLoader_->getNumChannels() : 0;
}

int IRProcessor::getLatencySamples() const {
  return latencySamples_;
}

void IRProcessor::processStereo(const Sample* inputL, const Sample* inputR, Sample* outputL,
                                Sample* outputR, FrameCount numFrames) {
  if (!irLoaded_) {
    std::copy(inputL, inputL + numFrames, outputL);
    std::copy(inputR, inputR + numFrames, outputR);
    return;
  }

  WDL_FFT_REAL* inputPtrs[2] = {const_cast<WDL_FFT_REAL*>(inputL),
                                const_cast<WDL_FFT_REAL*>(inputR)};
  convolutionEngine_->Add(inputPtrs, static_cast<int>(numFrames), 2);

  int available = convolutionEngine_->Avail(static_cast<int>(numFrames));
  if (available >= static_cast<int>(numFrames)) {
    WDL_FFT_REAL** outputPtr = convolutionEngine_->Get();
    std::copy(outputPtr[0], outputPtr[0] + numFrames, outputL);
    std::copy(outputPtr[1], outputPtr[1] + numFrames, outputR);
    convolutionEngine_->Advance(static_cast<int>(numFrames));
  } else {
    std::fill(outputL, outputL + numFrames, 0.0f);
    std::fill(outputR, outputR + numFrames, 0.0f);
  }
}

void IRProcessor::processDualMono(const Sample* inputL, const Sample* inputR, Sample* outputL,
                                  Sample* outputR, FrameCount numFrames) {
  processStereo(inputL, inputR, outputL, outputR, numFrames);
}

}  // namespace octob
