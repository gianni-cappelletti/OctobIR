#pragma once

#include "Types.hpp"

namespace octob
{

// Sidechain noise gate with Fortin Zuul-style topology.
// Clean input is the detection key; gate attenuation is applied to the processed signal.
// Peak detection with hysteresis and hold for responsive, chatter-free gating.
class NoiseGate
{
 public:
  NoiseGate();

  void setSampleRate(SampleRate sampleRate);
  void setThresholdDb(float thresholdDb);

  // keyInput: clean sidechain signal for level detection
  // signalInput/signalOutput: the signal being gated (may alias)
  void process(const Sample* keyInput, const Sample* signalInput, Sample* signalOutput,
               FrameCount numFrames);
  void reset();

  float getThresholdDb() const { return thresholdDb_; }
  bool isEnabled() const { return thresholdDb_ > kDisableThresholdDb; }
  float getGateGain() const { return envelope_; }

 private:
  enum class State
  {
    Open,
    Hold,
    Closing,
    Closed
  };

  SampleRate sampleRate_;
  float thresholdDb_;
  float closeThresholdDb_;

  float attackCoeff_;
  float releaseCoeff_;
  int holdSamples_;

  State state_;
  float envelope_;
  int holdCounter_;

  static constexpr float kHysteresisDb = 6.0f;
  static constexpr float kAttackMs = 0.01f;
  static constexpr float kHoldMs = 5.0f;
  static constexpr float kReleaseMs = 20.0f;
  static constexpr float kRangeDb = -80.0f;
  static constexpr float kDisableThresholdDb = -90.0f;

  float rangeLinear_;

  void updateParameters();
};

}  // namespace octob
