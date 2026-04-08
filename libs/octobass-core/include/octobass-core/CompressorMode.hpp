#pragma once

#include <algorithm>
#include <cmath>

#include "Types.hpp"

namespace octob
{

class CompressorMode
{
 public:
  virtual ~CompressorMode() = default;

  virtual void setSampleRate(SampleRate sampleRate) = 0;
  virtual void setAmount(float amount) = 0;
  virtual void process(const Sample* input, Sample* output, FrameCount numFrames) = 0;
  virtual void reset() = 0;
  virtual float getGainReductionDb() const = 0;

  // Static makeup gain derived purely from current parameters (no signal tracking).
  // Formula: -threshold * (1 - 1/ratio) / 2  for threshold/ratio modes.
  virtual float getStaticMakeupDb() const = 0;

 protected:
  static float linearToDb(float linear)
  {
    if (linear < 1e-30f)
      return -96.0f;
    return 20.0f * std::log10(linear);
  }

  static float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

  static float clamp(float value, float minVal, float maxVal)
  {
    return std::max(minVal, std::min(maxVal, value));
  }
};

}  // namespace octob
