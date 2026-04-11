#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "octobass-core/GraphicEQ.hpp"

using namespace octob;

namespace
{

constexpr double kPi = 3.14159265358979323846;

std::vector<float> generateSine(float freqHz, float sampleRate, size_t numSamples,
                                float amplitude = 1.0f)
{
  std::vector<float> buf(numSamples);
  for (size_t i = 0; i < numSamples; ++i)
    buf[i] = amplitude * static_cast<float>(std::sin(2.0 * kPi * freqHz * i / sampleRate));
  return buf;
}

double computeRMS(const std::vector<float>& buf, size_t start = 0, size_t len = 0)
{
  if (len == 0)
    len = buf.size() - start;
  double sum = 0.0;
  for (size_t i = start; i < start + len; ++i)
    sum += static_cast<double>(buf[i]) * buf[i];
  return std::sqrt(sum / static_cast<double>(len));
}

double rmsToDb(double rms)
{
  return (rms > 1e-10) ? 20.0 * std::log10(rms) : -200.0;
}

}  // namespace

class GraphicEQTest : public ::testing::Test
{
 protected:
  void SetUp() override { eq.setSampleRate(44100.0); }

  GraphicEQ eq;
};

TEST_F(GraphicEQTest, UnityPassThrough)
{
  constexpr size_t kNumSamples = 8192;
  constexpr size_t kSkip = 1024;

  auto input = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);
  eq.process(input.data(), output.data(), kNumSamples);

  double inputRMS = computeRMS(input, kSkip, kNumSamples - kSkip);
  double outputRMS = computeRMS(output, kSkip, kNumSamples - kSkip);
  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);

  EXPECT_NEAR(ratioDb, 0.0, 0.01) << "All bands at 0dB should produce unity gain";
}

TEST_F(GraphicEQTest, SingleBandBoost)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  // Band 14 has center frequency ~1000 Hz
  eq.setBandGain(14, 6.0f);

  auto input = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);
  eq.process(input.data(), output.data(), kNumSamples);

  double inputRMS = computeRMS(input, kSkip, kNumSamples - kSkip);
  double outputRMS = computeRMS(output, kSkip, kNumSamples - kSkip);
  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);

  EXPECT_NEAR(ratioDb, 6.0, 1.0) << "Band 14 at +6dB should boost 1kHz by ~6dB";
}

TEST_F(GraphicEQTest, SingleBandCut)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  eq.setBandGain(14, -6.0f);

  auto input = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);
  eq.process(input.data(), output.data(), kNumSamples);

  double inputRMS = computeRMS(input, kSkip, kNumSamples - kSkip);
  double outputRMS = computeRMS(output, kSkip, kNumSamples - kSkip);
  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);

  EXPECT_NEAR(ratioDb, -6.0, 1.0) << "Band 14 at -6dB should cut 1kHz by ~6dB";
}

TEST_F(GraphicEQTest, OutOfBandRejection)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  // Boost band 14 (1kHz) by 12dB, measure at 100Hz (band 4)
  eq.setBandGain(14, 12.0f);

  auto input = generateSine(100.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);
  eq.process(input.data(), output.data(), kNumSamples);

  double inputRMS = computeRMS(input, kSkip, kNumSamples - kSkip);
  double outputRMS = computeRMS(output, kSkip, kNumSamples - kSkip);
  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);

  EXPECT_NEAR(ratioDb, 0.0, 1.5)
      << "100Hz should be minimally affected by a 1kHz boost, got " << ratioDb << " dB";
}

TEST_F(GraphicEQTest, ProportionalQ_NarrowAtHighGain)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  // At +12dB, Q=2.0 (narrow). Measure boost at center vs 1 octave away.
  eq.setBandGain(14, 12.0f);

  // At center (1kHz)
  auto inputCenter = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> outputCenter(kNumSamples);
  eq.process(inputCenter.data(), outputCenter.data(), kNumSamples);
  double centerBoost =
      20.0 * std::log10(computeRMS(outputCenter, kSkip) / computeRMS(inputCenter, kSkip));

  eq.reset();

  // At 1 octave up (2kHz)
  auto inputOctave = generateSine(2000.0f, 44100.0f, kNumSamples);
  std::vector<float> outputOctave(kNumSamples);
  eq.process(inputOctave.data(), outputOctave.data(), kNumSamples);
  double octaveBoost =
      20.0 * std::log10(computeRMS(outputOctave, kSkip) / computeRMS(inputOctave, kSkip));

  // At narrow Q, the octave-away boost should be significantly less than center
  EXPECT_GT(centerBoost - octaveBoost, 6.0)
      << "At +12dB (narrow Q), 1 octave away should be >6dB less than center. "
      << "Center=" << centerBoost << "dB, Octave=" << octaveBoost << "dB";
}

TEST_F(GraphicEQTest, ProportionalQ_WideAtLowGain)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  // At +2dB, Q~0.35 (wide). Measure boost at center vs 1 octave away.
  eq.setBandGain(14, 2.0f);

  auto inputCenter = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> outputCenter(kNumSamples);
  eq.process(inputCenter.data(), outputCenter.data(), kNumSamples);
  double centerBoost =
      20.0 * std::log10(computeRMS(outputCenter, kSkip) / computeRMS(inputCenter, kSkip));

  eq.reset();

  auto inputOctave = generateSine(2000.0f, 44100.0f, kNumSamples);
  std::vector<float> outputOctave(kNumSamples);
  eq.process(inputOctave.data(), outputOctave.data(), kNumSamples);
  double octaveBoost =
      20.0 * std::log10(computeRMS(outputOctave, kSkip) / computeRMS(inputOctave, kSkip));

  // At wide Q, the octave-away boost should be relatively close to center
  EXPECT_LT(centerBoost - octaveBoost, 2.5)
      << "At +2dB (wide Q), 1 octave away should be within 2.5dB of center. "
      << "Center=" << centerBoost << "dB, Octave=" << octaveBoost << "dB";
}

TEST_F(GraphicEQTest, ResetClearsState)
{
  constexpr size_t kNumSamples = 512;
  auto input = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);

  eq.setBandGain(14, 6.0f);
  eq.process(input.data(), output.data(), kNumSamples);

  eq.reset();

  std::vector<float> silence(kNumSamples, 0.0f);
  eq.process(silence.data(), output.data(), kNumSamples);

  for (size_t i = 0; i < kNumSamples; ++i)
    EXPECT_FLOAT_EQ(output[i], 0.0f) << "Output not zero at sample " << i << " after reset";
}

TEST_F(GraphicEQTest, GainClamping)
{
  eq.setBandGain(0, 20.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(0), MaxGraphicEQGainDb);

  eq.setBandGain(0, -20.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(0), MinGraphicEQGainDb);

  eq.setBandGain(0, 5.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(0), 5.0f);
}

TEST_F(GraphicEQTest, InvalidBandIndex)
{
  eq.setBandGain(-1, 6.0f);
  eq.setBandGain(24, 6.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(-1), DefaultGraphicEQGainDb);
  EXPECT_FLOAT_EQ(eq.getBandGain(24), DefaultGraphicEQGainDb);
}

TEST_F(GraphicEQTest, SampleRateChange)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  eq.setSampleRate(96000.0);
  eq.setBandGain(14, 6.0f);

  auto input = generateSine(1000.0f, 96000.0f, kNumSamples);
  std::vector<float> output(kNumSamples);
  eq.process(input.data(), output.data(), kNumSamples);

  double inputRMS = computeRMS(input, kSkip, kNumSamples - kSkip);
  double outputRMS = computeRMS(output, kSkip, kNumSamples - kSkip);
  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);

  EXPECT_NEAR(ratioDb, 6.0, 1.0)
      << "Band 14 at +6dB should boost 1kHz by ~6dB at 96kHz sample rate";
}

TEST_F(GraphicEQTest, InPlaceProcessing)
{
  constexpr size_t kNumSamples = 8192;
  constexpr size_t kSkip = 2048;

  eq.setBandGain(14, 6.0f);

  auto buffer = generateSine(1000.0f, 44100.0f, kNumSamples);
  double inputRMS = computeRMS(buffer, kSkip, kNumSamples - kSkip);

  eq.process(buffer.data(), buffer.data(), kNumSamples);

  double outputRMS = computeRMS(buffer, kSkip, kNumSamples - kSkip);
  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);

  EXPECT_NEAR(ratioDb, 6.0, 1.0) << "In-place processing should work correctly";
}

TEST_F(GraphicEQTest, MultipleBands)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;

  // Boost low bands, cut high bands
  eq.setBandGain(4, 6.0f);    // 100 Hz
  eq.setBandGain(14, -6.0f);  // 1 kHz

  auto input100 = generateSine(100.0f, 44100.0f, kNumSamples);
  std::vector<float> output100(kNumSamples);
  eq.process(input100.data(), output100.data(), kNumSamples);
  double boost100 =
      20.0 * std::log10(computeRMS(output100, kSkip) / computeRMS(input100, kSkip));

  eq.reset();

  auto input1k = generateSine(1000.0f, 44100.0f, kNumSamples);
  std::vector<float> output1k(kNumSamples);
  eq.process(input1k.data(), output1k.data(), kNumSamples);
  double boost1k = 20.0 * std::log10(computeRMS(output1k, kSkip) / computeRMS(input1k, kSkip));

  EXPECT_GT(boost100, 3.0) << "100Hz should be boosted, got " << boost100 << "dB";
  EXPECT_LT(boost1k, -3.0) << "1kHz should be cut, got " << boost1k << "dB";
}
