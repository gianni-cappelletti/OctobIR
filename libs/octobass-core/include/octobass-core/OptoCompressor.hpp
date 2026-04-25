#pragma once

#include "CompressorMode.hpp"

namespace octob
{

// Feed-forward opto compressor inspired by the LA-2A.
// T4 cell model with program-dependent release for smooth, musical leveling.
class OptoCompressor : public CompressorMode
{
 public:
  OptoCompressor();

  void setSampleRate(SampleRate sampleRate) override;
  void setAmount(float amount) override;
  void process(const Sample* input, Sample* output, FrameCount numFrames) override;
  void reset() override;
  float getGainReductionDb() const override;
  float getStaticMakeupDb() const override;

 private:
  SampleRate sampleRate_;
  float amount_;

  // T4 cell state (models CdS photoresistor behavior)
  float fastState_;
  float slowState_;
  float charge_;

  // T4 coefficients (recalculated on sample rate change)
  float fastAttackCoeff_;
  float slowAttackCoeff_;
  float chargeRate_;
  float dischargeRate_;

  float gainReductionDb_;

  void updateCoefficients();
};

}  // namespace octob
