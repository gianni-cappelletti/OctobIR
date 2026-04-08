#include "octobass-core/FETCompressor.hpp"

#include <cmath>

namespace octob
{

namespace
{
// FET operating point targets at full amount
constexpr float kMinBias = 0.0f;
constexpr float kMaxBias = 1.5f;
constexpr float kMinKneeExponent = 1.0f;
constexpr float kMaxKneeExponent = 4.0f;

constexpr float kAttackMsLow = 0.8f;
constexpr float kAttackMsHigh = 0.05f;
constexpr float kReleaseMsLow = 100.0f;
constexpr float kReleaseMsHigh = 50.0f;

// Maximum gain reduction to prevent runaway in feedback loop
constexpr float kMaxGainReductionDb = -40.0f;

float msToCoeff(float ms, SampleRate sampleRate)
{
  if (ms <= 0.0f || sampleRate <= 0.0)
    return 0.0f;
  return std::exp(-1.0f / (static_cast<float>(sampleRate) * ms * 0.001f));
}
}  // namespace

FETCompressor::FETCompressor()
    : sampleRate_(44100.0),
      amount_(0.0f),
      bias_(0.0f),
      kneeExponent_(1.0f),
      attackCoeff_(0.0f),
      releaseCoeff_(0.0f),
      envelopeLinear_(0.0f),
      gainReductionDb_(0.0f)
{
  updateParameters();
}

void FETCompressor::setSampleRate(SampleRate sampleRate)
{
  sampleRate_ = sampleRate;
  updateParameters();
}

void FETCompressor::setAmount(float amount)
{
  amount_ = CompressorMode::clamp(amount, 0.0f, 1.0f);
  updateParameters();
}

void FETCompressor::process(const Sample* input, Sample* output, FrameCount numFrames)
{
  for (FrameCount i = 0; i < numFrames; ++i)
  {
    float in = input[i];

    // Feedback topology: apply current gain to get output, detect on output
    float gainLinear = CompressorMode::dbToLinear(gainReductionDb_);
    float out = in * gainLinear;

    // Peak detection on output (feedback)
    float detectedLevel = std::fabs(out);

    // Ballistic envelope follower (attack/release on the detected level)
    if (detectedLevel > envelopeLinear_)
    {
      envelopeLinear_ = attackCoeff_ * envelopeLinear_ + (1.0f - attackCoeff_) * detectedLevel;
    }
    else
    {
      envelopeLinear_ = releaseCoeff_ * envelopeLinear_ + (1.0f - releaseCoeff_) * detectedLevel;
    }

    // Denormal guard
    if (envelopeLinear_ < 1e-8f)
      envelopeLinear_ = 0.0f;

    // FET gain reduction: nonlinear square-law characteristic
    // gain = 1 / (1 + (envelope * bias)^kneeExponent)
    // At low amount: bias=0, so gain=1 (no compression)
    // At high amount: bias increases, kneeExponent increases for harder knee
    float controlSignal = envelopeLinear_ * bias_;
    float fetReduction = 1.0f / (1.0f + std::pow(controlSignal, kneeExponent_));

    gainReductionDb_ =
        CompressorMode::clamp(CompressorMode::linearToDb(fetReduction), kMaxGainReductionDb, 0.0f);

    output[i] = out;
  }
}

void FETCompressor::reset()
{
  envelopeLinear_ = 0.0f;
  gainReductionDb_ = 0.0f;
}

float FETCompressor::getGainReductionDb() const
{
  return gainReductionDb_;
}

void FETCompressor::updateParameters()
{
  // Interpolate FET parameters from amount
  bias_ = kMinBias + amount_ * (kMaxBias - kMinBias);
  kneeExponent_ = kMinKneeExponent + amount_ * (kMaxKneeExponent - kMinKneeExponent);

  // Attack gets faster as amount increases
  float attackMs = kAttackMsLow + amount_ * (kAttackMsHigh - kAttackMsLow);
  float releaseMs = kReleaseMsLow + amount_ * (kReleaseMsHigh - kReleaseMsLow);

  attackCoeff_ = msToCoeff(attackMs, sampleRate_);
  releaseCoeff_ = msToCoeff(releaseMs, sampleRate_);
}

}  // namespace octob
