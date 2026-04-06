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
  struct SVFState
  {
    Sample ic1eq = 0.0f;
    Sample ic2eq = 0.0f;
  };

  struct SVFCoeffs
  {
    float g = 0.0f;
    float k = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
  };

  SVFCoeffs coeffs_;
  SVFState lpState1_;
  SVFState lpState2_;
  SVFState hpState1_;
  SVFState hpState2_;

  float frequency_;
  SampleRate sampleRate_;

  void updateCoefficients();

  static Sample tickLP(const SVFCoeffs& c, SVFState& s, Sample input);
  static Sample tickHP(const SVFCoeffs& c, SVFState& s, Sample input);
};

}  // namespace octob
