#include "octobir-core/IRProcessor.hpp"

#include <convoengine.h>

namespace octob {

IRProcessor::IRProcessor()
    : impulseBuffer_(new WDL_ImpulseBuffer()),
      convolutionEngine_(new WDL_ConvolutionEngine_Div()),
      irLoader_(new IRLoader()),
      impulseBuffer2_(new WDL_ImpulseBuffer()),
      convolutionEngine2_(new WDL_ConvolutionEngine_Div()),
      irLoader2_(new IRLoader()) {}

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

bool IRProcessor::loadImpulseResponse2(const std::string& filepath, std::string& errorMessage) {
  IRLoadResult result = irLoader2_->loadFromFile(filepath);

  if (!result.success) {
    errorMessage = result.errorMessage;
    ir2Loaded_ = false;
    return false;
  }

  if (!irLoader2_->resampleAndInitialize(*impulseBuffer2_, sampleRate_)) {
    errorMessage = "Failed to resample IR2 to target sample rate";
    ir2Loaded_ = false;
    return false;
  }

  int irLength = impulseBuffer2_->GetLength();
  int irChannels = impulseBuffer2_->GetNumChannels();
  double irSampleRate = impulseBuffer2_->samplerate;

  if (irLength <= 0) {
    errorMessage = "IR2 buffer length is invalid: " + std::to_string(irLength);
    ir2Loaded_ = false;
    return false;
  }

  if (irChannels <= 0) {
    errorMessage = "IR2 buffer channels is invalid: " + std::to_string(irChannels);
    ir2Loaded_ = false;
    return false;
  }

  if (irSampleRate <= 0) {
    errorMessage = "IR2 sample rate is invalid: " + std::to_string(irSampleRate);
    ir2Loaded_ = false;
    return false;
  }

  latencySamples2_ = convolutionEngine2_->SetImpulse(impulseBuffer2_.get(), 64);
  if (latencySamples2_ < 0) {
    errorMessage = "Failed to initialize convolution engine with IR2 (returned " +
                   std::to_string(latencySamples2_) + "). IR2: " + std::to_string(irLength) +
                   " samples, " + std::to_string(irChannels) + " channels, " +
                   std::to_string(irSampleRate) + " Hz";
    ir2Loaded_ = false;
    latencySamples2_ = 0;
    return false;
  }

  currentIR2Path_ = filepath;
  ir2Loaded_ = true;
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

    if (ir2Loaded_) {
      irLoader2_->resampleAndInitialize(*impulseBuffer2_, sampleRate_);
      convolutionEngine2_->Reset();
      latencySamples2_ = convolutionEngine2_->SetImpulse(impulseBuffer2_.get(), 64);
    }
  }
}

void IRProcessor::setBlend(float blend) {
  blend_ = std::max(0.0f, std::min(1.0f, blend));
}

void IRProcessor::processMono(const Sample* input, Sample* output, FrameCount numFrames) {
  bool hasIR1 = irLoaded_;
  bool hasIR2 = ir2Loaded_;

  if (!hasIR1 && !hasIR2) {
    std::copy(input, input + numFrames, output);
    return;
  }

  float gain1 = std::sqrt(1.0f - blend_);
  float gain2 = std::sqrt(blend_);

  if (hasIR1 && hasIR2) {
    WDL_FFT_REAL* inputPtr = const_cast<WDL_FFT_REAL*>(input);
    convolutionEngine_->Add(&inputPtr, static_cast<int>(numFrames), 1);
    convolutionEngine2_->Add(&inputPtr, static_cast<int>(numFrames), 1);

    int available1 = convolutionEngine_->Avail(static_cast<int>(numFrames));
    int available2 = convolutionEngine2_->Avail(static_cast<int>(numFrames));

    if (available1 >= static_cast<int>(numFrames) && available2 >= static_cast<int>(numFrames)) {
      WDL_FFT_REAL** output1Ptr = convolutionEngine_->Get();
      WDL_FFT_REAL** output2Ptr = convolutionEngine2_->Get();

      for (FrameCount i = 0; i < numFrames; ++i) {
        output[i] = gain1 * output1Ptr[0][i] + gain2 * output2Ptr[0][i];
      }

      convolutionEngine_->Advance(static_cast<int>(numFrames));
      convolutionEngine2_->Advance(static_cast<int>(numFrames));
    } else {
      std::fill(output, output + numFrames, 0.0f);
    }
  } else if (hasIR1) {
    WDL_FFT_REAL* inputPtr = const_cast<WDL_FFT_REAL*>(input);
    convolutionEngine_->Add(&inputPtr, static_cast<int>(numFrames), 1);

    int available = convolutionEngine_->Avail(static_cast<int>(numFrames));
    if (available >= static_cast<int>(numFrames)) {
      WDL_FFT_REAL** outputPtr = convolutionEngine_->Get();
      for (FrameCount i = 0; i < numFrames; ++i) {
        output[i] = gain1 * outputPtr[0][i] + gain2 * input[i];
      }
      convolutionEngine_->Advance(static_cast<int>(numFrames));
    } else {
      std::fill(output, output + numFrames, 0.0f);
    }
  } else {
    WDL_FFT_REAL* inputPtr = const_cast<WDL_FFT_REAL*>(input);
    convolutionEngine2_->Add(&inputPtr, static_cast<int>(numFrames), 1);

    int available = convolutionEngine2_->Avail(static_cast<int>(numFrames));
    if (available >= static_cast<int>(numFrames)) {
      WDL_FFT_REAL** outputPtr = convolutionEngine2_->Get();
      for (FrameCount i = 0; i < numFrames; ++i) {
        output[i] = gain1 * input[i] + gain2 * outputPtr[0][i];
      }
      convolutionEngine2_->Advance(static_cast<int>(numFrames));
    } else {
      std::fill(output, output + numFrames, 0.0f);
    }
  }
}

void IRProcessor::reset() {
  if (convolutionEngine_) {
    convolutionEngine_->Reset();
  }
  if (convolutionEngine2_) {
    convolutionEngine2_->Reset();
  }
}

SampleRate IRProcessor::getIRSampleRate() const {
  return irLoader_ ? irLoader_->getIRSampleRate() : 0.0;
}

SampleRate IRProcessor::getIR2SampleRate() const {
  return irLoader2_ ? irLoader2_->getIRSampleRate() : 0.0;
}

size_t IRProcessor::getIRNumSamples() const {
  return irLoader_ ? irLoader_->getNumSamples() : 0;
}

size_t IRProcessor::getIR2NumSamples() const {
  return irLoader2_ ? irLoader2_->getNumSamples() : 0;
}

int IRProcessor::getNumIRChannels() const {
  return irLoader_ ? irLoader_->getNumChannels() : 0;
}

int IRProcessor::getNumIR2Channels() const {
  return irLoader2_ ? irLoader2_->getNumChannels() : 0;
}

int IRProcessor::getLatencySamples() const {
  return latencySamples_;
}

void IRProcessor::processStereo(const Sample* inputL, const Sample* inputR, Sample* outputL,
                                Sample* outputR, FrameCount numFrames) {
  bool hasIR1 = irLoaded_;
  bool hasIR2 = ir2Loaded_;

  if (!hasIR1 && !hasIR2) {
    std::copy(inputL, inputL + numFrames, outputL);
    std::copy(inputR, inputR + numFrames, outputR);
    return;
  }

  float gain1 = std::sqrt(1.0f - blend_);
  float gain2 = std::sqrt(blend_);

  WDL_FFT_REAL* inputPtrs[2] = {const_cast<WDL_FFT_REAL*>(inputL),
                                const_cast<WDL_FFT_REAL*>(inputR)};

  if (hasIR1 && hasIR2) {
    convolutionEngine_->Add(inputPtrs, static_cast<int>(numFrames), 2);
    convolutionEngine2_->Add(inputPtrs, static_cast<int>(numFrames), 2);

    int available1 = convolutionEngine_->Avail(static_cast<int>(numFrames));
    int available2 = convolutionEngine2_->Avail(static_cast<int>(numFrames));

    if (available1 >= static_cast<int>(numFrames) && available2 >= static_cast<int>(numFrames)) {
      WDL_FFT_REAL** output1Ptr = convolutionEngine_->Get();
      WDL_FFT_REAL** output2Ptr = convolutionEngine2_->Get();

      for (FrameCount i = 0; i < numFrames; ++i) {
        outputL[i] = gain1 * output1Ptr[0][i] + gain2 * output2Ptr[0][i];
        outputR[i] = gain1 * output1Ptr[1][i] + gain2 * output2Ptr[1][i];
      }

      convolutionEngine_->Advance(static_cast<int>(numFrames));
      convolutionEngine2_->Advance(static_cast<int>(numFrames));
    } else {
      std::fill(outputL, outputL + numFrames, 0.0f);
      std::fill(outputR, outputR + numFrames, 0.0f);
    }
  } else if (hasIR1) {
    convolutionEngine_->Add(inputPtrs, static_cast<int>(numFrames), 2);

    int available = convolutionEngine_->Avail(static_cast<int>(numFrames));
    if (available >= static_cast<int>(numFrames)) {
      WDL_FFT_REAL** outputPtr = convolutionEngine_->Get();
      for (FrameCount i = 0; i < numFrames; ++i) {
        outputL[i] = gain1 * outputPtr[0][i] + gain2 * inputL[i];
        outputR[i] = gain1 * outputPtr[1][i] + gain2 * inputR[i];
      }
      convolutionEngine_->Advance(static_cast<int>(numFrames));
    } else {
      std::fill(outputL, outputL + numFrames, 0.0f);
      std::fill(outputR, outputR + numFrames, 0.0f);
    }
  } else {
    convolutionEngine2_->Add(inputPtrs, static_cast<int>(numFrames), 2);

    int available = convolutionEngine2_->Avail(static_cast<int>(numFrames));
    if (available >= static_cast<int>(numFrames)) {
      WDL_FFT_REAL** outputPtr = convolutionEngine2_->Get();
      for (FrameCount i = 0; i < numFrames; ++i) {
        outputL[i] = gain1 * inputL[i] + gain2 * outputPtr[0][i];
        outputR[i] = gain1 * inputR[i] + gain2 * outputPtr[1][i];
      }
      convolutionEngine2_->Advance(static_cast<int>(numFrames));
    } else {
      std::fill(outputL, outputL + numFrames, 0.0f);
      std::fill(outputR, outputR + numFrames, 0.0f);
    }
  }
}

void IRProcessor::processDualMono(const Sample* inputL, const Sample* inputR, Sample* outputL,
                                  Sample* outputR, FrameCount numFrames) {
  processStereo(inputL, inputR, outputL, outputR, numFrames);
}

}  // namespace octob
