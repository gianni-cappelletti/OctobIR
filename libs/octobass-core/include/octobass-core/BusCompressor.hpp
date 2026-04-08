#pragma once

#include "CompressorMode.hpp"

namespace octob
{

// Feed-forward bus compressor modeled after the SSL G-bus.
// RMS detection with auto-release for cohesive "glue" compression.
class BusCompressor : public CompressorMode
{
 public:
  BusCompressor();

  void setSampleRate(SampleRate sampleRate) override;
  void setAmount(float amount) override;
  void process(const Sample* input, Sample* output, FrameCount numFrames) override;
  void reset() override;
  float getGainReductionDb() const override;

 private:
  SampleRate sampleRate_;
  float amount_;

  // Interpolated compression parameters
  float thresholdDb_;
  float ratio_;
  float kneeDb_;

  // Ballistics coefficients
  float attackCoeff_;
  float fastReleaseCoeff_;
  float slowReleaseCoeff_;
  float rmsCoeff_;

  // State
  double rmsSquared_;
  float envelopeDb_;
  float gainReductionDb_;

  void updateParameters();
  float computeStaticCurve(float inputDb) const;
};

}  // namespace octob
