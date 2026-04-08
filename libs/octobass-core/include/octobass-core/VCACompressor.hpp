#pragma once

#include "CompressorMode.hpp"

namespace octob
{

// Feed-forward VCA compressor modeled after the dbx 160.
// RMS detection with dual-release ballistics for transparent, tight bass control.
class VCACompressor : public CompressorMode
{
 public:
  VCACompressor();

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
