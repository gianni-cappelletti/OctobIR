#include "octobir-core/IRLoader.hpp"

#include <cmath>

#define DR_WAV_IMPLEMENTATION
#include <convoengine.h>
#include <resample.h>

#include "dr_wav.h"

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

  if (channels == 1) {
    irBuffer_.resize(irLength);
    for (size_t i = 0; i < irLength; i++) {
      irBuffer_[i] = sampleData[i];
    }
  } else if (channels == 2) {
    irBuffer_.resize(irLength * 2);
    for (size_t i = 0; i < irLength; i++) {
      irBuffer_[i * 2] = sampleData[i * 2];
      irBuffer_[i * 2 + 1] = sampleData[i * 2 + 1];
    }
  } else {
    irBuffer_.resize(irLength);
    for (size_t i = 0; i < irLength; i++) {
      irBuffer_[i] = sampleData[i * channels];
    }
  }

  drwav_free(sampleData, nullptr);

  constexpr float kIrCompensationGainDb = -17.0f;
  constexpr float kDbToLinear = 0.1151292546497023f;
  float irCompensationGain = std::exp(kIrCompensationGainDb * kDbToLinear);
  for (size_t i = 0; i < irBuffer_.size(); i++) {
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

  int outputChannels = 2;

  bool needsResampling = (irSampleRate_ != targetSampleRate);

  if (needsResampling) {
    size_t outFrames = (numSamples_ * static_cast<size_t>(targetSampleRate) +
                        static_cast<size_t>(irSampleRate_) / 2) /
                       static_cast<size_t>(irSampleRate_);

    int actualLength = impulseBuffer.SetLength(static_cast<int>(outFrames));
    if (actualLength <= 0) {
      return false;
    }

    impulseBuffer.SetNumChannels(outputChannels);
    if (impulseBuffer.GetNumChannels() != outputChannels) {
      return false;
    }

    impulseBuffer.samplerate = targetSampleRate;

    for (int ch = 0; ch < outputChannels; ch++) {
      WDL_Resampler resampler;
      resampler.SetMode(false, 1, false);
      resampler.SetRates(irSampleRate_, targetSampleRate);
      resampler.SetFilterParms(1.0, 0.693);

      std::vector<WDL_ResampleSample> resampledIr(outFrames + 64);

      WDL_ResampleSample* rsinbuf = nullptr;
      int inSamples = static_cast<int>(numSamples_);
      int outSamples = static_cast<int>(outFrames);

      int actualOutSamples = 0;
      while (inSamples > 0 || resampler.ResampleOut(resampledIr.data() + actualOutSamples,
                                                    inSamples, outSamples, 1) > 0) {
        int needed = resampler.ResamplePrepare(inSamples, 1, &rsinbuf);
        for (int i = 0; i < needed && inSamples > 0; i++) {
          int srcCh = (numChannels_ == 1) ? 0 : ch;
          size_t srcIdx = (numSamples_ - inSamples) * numChannels_ + srcCh;
          rsinbuf[i] = irBuffer_[srcIdx];
          inSamples--;
        }
        int processed =
            resampler.ResampleOut(resampledIr.data() + actualOutSamples, inSamples, outSamples, 1);
        if (processed <= 0)
          break;
        actualOutSamples += processed;
        outSamples -= processed;
      }

      WDL_FFT_REAL* irBufferPtr = impulseBuffer.impulses[ch].Get();
      if (!irBufferPtr) {
        return false;
      }

      for (int i = 0; i < actualLength; i++) {
        irBufferPtr[i] = (i < actualOutSamples) ? static_cast<WDL_FFT_REAL>(resampledIr[i]) : 0.0f;
      }
    }

    return true;
  } else {
    int actualLength = impulseBuffer.SetLength(static_cast<int>(numSamples_));
    if (actualLength <= 0) {
      return false;
    }

    impulseBuffer.SetNumChannels(outputChannels);
    if (impulseBuffer.GetNumChannels() != outputChannels) {
      return false;
    }

    impulseBuffer.samplerate = targetSampleRate;

    for (int ch = 0; ch < outputChannels; ch++) {
      WDL_FFT_REAL* irBufferPtr = impulseBuffer.impulses[ch].Get();
      if (!irBufferPtr) {
        return false;
      }

      for (int i = 0; i < actualLength; i++) {
        if (i < static_cast<int>(numSamples_)) {
          int srcCh = (numChannels_ == 1) ? 0 : ch;
          size_t srcIdx = i * numChannels_ + srcCh;
          irBufferPtr[i] = static_cast<WDL_FFT_REAL>(irBuffer_[srcIdx]);
        } else {
          irBufferPtr[i] = 0.0f;
        }
      }
    }

    return true;
  }
}

}  // namespace octob
