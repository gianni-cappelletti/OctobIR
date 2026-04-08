#pragma once

#include "BusCompressor.hpp"
#include "CompressorMode.hpp"
#include "FETCompressor.hpp"
#include "OptoCompressor.hpp"
#include "VCACompressor.hpp"

namespace octob
{

// Wrapper owning all four compressor modes with unified RMS-tracking makeup gain.
// Delegates processing to the active mode and applies makeup gain to ensure
// consistent output volume across modes for fair A/B comparison.
class Compressor
{
 public:
  Compressor();

  void setSampleRate(SampleRate sampleRate);
  void setSquash(float amount);
  void setMode(int mode);

  void process(const Sample* input, Sample* output, FrameCount numFrames);
  void reset();

  float getSquash() const { return squash_; }
  int getMode() const { return mode_; }
  float getGainReductionDb() const;

 private:
  VCACompressor vca_;
  OptoCompressor opto_;
  FETCompressor fet_;
  BusCompressor bus_;
  CompressorMode* activeMode_;

  SampleRate sampleRate_;
  float squash_;
  int mode_;

  // Unified RMS-tracking makeup gain state
  double inputRmsSquared_;
  double outputRmsSquared_;
  float smoothedMakeupDb_;
  float rmsAlpha_;
  float makeupSmoothAlpha_;

  static float clamp(float value, float minVal, float maxVal);
  void updateRmsCoefficients();
};

}  // namespace octob
