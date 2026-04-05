#include "octobass-core/Crossover.hpp"

#include <algorithm>
#include <cmath>

namespace octob
{

Crossover::Crossover() : frequency_(DefaultCrossoverFrequency), sampleRate_(44100.0)
{
  updateCoefficients();
}

void Crossover::setSampleRate(SampleRate sampleRate)
{
  if (sampleRate > 0.0)
  {
    sampleRate_ = sampleRate;
    updateCoefficients();
  }
}

void Crossover::setFrequency(float frequencyHz)
{
  frequency_ = std::max(MinCrossoverFrequency, std::min(MaxCrossoverFrequency, frequencyHz));
  updateCoefficients();
}

void Crossover::process(const Sample* input, Sample* lowOut, Sample* highOut, FrameCount numFrames)
{
  for (FrameCount i = 0; i < numFrames; ++i)
  {
    Sample in = input[i];

    // Low pass: two cascaded Butterworth LP sections = LR4 low
    Sample lp1 = processBiquad(lpCoeffs_, lpState1_, in);
    Sample lp2 = processBiquad(lpCoeffs_, lpState2_, lp1);
    lowOut[i] = lp2;

    // High pass: two cascaded Butterworth HP sections = LR4 high
    Sample hp1 = processBiquad(hpCoeffs_, hpState1_, in);
    Sample hp2 = processBiquad(hpCoeffs_, hpState2_, hp1);
    highOut[i] = hp2;
  }
}

void Crossover::reset()
{
  lpState1_ = BiquadState();
  lpState2_ = BiquadState();
  hpState1_ = BiquadState();
  hpState2_ = BiquadState();
}

void Crossover::updateCoefficients()
{
  // Butterworth 2nd-order coefficients via bilinear transform with frequency pre-warping.
  // LR4 = two cascaded identical Butterworth sections per band.

  const double pi = 3.14159265358979323846;
  const double fc = static_cast<double>(frequency_);
  const double fs = sampleRate_;

  // Pre-warp the cutoff frequency
  const double wc = 2.0 * fs * std::tan(pi * fc / fs);

  // Intermediate values
  const double k = wc / (2.0 * fs);
  const double k2 = k * k;
  const double sqrt2k = std::sqrt(2.0) * k;
  const double denom = 1.0 + sqrt2k + k2;

  // Lowpass Butterworth 2nd-order
  lpCoeffs_.b0 = static_cast<float>(k2 / denom);
  lpCoeffs_.b1 = static_cast<float>(2.0 * k2 / denom);
  lpCoeffs_.b2 = static_cast<float>(k2 / denom);
  lpCoeffs_.a1 = static_cast<float>(2.0 * (k2 - 1.0) / denom);
  lpCoeffs_.a2 = static_cast<float>((1.0 - sqrt2k + k2) / denom);

  // Highpass Butterworth 2nd-order
  hpCoeffs_.b0 = static_cast<float>(1.0 / denom);
  hpCoeffs_.b1 = static_cast<float>(-2.0 / denom);
  hpCoeffs_.b2 = static_cast<float>(1.0 / denom);
  hpCoeffs_.a1 = static_cast<float>(2.0 * (k2 - 1.0) / denom);
  hpCoeffs_.a2 = static_cast<float>((1.0 - sqrt2k + k2) / denom);
}

Sample Crossover::processBiquad(const BiquadCoeffs& c, BiquadState& s, Sample input)
{
  // Direct Form I
  Sample output = c.b0 * input + c.b1 * s.x1 + c.b2 * s.x2 - c.a1 * s.y1 - c.a2 * s.y2;

  s.x2 = s.x1;
  s.x1 = input;
  s.y2 = s.y1;
  s.y1 = output;

  return output;
}

}  // namespace octob
