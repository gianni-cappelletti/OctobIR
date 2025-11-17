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
    updateSmoothingCoefficients();

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
  blend_ = std::max(-1.0f, std::min(1.0f, blend));
  if (!dynamicModeEnabled_) {
    currentBlend_ = blend_;
    smoothedBlend_ = blend_;
  }
}

void IRProcessor::setDynamicModeEnabled(bool enabled) {
  dynamicModeEnabled_ = enabled;
  if (!enabled) {
    currentBlend_ = blend_;
    smoothedBlend_ = blend_;
  }
}

void IRProcessor::setSidechainEnabled(bool enabled) {
  sidechainEnabled_ = enabled;
}

void IRProcessor::setMinBlend(float minBlend) {
  minBlend_ = std::max(-1.0f, std::min(1.0f, minBlend));
}

void IRProcessor::setMaxBlend(float maxBlend) {
  maxBlend_ = std::max(-1.0f, std::min(1.0f, maxBlend));
}

void IRProcessor::setLowThreshold(float thresholdDb) {
  lowThresholdDb_ = std::max(-60.0f, std::min(0.0f, thresholdDb));
}

void IRProcessor::setHighThreshold(float thresholdDb) {
  highThresholdDb_ = std::max(-60.0f, std::min(0.0f, thresholdDb));
}

void IRProcessor::setAttackTime(float attackTimeMs) {
  attackTimeMs_ = std::max(1.0f, std::min(1000.0f, attackTimeMs));
  updateSmoothingCoefficients();
}

void IRProcessor::setReleaseTime(float releaseTimeMs) {
  releaseTimeMs_ = std::max(1.0f, std::min(1000.0f, releaseTimeMs));
  updateSmoothingCoefficients();
}

void IRProcessor::setOutputGain(float gainDb) {
  outputGainDb_ = std::max(-24.0f, std::min(24.0f, gainDb));
  outputGainLinear_ = std::pow(10.0f, outputGainDb_ / 20.0f);
}

void IRProcessor::processMono(const Sample* input, Sample* output, FrameCount numFrames) {
  bool hasIR1 = irLoaded_;
  bool hasIR2 = ir2Loaded_;

  if (!hasIR1 && !hasIR2) {
    std::copy(input, input + numFrames, output);
    applyOutputGain(output, numFrames);
    return;
  }

  float blendToUse = blend_;
  if (dynamicModeEnabled_ && !sidechainEnabled_) {
    currentInputLevelDb_ = detectPeakLevel(input, numFrames);
    float targetBlend = calculateDynamicBlend(currentInputLevelDb_);
    float coeff = (targetBlend > smoothedBlend_) ? attackCoeff_ : releaseCoeff_;
    smoothedBlend_ = smoothedBlend_ * coeff + targetBlend * (1.0f - coeff);
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else if (dynamicModeEnabled_) {
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else {
    currentBlend_ = blend_;
  }

  float normalizedBlend = (blendToUse + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

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

  applyOutputGain(output, numFrames);
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
    applyOutputGain(outputL, numFrames);
    applyOutputGain(outputR, numFrames);
    return;
  }

  float blendToUse = blend_;
  if (dynamicModeEnabled_ && !sidechainEnabled_) {
    float peakL = detectPeakLevel(inputL, numFrames);
    float peakR = detectPeakLevel(inputR, numFrames);
    currentInputLevelDb_ = std::max(peakL, peakR);
    float targetBlend = calculateDynamicBlend(currentInputLevelDb_);
    float coeff = (targetBlend > smoothedBlend_) ? attackCoeff_ : releaseCoeff_;
    smoothedBlend_ = smoothedBlend_ * coeff + targetBlend * (1.0f - coeff);
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else if (dynamicModeEnabled_) {
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else {
    currentBlend_ = blend_;
  }

  float normalizedBlend = (blendToUse + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

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

  applyOutputGain(outputL, numFrames);
  applyOutputGain(outputR, numFrames);
}

void IRProcessor::processDualMono(const Sample* inputL, const Sample* inputR, Sample* outputL,
                                  Sample* outputR, FrameCount numFrames) {
  processStereo(inputL, inputR, outputL, outputR, numFrames);
}

void IRProcessor::processMonoWithSidechain(const Sample* input, const Sample* sidechain,
                                           Sample* output, FrameCount numFrames) {
  bool hasIR1 = irLoaded_;
  bool hasIR2 = ir2Loaded_;

  if (!hasIR1 && !hasIR2) {
    std::copy(input, input + numFrames, output);
    applyOutputGain(output, numFrames);
    return;
  }

  float blendToUse = blend_;
  if (dynamicModeEnabled_ && sidechainEnabled_) {
    currentInputLevelDb_ = detectPeakLevel(sidechain, numFrames);
    float targetBlend = calculateDynamicBlend(currentInputLevelDb_);
    float coeff = (targetBlend > smoothedBlend_) ? attackCoeff_ : releaseCoeff_;
    smoothedBlend_ = smoothedBlend_ * coeff + targetBlend * (1.0f - coeff);
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else if (dynamicModeEnabled_) {
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else {
    currentBlend_ = blend_;
  }

  float normalizedBlend = (blendToUse + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

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

  applyOutputGain(output, numFrames);
}

void IRProcessor::processStereoWithSidechain(const Sample* inputL, const Sample* inputR,
                                             const Sample* sidechainL, const Sample* sidechainR,
                                             Sample* outputL, Sample* outputR,
                                             FrameCount numFrames) {
  bool hasIR1 = irLoaded_;
  bool hasIR2 = ir2Loaded_;

  if (!hasIR1 && !hasIR2) {
    std::copy(inputL, inputL + numFrames, outputL);
    std::copy(inputR, inputR + numFrames, outputR);
    applyOutputGain(outputL, numFrames);
    applyOutputGain(outputR, numFrames);
    return;
  }

  float blendToUse = blend_;
  if (dynamicModeEnabled_ && sidechainEnabled_) {
    float peakL = detectPeakLevel(sidechainL, numFrames);
    float peakR = detectPeakLevel(sidechainR, numFrames);
    currentInputLevelDb_ = std::max(peakL, peakR);
    float targetBlend = calculateDynamicBlend(currentInputLevelDb_);
    float coeff = (targetBlend > smoothedBlend_) ? attackCoeff_ : releaseCoeff_;
    smoothedBlend_ = smoothedBlend_ * coeff + targetBlend * (1.0f - coeff);
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else if (dynamicModeEnabled_) {
    blendToUse = smoothedBlend_;
    currentBlend_ = blendToUse;
  } else {
    currentBlend_ = blend_;
  }

  float normalizedBlend = (blendToUse + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

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

  applyOutputGain(outputL, numFrames);
  applyOutputGain(outputR, numFrames);
}

void IRProcessor::processDualMonoWithSidechain(const Sample* inputL, const Sample* inputR,
                                               const Sample* sidechainL, const Sample* sidechainR,
                                               Sample* outputL, Sample* outputR,
                                               FrameCount numFrames) {
  processStereoWithSidechain(inputL, inputR, sidechainL, sidechainR, outputL, outputR, numFrames);
}

float IRProcessor::calculateDynamicBlend(float inputLevelDb) const {
  if (lowThresholdDb_ == highThresholdDb_) {
    return inputLevelDb >= lowThresholdDb_ ? maxBlend_ : minBlend_;
  }

  if (inputLevelDb <= lowThresholdDb_) {
    return minBlend_;
  } else if (inputLevelDb >= highThresholdDb_) {
    return maxBlend_;
  } else {
    float t = (inputLevelDb - lowThresholdDb_) / (highThresholdDb_ - lowThresholdDb_);
    return minBlend_ + (maxBlend_ - minBlend_) * t;
  }
}

float IRProcessor::detectPeakLevel(const Sample* buffer, FrameCount numFrames) const {
  if (numFrames == 0) {
    return -96.0f;
  }

  float peak = 0.0f;
  for (FrameCount i = 0; i < numFrames; ++i) {
    float absSample = std::abs(buffer[i]);
    if (absSample > peak) {
      peak = absSample;
    }
  }

  if (peak < 1e-6f) {
    return -96.0f;
  }

  return 20.0f * std::log10(peak);
}

void IRProcessor::updateSmoothingCoefficients() {
  if (sampleRate_ <= 0.0) {
    attackCoeff_ = 0.0f;
    releaseCoeff_ = 0.0f;
    return;
  }

  if (attackTimeMs_ > 0.0f) {
    float attackTimeSeconds = attackTimeMs_ / 1000.0f;
    attackCoeff_ = std::exp(-1.0f / (attackTimeSeconds * static_cast<float>(sampleRate_)));
  } else {
    attackCoeff_ = 0.0f;
  }

  if (releaseTimeMs_ > 0.0f) {
    float releaseTimeSeconds = releaseTimeMs_ / 1000.0f;
    releaseCoeff_ = std::exp(-1.0f / (releaseTimeSeconds * static_cast<float>(sampleRate_)));
  } else {
    releaseCoeff_ = 0.0f;
  }
}

void IRProcessor::applyOutputGain(Sample* buffer, FrameCount numFrames) const {
  if (outputGainLinear_ == 1.0f) {
    return;
  }

  for (FrameCount i = 0; i < numFrames; ++i) {
    buffer[i] *= outputGainLinear_;
  }
}

}  // namespace octob
