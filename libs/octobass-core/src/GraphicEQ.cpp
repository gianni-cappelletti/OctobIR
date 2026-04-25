#include "octobass-core/GraphicEQ.hpp"

#include <algorithm>
#include <cmath>

namespace octob
{

GraphicEQ::GraphicEQ()
{
  gainsDb_.fill(DefaultGraphicEQGainDb);
}

void GraphicEQ::setSampleRate(SampleRate sampleRate)
{
  if (sampleRate > 0.0)
  {
    sampleRate_ = sampleRate;
    for (int i = 0; i < kGraphicEQNumBands; ++i)
      updateCoefficients(i);
  }
}

void GraphicEQ::setBandGain(int bandIndex, float gainDb)
{
  if (bandIndex < 0 || bandIndex >= kGraphicEQNumBands)
    return;

  gainDb = std::max(MinGraphicEQGainDb, std::min(MaxGraphicEQGainDb, gainDb));

  if (gainsDb_[static_cast<size_t>(bandIndex)] == gainDb)
    return;

  gainsDb_[static_cast<size_t>(bandIndex)] = gainDb;
  updateCoefficients(bandIndex);
}

float GraphicEQ::getBandGain(int bandIndex) const
{
  if (bandIndex < 0 || bandIndex >= kGraphicEQNumBands)
    return DefaultGraphicEQGainDb;

  return gainsDb_[static_cast<size_t>(bandIndex)];
}

void GraphicEQ::process(const Sample* input, Sample* output, FrameCount numFrames)
{
  if (numFrames == 0)
    return;

  if (activeBandMask_ == 0)
  {
    if (input != output)
      std::memcpy(output, input, numFrames * sizeof(Sample));
    return;
  }

  if (input != output)
    std::memcpy(output, input, numFrames * sizeof(Sample));

  for (int b = 0; b < kGraphicEQNumBands; ++b)
  {
    if (!(activeBandMask_ & (1u << b)))
      continue;

    auto& coeffs = coeffs_[static_cast<size_t>(b)];
    auto& state = states_[static_cast<size_t>(b)];
    for (FrameCount i = 0; i < numFrames; ++i)
      output[i] = tick(coeffs, state, output[i]);
  }
}

void GraphicEQ::reset()
{
  for (auto& s : states_)
  {
    s.z1 = 0.0f;
    s.z2 = 0.0f;
  }
}

void GraphicEQ::updateCoefficients(int bandIndex)
{
  auto idx = static_cast<size_t>(bandIndex);
  float gainDb = gainsDb_[idx];

  if (std::fabs(gainDb) < 0.01f)
  {
    // Pass-through: unity gain biquad
    coeffs_[idx] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    activeBandMask_ &= ~(1u << bandIndex);
    return;
  }

  activeBandMask_ |= (1u << bandIndex);

  const float pi = 3.14159265358979323846f;
  float centerFreq = kCenterFreqs[idx];
  float sr = static_cast<float>(sampleRate_);

  // RBJ Audio EQ Cookbook -- peaking EQ
  float A = std::pow(10.0f, gainDb / 40.0f);
  float w0 = 2.0f * pi * centerFreq / sr;
  float Q = computeQ(std::fabs(gainDb));
  float alpha = std::sin(w0) / (2.0f * Q);

  float b0 = 1.0f + alpha * A;
  float b1 = -2.0f * std::cos(w0);
  float b2 = 1.0f - alpha * A;
  float a0 = 1.0f + alpha / A;
  float a1 = -2.0f * std::cos(w0);
  float a2 = 1.0f - alpha / A;

  float invA0 = 1.0f / a0;
  coeffs_[idx].b0 = b0 * invA0;
  coeffs_[idx].b1 = b1 * invA0;
  coeffs_[idx].b2 = b2 * invA0;
  coeffs_[idx].a1 = a1 * invA0;
  coeffs_[idx].a2 = a2 * invA0;
}

Sample GraphicEQ::tick(const BiquadCoeffs& c, BiquadState& s, Sample input)
{
  // Transposed Direct Form II
  Sample output = c.b0 * input + s.z1;
  s.z1 = c.b1 * input - c.a1 * output + s.z2;
  s.z2 = c.b2 * input - c.a2 * output;
  return output;
}

float GraphicEQ::computeQ(float absGainDb)
{
  // Proportional Q: wider bandwidth at low gain, narrower at high gain.
  // Modeled after the API 550A characteristic.
  float normalized = absGainDb / MaxGraphicEQGainDb;
  normalized = std::max(0.0f, std::min(1.0f, normalized));
  return kQMin * std::pow(kQMax / kQMin, normalized);
}

float GraphicEQ::computeMagnitudeResponseDb(const float* gainsDb, float freqHz,
                                            SampleRate sampleRate)
{
  // Use double precision and complex evaluation to avoid catastrophic
  // float cancellation at low frequencies where cos(w) ~ 1.
  const double pi = 3.14159265358979323846;
  double w = 2.0 * pi * static_cast<double>(freqHz) / sampleRate;
  double sinw = std::sin(w);
  double cosw = std::cos(w);
  double sin2w = std::sin(2.0 * w);
  double cos2w = std::cos(2.0 * w);

  double totalMagSq = 1.0;

  for (int i = 0; i < kGraphicEQNumBands; ++i)
  {
    double gainDb = static_cast<double>(gainsDb[i]);
    if (std::fabs(gainDb) < 0.01)
      continue;

    auto idx = static_cast<size_t>(i);
    double A = std::pow(10.0, gainDb / 40.0);
    double w0 = 2.0 * pi * static_cast<double>(kCenterFreqs[idx]) / sampleRate;
    double Q = static_cast<double>(computeQ(static_cast<float>(std::fabs(gainDb))));
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);

    double b0 = (1.0 + alpha * A) / (1.0 + alpha / A);
    double b1 = (-2.0 * cosw0) / (1.0 + alpha / A);
    double b2 = (1.0 - alpha * A) / (1.0 + alpha / A);
    double a1 = (-2.0 * cosw0) / (1.0 + alpha / A);
    double a2 = (1.0 - alpha / A) / (1.0 + alpha / A);

    // Evaluate H(e^jw) using complex arithmetic:
    //   N = b0 + b1*e^(-jw) + b2*e^(-2jw)
    //   D = 1  + a1*e^(-jw) + a2*e^(-2jw)
    double numRe = b0 + b1 * cosw + b2 * cos2w;
    double numIm = -(b1 * sinw + b2 * sin2w);
    double denRe = 1.0 + a1 * cosw + a2 * cos2w;
    double denIm = -(a1 * sinw + a2 * sin2w);

    double numMagSq = numRe * numRe + numIm * numIm;
    double denMagSq = denRe * denRe + denIm * denIm;

    if (denMagSq > 1e-30)
      totalMagSq *= numMagSq / denMagSq;
  }

  if (totalMagSq <= 0.0)
    return 0.0f;

  return static_cast<float>(10.0 * std::log10(totalMagSq));
}

}  // namespace octob
