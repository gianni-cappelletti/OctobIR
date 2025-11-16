#include "octob-ir-core/IRLoader.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <WDL/ImpulseBuffer.h>
#include <WDL/resample.h>

namespace octob {

IRLoader::IRLoader() = default;
IRLoader::~IRLoader() = default;

IRLoadResult IRLoader::loadFromFile(const std::string& filepath) {
  IRLoadResult result;

  uint32_t channels;
  uint32_t sampleRate;
  drwav_uint64 totalPCMFrameCount;

  float* sampleData = drwav_open_file_and_read_pcm_frames_f32(
      filepath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

  if (sampleData == nullptr) {
    result.success = false;
    result.errorMessage = "Failed to open or read WAV file";
    return result;
  }

  size_t irLength = static_cast<size_t>(totalPCMFrameCount);
  irBuffer_.clear();
  irBuffer_.resize(irLength);

  if (channels == 1) {
    for (size_t i = 0; i < irLength; i++) {
      irBuffer_[i] = sampleData[i];
    }
  } else if (channels == 2) {
    for (size_t i = 0; i < irLength; i++) {
      irBuffer_[i] = (sampleData[i * 2] + sampleData[i * 2 + 1]) * 0.5f;
    }
  } else {
    for (size_t i = 0; i < irLength; i++) {
      irBuffer_[i] = sampleData[i * channels];
    }
  }

  drwav_free(sampleData, nullptr);

  constexpr float kIrCompensationGainDb = -17.0f;
  constexpr float kDbToLinear = 0.1151292546497023f;
  float irCompensationGain = std::exp(kIrCompensationGainDb * kDbToLinear);
  for (size_t i = 0; i < irLength; i++) {
    irBuffer_[i] *= irCompensationGain;
  }

  irSampleRate_ = sampleRate;
  numSamples_ = irLength;
  numChannels_ = channels;

  result.success = true;
  result.numSamples = numSamples_;
  result.numChannels = numChannels_;
  result.sampleRate = irSampleRate_;

  return result;
}

bool IRLoader::resampleAndInitialize(WDL_ImpulseBuffer& impulseBuffer,
                                      SampleRate targetSampleRate) {
  if (irBuffer_.empty()) {
    return false;
  }

  bool needsResampling = (irSampleRate_ != targetSampleRate);

  if (needsResampling) {
    WDL_Resampler resampler;
    resampler.SetMode(false, 1, false);
    resampler.SetRates(irSampleRate_, targetSampleRate);
    resampler.SetFilterParms(1.0, 0.693);

    size_t outFrames =
        (numSamples_ * static_cast<size_t>(targetSampleRate) +
         static_cast<size_t>(irSampleRate_) / 2) /
        static_cast<size_t>(irSampleRate_);
    std::vector<Sample> resampledIr(outFrames + 64);

    WDL_ResampleSample* rsinbuf = nullptr;
    int inSamples = static_cast<int>(numSamples_);
    int outSamples = static_cast<int>(outFrames);

    int actualOutSamples = 0;
    while (inSamples > 0 || resampler.ResampleOut(resampledIr.data() + actualOutSamples, inSamples,
                                                   outSamples, 1) > 0) {
      int needed = resampler.ResamplePrepare(inSamples, 1, &rsinbuf);
      for (int i = 0; i < needed && inSamples > 0; i++) {
        rsinbuf[i] = irBuffer_[numSamples_ - inSamples];
        inSamples--;
      }
      int processed = resampler.ResampleOut(resampledIr.data() + actualOutSamples, inSamples,
                                            outSamples, 1);
      if (processed <= 0) break;
      actualOutSamples += processed;
      outSamples -= processed;
    }

    resampledIr.resize(actualOutSamples);

    int actualLength = impulseBuffer.SetLength(actualOutSamples);
    if (actualLength <= 0) {
      return false;
    }

    impulseBuffer.SetNumChannels(1);
    if (impulseBuffer.GetNumChannels() != 1) {
      return false;
    }

    impulseBuffer.samplerate = targetSampleRate;

    WDL_FFT_REAL* irBufferPtr = impulseBuffer.impulses[0].Get();
    if (!irBufferPtr) {
      return false;
    }

    for (int i = 0; i < actualLength; i++) {
      irBufferPtr[i] = (i < actualOutSamples) ? resampledIr[i] : 0.0f;
    }

    return true;
  } else {
    int actualLength = impulseBuffer.SetLength(static_cast<int>(numSamples_));
    if (actualLength <= 0) {
      return false;
    }

    impulseBuffer.SetNumChannels(1);
    if (impulseBuffer.GetNumChannels() != 1) {
      return false;
    }

    impulseBuffer.samplerate = targetSampleRate;

    WDL_FFT_REAL* irBufferPtr = impulseBuffer.impulses[0].Get();
    if (!irBufferPtr) {
      return false;
    }

    for (int i = 0; i < actualLength; i++) {
      irBufferPtr[i] = (i < static_cast<int>(numSamples_)) ? irBuffer_[i] : 0.0f;
    }

    return true;
  }
}

}  // namespace octob
