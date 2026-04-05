#pragma once

#include "Types.hpp"

namespace octob
{

class Crossover
{
 public:
  Crossover();

  void setSampleRate(SampleRate sampleRate);
  void setFrequency(float frequencyHz);

  void process(const Sample* input, Sample* lowOut, Sample* highOut, FrameCount numFrames);

  void reset();

  float getFrequency() const { return frequency_; }
  SampleRate getSampleRate() const { return sampleRate_; }

 private:
  struct BiquadState
  {
    Sample x1 = 0.0f, x2 = 0.0f;
    Sample y1 = 0.0f, y2 = 0.0f;
  };

  struct BiquadCoeffs
  {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
  };

  BiquadCoeffs lpCoeffs_;
  BiquadCoeffs hpCoeffs_;
  BiquadState lpState1_;
  BiquadState lpState2_;
  BiquadState hpState1_;
  BiquadState hpState2_;

  float frequency_;
  SampleRate sampleRate_;

  void updateCoefficients();

  static Sample processBiquad(const BiquadCoeffs& c, BiquadState& s, Sample input);
};

}  // namespace octob
