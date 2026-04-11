#include "octobass-core/NoiseGate.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace octob
{

namespace
{
float msToCoeff(float ms, SampleRate sampleRate)
{
  if (ms <= 0.0f || sampleRate <= 0.0)
    return 0.0f;
  return std::exp(-1.0f / (static_cast<float>(sampleRate) * ms * 0.001f));
}
}  // namespace

NoiseGate::NoiseGate()
    : sampleRate_(44100.0),
      thresholdDb_(DefaultGateThresholdDb),
      closeThresholdDb_(DefaultGateThresholdDb - kHysteresisDb),
      attackCoeff_(0.0f),
      releaseCoeff_(0.0f),
      holdSamples_(0),
      state_(State::Closed),
      envelope_(0.0f),
      holdCounter_(0),
      rangeLinear_(std::pow(10.0f, kRangeDb / 20.0f))
{
  updateParameters();
}

void NoiseGate::setSampleRate(SampleRate sampleRate)
{
  sampleRate_ = sampleRate;
  updateParameters();
}

void NoiseGate::setThresholdDb(float thresholdDb)
{
  thresholdDb_ = std::max(MinGateThresholdDb, std::min(MaxGateThresholdDb, thresholdDb));
  closeThresholdDb_ = thresholdDb_ - kHysteresisDb;
}

void NoiseGate::process(const Sample* keyInput, const Sample* signalInput, Sample* signalOutput,
                        FrameCount numFrames)
{
  if (!isEnabled())
  {
    if (signalInput != signalOutput)
      std::memcpy(signalOutput, signalInput, numFrames * sizeof(Sample));
    return;
  }

  for (FrameCount i = 0; i < numFrames; ++i)
  {
    float absLevel = std::fabs(keyInput[i]);
    float levelDb = (absLevel > 1e-30f) ? std::log2(absLevel) * 6.02059991f : -96.0f;

    if (levelDb >= thresholdDb_)
    {
      state_ = State::Open;
      holdCounter_ = holdSamples_;
      // Fast attack toward 1.0
      envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * 1.0f;
    }
    else if (levelDb < closeThresholdDb_)
    {
      switch (state_)
      {
        case State::Open:
          state_ = State::Hold;
          [[fallthrough]];

        case State::Hold:
          --holdCounter_;
          if (holdCounter_ <= 0)
            state_ = State::Closing;
          break;

        case State::Closing:
          envelope_ = releaseCoeff_ * envelope_;
          if (envelope_ < rangeLinear_)
          {
            envelope_ = rangeLinear_;
            state_ = State::Closed;
          }
          break;

        case State::Closed:
          envelope_ = rangeLinear_;
          break;
      }
    }
    // In hysteresis zone: maintain current state, let hold/release continue if active
    else
    {
      if (state_ == State::Hold)
      {
        --holdCounter_;
        if (holdCounter_ <= 0)
          state_ = State::Closing;
      }
      else if (state_ == State::Closing)
      {
        envelope_ = releaseCoeff_ * envelope_;
        if (envelope_ < rangeLinear_)
        {
          envelope_ = rangeLinear_;
          state_ = State::Closed;
        }
      }
    }

    if (std::fabs(envelope_) < 1e-8f)
      envelope_ = rangeLinear_;

    signalOutput[i] = signalInput[i] * envelope_;
  }
}

void NoiseGate::reset()
{
  state_ = State::Closed;
  envelope_ = 0.0f;
  holdCounter_ = 0;
}

void NoiseGate::updateParameters()
{
  attackCoeff_ = msToCoeff(kAttackMs, sampleRate_);
  releaseCoeff_ = msToCoeff(kReleaseMs, sampleRate_);
  holdSamples_ = static_cast<int>(static_cast<float>(sampleRate_) * kHoldMs * 0.001f);
}

}  // namespace octob
