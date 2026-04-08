#include "octobass-core/Compressor.hpp"

#include <cmath>

namespace octob
{

namespace
{
constexpr float kRmsWindowMs = 300.0f;
constexpr float kMakeupSmoothMs = 500.0f;
constexpr float kMaxMakeupDb = 24.0f;

float msToAlpha(float ms, SampleRate sampleRate)
{
  if (ms <= 0.0f || sampleRate <= 0.0)
    return 0.0f;
  return std::exp(-1.0f / (static_cast<float>(sampleRate) * ms * 0.001f));
}
}  // namespace

Compressor::Compressor()
    : activeMode_(&vca_),
      sampleRate_(44100.0),
      squash_(DefaultSquashAmount),
      mode_(DefaultCompressionMode),
      inputRmsSquared_(0.0),
      outputRmsSquared_(0.0),
      smoothedMakeupDb_(0.0f),
      rmsAlpha_(0.0f),
      makeupSmoothAlpha_(0.0f)
{
  updateRmsCoefficients();
}

void Compressor::setSampleRate(SampleRate sampleRate)
{
  sampleRate_ = sampleRate;
  vca_.setSampleRate(sampleRate);
  opto_.setSampleRate(sampleRate);
  fet_.setSampleRate(sampleRate);
  bus_.setSampleRate(sampleRate);
  updateRmsCoefficients();
}

void Compressor::setSquash(float amount)
{
  squash_ = clamp(amount, MinSquashAmount, MaxSquashAmount);
  activeMode_->setAmount(squash_);
}

void Compressor::setMode(int mode)
{
  mode_ = static_cast<int>(
      clamp(static_cast<float>(mode), 0.0f, static_cast<float>(NumCompressionModes - 1)));

  switch (mode_)
  {
    case 0:
      activeMode_ = &vca_;
      break;
    case 1:
      activeMode_ = &opto_;
      break;
    case 2:
      activeMode_ = &fet_;
      break;
    case 3:
      activeMode_ = &bus_;
      break;
    default:
      activeMode_ = &vca_;
      break;
  }

  activeMode_->reset();
  activeMode_->setAmount(squash_);

  // Reset makeup gain tracking on mode switch for clean transition
  inputRmsSquared_ = 0.0;
  outputRmsSquared_ = 0.0;
  smoothedMakeupDb_ = 0.0f;
}

void Compressor::process(const Sample* input, Sample* output, FrameCount numFrames)
{
  // Delegate to active compressor mode
  activeMode_->process(input, output, numFrames);

  // Apply unified RMS-tracking makeup gain
  double alpha = static_cast<double>(rmsAlpha_);
  double oneMinusAlpha = 1.0 - alpha;
  float makeupSmooth = makeupSmoothAlpha_;

  for (FrameCount i = 0; i < numFrames; ++i)
  {
    double inSq = static_cast<double>(input[i]) * input[i];
    double outSq = static_cast<double>(output[i]) * output[i];

    inputRmsSquared_ = alpha * inputRmsSquared_ + oneMinusAlpha * inSq;
    outputRmsSquared_ = alpha * outputRmsSquared_ + oneMinusAlpha * outSq;

    // Compute RMS deficit in dB
    float deficitDb = 0.0f;
    if (outputRmsSquared_ > 1e-30 && inputRmsSquared_ > 1e-30)
    {
      float inputRmsDb = 10.0f * std::log10(static_cast<float>(inputRmsSquared_));
      float outputRmsDb = 10.0f * std::log10(static_cast<float>(outputRmsSquared_));
      deficitDb = inputRmsDb - outputRmsDb;
    }
    deficitDb = clamp(deficitDb, 0.0f, kMaxMakeupDb);

    // Smooth the makeup gain
    smoothedMakeupDb_ += (deficitDb - smoothedMakeupDb_) * (1.0f - makeupSmooth);

    // Denormal guard
    if (std::fabs(smoothedMakeupDb_) < 1e-8f)
      smoothedMakeupDb_ = 0.0f;

    float makeupLinear = std::pow(10.0f, smoothedMakeupDb_ / 20.0f);
    output[i] *= makeupLinear;
  }
}

void Compressor::reset()
{
  vca_.reset();
  opto_.reset();
  fet_.reset();
  bus_.reset();
  inputRmsSquared_ = 0.0;
  outputRmsSquared_ = 0.0;
  smoothedMakeupDb_ = 0.0f;
}

float Compressor::getGainReductionDb() const
{
  return activeMode_->getGainReductionDb();
}

float Compressor::clamp(float value, float minVal, float maxVal)
{
  return std::max(minVal, std::min(maxVal, value));
}

void Compressor::updateRmsCoefficients()
{
  rmsAlpha_ = msToAlpha(kRmsWindowMs, sampleRate_);
  makeupSmoothAlpha_ = msToAlpha(kMakeupSmoothMs, sampleRate_);
}

}  // namespace octob
