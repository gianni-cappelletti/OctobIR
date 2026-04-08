#pragma once

#include "CompressorMode.hpp"

namespace octob
{

// Feedback FET compressor modeled after the 1176.
// Nonlinear FET gain element with fast time constants for aggressive, punchy compression.
class FETCompressor : public CompressorMode
{
 public:
  FETCompressor();

  void setSampleRate(SampleRate sampleRate) override;
  void setAmount(float amount) override;
  void process(const Sample* input, Sample* output, FrameCount numFrames) override;
  void reset() override;
  float getGainReductionDb() const override;

 private:
  SampleRate sampleRate_;
  float amount_;

  // FET operating parameters (interpolated from amount)
  float bias_;
  float kneeExponent_;
  float attackCoeff_;
  float releaseCoeff_;

  // State
  float envelopeLinear_;
  float gainReductionDb_;

  void updateParameters();
};

}  // namespace octob
